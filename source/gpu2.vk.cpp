//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "gpu2.h"
#include "gpu2.vk.h"

using namespace gpu;

VulkanDevice::VulkanDevice(const DeviceInfo& info) : Device(info) {

}

void VulkanDevice::destroy() {
    
}

void VulkanDevice::waitIdle() {
    VK_CHECK(device.waitIdle());
}

void VulkanDevice::waitIdle(QueueType type) {
    VK_CHECK(queues[static_cast<uint32_t>(type)].handle.waitIdle());
}

Shader VulkanDevice::createShader(const ShaderInfo& info) {
    VulkanShader shader = {};
    shader.info = info;

    auto module_info = vk::ShaderModuleCreateInfo {}
        .setCodeSize(info.bytecode.size())
        .setPCode(info.bytecode.data());

    VK_NULL_ON_ERROR(device.createShaderModule(
        &module_info, 
        nullptr, 
        &shader.module));

    return shaders.add(shader);
}

void VulkanDevice::freeShader(Shader handle) {
    VulkanShader shader = shaders.remove(handle);

    if (shader.module) {
        device.destroyShaderModule(shader.module);
        shader.module = nullptr;
    }
}
