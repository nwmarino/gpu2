//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "include/igpu.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <stack>
#include <unordered_map>
#include <vector>

#ifndef NDEBUG
    #define IGPU_ASSERT(cond, msg) \
        do { \
            if (!(cond)) { \
                std::cerr << "Assertion failed: (" #cond "), " \
                          << "message: " << msg << ", " \
                          << "file: " << __FILE__ << ", " \
                          << "line: " << __LINE__ << std::endl; \
                std::abort(); \
            } \
        } while(false)
#else
    #define IGPU_ASSERT(cond, msg) ((void) 0)
#endif // NDEBUG

#ifdef IGPU_VULKAN
    #define VULKAN_HPP_NO_EXCEPTIONS
    #include "vulkan/vulkan.hpp"

    #define VMA_IMPLEMENTATION
    #if defined(__clang__)
        // The VMA header tends to contain a lot of warnings for clang that 
        // just clutter the building process.
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Weverything"
        #include "vma.h"
        #pragma clang diagnostic pop
    #else
        #include "vma.h"
    #endif // defined(__clang__)

    #define TEXTURE_BINDING 0
    #define RW_TEXTURE_BINDING 1
    #define SAMPLER_BINDING 2

    #define VK_CHECK(result) \
        do { \
            if ((result) != vk::Result::eSuccess) { \
                std::cerr << "Check failed: " \
                          << vk::to_string(result) << ", " \
                          << "file: " << __FILE__ << ", " \
                          << "line: " << __LINE__ << std::endl; \
                std::abort(); \
            } \
        } while (false)
#endif // IGPU_VULKAN

#ifdef IGPU_GLFW
    #include "GLFW/glfw3.h"
#endif // IGPU_GLFW

namespace gpu {

template<typename T>
class Pool final {
public:
    using key_type = uint64_t;
    using value_type = T;

private:
    std::vector<std::pair<value_type, bool>> m_data = {};
    std::stack<key_type> m_free = {};

public:
    Pool() = default;

    /// Returns the value in this pool with the given key, if it exists.
    const value_type& get(key_type k) const {
        IGPU_ASSERT(contains(k), "Invalid handle!");
        return m_data[k].first;
    }

    value_type& get(key_type k) {
        IGPU_ASSERT(contains(k), "Invalid handle!");
        return m_data[k].first;
    }

    /// Add the given value to this pool, and returns a key to it.
    key_type add(value_type v) {
        key_type k;

        if (m_free.empty()) {
            k = static_cast<key_type>(m_data.size());
            m_data.push_back({ std::move(v), true });
        } else {
            k = m_free.top();
            m_free.pop();
            m_data[k] = { std::move(v), true };
        }

        return k;
    }

    /// Removes and returns the entry with the given key, if it exists.
    value_type remove(key_type k) {
        IGPU_ASSERT(contains(k), "Invalid handle!");

        value_type v = std::move(m_data[k].first);
        m_data[k].second = false;
        m_free.push(k);

        return v;
    }

    /// Returns true if this pool contains an entry for the given key.
    bool contains(key_type k) const {
        return k < m_data.size() && m_data[k].second;
    }
};

#ifdef IGPU_VULKAN

struct QueueFamilyIndices final {
    uint32_t transfer = UINT_MAX;
    uint32_t graphics = UINT_MAX;
    uint32_t compute = UINT_MAX;

    /// Returns true if this set of queue families is complete, i.e. contains
    /// all necessary indices.
    bool complete() const {
        return transfer != UINT_MAX 
            && graphics != UINT_MAX 
            && compute != UINT_MAX;
    }
};

static vk::ImageAspectFlags VK_FormatToAspectMask(Format format) {
    switch (format) {
        case Format::eRGBA8Unorm:
            return vk::ImageAspectFlagBits::eColor;
        case Format::eD32Float:
            return vk::ImageAspectFlagBits::eDepth;
        default:
            return vk::ImageAspectFlagBits::eNone;
    }
}

static vk::PipelineBindPoint VK_PipelineTypeToBindPoint(PipelineType type) {
    switch (type) {
        case PipelineType::eGraphics:
            return vk::PipelineBindPoint::eGraphics;
        case PipelineType::eCompute:
            return vk::PipelineBindPoint::eCompute;
    }
}

static vk::CompareOp VK_ConvertCompareOp(CompareOp op) {
    switch (op) {
        case CompareOp::eNever:
            return vk::CompareOp::eNever;
        case CompareOp::eLess:
            return vk::CompareOp::eLess;
        case CompareOp::eLessEqual:
            return vk::CompareOp::eLessOrEqual;
        case CompareOp::eGreater:
            return vk::CompareOp::eGreater;
        case CompareOp::eGreaterEqual:
            return vk::CompareOp::eGreaterOrEqual;
        case CompareOp::eEqual:
            return vk::CompareOp::eEqual;
        case CompareOp::eNotEqual:
            return vk::CompareOp::eNotEqual;
        case CompareOp::eAlways:
            return vk::CompareOp::eAlways;
    }
}

static vk::AttachmentLoadOp VK_ConvertLoadOp(LoadOp op) {
    switch (op) {
        case LoadOp::eUnknown:
            return vk::AttachmentLoadOp::eDontCare;
        case LoadOp::eClear:
            return vk::AttachmentLoadOp::eClear;
        case LoadOp::eLoad:
            return vk::AttachmentLoadOp::eLoad;
    }
}

static vk::AttachmentStoreOp VK_ConvertStoreOp(StoreOp op) {
    switch (op) {
        case StoreOp::eUnknown:
            return vk::AttachmentStoreOp::eDontCare;
        case StoreOp::eStore:
            return vk::AttachmentStoreOp::eStore;
    }
}

static vk::PipelineStageFlags2 VK_ConvertPipelineStage(Stage stage) {
    switch (stage) {
        case Stage::eTransfer:
            return vk::PipelineStageFlagBits2::eTransfer;
        case Stage::eCompute:
            return vk::PipelineStageFlagBits2::eComputeShader;
        case Stage::eRasterColorOut:
            return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        case Stage::eVertex:
            return vk::PipelineStageFlagBits2::eVertexShader;
        case Stage::eFragment:
            return vk::PipelineStageFlagBits2::eFragmentShader;
    }
}

static vk::ShaderStageFlags VK_ConvertShaderStage(Shader shader) {
    switch (shader) {
        case Shader::eVertex:
            return vk::ShaderStageFlagBits::eVertex;
        case Shader::eFragment:
            return vk::ShaderStageFlagBits::eFragment;
        case Shader::eCompute:
            return vk::ShaderStageFlagBits::eCompute;
        default:
            break;
    }

    IGPU_ASSERT(false, "Invalid shader stage!");
}

static vk::Format VK_ConvertFormat(Format format) {
    switch (format) {
        case Format::eUndefined: 
            return vk::Format::eUndefined;
        case Format::eRGBA8Unorm: 
            return vk::Format::eR8G8B8A8Unorm;
        case Format::eD32Float: 
            return vk::Format::eD32Sfloat;
    }
}

static vk::PresentModeKHR VK_ConvertPresent(Present present) {
    switch (present) {
        case Present::eImmediate:
            return vk::PresentModeKHR::eImmediate;
        case Present::eFifo:
            return vk::PresentModeKHR::eFifo;
        case Present::eFifoRelaxed:
            return vk::PresentModeKHR::eFifoRelaxed;
        case Present::eMailbox:
            return vk::PresentModeKHR::eMailbox;
    }
}

static vk::ImageType VK_ConvertTextureType(TextureType type) {
    switch (type) {
        case TextureType::e1D:
        case TextureType::e1DArray:
            return vk::ImageType::e1D;
        case TextureType::e2D:
        case TextureType::e2DArray:
        case TextureType::eCube:
        case TextureType::eCubeArray:
            return vk::ImageType::e2D;
        case TextureType::e3D:
            return vk::ImageType::e3D;
    }
}

static vk::PrimitiveTopology VK_ConvertTopology(Topology topology) {
    switch (topology) {
        case Topology::eLineList: 
            return vk::PrimitiveTopology::eLineList;
        case Topology::eLineStrip: 
            return vk::PrimitiveTopology::eLineStrip;
        case Topology::eTriangleList: 
            return vk::PrimitiveTopology::eTriangleList;
        case Topology::eTriangleStrip: 
            return vk::PrimitiveTopology::eTriangleStrip;
        case Topology::eTriangleFan: 
            return vk::PrimitiveTopology::eTriangleFan;
    }
}

static vk::CullModeFlags VK_ConvertCullMode(Cull cull) {
    switch (cull) {
        case Cull::eNone:
            return vk::CullModeFlagBits::eNone;
        case Cull::eClockwise:
            return vk::CullModeFlagBits::eFront;
        case Cull::eCounterClockwise:
            return vk::CullModeFlagBits::eBack;
        case Cull::eAll:
            return vk::CullModeFlagBits::eFrontAndBack;
    }
}

static vk::PolygonMode VK_ConvertFillMode(Fill fill) {
    switch (fill) {
        case Fill::eFill:
            return vk::PolygonMode::eFill;
        case Fill::eLine:
            return vk::PolygonMode::eLine;
    }
}

static vk::ImageUsageFlagBits VK_ConvertUsage(Usage usage) {
    switch (usage) {
    case Usage::eTransferSrc:
        return vk::ImageUsageFlagBits::eTransferSrc;
    case Usage::eTransferDst:
        return vk::ImageUsageFlagBits::eTransferDst;
    case Usage::eSampled:
        return vk::ImageUsageFlagBits::eSampled;
    case Usage::eStorage:
        return vk::ImageUsageFlagBits::eStorage;
    case Usage::eColorAttachment:
        return vk::ImageUsageFlagBits::eColorAttachment;
    case Usage::eDepthStencilAttachment:
        return vk::ImageUsageFlagBits::eDepthStencilAttachment;
    }
}

static vk::SampleCountFlagBits VK_ConvertSampleCount(uint8_t sample_count) {
    switch (sample_count) {
        case 1:
            return vk::SampleCountFlagBits::e1;
        case 2:
            return vk::SampleCountFlagBits::e2;
        case 4:
            return vk::SampleCountFlagBits::e4;
        case 8:
            return vk::SampleCountFlagBits::e8;
        case 16:
            return vk::SampleCountFlagBits::e16;
    }

    return vk::SampleCountFlagBits::e1;
}

static vk::BlendOp VK_ConvertBlendOp(BlendOp op) {
    switch (op) {
        case BlendOp::eAdd:
            return vk::BlendOp::eAdd;
        case BlendOp::eSubtract:
            return vk::BlendOp::eSubtract;
        case BlendOp::eReverseSubtract:
            return vk::BlendOp::eReverseSubtract;
        case BlendOp::eMin:
            return vk::BlendOp::eMin;
        case BlendOp::eMax:
            return vk::BlendOp::eMax;
    }
}

static vk::BlendFactor VK_ConvertBlendFactor(Factor factor) {
    switch (factor) {
        case Factor::eZero:
            return vk::BlendFactor::eZero;
        case Factor::eOne:
            return vk::BlendFactor::eOne;
        case Factor::eSrcColor:
            return vk::BlendFactor::eSrcColor;
        case Factor::eOneMinusSrcColor:
            return vk::BlendFactor::eOneMinusSrcColor;
        case Factor::eDstColor:
            return vk::BlendFactor::eDstColor;
        case Factor::eOneMinusDstColor:
            return vk::BlendFactor::eOneMinusDstColor;
        case Factor::eSrcAlpha:
            return vk::BlendFactor::eSrcAlpha;
        case Factor::eOneMinusSrcAlpha:
            return vk::BlendFactor::eOneMinusSrcAlpha;
        case Factor::eDstAlpha:
            return vk::BlendFactor::eDstAlpha;
        case Factor::eOneMinusDstAlpha:
            return vk::BlendFactor::eOneMinusDstAlpha;
        case Factor::eConstantColor:
            return vk::BlendFactor::eConstantColor;
        case Factor::eOneMinusConstantColor:
            return vk::BlendFactor::eOneMinusConstantColor;
        case Factor::eConstantAlpha:
            return vk::BlendFactor::eConstantAlpha;
        case Factor::eOneMinusConstantAlpha:
            return vk::BlendFactor::eOneMinusConstantAlpha;
        case Factor::eSrcAlphaSaturate:
            return vk::BlendFactor::eSrcAlphaSaturate;
    }
}

static vk::StencilOp VK_ConvertStencilOp(StencilOp op) {
    switch (op) {
        case StencilOp::eKeep:
            return vk::StencilOp::eKeep;
        case StencilOp::eZero:
            return vk::StencilOp::eZero;
        case StencilOp::eReplace:
            return vk::StencilOp::eReplace;
        case StencilOp::eIncrementAndClamp:
            return vk::StencilOp::eIncrementAndClamp;
        case StencilOp::eDecrementAndClamp:
            return vk::StencilOp::eDecrementAndClamp;
        case StencilOp::eInvert:
            return vk::StencilOp::eInvert;
        case StencilOp::eIncrementAndWrap:
            return vk::StencilOp::eIncrementAndWrap;
        case StencilOp::eDecrementAndWrap:
            return vk::StencilOp::eDecrementAndWrap;
    }
}

/// Compute the set of queue family indices for a given physical device.
static QueueFamilyIndices computeIndices(vk::PhysicalDevice dev) {
    QueueFamilyIndices indices = {};

    std::vector<vk::QueueFamilyProperties> families = 
        dev.getQueueFamilyProperties();

    for (std::size_t i = 0; i < families.size(); ++i) {
        if (families[i].queueFlags & vk::QueueFlagBits::eTransfer)
            indices.graphics = i;

        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphics = i;

        if (families[i].queueFlags & vk::QueueFlagBits::eCompute)
            indices.compute = i;

        // If we've found all indices now, stop.
        if (indices.complete())
            break;
    }
    
    return indices;
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugMessengerCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
    // @Todo: Let user provide their own callback. 
    std::cerr << std::format("(Vulkan) {}", pCallbackData->pMessage);
    return vk::False;
}

#endif // IGPU_VULKAN

struct AllocationInfo final {
    MemoryType type;
    ptr device;
    void* host;

#ifdef IGPU_VULKAN
    vk::Buffer buffer;
    VmaAllocation alloc;
#endif // IGPU_VULKAN
};

struct Texture_T final {
    TextureInfo info;

    /// If this texture is borrowed e.g. from a swapchain.
    bool borrowed;

#ifdef IGPU_VULKAN
    vk::Image image;
    VmaAllocation alloc;
#endif // IGPU_VULKAN

    std::vector<TextureView> views;
};

struct TextureView_T final {
    Texture texture;
    TextureViewInfo info;

#ifdef IGPU_VULKAN
    vk::ImageView view;
#endif // IGPU_VULKAN
};

struct Queue_T final {
    QueueType type;

#ifdef IGPU_VULKAN
    vk::Queue queue;
    uint32_t family;
#endif // IGPU_VULKAN
};

struct Semaphore_T final {
#ifdef IGPU_VULKAN
    vk::Semaphore sema;
#endif // IGPU_VULKAN
};

struct Pipeline_T final {
    PipelineType type;

#ifdef IGPU_VULKAN
    vk::Pipeline pipeline;
#endif // IGPU_VULKAN
};

struct CommandList_T final {
#ifdef IGPU_VULKAN
    vk::CommandPool pool;
    vk::CommandBuffer buffer;
#endif // IGPU_VULKAN
};

struct RenderingDevice_T final {
#ifdef IGPU_VULKAN
    struct Swapchain final {
        vk::SwapchainKHR handle = nullptr;
        std::vector<Texture> textures = {};
        std::vector<vk::Semaphore> acquires = {};
        std::vector<vk::Semaphore> presents = {};
        uint32_t index = 0;
    };

    struct DescriptorBuffer final {
        vk::Buffer buffer = nullptr;
        VmaAllocation alloc = nullptr;
        ptr device = 0;
        void* host = nullptr;
    };

    vk::Instance instance = nullptr;
    vk::DebugUtilsMessengerEXT messenger = nullptr;
    vk::SurfaceKHR surface = nullptr;

    vk::PhysicalDevice physical_device = nullptr;
    vk::Device device = nullptr;

    VmaAllocator vma = nullptr;

    Swapchain swapchain = {};

    uint32_t num_sampled_images = 0;
    uint32_t num_storage_images = 0;
    uint32_t num_samplers = 0;

    DescriptorBuffer dbuffer = {};

    vk::DescriptorSetLayout descriptor_layout = nullptr;

    vk::PipelineLayout pipeline_layout = nullptr;
#endif // IGPU_VULKAN

    // Backend-agnostic fields.
    std::mutex memory_mutex;
    std::vector<Queue_T> queues = {};
    std::unordered_map<ptr, AllocationInfo> allocations = {};
    std::unordered_map<void*, ptr> addresses = {};

    Pool<CommandList_T> lists = {};
    Pool<Texture_T> textures = {};
    Pool<TextureView_T> views = {};
    Pool<Semaphore_T> semaphores = {};
    Pool<Pipeline_T> pipelines = {};

#ifdef IGPU_VULKAN
    void VK_CreateCore(const RenderingDeviceInfo& info) {
        std::vector<const char*> layers = {};
        std::vector<const char*> extensions = { 
            VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME 
        };

        if (info.validation) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
            extensions.push_back("VK_EXT_debug_utils");
        }

        auto app_info = vk::ApplicationInfo {}
            .setPApplicationName("App")
            .setPEngineName("IGPU")
            .setApiVersion(vk::ApiVersion13)
            .setApplicationVersion(vk::makeVersion(1, 0, 0))
            .setEngineVersion(vk::makeVersion(1, 0, 0));

        auto instance_info = vk::InstanceCreateInfo {}
            .setPApplicationInfo(&app_info);

    #ifdef IGPU_GLFW

        // Fetch any VK extensions required by GLFW.
        uint32_t num_exts = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&num_exts);
        for (uint32_t i = 0; i < num_exts; ++i)
            extensions.push_back(exts[i]);

    #endif // IGPU_GLFW

        auto msger_info = vk::DebugUtilsMessengerCreateInfoEXT {};

        // If VK validation layers were requested, then setup the debug 
        // messenger as well. 
        if (info.validation) {
            msger_info.setPfnUserCallback(DebugMessengerCallback);
            msger_info.setMessageSeverity(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose 
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo 
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning 
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
            msger_info.setMessageType(
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral 
                | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation 
                | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);

            instance_info.setPNext(&msger_info);
        }

        instance_info.setEnabledExtensionCount(extensions.size());
        instance_info.setPpEnabledExtensionNames(extensions.data());

        instance_info.setEnabledLayerCount(layers.size());
        instance_info.setPpEnabledLayerNames(layers.data());

        VK_CHECK(vk::createInstance(&instance_info, nullptr, &instance));

        // If validation layers were requested, then actually create the 
        // debug messenger now since the instance exists.
        if (info.validation) {
            auto creator = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

            IGPU_ASSERT(creator, 
                        "Failed to resolve 'vkCreateDebugUtilsMessengerEXT'!");

            creator(
                instance, 
                reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&msger_info), 
                nullptr,
                reinterpret_cast<VkDebugUtilsMessengerEXT*>(&messenger));
        }

    #ifdef IGPU_GLFW

        IGPU_ASSERT(info.window, "Window handle cannot be null!");

        // If GLFW is supported, then we assume the info window belongs to
        // the library and create the VK surface accordingly.
        VK_CHECK(vk::Result(glfwCreateWindowSurface(
            instance, 
            reinterpret_cast<GLFWwindow*>(info.window), 
            nullptr, 
            reinterpret_cast<VkSurfaceKHR*>(&surface))));

    #endif // IGPU_GLFW
    }

    void VK_CreateDevice(const RenderingDeviceInfo& info) {
        std::vector<const char*> extensions = {};

        std::vector<vk::PhysicalDevice> devices = 
            instance.enumeratePhysicalDevices().value;

        // @Todo: Replace with logging callback.
        IGPU_ASSERT(!devices.empty(), "No physical devices were found!");

        auto it = std::ranges::find_if(devices, [&](vk::PhysicalDevice dev) -> bool {
            std::set<std::string> required(extensions.begin(), extensions.end());
            std::vector<vk::ExtensionProperties> available = 
                dev.enumerateDeviceExtensionProperties().value;

            // Determine which of the required extensions are available from 
            // this device.
            for (const vk::ExtensionProperties& ext : available)
                required.erase(ext.extensionName);

            // One or more required extensions is not present; skip.
            if (!required.empty())
                return false;

            // Devices must support VK 1.3.
            if (dev.getProperties().apiVersion < vk::ApiVersion13)
                return false;

            // Devices must support queues for graphics, compute, and transfer
            // work.
            if (!computeIndices(dev).complete())
                return false;

            return true;
        });

        // @Todo: Replace with error callback.
        IGPU_ASSERT(it != devices.end(), "Failed to find a suitable GPU.");

        // Physical device chosen!
        physical_device = *it;

        QueueFamilyIndices indices = computeIndices(physical_device);
        IGPU_ASSERT(indices.complete(), "GPU queue families are incomplete!");

        float priority = 1.f;
        std::vector<vk::DeviceQueueCreateInfo> queue_infos = {
            vk::DeviceQueueCreateInfo {}
                .setQueueCount(1)
                .setQueueFamilyIndex(indices.graphics)
                .setPQueuePriorities(&priority),
            vk::DeviceQueueCreateInfo {}
                .setQueueCount(1)
                .setQueueFamilyIndex(indices.compute)
                .setPQueuePriorities(&priority),
            vk::DeviceQueueCreateInfo {}
                .setQueueCount(1)
                .setQueueFamilyIndex(indices.transfer)
                .setPQueuePriorities(&priority),
        };

        auto feats = vk::PhysicalDeviceFeatures {}
            .setDepthClamp(vk::True)
            .setDepthBiasClamp(vk::True)
            .setSamplerAnisotropy(vk::True);
            
        auto feats11 = vk::PhysicalDeviceVulkan11Features {}
            .setShaderDrawParameters(vk::True);

        auto feats12 = vk::PhysicalDeviceVulkan12Features {}
            .setBufferDeviceAddress(vk::True)
            .setDescriptorIndexing(vk::True)
            .setDescriptorBindingPartiallyBound(vk::True)
            .setShaderSampledImageArrayNonUniformIndexing(vk::True)
            .setDescriptorBindingSampledImageUpdateAfterBind(vk::True)
            .setRuntimeDescriptorArray(vk::True)
            .setDescriptorBindingVariableDescriptorCount(vk::True)
            .setTimelineSemaphore(vk::True)
            .setPNext(&feats11);

        auto feats13 = vk::PhysicalDeviceVulkan13Features {}
            .setDynamicRendering(vk::True)
            .setSynchronization2(vk::True)
            .setPNext(&feats12);

        auto feats2 = vk::PhysicalDeviceFeatures2 {}
            .setFeatures(feats)
            .setPNext(&feats13);

        auto device_info = vk::DeviceCreateInfo {}
            .setQueueCreateInfoCount(queue_infos.size())
            .setPQueueCreateInfos(queue_infos.data())
            .setEnabledExtensionCount(extensions.size())
            .setPpEnabledExtensionNames(extensions.data())
            .setPNext(&feats2);

        VK_CHECK(physical_device.createDevice(&device_info, nullptr, &device));

        Queue_T graphics = {};
        graphics.type = QueueType::eGraphics;
        graphics.queue = device.getQueue(indices.graphics, 0);
        graphics.family = indices.graphics;
        queues[static_cast<uint32_t>(QueueType::eGraphics)] = graphics;

        Queue_T compute = {};
        compute.type = QueueType::eCompute;
        compute.queue = device.getQueue(indices.compute, 0);
        compute.family = indices.compute;
        queues[static_cast<uint32_t>(QueueType::eCompute)] = compute;

        Queue_T transfer = {};
        transfer.type = QueueType::eTransfer;
        transfer.queue = device.getQueue(indices.transfer, 0);
        transfer.family = indices.transfer;
        queues[static_cast<uint32_t>(QueueType::eTransfer)] = transfer;
    }

    void VK_CreateAllocator(const RenderingDeviceInfo& info) {
        VmaVulkanFunctions funcs = {};
        funcs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        funcs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo vma_info = {};
        vma_info.instance = instance;
        vma_info.physicalDevice = physical_device;
        vma_info.device = device;
        vma_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vma_info.pVulkanFunctions = &funcs;

        VK_CHECK(vk::Result(vmaCreateAllocator(&vma_info, &vma)));
    }

    void VK_CreateSwapchain(const RenderingDeviceInfo& info) {
        Swapchain& sw = swapchain;

        vk::SurfaceCapabilitiesKHR caps = {};
        VK_CHECK(physical_device.getSurfaceCapabilitiesKHR(surface, &caps));

        auto swapchain_info = vk::SwapchainCreateInfoKHR {}
            .setSurface(surface)
            .setMinImageCount(caps.minImageCount)
            .setImageFormat(VK_ConvertFormat(info.format))
            .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
            .setImageExtent({ info.width, info.height })
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setPreTransform(caps.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setClipped(vk::True)
            .setPresentMode(VK_ConvertPresent(info.present));

        VK_CHECK(device.createSwapchainKHR(
            &swapchain_info, 
            nullptr, 
            &swapchain.handle));

        // Fetch the number of swapchain images in use.
        uint32_t num_images = 0;
        VK_CHECK(device.getSwapchainImagesKHR(
            swapchain.handle, 
            &num_images, 
            nullptr));

        // Fetch the actual swapchain image handles.
        std::vector<vk::Image> images(num_images);
        VK_CHECK(device.getSwapchainImagesKHR(
            swapchain.handle, 
            &num_images, 
            images.data()));

        sw.textures.resize(num_images);
        sw.acquires.resize(num_images);
        sw.presents.resize(num_images);

        auto sema_info = vk::SemaphoreCreateInfo {};

        for (std::size_t i = 0; i < num_images; ++i) {
            vk::Image image = images[i];

            Texture_T texture = {};
            texture.image = image;
            texture.alloc = nullptr;
            texture.views = {};
            texture.borrowed = true;
            texture.info = TextureInfo {
                .type = TextureType::e2D,
                .format = info.format,
                .usage = Usage::eColorAttachment,
                .dimensions = { info.width, info.height },
            };

            sw.textures[i] = textures.add(texture);

            VK_CHECK(device.createSemaphore(
                &sema_info, 
                nullptr, 
                &sw.acquires[i]));
            
            VK_CHECK(device.createSemaphore(
                &sema_info, 
                nullptr, 
                &sw.presents[i]));
        }
    }

    void VK_CreateDescriptors(const RenderingDeviceInfo& info) {
        vk::PhysicalDeviceDescriptorBufferPropertiesEXT db_props = {};
        vk::PhysicalDeviceProperties2 props = {};
        props.pNext = &db_props;

        physical_device.getProperties2(&props);

        const vk::PhysicalDeviceLimits& limits = props.properties.limits;

        num_sampled_images = limits.maxPerStageDescriptorSampledImages;
        num_storage_images = limits.maxPerStageDescriptorStorageImages;
        num_samplers = limits.maxPerStageDescriptorSamplers;

        std::array<vk::DescriptorBindingFlags, 3> flags = {
            vk::DescriptorBindingFlagBits::ePartiallyBound,
            vk::DescriptorBindingFlagBits::ePartiallyBound,
            vk::DescriptorBindingFlagBits::ePartiallyBound,
        };

        std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {
            vk::DescriptorSetLayoutBinding {}
                .setBinding(TEXTURE_BINDING)
                .setDescriptorType(vk::DescriptorType::eSampledImage)
                .setDescriptorCount(num_sampled_images)
                .setStageFlags(vk::ShaderStageFlagBits::eAll),
            vk::DescriptorSetLayoutBinding {}
                .setBinding(RW_TEXTURE_BINDING)
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setDescriptorCount(num_storage_images)
                .setStageFlags(vk::ShaderStageFlagBits::eAll),
            vk::DescriptorSetLayoutBinding {}
                .setBinding(SAMPLER_BINDING)
                .setDescriptorType(vk::DescriptorType::eSampler)
                .setDescriptorCount(num_samplers)
                .setStageFlags(vk::ShaderStageFlagBits::eAll),
        };

        auto flags_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo {}
            .setBindingCount(flags.size())
            .setPBindingFlags(flags.data());

        auto layout_info = vk::DescriptorSetLayoutCreateInfo {}
            .setBindingCount(bindings.size())
            .setPBindings(bindings.data())
            .setPNext(&flags_info);

        VK_CHECK(device.createDescriptorSetLayout(
            &layout_info, 
            nullptr, 
            &descriptor_layout));

        const auto align = [](uint64_t size, uint64_t align) -> uint64_t {
            return (size + align - 1) & ~(align - 1);
        };

        vk::DeviceSize layout_size = 0;
        device.getDescriptorSetLayoutSizeEXT(descriptor_layout, &layout_size);

        vk::DeviceSize db_size = align(
            layout_size, db_props.descriptorBufferOffsetAlignment);

        auto db_info = vk::BufferCreateInfo {}
            .setSize(db_size)
            .setUsage(vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT 
                    | vk::BufferUsageFlagBits::eShaderDeviceAddress);

        VmaAllocationCreateInfo vma_info = {};
        vma_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vma_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        VmaAllocationInfo alloc_info = {};
        VK_CHECK(vk::Result(vmaCreateBuffer(
            vma, 
            reinterpret_cast<VkBufferCreateInfo*>(&db_info), 
            &vma_info, 
            reinterpret_cast<VkBuffer*>(&dbuffer.buffer), 
            &dbuffer.alloc, 
            &alloc_info)));

        dbuffer.host = alloc_info.pMappedData;
        dbuffer.device = device.getBufferAddress(vk::BufferDeviceAddressInfo {}
            .setBuffer(dbuffer.buffer));
    }

    void VK_CreateLayout(const RenderingDeviceInfo& info) {
        std::vector<vk::PushConstantRange> ranges = {};

        for (int32_t i = 0; i < static_cast<int32_t>(Shader::eCount); ++i) {
            ranges.push_back(vk::PushConstantRange {}
                .setSize(sizeof(vk::DeviceAddress))
                .setOffset(i * sizeof(vk::DeviceAddress))
                .setStageFlags(VK_ConvertShaderStage(static_cast<Shader>(i))));
        }

        auto layout_info = vk::PipelineLayoutCreateInfo {}
            .setPushConstantRangeCount(ranges.size())
            .setPPushConstantRanges(ranges.data())
            .setSetLayoutCount(1)
            .setPSetLayouts(nullptr);

        VK_CHECK(device.createPipelineLayout(
            &layout_info, 
            nullptr, 
            &pipeline_layout));
    }

#endif // IGPU_VULKAN

    explicit RenderingDevice_T(const RenderingDeviceInfo& info) {
    #ifdef IGPU_VULKAN

        VK_CreateCore(info);
        VK_CreateDevice(info);
        VK_CreateAllocator(info);
        VK_CreateSwapchain(info);
        VK_CreateDescriptors(info);
        VK_CreateLayout(info);

    #endif // IGPU_VULKAN
    }

    ~RenderingDevice_T() {
    #ifdef IGPU_VULKAN

        // @Todo: Implement rest of destruction.

        if (pipeline_layout) {
            device.destroyPipelineLayout(pipeline_layout);
            pipeline_layout = nullptr;
        }

        if (dbuffer.buffer && dbuffer.alloc) {
            vmaDestroyBuffer(vma, dbuffer.buffer, dbuffer.alloc);
            dbuffer.buffer = nullptr;
            dbuffer.alloc = nullptr;
        }

        if (descriptor_layout) {
            device.destroyDescriptorSetLayout(descriptor_layout);
            descriptor_layout = nullptr;
        }

        if (instance) {
            instance.destroy();
            instance = nullptr;
        }

    #endif // IGPU_VULKAN
    }
};

#ifdef IGPU_VULKAN

static vk::PipelineShaderStageCreateInfo createShaderStage(
        RenderingDevice_T& device, 
        std::string_view spirv, 
        vk::ShaderStageFlagBits stage) {
    auto stage_info = vk::PipelineShaderStageCreateInfo {}
        .setStage(stage)
        .setPName("main");

    auto module_info = vk::ShaderModuleCreateInfo {}
        .setCodeSize(spirv.length())
        .setPCode(reinterpret_cast<const uint32_t*>(spirv.data()));

    VK_CHECK(device.device.createShaderModule(
        &module_info, 
        nullptr, 
        &stage_info.module));
    
    return stage_info;
}

#endif // IGPU_VULKAN

//>==---------------------------------------------------------------------------
//                          RenderingDevice Implementation
//>==---------------------------------------------------------------------------

RenderingDevice* RenderingDevice::Create(const RenderingDeviceInfo& info) {
    RenderingDevice_T* impl = new (std::nothrow) RenderingDevice_T(info);
    if (!impl)
        return nullptr;

    return new (std::nothrow) RenderingDevice(info, impl);
}

RenderingDevice::~RenderingDevice() {
    if (m_impl) {
        delete m_impl;
        m_impl = nullptr;
    }
}

ptr RenderingDevice::malloc(uint64_t size, MemoryType type) {
    std::lock_guard<std::mutex> lock(m_impl->memory_mutex);

    ptr addr = 0;

    AllocationInfo alloc = {};
    alloc.type = type;

#ifdef IGPU_VULKAN

    static constexpr vk::BufferUsageFlags usage = 
        vk::BufferUsageFlagBits::eStorageBuffer 
      | vk::BufferUsageFlagBits::eIndirectBuffer
      | vk::BufferUsageFlagBits::eShaderDeviceAddress 
      | vk::BufferUsageFlagBits::eTransferSrc 
      | vk::BufferUsageFlagBits::eTransferDst;

    vk::MemoryPropertyFlags mem_flags = {};
    VmaMemoryUsage mem_usage = {};

    switch (type) {
    case MemoryType::eDefault:
        mem_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        mem_flags = vk::MemoryPropertyFlagBits::eHostVisible 
                  | vk::MemoryPropertyFlagBits::eHostCoherent;
        break;
    case MemoryType::eDevice:
        mem_usage = VMA_MEMORY_USAGE_GPU_ONLY;
        mem_flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        break;
    case MemoryType::eReadback:
        mem_usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        mem_flags = vk::MemoryPropertyFlagBits::eHostVisible 
                  | vk::MemoryPropertyFlagBits::eHostCoherent 
                  | vk::MemoryPropertyFlagBits::eHostCached;
        break;
    }
 
    auto buffer_info = vk::BufferCreateInfo {}
        .setUsage(usage)
        .setSize(static_cast<vk::DeviceSize>(size))
        .setSharingMode(vk::SharingMode::eConcurrent);
    /* @Todo: Update these.
    buffer_info.queueFamilyIndexCount = ...;
    buffer_info.pQueueFamilyIndices = ...;
    */

    // If the request is not asking for device-only memory, then map the 
    // memory at the point of allocation so the host pointer may be
    // registered.
    VmaAllocationCreateFlags alloc_flags = {};
    if (type != MemoryType::eDevice)
        alloc_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationCreateInfo vma_info = {};
    vma_info.flags = alloc_flags;
    vma_info.requiredFlags = static_cast<VkBufferCreateFlags>(mem_flags);
    vma_info.usage = mem_usage;

    VmaAllocationInfo alloc_info = {};

    vk::Result result = vk::Result(vmaCreateBuffer(
        m_impl->vma, 
        reinterpret_cast<VkBufferCreateInfo*>(&buffer_info), 
        &vma_info, 
        reinterpret_cast<VkBuffer*>(&alloc.buffer), 
        &alloc.alloc, 
        &alloc_info));

    if (result != vk::Result::eSuccess)
        return 0;
    
    vk::BufferDeviceAddressInfo addr_info = {};
    addr_info.buffer = alloc.buffer;

    // Get the device address for the new memory buffer.
    addr = static_cast<ptr>(m_impl->device.getBufferAddress(&addr_info));
    if (!addr) {
        vmaDestroyBuffer(m_impl->vma, alloc.buffer, alloc.alloc);
        return 0;
    }

    alloc.device = addr;

    // If the memory is not device only, then register the host address.
    if (type != MemoryType::eDevice) {
        alloc.host = alloc_info.pMappedData;
        m_impl->addresses.emplace(alloc.host, addr);
    } else {
        alloc.host = nullptr;
    }

#endif // IGPU_VULKAN

    m_impl->allocations.emplace(addr, alloc);
    return addr;
}

void RenderingDevice::free(ptr p) {
    std::lock_guard<std::mutex> lock(m_impl->memory_mutex);

    // Look for the address in the device allocation table.
    auto it = m_impl->allocations.find(p);
    if (it == m_impl->allocations.end())
        return;

    AllocationInfo info = it->second;

#ifdef IGPU_VULKAN

    if (info.buffer && info.alloc)
        vmaDestroyBuffer(m_impl->vma, info.buffer, info.alloc);

#endif // IGPU_VULKAN

    m_impl->allocations.erase(it);
}

void* RenderingDevice::deviceToHostAddress(ptr p) {
    std::lock_guard<std::mutex> lock(m_impl->memory_mutex);

    // Look for the device address in the allocation table.
    auto it = m_impl->allocations.find(p);
    if (it != m_impl->allocations.end())
        return it->second.host;

    return nullptr;
}

ptr RenderingDevice::hostToDeviceAddress(void* p) {
    std::lock_guard<std::mutex> lock(m_impl->memory_mutex);

    // Look for the host address in the address translation table.
    auto it = m_impl->addresses.find(p);
    if (it != m_impl->addresses.end())
        return it->second;
    
    return 0;
}

Texture RenderingDevice::acquireSwapchainTexture() {
#ifdef IGPU_VULKAN

    RenderingDevice_T::Swapchain& swapchain = m_impl->swapchain;

    uint32_t index;
    VK_CHECK(m_impl->device.acquireNextImageKHR(
        swapchain.handle,
        UINT64_MAX, 
        swapchain.acquires[swapchain.index], 
        nullptr, 
        &index));

    swapchain.index = index;
    return swapchain.textures[index];

#endif // IGPU_VULKAN
}

void RenderingDevice::present(QueueType queue) {
#ifdef IGPU_VULKAN

    RenderingDevice_T::Swapchain& swapchain = m_impl->swapchain;

    Queue_T i_queue = m_impl->queues[static_cast<uint32_t>(queue)];
    uint32_t index = swapchain.index;

    VK_CHECK(i_queue.queue.presentKHR(vk::PresentInfoKHR {}
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&swapchain.presents[index])
        .setSwapchainCount(1)
        .setPSwapchains(&swapchain.handle)
        .setPImageIndices(&index)));

#endif // IGPU_VULKAN
}

void RenderingDevice::resizeSwapchain(uint32_t width, uint32_t height) {
#ifdef IGPU_VULKAN

    // @Todo: Shouldn't abort on failure.
    VK_CHECK(m_impl->device.waitIdle());

    vk::SurfaceCapabilitiesKHR caps = {};
    VK_CHECK(m_impl->physical_device.getSurfaceCapabilitiesKHR(
        m_impl->surface, 
        &caps));

    RenderingDevice_T::Swapchain& swapchain = m_impl->swapchain;
    vk::SwapchainKHR old_swapchain = swapchain.handle;

    auto info = vk::SwapchainCreateInfoKHR {}
        .setSurface(m_impl->surface)
        .setMinImageCount(caps.minImageCount)
        .setImageFormat(VK_ConvertFormat(m_info.format))
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageExtent({ width, height })
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setPreTransform(caps.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(vk::True)
        .setPresentMode(VK_ConvertPresent(m_info.present))
        .setOldSwapchain(old_swapchain);

    VK_CHECK(m_impl->device.createSwapchainKHR(
        &info, 
        nullptr, 
        &swapchain.handle));

    m_impl->device.destroySwapchainKHR(old_swapchain);

    for (vk::Semaphore sema : swapchain.acquires) {
        m_impl->device.destroySemaphore(sema);
    }

    for (vk::Semaphore sema : swapchain.presents) {
        m_impl->device.destroySemaphore(sema);
    }

    // Fetch the number of swapchain images in use.
    uint32_t num_images = 0;
    VK_CHECK(m_impl->device.getSwapchainImagesKHR(
        swapchain.handle, 
        &num_images, 
        nullptr));

    // Fetch the actual swapchain image handles.
    std::vector<vk::Image> images(num_images);
    VK_CHECK(m_impl->device.getSwapchainImagesKHR(
        swapchain.handle, 
        &num_images, 
        images.data()));

    for (Texture texture : swapchain.textures) {
        freeTexture(texture);
    }
 
    swapchain.textures.resize(num_images);
    swapchain.acquires.resize(num_images);
    swapchain.presents.resize(num_images);

    auto sema_info = vk::SemaphoreCreateInfo {};

    for (std::size_t i = 0; i < num_images; ++i) {
        vk::Image image = images[i];

        Texture_T texture = {};
        texture.image = image;
        texture.alloc = nullptr;
        texture.views = {};
        texture.borrowed = true;
        texture.info = TextureInfo {
            .type = TextureType::e2D,
            .format = m_info.format,
            .usage = Usage::eColorAttachment,
            .dimensions = { width, height },
        };

        swapchain.textures[i] = m_impl->textures.add(texture);

        VK_CHECK(m_impl->device.createSemaphore(
            &sema_info, 
            nullptr, 
            &swapchain.acquires[i]));
        
        VK_CHECK(m_impl->device.createSemaphore(
            &sema_info, 
            nullptr, 
            &swapchain.presents[i]));
    }

#endif // IGPU_VULKAN
}

Texture RenderingDevice::createTexture(const TextureInfo& info) {
    Texture_T texture = {};
    texture.info = info;

#ifdef IGPU_VULKAN

    // Enable the cube compatability flag if necessary.
    vk::ImageCreateFlags flags = {};
    if (info.type == TextureType::eCube || info.type == TextureType::eCubeArray)
        flags |= vk::ImageCreateFlagBits::eCubeCompatible;

    auto image_info = vk::ImageCreateInfo {}
        .setFlags(flags)
        .setFormat(VK_ConvertFormat(info.format))
        .setExtent({ info.dimensions[0], info.dimensions[1], info.dimensions[3] })
        .setImageType(VK_ConvertTextureType(info.type))
        .setMipLevels(info.mip_count)
        .setArrayLayers(info.layer_count)
        .setSamples(VK_ConvertSampleCount(info.sample_count))
        .setUsage(VK_ConvertUsage(info.usage))
        .setSharingMode(vk::SharingMode::eConcurrent)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setQueueFamilyIndexCount(1 /* @Todo */)
        .setPQueueFamilyIndices(nullptr /* @Todo */);

    VmaAllocationCreateInfo vma_info = {};
    vma_info.requiredFlags = VMA_MEMORY_USAGE_GPU_ONLY;

    VmaAllocationInfo alloc_info = {};

    vk::Result result = vk::Result(vmaCreateImage(
        m_impl->vma,
        reinterpret_cast<VkImageCreateInfo*>(&image_info),
        &vma_info,
        reinterpret_cast<VkImage*>(&texture.image),
        &texture.alloc,
        &alloc_info));

    if (result != vk::Result::eSuccess)
        return 0;

    CommandList cmd = beginRecording(QueueType::eTransfer);
    if (!cmd) {
        vmaDestroyImage(m_impl->vma, texture.image, texture.alloc);
        return 0;
    }

    auto barrier = vk::ImageMemoryBarrier2 {}
        .setImage(texture.image)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
        .setDstStageMask(vk::PipelineStageFlagBits2::eAllGraphics)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setSubresourceRange(vk::ImageSubresourceRange {}
            .setBaseMipLevel(0)
            .setBaseArrayLayer(0)
            .setLevelCount(info.mip_count)
            .setLayerCount(info.layer_count)
            .setAspectMask(VK_FormatToAspectMask(info.format)));

    m_impl->lists.get(cmd).buffer.pipelineBarrier2(vk::DependencyInfo {}
        .setImageMemoryBarrierCount(1)
        .setPImageMemoryBarriers(&barrier));

    submit(QueueType::eTransfer, { cmd });

#endif // IGPU_VULKAN

    return m_impl->textures.add(texture);
}

void RenderingDevice::freeTexture(Texture texture) {
    Texture_T i_texture = m_impl->textures.remove(texture);

#ifdef IGPU_VULKAN

    if (!i_texture.borrowed) {
        if (i_texture.image && i_texture.alloc)
            vmaDestroyImage(m_impl->vma, i_texture.image, i_texture.alloc);
    }

    // @Todo: Destroy all views.

#endif // IGPU_VULKAN
}

CommandList RenderingDevice::beginRecording(QueueType queue) {
    CommandList_T list = {};
    Queue_T i_queue = m_impl->queues[static_cast<uint32_t>(queue)];

#ifdef IGPU_VULKAN

    auto pool_info = vk::CommandPoolCreateInfo {}
        .setFlags(vk::CommandPoolCreateFlagBits::eTransient)
        .setQueueFamilyIndex(i_queue.family);
    
    vk::Result result = m_impl->device.createCommandPool(
        &pool_info, 
        nullptr, 
        &list.pool);

    if (result != vk::Result::eSuccess)
        return 0;

    auto buffer_info = vk::CommandBufferAllocateInfo {}
        .setCommandBufferCount(1)
        .setCommandPool(list.pool)
        .setLevel(vk::CommandBufferLevel::ePrimary);

    result = m_impl->device.allocateCommandBuffers(&buffer_info, &list.buffer);
    if (result != vk::Result::eSuccess) {
        m_impl->device.destroyCommandPool(list.pool);
        return 0;
    }

    auto begin_info = vk::CommandBufferBeginInfo {}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    if (list.buffer.begin(begin_info) != vk::Result::eSuccess) {
        m_impl->device.destroyCommandPool(list.pool);
        return 0;
    }

#endif // IGPU_VULKAN

    return m_impl->lists.add(list);
}

void RenderingDevice::submit(QueueType queue, 
                             const std::vector<CommandList>& lists,
                             TimelinePair signal, 
                             TimelinePair wait) {
    Queue_T i_queue = m_impl->queues[static_cast<uint32_t>(queue)];

#ifdef IGPU_VULKAN

    std::vector<vk::CommandBufferSubmitInfo> buffers = {};
    buffers.reserve(lists.size());

    for (CommandList list : lists) {
        buffers.push_back(vk::CommandBufferSubmitInfo {}
            .setCommandBuffer(m_impl->lists.get(list).buffer));
    }

    auto submit_info = vk::SubmitInfo2 {}
        .setCommandBufferInfoCount(static_cast<uint32_t>(buffers.size()))
        .setPCommandBufferInfos(buffers.data());

    if (signal.sema) {
        auto signal_info = vk::SemaphoreSubmitInfo {}
            .setSemaphore(m_impl->semaphores.get(signal.sema).sema)
            .setValue(signal.value);

        submit_info.setSignalSemaphoreInfoCount(1);
        submit_info.setPSignalSemaphoreInfos(&signal_info);
    }

    if (wait.sema) {
        auto wait_info = vk::SemaphoreSubmitInfo {}
            .setSemaphore(m_impl->semaphores.get(wait.sema).sema)
            .setValue(wait.value);

        submit_info.setWaitSemaphoreInfoCount(1);
        submit_info.setPWaitSemaphoreInfos(&wait_info);
    }

    // @Todo: Return custom result value.
    vk::Result result = i_queue.queue.submit2(submit_info);
    IGPU_ASSERT(result == vk::Result::eSuccess, 
                "Failed to submit command lists.");

#endif // IGPU_VULKAN
}

Semaphore RenderingDevice::createSemaphore(uint64_t value) {
    Semaphore_T sema = {};

#ifdef IGPU_VULKAN

    auto type_info = vk::SemaphoreTypeCreateInfo {}
        .setSemaphoreType(vk::SemaphoreType::eTimeline)
        .setInitialValue(value);

    auto sema_info = vk::SemaphoreCreateInfo {}
        .setPNext(&type_info);

    vk::Result result = m_impl->device.createSemaphore(
        &sema_info, 
        nullptr, 
        &sema.sema);

    if (result != vk::Result::eSuccess)
        return 0;

#endif // IGPU_VULKAN

    return m_impl->semaphores.add(sema);
}

void RenderingDevice::freeSemaphore(Semaphore sema) {
    Semaphore_T i_sema = m_impl->semaphores.remove(sema);

#ifdef IGPU_VULKAN

    if (i_sema.sema)
        m_impl->device.destroySemaphore(i_sema.sema);

#endif // IGPU_VULKAN
}

void RenderingDevice::waitSemaphore(Semaphore sema, uint64_t value) {
    Semaphore_T& i_sema = m_impl->semaphores.get(sema);

#ifdef IGPU_VULKAN

    auto wait_info = vk::SemaphoreWaitInfo {}
        .setSemaphoreCount(1)
        .setPSemaphores(&i_sema.sema)
        .setValues(value);

    // @Todo: Return custom result value.
    (void) m_impl->device.waitSemaphores(&wait_info, UINT64_C(1'000'000'000));

#endif // IGPU_VULKAN
}

Pipeline RenderingDevice::createComputePipeline(std::string_view compute) {
    Pipeline_T pipeline = {};
    pipeline.type = PipelineType::eCompute;

#ifdef IGPU_VULKAN

    auto pipeline_info = vk::ComputePipelineCreateInfo {}
        .setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT)
        .setLayout(m_impl->pipeline_layout)
        .setStage(createShaderStage(*m_impl, compute, vk::ShaderStageFlagBits::eCompute));

    if (!pipeline_info.stage.module)
        return 0;

    vk::Result result = m_impl->device.createComputePipelines(
        nullptr, 
        1, 
        &pipeline_info, 
        nullptr, 
        &pipeline.pipeline);

    // Shader module is useless now.
    if (pipeline_info.stage.module)
        m_impl->device.destroyShaderModule(pipeline_info.stage.module);

    if (result != vk::Result::eSuccess)
        return 0;

#endif // IGPU_VULKAN

    return m_impl->pipelines.add(pipeline);
}

Pipeline RenderingDevice::createGraphicsPipeline(std::string_view vertex, 
                                                 std::string_view fragment, 
                                                 const RasterInfo& info) {
    Pipeline_T pipeline = {};
    pipeline.type = PipelineType::eGraphics;

#ifdef IGPU_VULKAN

    // Collect color attachment formats as Vulkan formats.
    std::vector<vk::Format> formats = {};
    for (const AttachmentInfo& att : info.atts) {
        formats.push_back(VK_ConvertFormat(att.format));
    }

    std::vector<vk::PipelineShaderStageCreateInfo> stages = {
        createShaderStage(*m_impl, vertex, vk::ShaderStageFlagBits::eVertex),
        createShaderStage(*m_impl, fragment, vk::ShaderStageFlagBits::eFragment),
    };

    std::array<vk::DynamicState, 7> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
        vk::DynamicState::eDepthBias,
        vk::DynamicState::eDepthTestEnable,
        vk::DynamicState::eDepthWriteEnable,
        vk::DynamicState::eDepthCompareOp,
        vk::DynamicState::eStencilReference,
    };

    auto input_info = vk::PipelineVertexInputStateCreateInfo {};

    auto assembly_info = vk::PipelineInputAssemblyStateCreateInfo {}
        .setTopology(VK_ConvertTopology(info.topology));

    auto multisampling_info = vk::PipelineMultisampleStateCreateInfo {}
        .setRasterizationSamples(VK_ConvertSampleCount(info.sample_count))
        .setAlphaToCoverageEnable(info.alpha_to_coverage);

    auto dynamic_info = vk::PipelineDynamicStateCreateInfo {}
        .setDynamicStateCount(static_cast<uint32_t>(dynamic_states.size()))
        .setPDynamicStates(dynamic_states.data());

    // Viewport and scissor are left as dynamic state.
    auto viewport_info = vk::PipelineViewportStateCreateInfo {}
        .setViewportCount(1)
        .setScissorCount(1);

    auto raster_info = vk::PipelineRasterizationStateCreateInfo {}
        .setPolygonMode(VK_ConvertFillMode(info.fill))
        .setCullMode(VK_ConvertCullMode(info.cull))
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .setLineWidth(1.f)
        .setDepthBiasEnable(vk::True);

    auto render_info = vk::PipelineRenderingCreateInfo {}
        .setColorAttachmentCount(static_cast<uint32_t>(formats.size()))
        .setPColorAttachmentFormats(formats.data())
        .setDepthAttachmentFormat(VK_ConvertFormat(info.depth))
        .setStencilAttachmentFormat(VK_ConvertFormat(info.stencil));

    std::vector<vk::PipelineColorBlendAttachmentState> blend_states = {};
    blend_states.reserve(info.atts.size());

    for (const AttachmentInfo& att : info.atts) {
        blend_states.push_back(vk::PipelineColorBlendAttachmentState {}
            .setColorBlendOp(VK_ConvertBlendOp(att.color_op))
            .setSrcColorBlendFactor(VK_ConvertBlendFactor(att.src_color_factor))
            .setDstColorBlendFactor(VK_ConvertBlendFactor(att.dst_color_factor))
            .setAlphaBlendOp(VK_ConvertBlendOp(att.alpha_op))
            .setSrcAlphaBlendFactor(VK_ConvertBlendFactor(att.src_alpha_factor))
            .setDstAlphaBlendFactor(VK_ConvertBlendFactor(att.dst_alpha_factor))
            .setColorWriteMask(static_cast<vk::ColorComponentFlags>(att.color_write_mask)));
    }

    auto blend_info = vk::PipelineColorBlendStateCreateInfo {}
        .setAttachments(blend_states);

    auto depth_stencil_info = vk::PipelineDepthStencilStateCreateInfo {};

    auto pipeline_info = vk::GraphicsPipelineCreateInfo {}
        .setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT)
        .setStageCount(static_cast<uint32_t>(stages.size()))
        .setPStages(stages.data())
        .setLayout(m_impl->pipeline_layout)
        .setPVertexInputState(&input_info)
        .setPInputAssemblyState(&assembly_info)
        .setPViewportState(&viewport_info)
        .setPRasterizationState(&raster_info)
        .setPMultisampleState(&multisampling_info)
        .setPDepthStencilState(&depth_stencil_info)
        .setPColorBlendState(&blend_info)
        .setPDynamicState(&dynamic_info)
        .setPNext(&render_info);

    vk::Result result = m_impl->device.createGraphicsPipelines(
        nullptr, 
        1, 
        &pipeline_info, 
        nullptr, 
        &pipeline.pipeline);

    // Shader modules can be discarded now.
    for (vk::PipelineShaderStageCreateInfo& stage_info : stages) {
        if (stage_info.module) {
            m_impl->device.destroyShaderModule(stage_info.module);
            stage_info.module = nullptr;
        }
    }

    if (result != vk::Result::eSuccess)
        return 0;

#endif // IGPU_VULKAN

    return m_impl->pipelines.add(pipeline);
}

void RenderingDevice::freePipeline(Pipeline pipeline) {
    Pipeline_T i_pipeline = m_impl->pipelines.remove(pipeline);

#ifdef IGPU_VULKAN

    if (i_pipeline.pipeline)
        m_impl->device.destroyPipeline(i_pipeline.pipeline);

#endif // IGPU_VULKAN
}

void RenderingDevice::copy(CommandList cmd, ptr src, ptr dst, uint32_t size) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

    IGPU_ASSERT(m_impl->allocations.contains(src), 
                "Invalid device source address!");
    
    AllocationInfo src_alloc = m_impl->allocations[src];

    IGPU_ASSERT(m_impl->allocations.contains(dst), 
                "Invalid device destination address!");
    
    AllocationInfo dst_alloc = m_impl->allocations[dst];

#ifdef IGPU_VULKAN

    auto region = vk::BufferCopy2 {}
        .setSize(size);

    i_cmd.buffer.copyBuffer2(vk::CopyBufferInfo2 {}
        .setSrcBuffer(src_alloc.buffer)
        .setDstBuffer(dst_alloc.buffer)
        .setRegionCount(1)
        .setPRegions(&region));

#endif // IGPU_VULKAN
}

void RenderingDevice::copyToTexture(void* src, Texture dst, const TextureRegion& region) {
    Texture_T texture_ = m_impl->textures.get(dst);

    const TextureInfo& info = texture_.info;

#ifdef IGPU_VULKAN

    auto copy = vk::MemoryToImageCopy {}
        .setPHostPointer(src)
        .setMemoryRowLength(0)
        .setMemoryImageHeight(0)
        .setImageOffset({ region.offset[0], region.offset[1], region.offset[2] })
        .setImageExtent({ region.extent[0], region.extent[1], region.extent[2] })
        .setImageSubresource(vk::ImageSubresourceLayers {}
            .setMipLevel(region.mip)
            .setBaseArrayLayer(region.base_layer)
            .setLayerCount(region.layer_count)
            .setAspectMask(VK_FormatToAspectMask(info.format)));

    // @Todo: Return custom result value.
    (void) m_impl->device.copyMemoryToImage(vk::CopyMemoryToImageInfo {}
        .setDstImage(texture_.image)
        .setDstImageLayout(vk::ImageLayout::eGeneral)
        .setRegionCount(1)
        .setPRegions(&copy));

#endif // IGPU_VULKAN
}

void RenderingDevice::copyFromTexture(Texture src, void* dst, const TextureRegion& region) {
    Texture_T i_texture = m_impl->textures.get(src);

    const TextureInfo& info = i_texture.info;

#ifdef IGPU_VULKAN

    auto copy = vk::ImageToMemoryCopy {}
        .setPHostPointer(dst)
        .setMemoryRowLength(0)
        .setMemoryImageHeight(0)
        .setImageOffset({ region.offset[0], region.offset[1], region.offset[2] })
        .setImageExtent({ region.extent[0], region.extent[1], region.extent[2] })
        .setImageSubresource(vk::ImageSubresourceLayers {}
            .setMipLevel(region.mip)
            .setBaseArrayLayer(region.base_layer)
            .setLayerCount(region.layer_count)
            .setAspectMask(VK_FormatToAspectMask(info.format)));

    // @Todo: Return custom result value.
    (void) m_impl->device.copyImageToMemory(vk::CopyImageToMemoryInfo {}
        .setSrcImage(i_texture.image)
        .setSrcImageLayout(vk::ImageLayout::eGeneral)
        .setRegionCount(1)
        .setPRegions(&copy));

#endif // IGPU_VULKAN
}

void RenderingDevice::barrier(CommandList cmd, Stage before, Stage after) {
    CommandList_T cmd_ = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    auto barrier = vk::MemoryBarrier2 {}
        .setSrcStageMask(VK_ConvertPipelineStage(before))
        .setDstStageMask(VK_ConvertPipelineStage(after))
        .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
        .setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite 
                        | vk::AccessFlagBits2::eMemoryRead);

    cmd_.buffer.pipelineBarrier2(vk::DependencyInfo {}
        .setMemoryBarrierCount(1)
        .setPMemoryBarriers(&barrier));

#endif // IGPU_VULKAN
}

void RenderingDevice::beginRendering(CommandList cmd, 
                                     const RenderingInfo& info) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    std::vector<vk::RenderingAttachmentInfo> color_atts = {};
    color_atts.reserve(info.targets.size());

    for (const TargetInfo& target : info.targets) {
        TextureView_T view = m_impl->views.get(target.view);

        auto ccv = vk::ClearColorValue {
            target.clear_color.r,
            target.clear_color.g,
            target.clear_color.b,
            target.clear_color.a,
        };

        color_atts.push_back(vk::RenderingAttachmentInfo {}
            .setImageView(view.view)
            .setImageLayout(vk::ImageLayout::eGeneral)
            .setLoadOp(VK_ConvertLoadOp(target.load))
            .setStoreOp(VK_ConvertStoreOp(target.store))
            .setClearValue(ccv));
    }

    auto area = vk::Rect2D {}
        .setOffset({ info.area.x, info.area.y })
        .setExtent({ info.area.width, info.area.height });

    auto render_info = vk::RenderingInfo {}
        .setLayerCount(info.layer_count)
        .setRenderArea(area)
        .setColorAttachmentCount(static_cast<uint32_t>(color_atts.size()))
        .setPColorAttachments(color_atts.data());

    auto depth_att = vk::RenderingAttachmentInfo {};

    auto stencil_att = vk::RenderingAttachmentInfo {};

    if (info.depth) {
        const TargetInfo& target = info.depth.value();

        TextureView_T view = m_impl->views.get(target.view);

        auto cdsv = vk::ClearDepthStencilValue {}
            .setDepth(target.clear_depth)
            .setStencil(0);

        depth_att = vk::RenderingAttachmentInfo {}
            .setImageView(view.view)
            .setImageLayout(vk::ImageLayout::eGeneral)
            .setLoadOp(VK_ConvertLoadOp(target.load))
            .setStoreOp(VK_ConvertStoreOp(target.store))
            .setClearValue(cdsv);

        render_info.setPDepthAttachment(&depth_att);
    }

    if (info.stencil) {
        const TargetInfo& target = info.stencil.value();

        TextureView_T view = m_impl->views.get(target.view);

        auto cdsv = vk::ClearDepthStencilValue {}
            .setDepth(0.f)
            .setStencil(target.clear_stencil);

        stencil_att = vk::RenderingAttachmentInfo {}
            .setImageView(view.view)
            .setImageLayout(vk::ImageLayout::eGeneral)
            .setLoadOp(VK_ConvertLoadOp(target.load))
            .setStoreOp(VK_ConvertStoreOp(target.store))
            .setClearValue(cdsv);

        render_info.setPStencilAttachment(&stencil_att);
    }

    i_cmd.buffer.beginRendering(render_info);

#endif // IGPU_VULKAN
}

void RenderingDevice::endRendering(CommandList cmd) {
    CommandList_T cmd_ = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN
    
    cmd_.buffer.endRendering();

#endif // IGPU_VULKAN
}

void RenderingDevice::setActiveTextureHeapAddress(CommandList cmd, ptr addr) {
    // @Todo: Implement this.
}

void RenderingDevice::setPipeline(CommandList cmd, Pipeline pipeline) {
    CommandList_T cmd_ = m_impl->lists.get(cmd);
    Pipeline_T pipeline_ = m_impl->pipelines.get(pipeline);

#ifdef IGPU_VULKAN

    cmd_.buffer.bindPipeline(VK_PipelineTypeToBindPoint(pipeline_.type), 
                             pipeline_.pipeline);

#endif // IGPU_VULKAN
}

void RenderingDevice::setViewport(CommandList cmd, Viewport viewport) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    i_cmd.buffer.setViewport(0, vk::Viewport {}
        .setX(viewport.x)
        .setY(viewport.y)
        .setHeight(viewport.height)
        .setWidth(viewport.width)
        .setMinDepth(viewport.min_depth)
        .setMaxDepth(viewport.max_depth));

#endif // IGPU_VULKAN
}

void RenderingDevice::setScissor(CommandList cmd, Rect2D scissor) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    i_cmd.buffer.setScissor(0, vk::Rect2D {}
        .setOffset({ scissor.x, scissor.y })
        .setExtent({ scissor.width, scissor.height }));

#endif // IGPU_VULKAN
}

void RenderingDevice::setDepthBias(CommandList cmd, 
                                   float clamp, 
                                   float slope, 
                                   float constant) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    i_cmd.buffer.setDepthBias(constant, clamp, slope);

#endif // IGPU_VULKAN
}

void RenderingDevice::setDepthCompareOp(CommandList cmd, CompareOp op) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    i_cmd.buffer.setDepthCompareOp(VK_ConvertCompareOp(op));

#endif // IGPU_VULKAN
}

void RenderingDevice::setEnableDepthTest(CommandList cmd, bool value) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    i_cmd.buffer.setDepthTestEnable(value);

#endif // IGPU_VULKAN
}

void RenderingDevice::setEnableDepthWrite(CommandList cmd, bool value) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    i_cmd.buffer.setDepthWriteEnable(value);

#endif // IGPU_VULKAN
}

void RenderingDevice::drawInstanced(CommandList cmd,
                                    ptr vertex, 
                                    ptr fragment, 
                                    uint32_t vertices, 
                                    uint32_t instances) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    // @Todo: Setup push constants at the vertex and fragment offsets.
    // e.g. i_cmd.buffer.pushConstants(pipeline_layout, stage, offset, sizeof(GpuAddr), &vertex);
    i_cmd.buffer.draw(vertices, instances, 0, 0);

#endif // IGPU_VULKAN
}

void RenderingDevice::drawIndexedInstanced(CommandList cmd,
                                           ptr vertex,
                                           ptr fragment,
                                           ptr index,
                                           uint32_t indices,
                                           uint32_t instances) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    // @Todo: Setup pcs...

    // @Todo: Get the buffer and offset of the index data.
    vk::Buffer ibo = nullptr;
    vk::DeviceSize offset = 0;

    //i_cmd.buffer.bindIndexBuffer(ibo, ibo_offset, vk::IndexType::eUint32);
    i_cmd.buffer.drawIndexed(indices, instances, 0, 0, 0);

#endif // IGPU_VULKAN
}

void RenderingDevice::dispatch(CommandList cmd, 
                               ptr data, 
                               uint32_t x, 
                               uint32_t y, 
                               uint32_t z) {
    CommandList_T i_cmd = m_impl->lists.get(cmd);

#ifdef IGPU_VULKAN

    // @Todo: Setup push constants at the compute offset with the given data.
    i_cmd.buffer.dispatch(x, y, z);

#endif // IGPU_VULKAN
}

} // namespace gpu
