//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#ifndef IGPU_H_
#define IGPU_H_

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

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

enum class CompareOp : int32_t {
    eNever,
    eLess,
    eLessEqual,
    eGreater,
    eGreaterEqual,
    eEqual,
    eNotEqual,
    eAlways,
};

enum class BlendOp : int32_t {
    eAdd,
    eSubtract,
    eReverseSubtract,
    eMin,
    eMax,
};

enum class StencilOp : int32_t {
    eKeep,
    eZero,
    eReplace,
    eIncrementAndClamp,
    eDecrementAndClamp,
    eInvert,
    eIncrementAndWrap,
    eDecrementAndWrap,
};

enum class Stage : int32_t {
    eTransfer,
    eCompute,
    eRasterColorOut,
    eVertex,
    eFragment,
};

enum class Format : int32_t {
    eUndefined,
    eRGBA8Unorm,
    eD32Float,
};

enum class Cull : int32_t {
    eNone,
    eClockwise,
    eCounterClockwise,
    eAll,
};

enum class Fill : int32_t {
    eFill,
    eLine,
};

enum class DepthFlags : int32_t {
    eNone = 0,
    eRead = 1 << 1,
    eWrite = 1 << 2,
};

enum class TextureType : int32_t {
    e1D,
    e2D,
    e3D,
    eCube,
    e1DArray,
    e2DArray,
    eCubeArray,
};

enum class Topology : int32_t {
    eLineList,
    eLineStrip,
    eTriangleList,
    eTriangleStrip,
    eTriangleFan,
};

enum class Factor : int32_t {
    eZero,
    eOne,
    eSrcColor,
    eOneMinusSrcColor,
    eDstColor,
    eOneMinusDstColor,
    eSrcAlpha,
    eOneMinusSrcAlpha,
    eDstAlpha,
    eOneMinusDstAlpha,
    eConstantColor,
    eOneMinusConstantColor,
    eConstantAlpha,
    eOneMinusConstantAlpha,
    eSrcAlphaSaturate,
};

enum class Usage : int32_t {
    eUndefined,
    eTransferSrc,
    eTransferDst,
    eSampled,
    eStorage,
    eColorAttachment,
    eDepthStencilAttachment,
};

enum class Filter : int32_t {
    eNearest,
    eLinear,
};

enum class AddressMode : int32_t {
    eRepeat,
    eMirroredRepeat,
    eClampToEdge,
    eClampToBorder,
    eMirrorClampToEdge, 
};

enum class BorderColor : int32_t {
    eFloatTransparentBlack,
    eIntTransparentBlack,
    eFloatOpaqueBlack,
    eIntOpaqueBlack,
    eFloatOpaqueWhite,
    eIntOpaqueWhite,  
};

enum class QueueType : int32_t {
    eGraphics,
    eCompute,
    eTransfer,
};

struct Rect2D {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

struct Viewport {
    float x;
    float y;
    float width;
    float height;
    float min_depth;
    float max_depth;
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

struct TextureInfo {
    TextureType type = TextureType::e2D;
    Format format = Format::eUndefined;
    Usage usage = Usage::eUndefined;
    uint16_t layer_count = 1;
    uint8_t mip_count = 1;
    uint8_t sample_count = 1;
};

struct ViewInfo {
    Format format = Format::eUndefined;
    uint16_t base_layer = 0;
    uint16_t layer_count = 1;
    uint8_t base_mip = 0;
    uint8_t mip_count = 1;
};

enum class Sampler : int32_t {
    ePointClamp,
    ePointWrap,
    eLinearClamp,
    eLinearWrap,
    eAnisotropicClamp,
    eAnisotropicWrap,
};

struct SamplerInfo {
    Filter min = Filter::eLinear;
    Filter mag = Filter::eLinear;
    Filter mip = Filter::eLinear;
    AddressMode u = AddressMode::eClampToEdge;
    AddressMode v = AddressMode::eClampToEdge;
    AddressMode w = AddressMode::eClampToEdge;
    float mip_load_bias = 0.5f;
    float mip_lod = 0.0f;
    bool enable_anisotropy = false;
    float max_anisotropy = 0.0f;
    CompareOp compare = CompareOp::eAlways;
    float min_lod = 0.0f;
    float max_lod = 100.0f;
    BorderColor border;
};

struct RasterInfo {
    Topology topology = Topology::eTriangleList;
    Cull cull = Cull::eNone;
    Fill fill = Fill::eFill;
    bool alpha_to_coverage = false;
    bool support_dual_source_blending = false;
    uint8_t sample_count = 1;
    std::span<Format> targets = {};
    Format depth_format = Format::eUndefined;
    Format stencil_format = Format::eUndefined;
};

struct BlendStateInfo {
    BlendOp color_op = BlendOp::eAdd;
    Factor src_color_factor = Factor::eOne;
    Factor dst_color_factor = Factor::eZero;
    BlendOp alpha_op = BlendOp::eAdd;
    Factor src_alpha_factor = Factor::eOne;
    Factor dst_alpha_factor = Factor::eZero;
    uint8_t color_write_mask = 0xFF;
};

struct DepthStencilStateInfo {
    DepthFlags flags = DepthFlags::eNone;
    CompareOp test = CompareOp::eAlways;
    float depth_bias = 0.0f;
    float depth_bias_clamp = 0.0f;
    float depth_bias_slope_factor = 0.0f;
    uint8_t stencil_read_mask = 0xFF;
    uint8_t stencil_write_mask = 0xFF;
    StencilOp stencil_front;
    StencilOp stencil_back;
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

struct TextureSizeAlign {
    uint32_t size;
    uint32_t align;
};

struct TextureDescriptor {
    uint64_t data[4];
};

class CommandBuffer {
public:
    void copy(void* gdest, void* gsrc);
    void copyToTexture(void* gdest, void* gsrc, Texture texture);
    void copyFromTexture(void* gdest, void* gsrc, Texture texture);

    void signalAfter(Stage stage, void* gptr, uint64_t value);
    void waitBefore(Stage stage, void* gptr, uint64_t value, CompareOp op);

    void setPipeline(Pipeline pipeline);

    void setViewport(Viewport viewport);
    void setScissor(Rect2D rect);

    void setBlendState(BlendState state);
    void setDepthStencilState(DepthStencilState state);

    void setActiveTextureHeapPtr(void* gptr);

    void beginRendering(const RenderingInfo& info);

    void endRendering();

    void dispatch(void* data, uint32_t x, uint32_t y, uint32_t z);

    void barrier(Stage before, Stage after);
};

using GpuAddr = uint64_t;

struct RenderingDeviceInfo {
    void* window = nullptr;
    Format surface = Format::eRGBA8Unorm;
    bool validation = false;
};

class RenderingDevice {
    struct RenderingDevice_T* m_impl = nullptr;

    explicit RenderingDevice(RenderingDevice_T* impl) : m_impl(impl) {}

public:
    static RenderingDevice* Create(const RenderingDeviceInfo& info);

    ~RenderingDevice();

    RenderingDevice(const RenderingDevice&) = delete;
    void operator=(const RenderingDevice&) = delete;

    RenderingDevice(RenderingDevice&&) noexcept = delete;
    void operator=(RenderingDevice&&) noexcept = delete;

    GpuAddr malloc(uint64_t size, MemoryType type = MemoryType::eDefault);
    void free(GpuAddr addr);
    
    void* deviceToHostAddress(GpuAddr addr);
    GpuAddr hostToDeviceAddress(void* ptr);

    Texture createTexture(const TextureInfo& info, void* gptr);    
    void freeTexture(Texture texture);
    TextureSizeAlign textureSizeAlign(const TextureInfo& info);
    TextureDescriptor getTextureViewDescriptor(Texture texture, const ViewInfo& info);
    TextureDescriptor getRWTextureViewDescriptor(Texture texture, const ViewInfo& info);

    Queue createQueue(QueueType type);
    CommandBuffer beginRecording(Queue queue);
    void submit(Queue queue, std::span<CommandBuffer> buffers);

    Semaphore createSemaphore();
    void freeSemaphore(Semaphore sema);
    void waitSemaphore(Semaphore sema, uint64_t value);

    Pipeline createComputePipeline(std::string_view compute);
    Pipeline createGraphicsPipeline(std::string_view vertex, 
                                    std::string_view fragment, 
                                    const RasterInfo& info);
    Pipeline createMeshletPipeline(std::string_view meshlet, 
                                   std::string_view fragment, 
                                   const RasterInfo& info);
    void freePipeline(Pipeline pipeline);

    BlendState createBlendState(const BlendStateInfo& info);
    void freeBlendState(BlendState state);

    DepthStencilState createDepthStencilState(const DepthStencilStateInfo& info);
    void freeDepthStencilState(DepthStencilState state);

    Texture getSwapchainTexture(Semaphore acquire);
    void present(Queue queue, Semaphore wait);
    void resizeSwapchain(uint32_t width, uint32_t height);
};

} // namespace gpu

#endif // IGPU_H_
