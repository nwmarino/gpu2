//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#ifndef GPU2_VULKAN_H_
#define GPU2_VULKAN_H_

#include "gpu2.h"
#include "gpu2.in.h"

#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

#define VULKAN_HPP_NO_EXCEPTIONS
#include "vulkan/vulkan.hpp"

struct VmaAllocation_T;
struct VmaAllocator_T;

using VmaAllocation = VmaAllocation_T*;
using VmaAllocator = VmaAllocator_T*;

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

#define VK_NULL_ON_ERROR(result) \
    do { \
        if ((result) != vk::Result::eSuccess) { \
            return null; \
        } \
    } while(false)

#define SAMPLER_BINDING 0
#define TEXTURE_BINDING 1
#define RW_TEXTURE_BINDING 2

namespace gpu {

struct VulkanAlloc {
    MemoryType type = {};
    ptr device = null;
    void* host = nullptr;
    vk::Buffer buffer = nullptr;
    VmaAllocation alloc = nullptr;
};

struct VulkanTexture {
    TextureInfo info = {};
    bool borrowed = false;
    vk::Image image = nullptr;
    VmaAllocation alloc = nullptr;
    std::map<vk::ImageViewCreateInfo, vk::ImageView> views = {};
};

struct VulkanSampler {
    SamplerInfo info = {};
    vk::Sampler handle = nullptr;
};

struct VulkanShader {
    ShaderInfo info = {};
    vk::ShaderModule module = nullptr;
};

struct VulkanSwapchain {
    SwapchainInfo info = {};
    vk::SurfaceKHR surface = nullptr;
    vk::SwapchainKHR handle = nullptr;
    uint32_t index = 0;
    std::vector<Texture> textures = {};
    std::vector<Semaphore> semaphores = {};
};

struct VulkanCommandList {
    vk::CommandBuffer buffer = nullptr;
    vk::CommandPool pool = nullptr;
    vk::Fence fence = nullptr;
};

struct VulkanPipeline {
    PipelineType type = {};
    vk::Pipeline handle = nullptr;
};

struct VulkanQueue {
    QueueType type = {};
    vk::Queue handle = nullptr;
    uint32_t family = 0;
    std::vector<VulkanCommandList> pending = {};
};

struct VulkanSemaphore {
    vk::Semaphore handle = nullptr;
};

struct VulkanFence {
    vk::Fence handle = nullptr;
};

template<typename T>
class IndexHeap {
public:
    using index_type = T;

private:
    index_type m_bump = 0;
    std::stack<index_type> m_free = {};

public:
    IndexHeap(index_type start = 0) : m_bump(start) {}

    index_type alloc() {
        if (m_free.empty()) {
            return m_bump++;
        } else {
            index_type i = m_free.top();
            m_free.pop();
            return i;
        }
    }

    void release(index_type i) {
        m_free.push(i);
    }
};

struct VulkanDevice : public Device {
    vk::Instance instance = nullptr;
    vk::DebugUtilsMessengerEXT messenger = nullptr;
    vk::PhysicalDevice physical_device = nullptr;
    vk::Device device = nullptr;
    VmaAllocator allocator = nullptr;

    vk::DescriptorSetLayout descriptor_layout = nullptr;
    vk::DescriptorPool descriptor_pool = nullptr;
    vk::DescriptorSet descriptor_set = nullptr;
    vk::PipelineLayout pipeline_layout = nullptr;

    std::mutex memory_mutex;
    std::vector<VulkanQueue> queues = {};
    std::unordered_map<ptr, VulkanAlloc> allocs = {};

    IndexHeap<Descriptor> sampler_descriptors = {};
    IndexHeap<Descriptor> texture_descriptors = {};
    IndexHeap<Descriptor> rw_texture_descriptors = {};

    uint32_t num_sampler_descriptors = 0;
    uint32_t num_texture_descriptors = 0;
    uint32_t num_rw_texture_descriptors = 0;

    Pool<VulkanTexture> textures = {};
    Pool<VulkanSampler> samplers = {};
    Pool<VulkanShader> shaders = {};
    Pool<VulkanSwapchain> swapchains = {};
    Pool<VulkanCommandList> commands = {};
    Pool<VulkanPipeline> pipelines = {};
    Pool<VulkanSemaphore> semaphores = {};
    Pool<VulkanFence> fences = {};

    explicit VulkanDevice(const DeviceInfo& info);

    void createVulkanCore();
    void createLogicalDevice();
    void createMemoryAllocator();
    void createDescriptorSetup();
    void createPipelineLayout();

    void cleanupCommands(QueueType);

    void destroy() override;

    void waitIdle() override;
    void waitIdle(QueueType) override;

    AllocResult malloc(uint64_t size, MemoryType type) override;
    AllocResult malloc(uint64_t size, uint64_t align, MemoryType type) override;
    void free(ptr) override;
    void* deviceToHostPointer(ptr) override;

    Texture createTexture(const TextureInfo&) override;
    void freeTexture(Texture) override;

    void copyToTexture(void* src, Texture dst, const CopyRegion& region) override;
    void copyFromTexture(Texture src, void* dst, const CopyRegion& region) override;

    Sampler createSampler(const SamplerInfo&) override;
    void freeSampler(Sampler) override;

    Shader createShader(const ShaderInfo&) override;
    void freeShader(Shader) override;

    Pipeline createGraphicsPipeline(Shader vertex, Shader fragment, const RasterInfo& info) override;
    Pipeline createComputePipeline(Shader compute) override;
    void freePipeline(Pipeline) override;

    Swapchain createSwapchain(const SwapchainInfo&) override;
    void freeSwapchain(Swapchain) override;
    void resizeSwapchain(Swapchain, uint32_t width, uint32_t height) override;
    Pair<Texture, Semaphore> acquireSwapchainTarget(Swapchain, Semaphore) override;
    void present(Swapchain, Semaphore) override;

    Semaphore createSemaphore() override;
    void freeSemaphore(Semaphore) override;

    Fence createFence() override;
    void freeFence(Fence) override;
    void waitForFences(const std::vector<Fence>&) override;
    void resetFences(const std::vector<Fence>&) override;

    Descriptor getSamplerDescriptor(Sampler) override;
    Descriptor getTextureDescriptor(Texture, TextureViewInfo) override;
    Descriptor getRWTextureDescriptor(Texture, TextureViewInfo) override;
    void releaseSamplerDescriptor(Descriptor) override;
    void releaseTextureDescriptor(Descriptor) override;
    void releaseRWTextureDescriptor(Descriptor) override;

    CommandList beginRecording(QueueType) override;
    void submit(QueueType, 
                CommandList, 
                Semaphore signal = null, 
                Semaphore wait = null) override;

    void copy(CommandList, ptr src, ptr dst, uint64_t size) override;

    void clearTextureColor(CommandList, const ClearTextureInfo&) override;
    void clearTextureDepth(CommandList, const ClearTextureInfo&) override;

    void barrier(CommandList, StageFlags before, StageFlags after) override;
    void barrier(CommandList, Texture, TextureState prev, TextureState next) override;

    void beginRendering(CommandList, const RenderInfo&) override;
    void endRendering(CommandList) override;

    void setPipeline(CommandList, Pipeline) override;
    void setViewport(CommandList, Viewport) override;
    void setScissor(CommandList, Scissor) override;

    void drawInstanced(CommandList, 
                       ptr vertex, 
                       ptr fragment, 
                       uint32_t vertices, 
                       uint32_t instances) override;
    void drawIndexedInstances(CommandList,
                              ptr vertex, 
                              ptr fragment, 
                              ptr index,
                              IndexType type,
                              uint32_t indices, 
                              uint32_t instances) override;
    void dispatch(CommandList, 
                  ptr compute, 
                  uint32_t x, 
                  uint32_t y, 
                  uint32_t z) override;
};

vk::PipelineBindPoint convert(PipelineType type);
vk::IndexType convert(IndexType type);
vk::ImageType convert(TextureType type);
vk::ImageLayout convert(TextureState state);
vk::PresentModeKHR convert(PresentMode mode);
vk::Format convert(Format format);
vk::Filter convert(Filter filter);
vk::SamplerAddressMode convert(AddressMode mode);
vk::BorderColor convert(BorderColor color);
vk::PrimitiveTopology convert(Topology topology);
vk::PolygonMode convert(FillMode fill);
vk::FrontFace convert(FrontFace face);
vk::AttachmentLoadOp convert(LoadOp op);
vk::AttachmentStoreOp convert(StoreOp op);
vk::CompareOp convert(CompareOp op);
vk::BlendOp convert(BlendOp op);
vk::StencilOp convert(StencilOp op);
vk::BlendFactor convert(Factor factor);
vk::CullModeFlags convert(CullMode cull);
vk::PipelineStageFlags2 convert(StageFlags stage);
vk::ShaderStageFlags convert(ShaderStageFlags stage);
vk::ImageUsageFlags convert(UsageFlags usage);

vk::ImageViewType convertViewType(TextureType type);
vk::SamplerMipmapMode convertMipmapFilter(Filter filter);
vk::ImageAspectFlags convertAspectMask(Format format);
vk::SampleCountFlagBits convertSampleCount(uint32_t samples);

} // namespace gpu

#endif // GPU2_VULKAN_H_
