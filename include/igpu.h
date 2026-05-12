//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#ifndef IGPU_H_
#define IGPU_H_

#include <cstdint>
#include <optional>
#include <span>

namespace gpu {

struct RenderingDevice_T;
struct CommandBuffer_T;

struct Texture_T;
struct BlendState_T;
struct DepthStencilState_T;
struct Pipeline_T;
struct Queue_T;
struct Semaphore_T;

using Texture = Texture_T*;
using BlendState = BlendState_T*;
using DepthStencilState = DepthStencilState_T*;
using Pipeline = Pipeline_T*;
using Queue = Queue_T*;
using Semaphore = Semaphore_T*;

enum class MemoryType : int32_t {
    /// Memory supporting both device and host usage.
    eDefault,
    /// Device-only memory.
    eDevice,
    /// Coherent memory meant for host readbacks.
    eReadback,
};

enum class LoadOp : int32_t {
    eUnknown,
    eClear,
    eLoad,
};

enum class StoreOp : int32_t {
    eUnknown,
    eStore,
};

enum class Stage : int32 {
    eTransfer,
    eCompute,
    eRasterColorOut,
    eVertex,
    eFragment,
};

struct Rect2D {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

struct Color {
    float r;
    float g;
    float b;
    float a;
};

using ClearColorValue = Color;

struct ClearDepthStencilValue {
    float depth;
    uint32_t stencil;
};

struct AttachmentInfo {
    LoadOp load;
    StoreOp store;

    union {
        ClearColorValue clear_color;
        ClearDepthStencilValue clear_depth;
    };
};

struct RenderingInfo {
    Rect2D area;
    uint16_t layer_count = 1;
    std::span<AttachmentInfo> color_atts = {};
    std::optional<AttachmentInfo> depth_att = {};
    std::optional<AttachmentInfo> stencil_att = {};
};

class CommandBuffer {
public:
    void setPipeline(Pipeline pipeline);

    void setActiveTextureHeapPtr(void* gptr);

    void beginRendering(const RenderingInfo& info);

    void endRendering();

    void dispatch(void* data, uint32_t x, uint32_t y, uint32_t z);

    void barrier(Stage before, Stage after);
};

class RenderingDevice {
public:
    void* malloc(uint32_t bytes, MemoryType memory = MemoryType::eDefault);
    void* malloc(uint32_t bytes, uint32_t align, MemoryType memory = MemoryType::eDefault);
    void  free(void* gptr);
    void* hostToDevicePointer(void* ptr);

    Texture createTexture();

    Queue createQueue();
    void  freeQueue(Queue queue);

    Semaphore createSemaphore();
    void      freeSemaphore(Semaphore sema);

    void      waitSemaphore(Semaphore sema, uint64_t value);

};

} // namespace gpu

#endif // IGPU_H_
