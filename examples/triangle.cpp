//
//  triangle.cpp
//
//  Copyright (c) 2026 Nick Marino.
//  All rights reserved.
//

#include "examples/common.h"
#include "source/gpu2.h"

#include "glfw/glfw3.h"

static constexpr uint32_t FramesInFlight = 3;

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Triangle", nullptr, nullptr);

    gpu::DeviceInfo device_info =  {};
    device_info.backend = gpu::Backend::Vulkan;
    device_info.validation = true;

    gpu::Device* device = gpu::createDevice(device_info);

    gpu::SwapchainInfo swapchain_info = {};
    swapchain_info.window = window;
    swapchain_info.width = 800;
    swapchain_info.height = 600;
    swapchain_info.frames_in_flight = FramesInFlight;

    gpu::Swapchain swapchain = device->createSwapchain(swapchain_info);

    gpu::Shader vertex = device->createShader(gpu::ShaderInfo {
        .bytecode = readFile("C:/Users/nwmar/igpu/examples/shaders/triangle.vert.spv"),
        .entry = "main",
        .stage = gpu::ShaderStageFlags::Vertex,
    });

    gpu::Shader fragment = device->createShader(gpu::ShaderInfo {
        .bytecode = readFile("C:/Users/nwmar/igpu/examples/shaders/triangle.frag.spv"),
        .entry = "main",
        .stage = gpu::ShaderStageFlags::Fragment,
    });

    gpu::RasterInfo raster_info = {};
    raster_info.cull = gpu::CullMode::None;
    raster_info.fill = gpu::FillMode::Solid;
    raster_info.attachments = {
        gpu::AttachmentInfo {
            .format = gpu::Format::R8G8B8A8_UNORM,
        },
    };

    gpu::Pipeline pipeline = device->createGraphicsPipeline(
        vertex, 
        fragment, 
        raster_info);

    gpu::Alloc<float> vertices = device->malloc<float>(6);
    vertices.host[0] = -0.5f;
    vertices.host[1] =  0.5f;
    vertices.host[2] =  0.5f;
    vertices.host[3] =  0.5f;
    vertices.host[4] =  0.0f;
    vertices.host[5] = -0.5f;

    gpu::Viewport viewport = {};
    viewport.width = 800;
    viewport.height = 600;

    gpu::Scissor scissor = {};
    scissor.width = 800;
    scissor.height = 600;

    gpu::Semaphore sema = device->createSemaphore(0);
    uint32_t next_frame = 1;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (next_frame > FramesInFlight)
            device->waitSemaphore(sema, next_frame - FramesInFlight);

        gpu::CommandList cmd = device->beginRecording(gpu::QueueType::Graphics);

        gpu::Texture texture = device->acquireSwapchainTexture(swapchain, cmd);

        gpu::TextureViewInfo view = {};
        view.format = gpu::Format::R8G8B8A8_UNORM;
        view.base_layer = 0;
        view.layer_count = 1;
        view.base_mip = 0;
        view.mip_count = 1;

        gpu::RenderInfo render_info = {};
        render_info.area = scissor;
        render_info.targets = {
            gpu::TargetInfo {
                .texture = texture,
                .view = view,
                .load = gpu::LoadOp::Clear,
                .store = gpu::StoreOp::Store,
                .clear_color = { 0.f, 0.f, 1.f },
            },
        };

        device->beginRendering(cmd, render_info);

        device->setPipeline(cmd, pipeline);
        device->setViewport(cmd, viewport);
        device->setScissor(cmd, scissor);

        device->drawInstanced(cmd, vertices.device, 0, 3, 1);

        device->endRendering(cmd);

        device->present(swapchain, cmd, sema, next_frame++);
    }

    device->waitIdle();

    device->freeSemaphore(sema);

    device->freePipeline(pipeline);
    device->freeShader(vertex);
    device->freeShader(fragment);

    device->free(vertices.device);
    device->freeSwapchain(swapchain);
    
    device->destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
}
