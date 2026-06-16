//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "gpu2.h"
#include "gpu2.vk.h"

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"

#include <cstdint>
#include <set>

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

using namespace gpu;

struct QueueFamilyIndices final {
    uint32_t transfer = UINT_MAX;
    uint32_t graphics = UINT_MAX;
    uint32_t compute = UINT_MAX;

    /// Returns true if this set of queue families is complete.
    bool complete() const {
        return transfer != UINT_MAX 
            && graphics != UINT_MAX 
            && compute != UINT_MAX;
    }
};

QueueFamilyIndices computeIndices(vk::PhysicalDevice device) {
    QueueFamilyIndices indices = {};

    std::vector<vk::QueueFamilyProperties> families = 
        device.getQueueFamilyProperties();

    for (std::size_t i = 0; i < families.size(); ++i) {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphics = i;

        if (families[i].queueFlags & vk::QueueFlagBits::eCompute)
            indices.compute = i;

        if (families[i].queueFlags & vk::QueueFlagBits::eTransfer)
            indices.transfer = i;

        // If we've found all indices now, stop.
        if (indices.complete())
            break;
    }

    return indices;
};

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugMessengerCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
    // @Todo: Let user provide their own callback. 
    std::cerr << std::format("(Vulkan) {}\n", pCallbackData->pMessage);
    return vk::False;
}

vk::PipelineBindPoint gpu::convert(PipelineType type) {
    switch (type) {
        case PipelineType::Graphics:
            return vk::PipelineBindPoint::eGraphics;
        case PipelineType::Compute:
            return vk::PipelineBindPoint::eCompute;
    }
}

vk::ImageType gpu::convert(TextureType type) {
    switch (type) {
        case TextureType::D1:
            return vk::ImageType::e1D;
        case TextureType::D2:
            return vk::ImageType::e2D;
        case TextureType::D3:
            return vk::ImageType::e3D;
    }
}

vk::ImageLayout gpu::convert(TextureLayout layout) {
    switch (layout) {
        case TextureLayout::Undefined:
            return vk::ImageLayout::eUndefined;
        case TextureLayout::General:
            return vk::ImageLayout::eGeneral;
        case TextureLayout::ShaderRead:
            return vk::ImageLayout::eShaderReadOnlyOptimal;
        case TextureLayout::ColorTarget:
            return vk::ImageLayout::eColorAttachmentOptimal;
        case TextureLayout::DepthStencilTarget:
            return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        case TextureLayout::TransferSrc:
            return vk::ImageLayout::eTransferSrcOptimal;
        case TextureLayout::TransferDst:
            return vk::ImageLayout::eTransferDstOptimal;
        case TextureLayout::Present:
            return vk::ImageLayout::ePresentSrcKHR;
    }
}

vk::PresentModeKHR gpu::convert(PresentMode mode) {
    switch (mode) {
        case PresentMode::Immediate:
            return vk::PresentModeKHR::eImmediate;
        case PresentMode::Fifo:
            return vk::PresentModeKHR::eFifo;
        case PresentMode::FifoRelaxed:
            return vk::PresentModeKHR::eFifoRelaxed;
        case PresentMode::Mailbox:
            return vk::PresentModeKHR::eMailbox;
    }
}

vk::Format gpu::convert(Format format) {
    switch (format) {
        case Format::Undefined:
            return vk::Format::eUndefined;
        case Format::R8G8B8A8_UNORM:
            return vk::Format::eR8G8B8A8Unorm;
        case Format::R8G8B8A8_SRGB:
            return vk::Format::eR8G8B8A8Srgb;
        case Format::R16G16_SNORM:
            return vk::Format::eR16G16Snorm;
        case Format::R8_UNORM:
            return vk::Format::eR8Unorm;
        case Format::R32_FLOAT:
            return vk::Format::eR32Sfloat;
        case Format::R32G32_FLOAT:
            return vk::Format::eR32G32Sfloat;
        case Format::R32G32B32A32_FLOAT:
            return vk::Format::eR32G32B32A32Sfloat;
        case Format::R16_UINT:
            return vk::Format::eR16Uint;
        case Format::R32_UINT:
            return vk::Format::eR32Uint;
        case Format::R32_SFLOAT:
            return vk::Format::eR32Sfloat;
        case Format::D32_FLOAT:
            return vk::Format::eD32Sfloat;
        case Format::D24_UNORM_S8_UINT:
            return vk::Format::eD24UnormS8Uint;
    }
}

vk::Filter gpu::convert(Filter filter) {
    switch (filter) {
        case Filter::Nearest:
            return vk::Filter::eNearest;
        case Filter::Linear:
            return vk::Filter::eLinear;
    }
}

vk::SamplerAddressMode gpu::convert(AddressMode mode) {
    switch (mode) {
        case AddressMode::Repeat:
            return vk::SamplerAddressMode::eRepeat;
        case AddressMode::MirroredRepeat:
            return vk::SamplerAddressMode::eMirroredRepeat;
        case AddressMode::ClampToEdge:
            return vk::SamplerAddressMode::eClampToEdge;
        case AddressMode::ClampToBorder:
            return vk::SamplerAddressMode::eClampToBorder;
        case AddressMode::MirrorClampToEdge:
            return vk::SamplerAddressMode::eMirrorClampToEdge;
    }
}

vk::BorderColor gpu::convert(BorderColor color) {
    switch (color) {
        case BorderColor::FloatTransparentBlack:
            return vk::BorderColor::eFloatTransparentBlack;
        case BorderColor::IntTransparentBlack:
            return vk::BorderColor::eIntTransparentBlack;
        case BorderColor::FloatOpaqueBlack:
            return vk::BorderColor::eFloatOpaqueBlack;
        case BorderColor::IntOpaqueBlack:
            return vk::BorderColor::eIntOpaqueBlack;
        case BorderColor::FloatOpaqueWhite:
            return vk::BorderColor::eFloatOpaqueWhite;
        case BorderColor::IntOpaqueWhite:
            return vk::BorderColor::eIntOpaqueWhite;
    }
}

vk::PrimitiveTopology gpu::convert(Topology topology) {
    switch (topology) {
        case Topology::LineList:
            return vk::PrimitiveTopology::eLineList;
        case Topology::LineStrip:
            return vk::PrimitiveTopology::eLineStrip;
        case Topology::TriangleList:
            return vk::PrimitiveTopology::eTriangleList;
        case Topology::TriangleStrip:
            return vk::PrimitiveTopology::eTriangleStrip;
        case Topology::TriangleFan:
            return vk::PrimitiveTopology::eTriangleFan;
    }
}

vk::PolygonMode gpu::convert(FillMode fill) {
    switch (fill) {
        case FillMode::Solid:
            return vk::PolygonMode::eFill;
        case FillMode::Wireframe:
            return vk::PolygonMode::eLine;
    }
}

vk::FrontFace gpu::convert(FrontFace face) {
    switch (face) {
        case FrontFace::Clockwise:
            return vk::FrontFace::eClockwise;
        case FrontFace::CounterClockwise:
            return vk::FrontFace::eCounterClockwise;
    }
}

vk::AttachmentLoadOp gpu::convert(LoadOp op) {
    switch (op) {
        case LoadOp::Unknown:
            return vk::AttachmentLoadOp::eDontCare;
        case LoadOp::Load:
            return vk::AttachmentLoadOp::eLoad;
        case LoadOp::Clear:
            return vk::AttachmentLoadOp::eClear;
    }
}

vk::AttachmentStoreOp gpu::convert(StoreOp op) {
    switch (op) {
        case StoreOp::Unknown:
            return vk::AttachmentStoreOp::eDontCare;
        case StoreOp::Store:
            return vk::AttachmentStoreOp::eStore;
    }
}

vk::CompareOp gpu::convert(CompareOp op) {
    switch (op) {
        case CompareOp::Never:
            return vk::CompareOp::eNever;
        case CompareOp::Less:
            return vk::CompareOp::eLess;
        case CompareOp::LessEqual:
            return vk::CompareOp::eLessOrEqual;
        case CompareOp::Greater:
            return vk::CompareOp::eGreater;
        case CompareOp::GreaterEqual:
            return vk::CompareOp::eGreaterOrEqual;
        case CompareOp::Equal:
            return vk::CompareOp::eEqual;
        case CompareOp::NotEqual:
            return vk::CompareOp::eNotEqual;
        case CompareOp::Always:
            return vk::CompareOp::eAlways;
    }
}

vk::BlendOp gpu::convert(BlendOp op) {
    switch (op) {
        case BlendOp::Add:
            return vk::BlendOp::eAdd;
        case BlendOp::Subtract:
            return vk::BlendOp::eSubtract;
        case BlendOp::ReverseSubtract:
            return vk::BlendOp::eReverseSubtract;
        case BlendOp::Min:
            return vk::BlendOp::eMin;
        case BlendOp::Max:
            return vk::BlendOp::eMax;
    }
}

vk::StencilOp gpu::convert(StencilOp op) {
    switch (op) {
        case StencilOp::Keep:
            return vk::StencilOp::eKeep;
        case StencilOp::Zero:
            return vk::StencilOp::eZero;
        case StencilOp::Replace:
            return vk::StencilOp::eReplace;
        case StencilOp::IncrementAndClamp:
            return vk::StencilOp::eIncrementAndClamp;
        case StencilOp::DecrementAndClamp:
            return vk::StencilOp::eDecrementAndClamp;
        case StencilOp::Invert:
            return vk::StencilOp::eInvert;
        case StencilOp::IncrementAndWrap:
            return vk::StencilOp::eIncrementAndWrap;
        case StencilOp::DecrementAndWrap:
            return vk::StencilOp::eDecrementAndWrap;
    }
}

vk::BlendFactor gpu::convert(Factor factor) {
    switch (factor) {
        case Factor::Zero:
            return vk::BlendFactor::eZero;
        case Factor::One:
            return vk::BlendFactor::eOne;
        case Factor::SrcColor:
            return vk::BlendFactor::eSrcColor;
        case Factor::OneMinusSrcColor:
            return vk::BlendFactor::eOneMinusSrcColor;
        case Factor::DstColor:
            return vk::BlendFactor::eDstColor;
        case Factor::OneMinusDstColor:
            return vk::BlendFactor::eOneMinusDstColor;
        case Factor::SrcAlpha:
            return vk::BlendFactor::eSrcAlpha;
        case Factor::OneMinusSrcAlpha:
            return vk::BlendFactor::eOneMinusSrcAlpha;
        case Factor::DstAlpha:
            return vk::BlendFactor::eDstAlpha;
        case Factor::OneMinusDstAlpha:
            return vk::BlendFactor::eOneMinusDstAlpha;
        case Factor::ConstantColor:
            return vk::BlendFactor::eConstantColor;
        case Factor::OneMinusConstantColor:
            return vk::BlendFactor::eOneMinusConstantColor;
        case Factor::ConstantAlpha:
            return vk::BlendFactor::eConstantAlpha;
        case Factor::OneMinusConstantAlpha:
            return vk::BlendFactor::eOneMinusConstantAlpha;
        case Factor::SrcAlphaSaturate:
            return vk::BlendFactor::eSrcAlphaSaturate;
    }
}

vk::CullModeFlags gpu::convert(CullMode cull) {
    switch (cull) {
        case CullMode::None:
            return vk::CullModeFlagBits::eNone;
        case CullMode::Front:
            return vk::CullModeFlagBits::eFront;
        case CullMode::Back:
            return vk::CullModeFlagBits::eBack;
        case CullMode::Both:
            return vk::CullModeFlagBits::eFrontAndBack;
    }
}

vk::PipelineStageFlags2 gpu::convert(StageFlags stage) {
    vk::PipelineStageFlags2 flags = {};

    if (stage & StageFlags::Transfer)
        flags |= vk::PipelineStageFlagBits2::eTransfer;

    if (stage & StageFlags::RasterColorOut)
        flags |= vk::PipelineStageFlagBits2::eColorAttachmentOutput;

    if (stage & StageFlags::Vertex)
        flags |= vk::PipelineStageFlagBits2::eVertexShader;

    if (stage & StageFlags::Fragment)
        flags |= vk::PipelineStageFlagBits2::eFragmentShader;

    if (stage & StageFlags::Compute)
        flags |= vk::PipelineStageFlagBits2::eComputeShader;
    
    return flags;
}

vk::ShaderStageFlags gpu::convert(ShaderStageFlags stage) {
    vk::ShaderStageFlags flags = {};

    if (stage & ShaderStageFlags::Vertex)
        flags |= vk::ShaderStageFlagBits::eVertex;

    if (stage & ShaderStageFlags::Fragment)
        flags |= vk::ShaderStageFlagBits::eFragment;

    if (stage & ShaderStageFlags::Compute)
        flags |= vk::ShaderStageFlagBits::eCompute;

    return flags;
}

vk::ImageUsageFlags gpu::convert(UsageFlags usage) {
    vk::ImageUsageFlags flags = {};
    
    if (usage & UsageFlags::TransferSrc)
        flags |= vk::ImageUsageFlagBits::eTransferSrc;

    if (usage & UsageFlags::TransferDst)
        flags |= vk::ImageUsageFlagBits::eTransferDst;

    if (usage & UsageFlags::Sampled)
        flags |= vk::ImageUsageFlagBits::eSampled;

    if (usage & UsageFlags::Storage)
        flags |= vk::ImageUsageFlagBits::eStorage;

    if (usage & UsageFlags::ColorAttachment)
        flags |= vk::ImageUsageFlagBits::eColorAttachment;

    if (usage & UsageFlags::DepthStencilAttachment)
        flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

    return flags;
}

vk::ImageViewType gpu::convertViewType(TextureType type) {
    switch (type) {
        case TextureType::D1:
            return vk::ImageViewType::e1D; 
        case TextureType::D2:
            return vk::ImageViewType::e2D;
        case TextureType::D3:
            return vk::ImageViewType::e3D;
    }
}

vk::SamplerMipmapMode gpu::convertMipmapFilter(Filter filter) {
    switch (filter) {
        case Filter::Nearest:
            return vk::SamplerMipmapMode::eNearest;
        case Filter::Linear:
            return vk::SamplerMipmapMode::eLinear;
    }
}

vk::ImageAspectFlags gpu::convertAspectMask(Format format) {
    switch (format) {
        case Format::Undefined:
            return vk::ImageAspectFlagBits::eNone;
        case Format::R8G8B8A8_UNORM:
        case Format::R8G8B8A8_SRGB:
        case Format::R16G16_SNORM:
        case Format::R32_FLOAT:
        case Format::R32G32_FLOAT:
        case Format::R32G32B32A32_FLOAT:
        case Format::R8_UNORM:
        case Format::R16_UINT:
        case Format::R32_UINT:
        case Format::R32_SFLOAT:
            return vk::ImageAspectFlagBits::eColor;
        case Format::D32_FLOAT:
        case Format::D24_UNORM_S8_UINT:
            return vk::ImageAspectFlagBits::eDepth;
    }
}

vk::SampleCountFlagBits gpu::convertSampleCount(uint32_t samples) {
    switch (samples) {
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
        case 32:
            return vk::SampleCountFlagBits::e32;
        case 64:
            return vk::SampleCountFlagBits::e64;
    }

    GPU2_ASSERT(false, "Invalid sample count!");
}

VulkanDevice::VulkanDevice(const DeviceInfo& info) : Device(info) {
    createVulkanCore();
    createLogicalDevice();
    createMemoryAllocator();
    createDescriptorSetup();
    createPipelineLayout();
}

void VulkanDevice::createVulkanCore() {
    std::vector<const char*> layers = {};
    std::vector<const char*> extensions = {
        "VK_KHR_win32_surface",
        VK_KHR_SURFACE_EXTENSION_NAME,
    };

    if (info.validation) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
        extensions.push_back("VK_EXT_debug_utils");
    }

    auto app_info = vk::ApplicationInfo {}
        .setPApplicationName("Game")
        .setPEngineName("gpu2")
        .setApiVersion(vk::ApiVersion13)
        .setApplicationVersion(vk::makeVersion(1, 0, 0))
        .setEngineVersion(vk::makeVersion(1, 0, 0));

    auto instance_info = vk::InstanceCreateInfo {}
        .setPApplicationInfo(&app_info);

    auto messenger_info = vk::DebugUtilsMessengerCreateInfoEXT {};

    // If VK validation layers were requested, then setup the debug 
    // messenger as well. 
    if (info.validation) {
        messenger_info.setPfnUserCallback(DebugMessengerCallback);
        messenger_info.setMessageSeverity(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose 
          | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo 
          | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning 
          | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        messenger_info.setMessageType(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral 
          | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation 
          | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);

        instance_info.setPNext(&messenger_info);
    }

    instance_info.setEnabledExtensionCount(extensions.size());
    instance_info.setPpEnabledExtensionNames(extensions.data());

    instance_info.setEnabledLayerCount(layers.size());
    instance_info.setPpEnabledLayerNames(layers.data());

    VK_CHECK(vk::createInstance(&instance_info, nullptr, &instance));

    if (!info.validation)
        return;

    auto creator = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    GPU2_ASSERT(creator, "Failed to resolve 'vkCreateDebugUtilsMessengerEXT'");

    creator(
        instance,
        reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&messenger_info), 
        nullptr,
        reinterpret_cast<VkDebugUtilsMessengerEXT*>(&messenger));
}

void VulkanDevice::createLogicalDevice() {
    std::vector<const char*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    std::vector<vk::PhysicalDevice> devices = 
        instance.enumeratePhysicalDevices().value;

    // @Todo: Replace with logging callback.
    GPU2_ASSERT(!devices.empty(), "No physical devices were found!");

    auto it = std::ranges::find_if(devices, [&](vk::PhysicalDevice dev) -> bool {
        std::set<std::string> required(extensions.begin(), extensions.end());
        std::vector<vk::ExtensionProperties> available = 
            dev.enumerateDeviceExtensionProperties().value;

        for (const vk::ExtensionProperties& ext : available)
            required.erase(ext.extensionName);

        if (!required.empty())
            return false;

        if (dev.getProperties().apiVersion < vk::ApiVersion13)
            return false;

        if (!computeIndices(dev).complete())
            return false;

        return true;
    });

    // @Todo: Replace with error callback.
    GPU2_ASSERT(it != devices.end(), "Failed to find a suitable GPU.");

    physical_device = *it;

    QueueFamilyIndices indices = computeIndices(physical_device);
    GPU2_ASSERT(indices.complete(), "Not all queue type are supported!");

    float priority = 1.f;
    std::set<uint32_t> families = {
        indices.graphics,
        indices.compute,
        indices.transfer,
    };

    std::vector<vk::DeviceQueueCreateInfo> queue_infos = {};

    for (uint32_t family : families) {
        queue_infos.push_back(vk::DeviceQueueCreateInfo {}
            .setQueueCount(1)
            .setQueueFamilyIndex(family)
            .setPQueuePriorities(&priority));
    }

    auto feats = vk::PhysicalDeviceFeatures {}
        .setDepthClamp(vk::True)
        .setDepthBiasClamp(vk::True)
        .setShaderInt64(vk::True)
        .setSamplerAnisotropy(vk::True);

    auto feats11 = vk::PhysicalDeviceVulkan11Features {}
        .setShaderDrawParameters(vk::True);

    auto feats12 = vk::PhysicalDeviceVulkan12Features {}
        .setBufferDeviceAddress(vk::True)
        .setDescriptorIndexing(vk::True)
        .setDescriptorBindingPartiallyBound(vk::True)
        .setShaderSampledImageArrayNonUniformIndexing(vk::True)
        .setDescriptorBindingSampledImageUpdateAfterBind(vk::True)
        .setDescriptorBindingVariableDescriptorCount(vk::True)
        .setRuntimeDescriptorArray(vk::True)
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

    queues.resize(3);

    queues[static_cast<uint32_t>(QueueType::Graphics)] = VulkanQueue {
        .type = QueueType::Graphics,
        .handle = device.getQueue(indices.graphics, 0),
        .family = indices.graphics,
        .pending = {},
    };

    queues[static_cast<uint32_t>(QueueType::Compute)] = VulkanQueue {
        .type = QueueType::Compute,
        .handle = device.getQueue(indices.compute, 0),
        .family = indices.compute,
        .pending = {},
    };

    queues[static_cast<uint32_t>(QueueType::Transfer)] = VulkanQueue {
        .type = QueueType::Transfer,
        .handle = device.getQueue(indices.transfer, 0),
        .family = indices.transfer,
        .pending = {},
    };
}

void VulkanDevice::createMemoryAllocator() {
    VmaVulkanFunctions funcs = {};
    funcs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    funcs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo vma_info = {};
    vma_info.instance = instance;
    vma_info.physicalDevice = physical_device;
    vma_info.device = device;
    vma_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vma_info.pVulkanFunctions = &funcs;

    VK_CHECK(vk::Result(vmaCreateAllocator(&vma_info, &allocator)));
}

void VulkanDevice::createDescriptorSetup() {
    vk::PhysicalDeviceProperties props = physical_device.getProperties();

    num_sampler_descriptors = props.limits.maxPerStageDescriptorSamplers;
    num_texture_descriptors = props.limits.maxPerStageDescriptorSampledImages;
    num_rw_texture_descriptors = props.limits.maxPerStageDescriptorStorageImages;

    std::array<vk::DescriptorBindingFlags, 3> flags = {
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound,
        vk::DescriptorBindingFlagBits::ePartiallyBound,
    };

    std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {
        vk::DescriptorSetLayoutBinding {}
            .setBinding(SAMPLER_BINDING)
            .setDescriptorType(vk::DescriptorType::eSampler)
            .setDescriptorCount(num_sampler_descriptors)
            .setStageFlags(vk::ShaderStageFlagBits::eAll),
        vk::DescriptorSetLayoutBinding {}
            .setBinding(TEXTURE_BINDING)
            .setDescriptorType(vk::DescriptorType::eSampledImage)
            .setDescriptorCount(num_texture_descriptors)
            .setStageFlags(vk::ShaderStageFlagBits::eAll),
        vk::DescriptorSetLayoutBinding {}
            .setBinding(RW_TEXTURE_BINDING)
            .setDescriptorType(vk::DescriptorType::eStorageImage)
            .setDescriptorCount(num_rw_texture_descriptors)
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

    std::array<vk::DescriptorPoolSize, 3> sizes = {
        vk::DescriptorPoolSize {}
            .setType(vk::DescriptorType::eSampler)
            .setDescriptorCount(num_sampler_descriptors),
        vk::DescriptorPoolSize {}
            .setType(vk::DescriptorType::eSampledImage)
            .setDescriptorCount(num_texture_descriptors),
        vk::DescriptorPoolSize {}
            .setType(vk::DescriptorType::eStorageImage)
            .setDescriptorCount(num_rw_texture_descriptors),
    };

    auto pool_info = vk::DescriptorPoolCreateInfo {}
        .setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
        .setPoolSizeCount(sizes.size())
        .setPPoolSizes(sizes.data())
        .setMaxSets(1);

    VK_CHECK(device.createDescriptorPool(
        &pool_info, 
        nullptr, 
        &descriptor_pool));

    auto alloc_info = vk::DescriptorSetAllocateInfo {}
        .setDescriptorPool(descriptor_pool)
        .setDescriptorSetCount(1)
        .setPSetLayouts(&descriptor_layout);

    VK_CHECK(device.allocateDescriptorSets(&alloc_info, &descriptor_set));
}

void VulkanDevice::createPipelineLayout() {
    std::array<vk::PushConstantRange, 3> ranges = {
        vk::PushConstantRange {}
            .setOffset(0)
            .setSize(sizeof(vk::DeviceAddress))
            .setStageFlags(vk::ShaderStageFlagBits::eVertex),
        vk::PushConstantRange {}
            .setOffset(1 * sizeof(vk::DeviceAddress))
            .setSize(sizeof(vk::DeviceAddress))
            .setStageFlags(vk::ShaderStageFlagBits::eFragment),
        vk::PushConstantRange {}
            .setOffset(2 * sizeof(vk::DeviceAddress))
            .setSize(sizeof(vk::DeviceAddress))
            .setStageFlags(vk::ShaderStageFlagBits::eCompute),
    };

    auto layout_info = vk::PipelineLayoutCreateInfo {}
        .setPushConstantRangeCount(ranges.size())
        .setPPushConstantRanges(ranges.data())
        .setSetLayoutCount(1)
        .setPSetLayouts(&descriptor_layout);

    VK_CHECK(device.createPipelineLayout(
        &layout_info, 
        nullptr, 
        &pipeline_layout));
}

void VulkanDevice::determineProperties() {
    
}

void VulkanDevice::cleanupCommands(QueueType type) {
    VulkanQueue& queue = queues[static_cast<uint32_t>(type)];

    for (auto it = queue.pending.begin(); it != queue.pending.end(); ) {
        VulkanCommandList& cmd = *it;
        
        if (device.getFenceStatus(cmd.fence) != vk::Result::eSuccess) {
            it++;
            continue;
        }

        if (cmd.fence) {
            device.destroyFence(cmd.fence);
            cmd.fence = nullptr;
        }

        if (cmd.buffer) {
            device.freeCommandBuffers(cmd.pool, cmd.buffer);
            cmd.buffer = nullptr;
        }

        if (cmd.pool) {
            device.destroyCommandPool(cmd.pool);
            cmd.pool = nullptr;
        }
        
        it = queue.pending.erase(it);
    }
}

void VulkanDevice::destroy() {
    waitIdle();

    for (VulkanQueue& queue : queues) {
        waitIdle(queue.type);
        cleanupCommands(queue.type);

        GPU2_ASSERT(queue.pending.empty(), 
                    "Failed to cleanup pending command lists!");
    }

    queues.clear();

    GPU2_ASSERT(allocs.empty(), "Some memory allocations have not been free'd!");
    GPU2_ASSERT(textures.empty(), "Some textures have not been destroyed!");
    GPU2_ASSERT(samplers.empty(), "Some samplers have not been destroyed!");
    GPU2_ASSERT(shaders.empty(), "Some shaders have not been destroyed!");
    GPU2_ASSERT(swapchains.empty(), "Some swapchains have not been destroyed!");
    GPU2_ASSERT(commands.empty(), "Some command lists have not been destroyed!");
    GPU2_ASSERT(pipelines.empty(), "Some pipelines have not been destroyed!");
    GPU2_ASSERT(semaphores.empty(), "Some semaphores have not been destroyed!");
    GPU2_ASSERT(fences.empty(), "Some fences have not been destroyed!");

    if (pipeline_layout) {
        device.destroyPipelineLayout(pipeline_layout);
        pipeline_layout = nullptr;
    }

    if (descriptor_pool) {
        device.destroyDescriptorPool(descriptor_pool);
        descriptor_set = nullptr;
        descriptor_pool = nullptr;
    }

    if (descriptor_layout) {
        device.destroyDescriptorSetLayout(descriptor_layout);
        descriptor_layout = nullptr;
    }

    if (allocator) {
        vmaDestroyAllocator(allocator);
        allocator = nullptr;
    }

    if (device) {
        device.destroy();
        device = nullptr;
    }

    physical_device = nullptr;

    if (messenger) {
        auto deleter = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>( 
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

        GPU2_ASSERT(deleter, "Failed to resolve 'vkDestroyDebugUtilsMessengerEXT'");

        deleter(instance, messenger, nullptr);
        messenger = nullptr;
    }

    if (instance) {
        instance.destroy();
        instance = nullptr;
    }
}

void VulkanDevice::waitIdle() {
    VK_CHECK(device.waitIdle());
}

void VulkanDevice::waitIdle(QueueType type) {
    VK_CHECK(queues[static_cast<uint32_t>(type)].handle.waitIdle());
}

gpu::ptr VulkanDevice::malloc(uint32_t size, MemoryType type, void** mapped) {
    std::lock_guard<std::mutex> lock(memory_mutex);

    VulkanAlloc alloc = {};
    alloc.type = type;

    static constexpr vk::BufferUsageFlags usage = 
        vk::BufferUsageFlagBits::eStorageBuffer 
      | vk::BufferUsageFlagBits::eIndirectBuffer
      | vk::BufferUsageFlagBits::eShaderDeviceAddress 
      | vk::BufferUsageFlagBits::eTransferSrc 
      | vk::BufferUsageFlagBits::eTransferDst;

    vk::MemoryPropertyFlags mem_flags = {};
    VmaMemoryUsage mem_usage = {};

    switch (type) {
        case MemoryType::Upload:
            mem_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            mem_flags = vk::MemoryPropertyFlagBits::eHostVisible 
                      | vk::MemoryPropertyFlagBits::eHostCoherent;
            break;
        case MemoryType::Device:
            mem_usage = VMA_MEMORY_USAGE_GPU_ONLY;
            mem_flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
            break;
        case MemoryType::Readback:
            mem_usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
            mem_flags = vk::MemoryPropertyFlagBits::eHostVisible 
                      | vk::MemoryPropertyFlagBits::eHostCoherent 
                      | vk::MemoryPropertyFlagBits::eHostCached;
            break;
    }
 
    auto buffer_info = vk::BufferCreateInfo {}
        .setUsage(usage)
        .setSize(static_cast<vk::DeviceSize>(size))
        .setSharingMode(vk::SharingMode::eExclusive);

    // If the request is not asking for device-only memory, then map the memory
    // immediately.
    VmaAllocationCreateFlags alloc_flags = {};
    if (type != MemoryType::Device)
        alloc_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationCreateInfo vma_info = {};
    vma_info.flags = alloc_flags;
    vma_info.requiredFlags = static_cast<VkBufferCreateFlags>(mem_flags);
    vma_info.usage = mem_usage;

    VmaAllocationInfo alloc_info = {};

    vk::Result result = vk::Result(vmaCreateBuffer(
        allocator, 
        reinterpret_cast<VkBufferCreateInfo*>(&buffer_info), 
        &vma_info, 
        reinterpret_cast<VkBuffer*>(&alloc.buffer), 
        &alloc.alloc, 
        &alloc_info));

    if (result != vk::Result::eSuccess)
        return null;
    
    auto addr_info = vk::BufferDeviceAddressInfo {}
        .setBuffer(alloc.buffer);

    // Get the device address for the new memory buffer.
    alloc.device = static_cast<ptr>(device.getBufferAddress(&addr_info));
    if (!alloc.device) {
        vmaDestroyBuffer(allocator, alloc.buffer, alloc.alloc);
        return null;
    }

    // If the memory is not device only, then register the host address.
    if (type != MemoryType::Device) {
        alloc.host = alloc_info.pMappedData;
    } else {
        alloc.host = nullptr;
    }

    allocs.emplace(alloc.device, alloc);

    if (mapped)
        *mapped = alloc.host;

    return alloc.device;
}

void VulkanDevice::free(ptr device) {
    std::lock_guard<std::mutex> lock(memory_mutex);

    auto it = allocs.find(device);
    if (it == allocs.end())
        return;

    VulkanAlloc& alloc = it->second;

    if (alloc.buffer) {
        vmaDestroyBuffer(allocator, alloc.buffer, alloc.alloc);
        alloc.buffer = nullptr;
        alloc.alloc = nullptr;
    }

    allocs.erase(it);
}

void* VulkanDevice::deviceToHostPointer(ptr device) {
    std::lock_guard<std::mutex> lock(memory_mutex);

    auto it = allocs.find(device);
    return it != allocs.end() ? it->second.host : nullptr;
}

Texture VulkanDevice::createTexture(const TextureInfo& info) {
    VulkanTexture texture = {};
    texture.info = info;

    std::array<uint32_t, 3> queue_families = {
        queues[static_cast<uint32_t>(QueueType::Graphics)].family,
        queues[static_cast<uint32_t>(QueueType::Compute)].family,
        queues[static_cast<uint32_t>(QueueType::Transfer)].family,
    };
    
    auto image_info = vk::ImageCreateInfo {}
        .setFormat(convert(info.format))
        .setExtent({ info.area.x, info.area.y, info.area.z })
        .setImageType(convert(info.type))
        .setMipLevels(info.mip_count)
        .setArrayLayers(1) /* @Todo: Make this not hard? */
        .setSamples(convertSampleCount(info.sample_count))
        .setUsage(convert(info.usage))
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setSharingMode(vk::SharingMode::eExclusive);

    VmaAllocationCreateInfo vma_info = {};
    vma_info.requiredFlags = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_NULL_ON_ERROR(vk::Result(vmaCreateImage(
        allocator,
        reinterpret_cast<VkImageCreateInfo*>(&image_info),
        &vma_info,
        reinterpret_cast<VkImage*>(&texture.image),
        &texture.alloc,
        nullptr)));

    CommandList cmd = beginRecording(QueueType::Transfer);
    if (!cmd) {
        vmaDestroyImage(allocator, texture.image, texture.alloc);
        return null;
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
            .setLayerCount(1) /* @Todo: Maybe not hard-coded. */
            .setAspectMask(convertAspectMask(info.format)));

    commands.get(cmd).buffer.pipelineBarrier2(vk::DependencyInfo {}
        .setImageMemoryBarrierCount(1)
        .setPImageMemoryBarriers(&barrier));

    submit(QueueType::Transfer, cmd, null, null);

    return textures.add(texture);
}

void VulkanDevice::freeTexture(Texture T) {
    VulkanTexture texture = textures.remove(T);

    for (auto& [info, view] : texture.views) {
        if (view)
            device.destroyImageView(view);
    }

    texture.views.clear();

    if (texture.borrowed)
        return;

    if (texture.image && texture.alloc) {
        vmaDestroyImage(allocator, texture.image, texture.alloc);
        texture.image = nullptr;
        texture.alloc = nullptr;
    }
}

void VulkanDevice::copyToTexture(void* src, Texture dst, const CopyRegion& region) {
    VulkanTexture& texture = textures.get(dst);

    auto subresource = vk::ImageSubresourceLayers {}
        .setAspectMask(convertAspectMask(texture.info.format))
        .setBaseArrayLayer(region.base_layer)
        .setLayerCount(region.layer_count)
        .setMipLevel(region.mip_level);

    auto copy = vk::MemoryToImageCopy {}
        .setPHostPointer(src)
        .setMemoryRowLength(0)
        .setMemoryImageHeight(0)
        .setImageOffset({ region.offset.x, region.offset.y, region.offset.z })
        .setImageExtent({ region.area.x, region.area.y, region.area.z })
        .setImageSubresource(subresource);

    VK_CHECK(device.copyMemoryToImage(vk::CopyMemoryToImageInfo {}
        .setDstImage(texture.image)
        .setDstImageLayout(vk::ImageLayout::eTransferDstOptimal)
        .setRegionCount(1)
        .setPRegions(&copy)));
}

void VulkanDevice::copyFromTexture(Texture src, void* dst, const CopyRegion& region) {
    VulkanTexture& texture = textures.get(src);

    auto subresource = vk::ImageSubresourceLayers {}
        .setAspectMask(convertAspectMask(texture.info.format))
        .setBaseArrayLayer(region.base_layer)
        .setLayerCount(region.layer_count)
        .setMipLevel(region.mip_level);

    auto copy = vk::ImageToMemoryCopy {}
        .setPHostPointer(dst)
        .setMemoryRowLength(0)
        .setMemoryImageHeight(0)
        .setImageOffset({ region.offset.x, region.offset.y, region.offset.z })
        .setImageExtent({ region.area.x, region.area.y, region.area.z })
        .setImageSubresource(subresource);

    VK_CHECK(device.copyImageToMemory(vk::CopyImageToMemoryInfo {}
        .setSrcImage(texture.image)
        .setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setRegionCount(1)
        .setPRegions(&copy)));
}

Sampler VulkanDevice::createSampler(const SamplerInfo& info) {
    VulkanSampler sampler = {};
    sampler.info = info;

    auto sampler_info = vk::SamplerCreateInfo {}
        .setMinFilter(convert(info.min))
        .setMagFilter(convert(info.mag))
        .setMipmapMode(convertMipmapFilter(info.mip))
        .setAddressModeU(convert(info.u))
        .setAddressModeV(convert(info.v))
        .setAddressModeW(convert(info.w))
        .setAnisotropyEnable(info.enable_anisotropy)
        .setMaxAnisotropy(info.max_anisotropy)
        .setMinLod(info.min_lod)
        .setMaxLod(info.max_lod)
        .setCompareEnable(info.enable_compare)
        .setCompareOp(convert(info.compare_op));

    VK_NULL_ON_ERROR(device.createSampler(
        &sampler_info, 
        nullptr, 
        &sampler.handle));

    return samplers.add(sampler);
}

void VulkanDevice::freeSampler(Sampler S) {
    VulkanSampler sampler = samplers.remove(S);

    if (sampler.handle) {
        device.destroySampler(sampler.handle);
        sampler.handle = nullptr;
    }
}

Shader VulkanDevice::createShader(const ShaderInfo& info) {
    VulkanShader shader = {};
    shader.info = info;

    auto module_info = vk::ShaderModuleCreateInfo {}
        .setCodeSize(info.bytecode.size())
        .setPCode(reinterpret_cast<const uint32_t*>(info.bytecode.data()));

    VK_NULL_ON_ERROR(device.createShaderModule(
        &module_info, 
        nullptr, 
        &shader.module));

    return shaders.add(shader);
}

void VulkanDevice::freeShader(Shader S) {
    VulkanShader shader = shaders.remove(S);

    if (shader.module) {
        device.destroyShaderModule(shader.module);
        shader.module = nullptr;
    }
}

Pipeline VulkanDevice::createGraphicsPipeline(Shader V, 
                                              Shader F, 
                                              const RasterInfo& info) {
    VulkanShader& vertex = shaders.get(V);
    VulkanShader& fragment = shaders.get(F);

    VulkanPipeline pipeline = {};
    pipeline.type = PipelineType::Graphics;

    std::vector<vk::Format> formats = {};
    for (const AttachmentInfo& att : info.attachments) {
        formats.push_back(convert(att.format));
    }

    std::array<vk::PipelineShaderStageCreateInfo, 2> stages = {
        vk::PipelineShaderStageCreateInfo {}
            .setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(vertex.module)
            .setPName(vertex.info.entry.data()),
        vk::PipelineShaderStageCreateInfo {}
            .setStage(vk::ShaderStageFlagBits::eFragment)
            .setModule(fragment.module)
            .setPName(fragment.info.entry.data()),
    };

    std::array<vk::DynamicState, 2> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    auto input_info = vk::PipelineVertexInputStateCreateInfo {};

    auto assembly_info = vk::PipelineInputAssemblyStateCreateInfo {}
        .setTopology(convert(info.topology));

    auto multisampling_info = vk::PipelineMultisampleStateCreateInfo {}
        .setRasterizationSamples(convertSampleCount(info.sample_count))
        .setAlphaToCoverageEnable(false); /* @Todo: Make optional. */

    auto dynamic_info = vk::PipelineDynamicStateCreateInfo {}
        .setDynamicStateCount(dynamic_states.size())
        .setPDynamicStates(dynamic_states.data());

    auto viewport_info = vk::PipelineViewportStateCreateInfo {}
        .setViewportCount(1)
        .setScissorCount(1);

    auto raster_info = vk::PipelineRasterizationStateCreateInfo {}
        .setPolygonMode(convert(info.fill))
        .setCullMode(convert(info.cull))
        .setFrontFace(convert(info.face))
        .setDepthBiasEnable(info.depth_bias_enable)
        .setDepthBiasClamp(info.depth_bias_clamp)
        .setDepthBiasSlopeFactor(info.depth_bias_slope)
        .setDepthBiasConstantFactor(info.depth_bias_constant)
        .setLineWidth(1.f);

    auto render_info = vk::PipelineRenderingCreateInfo {}
        .setColorAttachmentCount(formats.size())
        .setPColorAttachmentFormats(formats.data())
        .setDepthAttachmentFormat(convert(info.depth));

    std::vector<vk::PipelineColorBlendAttachmentState> blend_states = {};
    blend_states.reserve(info.attachments.size());

    for (const AttachmentInfo& att : info.attachments) {
        blend_states.push_back(vk::PipelineColorBlendAttachmentState {}
            .setColorBlendOp(convert(att.color_op))
            .setSrcColorBlendFactor(convert(att.src_color_factor))
            .setDstColorBlendFactor(convert(att.dst_color_factor))
            .setAlphaBlendOp(convert(att.alpha_op))
            .setSrcAlphaBlendFactor(convert(att.src_alpha_factor))
            .setDstAlphaBlendFactor(convert(att.dst_alpha_factor))
            .setColorWriteMask(static_cast<vk::ColorComponentFlags>(att.color_write_mask)));
    }

    auto blend_info = vk::PipelineColorBlendStateCreateInfo {}
        .setAttachments(blend_states);

    auto depth_stencil_info = vk::PipelineDepthStencilStateCreateInfo {}
        .setDepthTestEnable(info.depth_test)
        .setDepthWriteEnable(info.depth_write)
        .setDepthCompareOp(convert(info.depth_compare));

    auto pipeline_info = vk::GraphicsPipelineCreateInfo {}
        .setLayout(pipeline_layout)
        .setStageCount(stages.size())
        .setPStages(stages.data())
        .setPVertexInputState(&input_info)
        .setPInputAssemblyState(&assembly_info)
        .setPViewportState(&viewport_info)
        .setPRasterizationState(&raster_info)
        .setPMultisampleState(&multisampling_info)
        .setPColorBlendState(&blend_info)
        .setPDynamicState(&dynamic_info)
        .setPNext(&render_info);

    if (info.depth != Format::Undefined)
        pipeline_info.setPDepthStencilState(&depth_stencil_info);

    VK_NULL_ON_ERROR(device.createGraphicsPipelines(
        nullptr, 
        1, 
        &pipeline_info, 
        nullptr, 
        &pipeline.handle));

    return pipelines.add(pipeline);
}

Pipeline VulkanDevice::createComputePipeline(Shader C) {
    VulkanShader& compute = shaders.get(C);

    VulkanPipeline pipeline = {};
    pipeline.type = PipelineType::Compute;

    auto compute_info= vk::PipelineShaderStageCreateInfo {}
        .setStage(vk::ShaderStageFlagBits::eCompute)    
        .setModule(compute.module)
        .setPName(compute.info.entry.data());

    auto pipeline_info = vk::ComputePipelineCreateInfo {}
        .setLayout(pipeline_layout)
        .setStage(compute_info);

    VK_NULL_ON_ERROR(device.createComputePipelines(
        nullptr, 
        1, 
        &pipeline_info, 
        nullptr, 
        &pipeline.handle));                                             

    return pipelines.add(pipeline);
}

void VulkanDevice::freePipeline(Pipeline P) {
    VulkanPipeline pipeline = pipelines.remove(P);

    if (pipeline.handle) {
        device.destroyPipeline(pipeline.handle);
        pipeline.handle = nullptr;
    }
}

Semaphore VulkanDevice::createSemaphore(uint64_t value) {
    vk::Semaphore semaphore = nullptr;

    auto type_info = vk::SemaphoreTypeCreateInfo {}
        .setSemaphoreType(vk::SemaphoreType::eTimeline)
        .setInitialValue(value);

    auto sema_info = vk::SemaphoreCreateInfo {}
        .setPNext(&type_info);

    VK_NULL_ON_ERROR(device.createSemaphore(
        &sema_info, 
        nullptr, 
        &semaphore));

    return semaphores.add(semaphore);
}

void VulkanDevice::freeSemaphore(Semaphore S) {
    vk::Semaphore semaphore = semaphores.remove(S);

    if (semaphore)
        device.destroySemaphore(semaphore);
}

void VulkanDevice::waitSemaphore(Semaphore sema, uint64_t value) {
    vk::Semaphore vk_sema = semaphores.get(sema);

    auto wait_info = vk::SemaphoreWaitInfo {}
        .setSemaphores(vk_sema)
        .setValues(value);

    VK_CHECK(device.waitSemaphores(wait_info, UINT64_C(1'000'000'000)));
}

Swapchain VulkanDevice::createSwapchain(const SwapchainInfo& info) {
    VulkanSwapchain swapchain = {};
    swapchain.info = info;

    VK_NULL_ON_ERROR(vk::Result(glfwCreateWindowSurface(
        instance, 
        reinterpret_cast<GLFWwindow*>(info.window), 
        nullptr, 
        reinterpret_cast<VkSurfaceKHR*>(&swapchain.surface))));

    vk::SurfaceCapabilitiesKHR caps = {};
    VK_NULL_ON_ERROR(physical_device.getSurfaceCapabilitiesKHR(
        swapchain.surface, 
        &caps));

    auto swapchain_info = vk::SwapchainCreateInfoKHR {}
        .setSurface(swapchain.surface)
        .setPresentMode(convert(info.present))
        .setMinImageCount(caps.minImageCount)
        .setImageFormat(convert(info.format))
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageExtent({ info.width, info.height })
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .setPreTransform(caps.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(vk::True);

    VK_NULL_ON_ERROR(device.createSwapchainKHR(
        &swapchain_info, 
        nullptr, 
        &swapchain.handle));

    uint32_t num_images = 0;
    VK_NULL_ON_ERROR(device.getSwapchainImagesKHR(
        swapchain.handle, 
        &num_images, 
        nullptr));

    std::vector<vk::Image> images(num_images);
    VK_NULL_ON_ERROR(device.getSwapchainImagesKHR(
        swapchain.handle, 
        &num_images, 
        images.data()));

    swapchain.textures.resize(num_images);
    swapchain.image_available_semas.resize(info.frames_in_flight);
    swapchain.render_finished_semas.resize(info.frames_in_flight);

    for (uint32_t i = 0; i < num_images; ++i) {
        vk::Image image = images[i];

        VulkanTexture texture = {};
        texture.image = image;
        texture.borrowed = true;
        texture.info = TextureInfo {
            .type = TextureType::D2,
            .format = info.format,
            .usage = UsageFlags::ColorAttachment,
            .area = { info.width, info.height },
        };

        swapchain.textures[i] = textures.add(texture);
    }

    auto sema_info = vk::SemaphoreCreateInfo {};

    for (uint32_t i = 0; i < info.frames_in_flight; ++i) {
        VK_CHECK(device.createSemaphore(
            &sema_info, 
            nullptr, 
            &swapchain.image_available_semas[i]));

        VK_CHECK(device.createSemaphore(
            &sema_info, 
            nullptr, 
            &swapchain.render_finished_semas[i]));
    }

    return swapchains.add(swapchain);
}

void VulkanDevice::freeSwapchain(Swapchain SW) {
    VulkanSwapchain swapchain = swapchains.remove(SW);

    for (Texture texture : swapchain.textures) {
        freeTexture(texture);        
    }

    for (vk::Semaphore sema : swapchain.image_available_semas) {
        device.destroySemaphore(sema);
    }

    for (vk::Semaphore sema : swapchain.render_finished_semas) {
        device.destroySemaphore(sema);
    }

    if (swapchain.handle) {
        device.destroySwapchainKHR(swapchain.handle);
        swapchain.handle = nullptr;
    }

    if (swapchain.surface) {
        instance.destroySurfaceKHR(swapchain.surface);
        swapchain.surface = nullptr;
    }
}

void VulkanDevice::resizeSwapchain(Swapchain SW, uint32_t width, uint32_t height) {
    waitIdle();

    VulkanSwapchain& swapchain = swapchains.get(SW);

    vk::SurfaceCapabilitiesKHR caps = {};
    VK_CHECK(physical_device.getSurfaceCapabilitiesKHR(swapchain.surface, &caps));

    vk::SwapchainKHR old_swapchain = swapchain.handle;

    auto swapchain_info = vk::SwapchainCreateInfoKHR {}
        .setSurface(swapchain.surface)
        .setPresentMode(convert(swapchain.info.present))
        .setMinImageCount(caps.minImageCount)
        .setImageFormat(convert(swapchain.info.format))
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageExtent({ width, height })
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setPreTransform(caps.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(vk::True)
        .setOldSwapchain(old_swapchain);

    VK_CHECK(device.createSwapchainKHR(
        &swapchain_info, 
        nullptr, 
        &swapchain.handle));

    device.destroySwapchainKHR(old_swapchain);

    uint32_t num_images = 0;
    VK_CHECK(device.getSwapchainImagesKHR(
        swapchain.handle, 
        &num_images, 
        nullptr));

    std::vector<vk::Image> images(num_images);
    VK_CHECK(device.getSwapchainImagesKHR(
        swapchain.handle, 
        &num_images, 
        images.data()));

    for (Texture texture : swapchain.textures) {
        freeTexture(texture);
    }
 
    swapchain.textures.resize(num_images);
    swapchain.image_available_semas.resize(num_images);

    for (std::size_t i = 0; i < num_images; ++i) {
        vk::Image image = images[i];

        VulkanTexture texture = {};
        texture.image = image;
        texture.borrowed = true;
        texture.info = TextureInfo {
            .type = TextureType::D2,
            .format = swapchain.info.format,
            .usage = UsageFlags::ColorAttachment,
            .area = { width, height },
        };

        swapchain.textures[i] = textures.add(texture);
    }
}

Texture VulkanDevice::acquireSwapchainTexture(Swapchain S) {
    VulkanSwapchain& swapchain = swapchains.get(S);
    vk::Semaphore sema = swapchain.image_available_semas[swapchain.frame_index];

    vk::Result result = device.acquireNextImageKHR(
        swapchain.handle, 
        UINT64_MAX, 
        sema, 
        nullptr, 
        &swapchain.image_index);

    return result == vk::Result::eSuccess 
        ? swapchain.textures[swapchain.image_index]
        : null;
}

void VulkanDevice::present(Swapchain swapchain, CommandList cmd, Semaphore sema, uint64_t value) {
    VulkanSwapchain& vk_swapchain = swapchains.get(swapchain);
    vk::Semaphore vk_sema = semaphores.get(sema);
    vk::Semaphore image_available = vk_swapchain.image_available_semas[vk_swapchain.frame_index];
    vk::Semaphore render_finished = vk_swapchain.render_finished_semas[vk_swapchain.frame_index];
    VulkanQueue& queue = queues[static_cast<uint32_t>(QueueType::Graphics)];

    VulkanCommandList vk_cmd = commands.remove(cmd);
    VK_CHECK(vk_cmd.buffer.end());
    queue.pending.push_back(vk_cmd);

    auto buffer_info = vk::CommandBufferSubmitInfo {}
        .setCommandBuffer(vk_cmd.buffer);

    std::array<vk::SemaphoreSubmitInfo, 1> waits = {
        vk::SemaphoreSubmitInfo {}
            .setSemaphore(image_available)
            .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput),
    };

    std::array<vk::SemaphoreSubmitInfo, 2> signals = {
        vk::SemaphoreSubmitInfo {}
            .setSemaphore(render_finished)
            .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput),
        vk::SemaphoreSubmitInfo {}
            .setSemaphore(vk_sema)
            .setValue(value),
    };

    auto submit_info = vk::SubmitInfo2 {}
        .setCommandBufferInfos(buffer_info)
        .setWaitSemaphoreInfos(waits)
        .setSignalSemaphoreInfos(signals);

    VK_CHECK(queue.handle.submit2(1, &submit_info, vk_cmd.fence));

    VK_CHECK(queue.handle.presentKHR(vk::PresentInfoKHR {}
        .setSwapchains(vk_swapchain.handle)
        .setWaitSemaphores(render_finished)
        .setPImageIndices(&vk_swapchain.image_index)));

    vk_swapchain.frame_index = (vk_swapchain.frame_index + 1) % vk_swapchain.info.frames_in_flight;
}

Descriptor VulkanDevice::getSamplerDescriptor(Sampler S) {
    VulkanSampler& sampler = samplers.get(S);

    Descriptor desc = sampler_descriptors.alloc();

    auto image_info = vk::DescriptorImageInfo {}
        .setSampler(sampler.handle);

    auto write = vk::WriteDescriptorSet {}
        .setDstSet(descriptor_set)
        .setDstBinding(SAMPLER_BINDING)
        .setDstArrayElement(desc)
        .setDescriptorType(vk::DescriptorType::eSampler)    
        .setDescriptorCount(1)
        .setImageInfo(image_info);

    device.updateDescriptorSets(1, &write, 0, nullptr);
    return desc;
}

Descriptor VulkanDevice::getTextureDescriptor(Texture T, TextureViewInfo info) {
    VulkanTexture& texture = textures.get(T);

    auto range = vk::ImageSubresourceRange {}
        .setAspectMask(convertAspectMask(texture.info.format))
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setBaseMipLevel(0)
        .setLevelCount(texture.info.mip_count);

    auto view_info = vk::ImageViewCreateInfo {}
        .setImage(texture.image)
        .setViewType(convertViewType(texture.info.type))
        .setFormat(convert(texture.info.format))
        .setSubresourceRange(range);

    vk::ImageView view = nullptr;

    auto it = texture.views.find(view_info);
    if (it != texture.views.end()) {
        view = it->second;
    } else {
        VK_CHECK(device.createImageView(&view_info, nullptr, &view));
        texture.views.emplace(view_info, view);
    }

    Descriptor desc = sampler_descriptors.alloc();

    auto image_info = vk::DescriptorImageInfo {}
        .setImageLayout(vk::ImageLayout::eGeneral)
        .setImageView(view);

    auto write = vk::WriteDescriptorSet {}
        .setDstSet(descriptor_set)
        .setDstBinding(TEXTURE_BINDING)
        .setDstArrayElement(desc)
        .setDescriptorType(vk::DescriptorType::eSampledImage)    
        .setDescriptorCount(1)
        .setImageInfo(image_info);

    device.updateDescriptorSets(1, &write, 0, nullptr);
    return desc;
}

Descriptor VulkanDevice::getRWTextureDescriptor(Texture T, TextureViewInfo info) {
    VulkanTexture& texture = textures.get(T);

    auto range = vk::ImageSubresourceRange {}
        .setAspectMask(convertAspectMask(texture.info.format))
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setBaseMipLevel(0)
        .setLevelCount(texture.info.mip_count);

    auto view_info = vk::ImageViewCreateInfo {}
        .setImage(texture.image)
        .setViewType(convertViewType(texture.info.type))
        .setFormat(convert(texture.info.format))
        .setSubresourceRange(range);

    vk::ImageView view = nullptr;

    auto it = texture.views.find(view_info);
    if (it != texture.views.end()) {
        view = it->second;
    } else {
        VK_CHECK(device.createImageView(&view_info, nullptr, &view));
        texture.views.emplace(view_info, view);
    }

    Descriptor desc = sampler_descriptors.alloc();

    auto image_info = vk::DescriptorImageInfo {}
        .setImageLayout(vk::ImageLayout::eGeneral)
        .setImageView(view);

    auto write = vk::WriteDescriptorSet {}
        .setDstSet(descriptor_set)
        .setDstBinding(TEXTURE_BINDING)
        .setDstArrayElement(desc)
        .setDescriptorType(vk::DescriptorType::eSampledImage)    
        .setDescriptorCount(1)
        .setImageInfo(image_info);

    device.updateDescriptorSets(1, &write, 0, nullptr);
    return desc;
}

void VulkanDevice::releaseSamplerDescriptor(Descriptor desc) {
    sampler_descriptors.release(desc);
}

void VulkanDevice::releaseTextureDescriptor(Descriptor desc) {
    texture_descriptors.release(desc);
}

void VulkanDevice::releaseRWTextureDescriptor(Descriptor desc) {
    rw_texture_descriptors.release(desc);
}

CommandList VulkanDevice::beginRecording(QueueType type) {
    cleanupCommands(type);

    VulkanCommandList cmd = {};

    auto pool_info = vk::CommandPoolCreateInfo {}
        .setFlags(vk::CommandPoolCreateFlagBits::eTransient)
        .setQueueFamilyIndex(queues[static_cast<uint32_t>(type)].family);
    
    VK_NULL_ON_ERROR(device.createCommandPool(
        &pool_info, 
        nullptr, 
        &cmd.pool));

    auto buffer_info = vk::CommandBufferAllocateInfo {}
        .setCommandBufferCount(1)
        .setCommandPool(cmd.pool)
        .setLevel(vk::CommandBufferLevel::ePrimary);

    if (device.allocateCommandBuffers(&buffer_info, 
                                      &cmd.buffer) != vk::Result::eSuccess) {
        device.destroyCommandPool(cmd.pool);
        return null;
    }

    auto fence_info = vk::FenceCreateInfo {};

    if (device.createFence(&fence_info, 
                           nullptr, 
                           &cmd.fence) != vk::Result::eSuccess) {
        device.destroyCommandPool(cmd.pool);
        return null;
    }

    auto begin_info = vk::CommandBufferBeginInfo {}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    if (cmd.buffer.begin(begin_info) != vk::Result::eSuccess) {
        device.destroyCommandPool(cmd.pool);
        return null;
    }

    return commands.add(cmd);
}

void VulkanDevice::submit(
        QueueType queue, 
        CommandList handle, 
        Semaphore signal, 
        Semaphore wait) {
    VulkanQueue& vk_queue = queues[static_cast<uint32_t>(queue)];

    VulkanCommandList cmd = commands.remove(handle);
    VK_CHECK(cmd.buffer.end());
    vk_queue.pending.push_back(cmd);

    auto buffer_info = vk::CommandBufferSubmitInfo {}
        .setCommandBuffer(cmd.buffer);

    auto submit_info = vk::SubmitInfo2 {}
        .setCommandBufferInfos(buffer_info);

    auto wait_info = vk::SemaphoreSubmitInfo {};
    auto signal_info = vk::SemaphoreSubmitInfo {};

    if (wait) {
        wait_info = vk::SemaphoreSubmitInfo {}
            .setSemaphore(semaphores.get(wait))
            .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

        submit_info.setWaitSemaphoreInfos(wait_info);
    }
    
    if (signal) {
        signal_info = vk::SemaphoreSubmitInfo {}
            .setSemaphore(semaphores.get(signal))
            .setStageMask(vk::PipelineStageFlagBits2::eAllGraphics);

        submit_info.setSignalSemaphoreInfos(signal_info);
    }    

    VK_CHECK(vk_queue.handle.submit2(1, &submit_info, cmd.fence));
}

void VulkanDevice::copy(CommandList CL, ptr src, ptr dst, uint64_t size) {
    VulkanCommandList& cmd = commands.get(CL);

    auto region = vk::BufferCopy2 {}
        .setSize(size);

    cmd.buffer.copyBuffer2(vk::CopyBufferInfo2 {}
        .setSrcBuffer(allocs[src].buffer)
        .setDstBuffer(allocs[dst].buffer)
        .setRegionCount(1)
        .setPRegions(&region));
}

void VulkanDevice::barrier(
        CommandList CL, 
        StageFlags before, 
        StageFlags after) {
    VulkanCommandList& cmd = commands.get(CL);

    auto barrier = vk::MemoryBarrier2 {}
        .setSrcStageMask(convert(before))
        .setDstStageMask(convert(after))
        .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
        .setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite 
                        | vk::AccessFlagBits2::eMemoryRead);

    cmd.buffer.pipelineBarrier2(vk::DependencyInfo {}
        .setMemoryBarriers(barrier));
}

void VulkanDevice::barrier(
        CommandList CL, 
        Texture T, 
        TextureLayout before, 
        TextureLayout after) {
    VulkanCommandList& cmd = commands.get(CL);
    VulkanTexture& texture = textures.get(T);

    auto range = vk::ImageSubresourceRange {}
        .setAspectMask(convertAspectMask(texture.info.format))
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setBaseMipLevel(0)
        .setLevelCount(texture.info.mip_count);

    auto barrier = vk::ImageMemoryBarrier2 {}
        .setImage(texture.image)
        .setOldLayout(convert(before))
        .setNewLayout(convert(after))
        .setSubresourceRange(range)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        .setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite 
                        | vk::AccessFlagBits2::eMemoryRead);

    cmd.buffer.pipelineBarrier2(vk::DependencyInfo {}
        .setImageMemoryBarriers(barrier));
}

void VulkanDevice::beginRendering(CommandList CL, const RenderInfo& info) {
    VulkanCommandList& cmd = commands.get(CL);

    auto createAttachment = [&](const TargetInfo& target, bool ds) -> vk::RenderingAttachmentInfo {
        VulkanTexture& texture = textures.get(target.texture);

        auto range = vk::ImageSubresourceRange {}
            .setAspectMask(convertAspectMask(texture.info.format))
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setBaseMipLevel(0)
            .setLevelCount(texture.info.mip_count);

        auto view_info = vk::ImageViewCreateInfo {}
            .setImage(texture.image)
            .setViewType(convertViewType(texture.info.type))
            .setFormat(convert(texture.info.format))
            .setSubresourceRange(range);

        vk::ImageView view = nullptr;

        auto it = texture.views.find(view_info);
        if (it != texture.views.end()) {
            view = it->second;
        } else {
            VK_CHECK(device.createImageView(&view_info, nullptr, &view));
            texture.views.emplace(view_info, view);
        }

        auto cv = vk::ClearValue {};

        if (ds) {
            cv = vk::ClearDepthStencilValue {
                target.clear_depth.depth,
                target.clear_depth.stencil,
            };
        } else {
            cv = vk::ClearColorValue {
                target.clear_color.r,
                target.clear_color.g,
                target.clear_color.b,
                target.clear_color.a,
            };
        }

        return vk::RenderingAttachmentInfo {}
            .setImageView(view)
            .setImageLayout(ds ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(convert(target.load))
            .setStoreOp(convert(target.store))
            .setClearValue(cv);
    };

    std::vector<vk::RenderingAttachmentInfo> attachments = {};
    attachments.reserve(info.targets.size());

    for (const TargetInfo& target : info.targets) {
        attachments.push_back(createAttachment(target, false));
    }

    auto depth_stencil = vk::RenderingAttachmentInfo {};

    if (info.depth.has_value())
        depth_stencil = createAttachment(info.depth.value(), true);

    auto area = vk::Rect2D {}
        .setOffset({ info.area.x, info.area.y })
        .setExtent({ info.area.width, info.area.height });

    cmd.buffer.beginRendering(vk::RenderingInfo {}
        .setLayerCount(1)
        .setRenderArea(area)
        .setColorAttachmentCount(attachments.size())
        .setPColorAttachments(attachments.data()));
}

void VulkanDevice::endRendering(CommandList CL) {
    VulkanCommandList& cmd = commands.get(CL);

    cmd.buffer.endRendering();
}

void VulkanDevice::setPipeline(CommandList CL, Pipeline P) {
    VulkanCommandList& cmd = commands.get(CL);
    VulkanPipeline& pipeline = pipelines.get(P);

    cmd.buffer.bindPipeline(convert(pipeline.type), pipeline.handle);
    cmd.buffer.bindDescriptorSets(
        convert(pipeline.type), 
        pipeline_layout, 
        0, 
        descriptor_set, 
        {});
}

void VulkanDevice::setViewport(CommandList CL, Viewport viewport) {
    VulkanCommandList& cmd = commands.get(CL);

    cmd.buffer.setViewport(0, vk::Viewport {}
        .setX(viewport.x)
        .setY(viewport.y)
        .setWidth(viewport.width)
        .setHeight(viewport.height)
        .setMinDepth(viewport.min_depth)
        .setMaxDepth(viewport.max_depth));
}

void VulkanDevice::setScissor(CommandList CL, Scissor scissor) {
    VulkanCommandList& cmd = commands.get(CL);

    cmd.buffer.setScissor(0, vk::Rect2D {}
        .setOffset({ scissor.x, scissor.y })
        .setExtent({ scissor.width, scissor.height }));
}

void VulkanDevice::drawInstanced(
        CommandList CL, ptr vertex, ptr fragment, uint32_t vertices, 
        uint32_t instances) {
    VulkanCommandList& cmd = commands.get(CL);
    
    cmd.buffer.pushConstants(
        pipeline_layout,
        vk::ShaderStageFlagBits::eVertex,
        VERTEX_STAGE * sizeof(vk::DeviceAddress),
        sizeof(vk::DeviceAddress),
        &vertex);

    cmd.buffer.pushConstants(
        pipeline_layout,
        vk::ShaderStageFlagBits::eFragment,
        FRAGMENT_STAGE * sizeof(vk::DeviceAddress),
        sizeof(vk::DeviceAddress),
        &fragment);

    cmd.buffer.draw(vertices, instances, 0, 0);
}

void VulkanDevice::drawIndexedInstanced(
        CommandList CL, 
        ptr vertex, 
        ptr fragment, 
        ptr index, 
        uint32_t indices, 
        uint32_t offset,
        uint32_t instances) {
    VulkanCommandList& cmd = commands.get(CL);
 
    cmd.buffer.pushConstants(
        pipeline_layout,
        vk::ShaderStageFlagBits::eVertex,
        VERTEX_STAGE * sizeof(vk::DeviceAddress),
        sizeof(vk::DeviceAddress),
        &vertex);

    cmd.buffer.pushConstants(
        pipeline_layout,
        vk::ShaderStageFlagBits::eFragment,
        FRAGMENT_STAGE * sizeof(vk::DeviceAddress),
        sizeof(vk::DeviceAddress),
        &fragment);

    cmd.buffer.bindIndexBuffer(allocs[index].buffer, 0, vk::IndexType::eUint32);
    cmd.buffer.drawIndexed(indices, instances, offset, 0, 0);
}

void VulkanDevice::dispatch(
        CommandList CL, 
        ptr compute, 
        uint32_t x, 
        uint32_t y,
        uint32_t z) {
    VulkanCommandList& cmd = commands.get(CL);
    
    cmd.buffer.pushConstants(
        pipeline_layout, 
        vk::ShaderStageFlagBits::eCompute, 
        COMPUTE_STAGE * sizeof(vk::DeviceAddress),
        sizeof(vk::DeviceAddress),
        &compute);

    cmd.buffer.dispatch(x, y, z);
}
