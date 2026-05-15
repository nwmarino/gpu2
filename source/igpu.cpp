//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "../include/igpu.h"

#ifdef IGPU_VULKAN
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
#endif // IGPU_VULKAN

#ifdef IGPU_GLFW
    #include "GLFW/glfw3.h"
#endif // IGPU_GLFW

#include <unordered_map>

namespace gpu {

struct AllocationInfo {
    MemoryType type;
    GpuAddr addr;
    void* host;

#ifdef IGPU_VULKAN
    VkBuffer buffer;
    VmaAllocation alloc;
#endif // IGPU_VULKAN
};

struct RenderingDevice_T final {
#ifdef IGPU_VULKAN
    vk::Instance instance = nullptr;
    vk::DebugUtilsMessengerEXT messenger = nullptr;

    vk::PhysicalDevice physical_device = nullptr;
    vk::Device device = nullptr;

    vk::Queue graphics = nullptr;
    vk::Queue compute = nullptr;
    vk::Queue present = nullptr;

    vk::SurfaceKHR surface = nullptr;
    vk::SwapchainKHR swapchain = nullptr;

    VmaAllocator vma = nullptr;
#endif // IGPU_VULKAN

    // Backend-agnostic fields.
    std::mutex memory_mutex;
    std::unordered_map<GpuAddr, AllocationInfo> allocations = {};
    std::unordered_map<void*, GpuAddr> addresses = {};
};

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

#ifdef IGPU_VULKAN
    vk::MemoryPropertyFlags mem_flags = {};
    VmaMemoryUsage mem_usage = {};

    switch (type) {
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
    buffer_info.size = static_cast<vk::DeviceSize>(size);
    buffer_info.sharingMode = vk::SharingMode::eConcurrent;
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

    vk::Buffer buffer = nullptr;
    VmaAllocation alloc = nullptr;
    VmaAllocationInfo alloc_info = {};

    vk::Result result = vk::Result(vmaCreateBuffer(
        m_impl->vma, 
        reinterpret_cast<VkBufferCreateInfo*>(&buffer_info), 
        &vma_info, 
        reinterpret_cast<VkBuffer*>(&buffer), 
        &alloc, 
        &alloc_info));

    if (result != vk::Result::eSuccess)
        return 0;
    
    vk::BufferDeviceAddressInfo addr_info = {};
    addr_info.buffer = buffer;

    // Get the device address for the new memory buffer.
    addr = static_cast<GpuAddr>(m_impl->device.getBufferAddress(&addr_info));
    if (!addr) {
        vmaDestroyBuffer(m_impl->vma, buffer, alloc);
        return 0;
    }

    AllocationInfo AI = {};
    AI.type = type;
    AI.addr = static_cast<GpuAddr>(addr);
    AI.alloc = alloc;
    AI.buffer = buffer;

    // If the memory is not device only, then register the host address.
    if (type != MemoryType::eDevice) {
        AI.host = alloc_info.pMappedData;
        m_impl->addresses.emplace(AI.host, addr);
    }

    m_impl->allocations.emplace(addr, AI);
#endif // IGPU_VULKAN

    return addr;
}

void RenderingDevice::free(GpuAddr addr) {
    std::lock_guard<std::mutex> lock(m_impl->memory_mutex);

    // Look for the address in the device allocation table.
    auto it = m_impl->allocations.find(addr);
    if (it == m_impl->allocations.end())
        return;

    AllocationInfo info = it->second; // Important copy.
    info.addr = 0;

#ifdef IGPU_VULKAN
    vmaDestroyBuffer(m_impl->vma, info.buffer, info.alloc);
    info.alloc = nullptr;
    info.buffer = nullptr;
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

} // namespace gpu
