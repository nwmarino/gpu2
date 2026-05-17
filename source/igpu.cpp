//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "include/igpu.h"

#include <array>
#include <iostream>

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
#endif // IGPU_VULKAN

#ifdef IGPU_GLFW
    #include "GLFW/glfw3.h"
#endif // IGPU_GLFW

#include <unordered_map>

namespace gpu {

struct AllocationInfo final {
    MemoryType type;
    GpuAddr device;
    void* host;

#ifdef IGPU_VULKAN
    vk::Buffer buffer;
    VmaAllocation alloc;
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
#ifdef IGPU_VULKAN
    vk::Pipeline pipeline;
#endif // IGPU_VULKAN
};

struct CommandList_T final {
#ifdef IGPU_VULKAN
    vk::CommandPool pool = nullptr;
    vk::CommandBuffer cmd = nullptr;
#endif // IGPU_VULKAN
};

struct RenderingDevice_T final {
#ifdef IGPU_VULKAN
    vk::Instance instance = nullptr;
    vk::DebugUtilsMessengerEXT messenger = nullptr;

    vk::PhysicalDevice physical_device = nullptr;
    vk::Device device = nullptr;

    vk::SurfaceKHR surface = nullptr;
    vk::SwapchainKHR swapchain = nullptr;

    vk::Fence submit_fence = nullptr;
    vk::CommandPool command_pool = nullptr;
    vk::PipelineLayout pipeline_layout = nullptr;

    VmaAllocator vma = nullptr;
#endif // IGPU_VULKAN

    // Backend-agnostic fields.
    std::mutex memory_mutex;
    std::unordered_map<GpuAddr, AllocationInfo> allocations = {};
    std::unordered_map<void*, GpuAddr> addresses = {};

    std::unordered_map<CommandList, CommandList_T> commands = {};
    std::unordered_map<QueueType, Queue_T> queues = {};
    std::unordered_map<Semaphore, Semaphore_T> semaphores = {};
    std::unordered_map<Pipeline, Pipeline_T> pipelines = {};

    CommandList_T& getCommandList(CommandList list) {
        auto it = commands.find(list);
        IGPU_ASSERT(it != commands.end(), "invalid CommandList handle!");
        return it->second;
    }

    Queue_T& getQueue(QueueType queue) {
        return queues.at(queue);
    }

    Semaphore_T& getSemaphore(Semaphore sema) {
        auto it = semaphores.find(sema);
        IGPU_ASSERT(it != semaphores.end(), "invalid Semaphore handle!");
        return it->second;
    }

    Pipeline_T& getPipeline(Pipeline pipeline) {
        auto it = pipelines.find(pipeline);
        IGPU_ASSERT(it != pipelines.end(), "invalid Pipeline handle!");
        return it->second;
    }
};

#ifdef IGPU_VULKAN

static vk::Format formatToVulkan(Format format) {
    switch (format) {
        case Format::eUndefined: 
            return vk::Format::eUndefined;
        case Format::eRGBA8Unorm: 
            return vk::Format::eR8G8B8A8Unorm;
        case Format::eD32Float: 
            return vk::Format::eD32Sfloat;
    }
}

static vk::PrimitiveTopology topologyToVulkan(Topology topology) {
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

static vk::CullModeFlags cullToVulkan(Cull cull) {
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

static vk::PolygonMode fillToVulkan(Fill fill) {
    switch (fill) {
        case Fill::eFill:
            return vk::PolygonMode::eFill;
        case Fill::eLine:
            return vk::PolygonMode::eLine;
    }
}

static vk::SampleCountFlagBits samplesToVulkan(uint8_t sample_count) {
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

static vk::BlendOp blendOpToVulkan(BlendOp op) {
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

static vk::BlendFactor blendFactorToVulkan(Factor factor) {
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

static vk::StencilOp stencilOpToVulkan(StencilOp op) {
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

static vk::PipelineShaderStageCreateInfo createShaderStage(
        RenderingDevice_T& rd, 
        std::string_view spirv, 
        vk::ShaderStageFlagBits stage) {
    auto stage_info = vk::PipelineShaderStageCreateInfo {}
        .setStage(stage)
        .setPName("main");

    auto module_info = vk::ShaderModuleCreateInfo {}
        .setCodeSize(static_cast<uint32_t>(spirv.length()))
        .setPCode(reinterpret_cast<const uint32_t*>(spirv.data()));

    // @Todo: Result value should be considered.
    (void) rd.device.createShaderModule(&module_info, 
                                        nullptr, 
                                        &stage_info.module);
    return stage_info;
}

#endif // IGPU_VULKAN

//>==--------------------------------------------------------------------------
//                          RenderingDevice Implementation
//>==--------------------------------------------------------------------------

RenderingDevice* RenderingDevice::Create(const RenderingDeviceInfo& info) {
    return nullptr;
}

RenderingDevice::~RenderingDevice() {

}

GpuAddr RenderingDevice::malloc(uint64_t size, MemoryType type) {
    std::lock_guard<std::mutex> lock(m_impl->memory_mutex);

    GpuAddr addr = 0;

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
    /*
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
    addr = static_cast<GpuAddr>(m_impl->device.getBufferAddress(&addr_info));
    if (!addr) {
        vmaDestroyBuffer(m_impl->vma, alloc.buffer, alloc.alloc);
        return 0;
    }

    alloc.device = static_cast<GpuAddr>(addr);

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

void RenderingDevice::free(GpuAddr addr) {
    std::lock_guard<std::mutex> lock(m_impl->memory_mutex);

    // Look for the address in the device allocation table.
    auto it = m_impl->allocations.find(addr);
    if (it == m_impl->allocations.end())
        return;

    AllocationInfo info = it->second;

#ifdef IGPU_VULKAN

    // @Todo: Check for null allocation here, maybe.
    vmaDestroyBuffer(m_impl->vma, info.buffer, info.alloc);

#endif // IGPU_VULKAN

    m_impl->allocations.erase(it);
}

void* RenderingDevice::deviceToHostAddress(GpuAddr addr) {
    std::lock_guard<std::mutex> lock(m_impl->memory_mutex);

    // Look for the device address in the allocation table.
    auto it = m_impl->allocations.find(addr);
    if (it != m_impl->allocations.end())
        return it->second.host;

    return nullptr;
}

GpuAddr RenderingDevice::hostToDeviceAddress(void* ptr) {
    std::lock_guard<std::mutex> lock(m_impl->memory_mutex);

    // Look for the host address in the address translation table.
    auto it = m_impl->addresses.find(ptr);
    if (it != m_impl->addresses.end())
        return it->second;
    
    return 0;
}

CommandList RenderingDevice::beginRecording(QueueType queue) {
    CommandList_T list = {};

#ifdef IGPU_VULKAN

    auto pool_info = vk::CommandPoolCreateInfo {}
        .setFlags(vk::CommandPoolCreateFlagBits::eTransient)
        .setQueueFamilyIndex(m_impl->getQueue(queue).family);
    
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

    result = m_impl->device.allocateCommandBuffers(&buffer_info, &list.cmd);
    if (result != vk::Result::eSuccess) {
        m_impl->device.destroyCommandPool(list.pool);
        return 0;
    }

    auto begin_info = vk::CommandBufferBeginInfo {}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    if (list.cmd.begin(begin_info) != vk::Result::eSuccess) {
        m_impl->device.destroyCommandPool(list.pool);
        return 0;
    }

#endif // IGPU_VULKAN

    CommandList handle = m_impl->commands.size();
    m_impl->commands.emplace(handle, list);
    return handle;
}

void RenderingDevice::submit(QueueType queue, 
                             TimelinePair signal, 
                             TimelinePair wait, 
                             std::span<CommandList> lists) {
#ifdef IGPU_VULKAN

    std::vector<vk::CommandBufferSubmitInfo> buffers = {};
    buffers.reserve(lists.size());

    for (CommandList list : lists) {
        buffers.push_back(vk::CommandBufferSubmitInfo {}
            .setCommandBuffer(m_impl->getCommandList(list).cmd));
    }

    auto signal_info = vk::SemaphoreSubmitInfo {}
        .setSemaphore(m_impl->getSemaphore(signal.sema).sema)
        .setValue(signal.value);

    auto wait_info = vk::SemaphoreSubmitInfo {}
        .setSemaphore(m_impl->getSemaphore(wait.sema).sema)
        .setValue(wait.value);

    auto submit_info = vk::SubmitInfo2 {}
        .setCommandBufferInfoCount(static_cast<uint32_t>(buffers.size()))
        .setPCommandBufferInfos(buffers.data())
        .setSignalSemaphoreInfoCount(1)
        .setPSignalSemaphoreInfos(&signal_info)
        .setWaitSemaphoreInfoCount(1)
        .setPWaitSemaphoreInfos(&wait_info);

    vk::Result result = m_impl->getQueue(queue).queue.submit2(submit_info);
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

    Semaphore handle = m_impl->semaphores.size();
    m_impl->semaphores.emplace(handle, sema);
    return handle;
}

void RenderingDevice::freeSemaphore(Semaphore sema) {
    // Find the underlying semaphore resource.
    auto it = m_impl->semaphores.find(sema);
    if (it == m_impl->semaphores.end())
        return; // Handle is invalid.

    Semaphore_T& igpu_sema = it->second;

#ifdef IGPU_VULKAN

    if (igpu_sema.sema) {
        m_impl->device.destroySemaphore(igpu_sema.sema);
        igpu_sema.sema = nullptr;
    }

#endif // IGPU_VULKAN

    m_impl->semaphores.erase(it);
}

void RenderingDevice::waitSemaphore(Semaphore sema, uint64_t value) {
    Semaphore_T& igpu_sema = m_impl->getSemaphore(sema);

#ifdef IGPU_VULKAN

    auto wait_info = vk::SemaphoreWaitInfo {}
        .setSemaphoreCount(1)
        .setPSemaphores(&igpu_sema.sema);

    // @Todo: Figure out what to do with result.
    (void) m_impl->device.waitSemaphores(&wait_info, value);

#endif // IGPU_VULKAN
}

Pipeline RenderingDevice::createComputePipeline(std::string_view compute) {
    Pipeline_T pipeline = {};

#ifdef IGPU_VULKAN

    auto pipeline_info = vk::ComputePipelineCreateInfo {}
        .setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT)
        .setLayout(m_impl->pipeline_layout)
        .setStage(createShaderStage(*m_impl, compute, vk::ShaderStageFlagBits::eCompute));

    vk::Result result = m_impl->device.createComputePipelines(
        nullptr, 1, &pipeline_info, nullptr, &pipeline.pipeline);

    // Shader module is useless now.
    if (pipeline_info.stage.module)
        m_impl->device.destroyShaderModule(pipeline_info.stage.module);

    if (result != vk::Result::eSuccess)
        return 0;

#endif // IGPU_VULKAN

    Pipeline handle = m_impl->pipelines.size();
    m_impl->pipelines.emplace(handle, pipeline);
    return handle;
}

Pipeline RenderingDevice::createGraphicsPipeline(std::string_view vertex, 
                                                 std::string_view fragment, 
                                                 const RasterInfo& info) {
    Pipeline_T pipeline = {};

#ifdef IGPU_VULKAN

    // Collect color attachment formats as Vulkan formats.
    std::vector<vk::Format> formats = {};
    for (const AttachmentInfo& att : info.atts)
        formats.push_back(formatToVulkan(att.format));

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
        .setTopology(topologyToVulkan(info.topology));

    auto multisampling_info = vk::PipelineMultisampleStateCreateInfo {}
        .setRasterizationSamples(samplesToVulkan(info.sample_count))
        .setAlphaToCoverageEnable(info.alpha_to_coverage);

    auto dynamic_info = vk::PipelineDynamicStateCreateInfo {}
        .setDynamicStateCount(static_cast<uint32_t>(dynamic_states.size()))
        .setPDynamicStates(dynamic_states.data());

    // Viewport and scissor are left as dynamic state.
    auto viewport_info = vk::PipelineViewportStateCreateInfo {}
        .setViewportCount(1)
        .setScissorCount(1);

    auto raster_info = vk::PipelineRasterizationStateCreateInfo {}
        .setPolygonMode(fillToVulkan(info.fill))
        .setCullMode(cullToVulkan(info.cull))
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .setLineWidth(1.f)
        .setDepthBiasEnable(vk::True);

    auto render_info = vk::PipelineRenderingCreateInfo {}
        .setColorAttachmentCount(static_cast<uint32_t>(formats.size()))
        .setPColorAttachmentFormats(formats.data())
        .setDepthAttachmentFormat(formatToVulkan(info.depth))
        .setStencilAttachmentFormat(formatToVulkan(info.stencil));

    std::vector<vk::PipelineColorBlendAttachmentState> blend_states = {};
    blend_states.reserve(info.atts.size());

    for (const AttachmentInfo& att : info.atts) {
        blend_states.push_back(vk::PipelineColorBlendAttachmentState {}
            .setColorBlendOp(blendOpToVulkan(att.color_op))
            .setSrcColorBlendFactor(blendFactorToVulkan(att.src_color_factor))
            .setDstColorBlendFactor(blendFactorToVulkan(att.dst_color_factor))
            .setAlphaBlendOp(blendOpToVulkan(att.alpha_op))
            .setSrcAlphaBlendFactor(blendFactorToVulkan(att.src_alpha_factor))
            .setDstAlphaBlendFactor(blendFactorToVulkan(att.dst_alpha_factor))
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

    Pipeline handle = m_impl->pipelines.size();
    m_impl->pipelines.emplace(handle, pipeline);
    return handle;
}

void RenderingDevice::freePipeline(Pipeline pipeline) {
    // Find the underlying pipeline resource.
    auto it = m_impl->pipelines.find(pipeline);
    if (it == m_impl->pipelines.end())
        return; // Handle is invalid.

    Pipeline_T& igpu_pipeline = it->second;

#ifdef IGPU_VULKAN

    if (igpu_pipeline.pipeline) {
        m_impl->device.destroyPipeline(igpu_pipeline.pipeline);
        igpu_pipeline.pipeline = nullptr;
    }

#endif // IGPU_VULKAN

    m_impl->pipelines.erase(it);
}

} // namespace gpu
