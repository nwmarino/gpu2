//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#ifndef GPU2_VULKAN_H_
#define GPU2_VULKAN_H_

#include "gpu2.in.h"

#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

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

#define TEXTURE_BINDING 0
#define RW_TEXTURE_BINDING 1
#define SAMPLER_BINDING 2

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
    vk::SwapchainKHR handle = nullptr;
};

struct VulkanCommandList {
    vk::CommandBuffer buffer = nullptr;
    vk::CommandPool pool = nullptr;
    vk::Fence fence = nullptr;
};

struct VulkanPipeline {
    PipelineType type = {};
    vk::Pipeline pipeline = nullptr;
};

struct VulkanQueue {
    QueueType type = {};
    vk::Queue handle = nullptr;
    uint32_t family = 0;
};

struct VulkanSemaphore {
    vk::Semaphore handle = nullptr;
};

struct VulkanFence {
    vk::Fence handle = nullptr;
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

    uint32_t num_sampled_images = 0;
    uint32_t num_storage_images = 0;
    uint32_t num_samplers = 0;

    uint32_t sampled_image_size = 0;
    uint32_t storage_image_size = 0;
    uint32_t sampler_size = 0;

    Pool<VulkanTexture> textures = {};
    Pool<VulkanSampler> samplers = {};
    Pool<VulkanShader> shaders = {};
    Pool<VulkanSwapchain> swapchains = {};
    Pool<VulkanCommandList> commands = {};
    Pool<VulkanPipeline> pipelines = {};
    Pool<VulkanSemaphore> semaphores = {};
    Pool<VulkanFence> fences = {};

    explicit VulkanDevice(const DeviceInfo& info);

    void destroy() override;

    void waitIdle() override;
    void waitIdle(QueueType) override;

    Shader createShader(const ShaderInfo&) override;
    void freeShader(Shader) override;
};

vk::PipelineBindPoint convert(PipelineType type);
vk::ImageType convert(TextureType type);
vk::PresentModeKHR convert(PresentMode mode);
vk::Format convert(Format format);
vk::Filter convert(Filter filter);
vk::SamplerAddressMode convert(AddressMode mode);
vk::BorderColor convert(BorderColor color);
vk::PrimitiveTopology convert(Topology topology);
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

vk::SamplerMipmapMode convertMipmapFilter(Filter filter);
vk::ImageAspectFlags convertAspectMask(Format format);
vk::SampleCountFlags convertSampleCount(uint32_t samples);

} // namespace gpu

#endif // GPU2_VULKAN_H_
