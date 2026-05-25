//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "include/igpu.h"

#include <cassert>
#include <fstream>
#include <string>

#define FRAMES_IN_FLIGHT 3

static std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    std::streamsize size = file.tellg();
    std::string contents = "";
    contents.resize(size);
    file.seekg(0);
    file.read(contents.data(), size);
    file.close();
    return contents;
}

int main() {
    auto device_info = gpu::DeviceInfo {}
        .setEnableValidation(true);

    gpu::Device* device = gpu::Device::Create(device_info);

    std::string compute_ir = readFile("C:/Users/nwmar/igpu/examples/shaders/compute.comp.spv");

    gpu::ptr gdata = device->malloc(sizeof(float) * 128);
    float* hdata = static_cast<float*>(device->deviceToHostAddress(gdata));

    // Set the entire buffer to 0.0, should be 42.0 after dispatch.
    for (uint32_t i = 0; i < 128u; ++i) {
        hdata[i] = 0.0f;
    }

    gpu::Pipeline pipeline = device->createComputePipeline(compute_ir);

    gpu::CommandList cmd = device->beginRecording(gpu::QueueType::eCompute);

    device->setPipeline(cmd, pipeline);

    device->dispatch(cmd, gdata, 2, 1, 1);

    device->submit(gpu::QueueType::eCompute, { cmd });

    device->waitIdle();

    for (uint32_t i = 0; i < 128u; ++i) {
        assert(hdata[i] == 42.0);
    }

    device->free(gdata);

    device->freePipeline(pipeline);
    
    delete device;
}

