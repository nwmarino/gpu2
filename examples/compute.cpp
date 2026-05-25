//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "include/igpu.h"

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
    auto device_info = gpu::RenderingDeviceInfo {}
        .setEnableValidation(true);

    gpu::RenderingDevice* device = gpu::RenderingDevice::Create(device_info);

    std::string compute_ir = readFile("C:/Users/nwmar/igpu/examples/shaders/compute.comp.spv");

    gpu::Pipeline pipeline = device->createComputePipeline(compute_ir);

    gpu::CommandList cmd = device->beginRecording(gpu::QueueType::eCompute);

    device->setPipeline(cmd, pipeline);

    device->dispatch(cmd, 0, 1, 0, 0);

    device->submit(gpu::QueueType::eCompute, { cmd });

    device->waitIdle();

    device->freePipeline(pipeline);
    
    delete device;
}

