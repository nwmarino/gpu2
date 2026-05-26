//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "common.h"

#include "source/igpu.h"

#include "glfw/glfw3.h"

#define FRAMES_IN_FLIGHT 3

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Triangle", nullptr, nullptr);

    auto device_info = gpu::DeviceInfo {}
        .setWindow(window)
        .setWidth(800)
        .setHeight(600)
        .setFramesInFlight(FRAMES_IN_FLIGHT)
        .setEnableValidation(true);

    gpu::Device* device = gpu::Device::Create(device_info);

    std::string vertex_ir = readFile("C:/Users/nwmar/igpu/examples/shaders/triangle.vert.spv");
    std::string fragment_ir = readFile("C:/Users/nwmar/igpu/examples/shaders/triangle.frag.spv");

    std::vector<gpu::AttachmentInfo> atts = {
        gpu::AttachmentInfo {
            .format = gpu::Format::eRGBA8Unorm,
        },
    };

    auto raster_info = gpu::RasterInfo {}
        .setCull(gpu::Cull::eNone)
        .setFill(gpu::Fill::eFill)
        .setAttachments(atts);

    gpu::Pipeline pipeline = device->createGraphicsPipeline(
        vertex_ir, 
        fragment_ir, 
        raster_info);

    struct Vertex {
        float data[7];
    };

    float* vs_h = nullptr;
    gpu::ptr vs_g = device->malloc<float>(1024, gpu::MemoryType::eDefault, &vs_h);

    vs_h[0] = -0.5f;
    vs_h[1] =  0.5f;
    vs_h[2] =  0.5f;
    vs_h[3] =  0.5f;
    vs_h[4] =  0.0f;
    vs_h[5] = -0.5f;
    
    gpu::Viewport viewport = {};
    viewport.width = 800;
    viewport.height = 600;

    gpu::Rect2D scissor = {};
    scissor.width = 800;
    scissor.height = 600;

    gpu::Semaphore sema = device->createSemaphore(0);
    uint32_t next_frame = 1;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (next_frame > FRAMES_IN_FLIGHT)
            device->waitSemaphore(sema, next_frame - FRAMES_IN_FLIGHT);

        gpu::CommandList cmd = device->beginRecording(gpu::QueueType::eGraphics);

        gpu::TextureViewInfo view = {};
        view.format = gpu::Format::eRGBA8Unorm;
        view.base_layer = 0;
        view.layer_count = 1;
        view.base_mip = 0;
        view.mip_count = 1;

        gpu::RenderingInfo render_info = {};
        render_info.area = scissor;
        render_info.layer_count = 1;
        render_info.targets = {
            gpu::TargetInfo {}
                .setTexture(device->acquireSwapchainTexture())
                .setViewInfo(view)
                .setLoadOp(gpu::LoadOp::eClear)
                .setClearColor({ 0.f, 0.f, 1.f })
                .setStoreOp(gpu::StoreOp::eStore),
        };

        device->beginRendering(cmd, render_info);

        device->setPipeline(cmd, pipeline);
        device->setViewport(cmd, viewport);
        device->setScissor(cmd, scissor);
        device->setEnableDepthTest(cmd, false);
        device->setDepthBias(cmd, 0.0f, 0.0f, 0.0f);

        device->drawInstanced(cmd, vs_g, 0, 3, 1);

        device->endRendering(cmd);

        device->submitAndPresent(cmd, sema, next_frame++);
    }

    device->waitIdle();

    device->freeSemaphore(sema);

    device->freePipeline(pipeline);

    device->free(vs_g);
    
    delete device;

    glfwDestroyWindow(window);
    glfwTerminate();
}
