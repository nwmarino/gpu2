//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "../include/igpu.h"

#define VULKAN_HPP_NO_EXCEPTIONS
#include "vulkan/vulkan.hpp"

#define VMA_IMPLEMENTATION
#if defined(__clang__)
    // The VMA header tends to contain a lot of warnings for clang that just 
    // clutter the building process.
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Weverything"
    #include "vma.h"
    #pragma clang diagnostic pop
#else
    #include "vma.h"
#endif // defined(__clang__)

namespace gpu {

struct RenderingDevice_T final {
#ifdef IGPU_VULKAN
    vk::Instance m_instance = nullptr;
    vk::DebugUtilsMessengerEXT m_messenger = nullptr;

    vk::PhysicalDevice m_physical_device = nullptr;
    vk::Device m_device = nullptr;

    vk::Queue m_graphics = nullptr;
    vk::Queue m_compute = nullptr;
    vk::Queue m_present = nullptr;

    vk::SurfaceKHR m_surface = nullptr;
    vk::SwapchainKHR m_swapchain = nullptr;

    VmaAllocator m_vma = nullptr;
#endif // IGPU_VULKAN

    // Backend-agnostic fields.
    std::mutex m_memory_mutex;
};

//>==--------------------------------------------------------------------------
//                          RenderingDevice Implementation
//>==--------------------------------------------------------------------------

RenderingDevice* RenderingDevice::Create(const RenderingDeviceInfo& info) {

}

RenderingDevice::~RenderingDevice() {

}

void* RenderingDevice::malloc(uint32_t bytes, MemoryType memory) {

}

void* RenderingDevice::malloc(uint32_t bytes, uint32_t align, MemoryType memory) {
    std::lock_guard<std::mutex> lock(m_impl->m_memory_mutex);

#ifdef IGPU_VULKAN

    vk::MemoryPropertyFlags mem_flags = {};
    VmaMemoryUsage mem_usage = {};

    switch (memory) {
    case MemoryType::eDefault:
        mem_flags = vk::MemoryPropertyFlagBits::eHostVisible 
                  | vk::MemoryPropertyFlagBits::eHostCoherent;
        mem_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        break;
    case MemoryType::eDevice:
        mem_flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        mem_usage = VMA_MEMORY_USAGE_GPU_ONLY;
        break;
    case MemoryType::eReadback:
        mem_flags = vk::MemoryPropertyFlagBits::eHostVisible 
                  | vk::MemoryPropertyFlagBits::eHostCoherent 
                  | vk::MemoryPropertyFlagBits::eHostCached;
        mem_usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        break;
    }
 
    vk::BufferCreateInfo buffer_info = {};
    buffer_info.usage = vk::BufferUsageFlagBits::eStorageBuffer 
                      | vk::BufferUsageFlagBits::eIndirectBuffer
                      | vk::BufferUsageFlagBits::eShaderDeviceAddress 
                      | vk::BufferUsageFlagBits::eTransferSrc 
                      | vk::BufferUsageFlagBits::eTransferDst;
    buffer_info.size = static_cast<vk::DeviceSize>(bytes);
    buffer_info.sharingMode = vk::SharingMode::eConcurrent;
    /*
    buffer_info.queueFamilyIndexCount = ...;
    buffer_info.pQueueFamilyIndices = ...;
    */

    VmaAllocationCreateFlags alloc_flags = {};
    if (memory != MemoryType::eDevice)
        alloc_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.flags = alloc_flags;
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

#endif // IGPU_VULKAN
}

} // namespace gpu
