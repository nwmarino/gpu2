//
//  compute.cpp
//
//  Copyright (c) 2026 Nick Marino.
//  All rights reserved.
//

#include "examples/common.h"
#include "source/gpu2.h"

#include <cassert>

int main() {
    gpu::DeviceInfo device_info = {};
    device_info.backend = gpu::Backend::Vulkan;
    device_info.validation = true;

    gpu::Device* device = gpu::createDevice(device_info);

    gpu::Alloc<float> data = device->malloc<float>(128);

    // Set the entire buffer to 0.0, should be 42.0 after dispatch.
    for (uint32_t i = 0; i < 128u; ++i) {
        data.host[i] = 0.f;
    }

    gpu::Shader shader = device->createShader(gpu::ShaderInfo {
        .bytecode = readFile("shaders/compute.comp.spv"),
        .entry = "main",
        .stage = gpu::ShaderStageFlags::Compute,
    });

    gpu::Pipeline pipeline = device->createComputePipeline(shader);

    gpu::CommandList cmd = device->beginRecording(gpu::QueueType::Compute);

    device->setPipeline(cmd, pipeline);

    device->dispatch(cmd, data.device, 2, 1, 1);

    device->submit(gpu::QueueType::Compute, cmd);

    device->waitIdle();

    for (uint32_t i = 0; i < 128u; ++i) {
        assert(reinterpret_cast<float*>(data.host)[i] == 42.0);
    }

    device->free(data.device);

    device->freePipeline(pipeline);

    device->freeShader(shader);
    
    device->destroy();
}
