//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#ifndef IGPU_H_
#define IGPU_H_

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace gpu {

using ptr = uint64_t;

using CommandList = uint64_t;
using Pipeline = uint64_t;
using Sampler = uint64_t;
using Semaphore = uint64_t;
using Texture = uint64_t;
using TextureView = uint64_t;

struct RenderingDevice_T;

static constexpr uint64_t InvalidHandle = ~0ull;

/// The different types of host/device memory that can be allocated.
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

enum class Shader : uint32_t {
    eVertex = 0,
    eFragment = 1,
    eCompute = 2,
    eCount,
};

enum class PipelineType : int32_t {
    eGraphics,
    eCompute,
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

enum class QueueType : uint32_t {
    eGraphics = 0,
    eCompute = 1,
    eTransfer = 2,
    eCount,
};

struct Rect2D final {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

struct Viewport final {
    float x;
    float y;
    float width;
    float height;
    float min_depth;
    float max_depth;
};

struct Color final {
    float r;
    float g;
    float b;
    float a;
};

using ClearColorValue = Color;

using ClearDepthValue = float;

using ClearStencilValue = uint32_t;

struct TimelinePair final {
    Semaphore sema = 0;
    uint64_t value = 0;
};

struct TextureInfo final {
    TextureType type = TextureType::e2D;
    Format format = Format::eUndefined;
    Usage usage;
    uint8_t layer_count = 1;
    uint8_t mip_count = 1;
    uint8_t sample_count = 1;
    std::array<uint32_t, 3> dimensions;
};

struct TextureRegion final {
    uint8_t mip = 0;
    uint8_t base_layer = 0;
    uint8_t layer_count = 1;
    std::array<int32_t, 3> offset = {};
    std::array<uint32_t, 3> extent = {};
};

struct TextureViewInfo final {
    Format format = Format::eUndefined;
    uint8_t base_layer = 0;
    uint8_t layer_count = 1;
    uint8_t base_mip = 0;
    uint8_t mip_count = 1;
};

struct TextureDescriptor final {
    uint64_t data[8];
};

struct SamplerDescriptor final {
    uint64_t data[4];
};

struct SamplerInfo final {
    Filter min = Filter::eLinear;
    Filter mag = Filter::eLinear;
    Filter mip = Filter::eLinear;
    AddressMode u = AddressMode::eClampToEdge;
    AddressMode v = AddressMode::eClampToEdge;
    AddressMode w = AddressMode::eClampToEdge;
    float mip_lod_bias = 0.5f;
    bool enable_anisotropy = false;
    float max_anisotropy = 0.0f;
    CompareOp compare = CompareOp::eAlways;
    float min_lod = 0.0f;
    float max_lod = 100.0f;
    BorderColor border;
};

struct AttachmentInfo final {
    Format format = Format::eUndefined;
    BlendOp color_op = BlendOp::eAdd;
    Factor src_color_factor = Factor::eOne;
    Factor dst_color_factor = Factor::eZero;
    BlendOp alpha_op = BlendOp::eAdd;
    Factor src_alpha_factor = Factor::eOne;
    Factor dst_alpha_factor = Factor::eZero;
    uint32_t color_write_mask = 0xFF;
};

struct RasterInfo final {
    Topology topology = Topology::eTriangleList;
    Cull cull = Cull::eNone;
    Fill fill = Fill::eFill;
    bool alpha_to_coverage = false;
    uint8_t sample_count = 1;
    std::span<AttachmentInfo> atts = {};
    Format depth = Format::eUndefined;
    Format stencil = Format::eUndefined;

    constexpr RasterInfo& setTopology(Topology value) {
        topology = value;
        return *this;
    }

    constexpr RasterInfo& setCull(Cull value) {
        cull = value;
        return *this;
    }

    constexpr RasterInfo& setFill(Fill value) {
        fill = value;
        return *this;
    }

    constexpr RasterInfo& setEnableAlphaToCoverage(bool value) {
        alpha_to_coverage = value;
        return *this;
    }

    constexpr RasterInfo& setSampleCount(uint8_t value) {
        sample_count = value;
        return *this;
    }

    constexpr RasterInfo& setAttachments(const std::span<AttachmentInfo>& value) {
        atts = value;
        return *this;
    }

    constexpr RasterInfo& setDepth(Format value) {
        depth = value;
        return *this;
    }

    constexpr RasterInfo& setStencil(Format value) {
        stencil = value;
        return *this;
    }
};

struct TargetInfo final {
    Texture texture;
    TextureViewInfo view;
    LoadOp load;
    StoreOp store;

    union {
        ClearColorValue clear_color;
        ClearDepthValue clear_depth;
        ClearStencilValue clear_stencil;
    };
};

struct RenderingInfo final {
    Rect2D area = {};
    uint8_t layer_count = 1;
    std::span<TargetInfo> targets = {};
    std::optional<TargetInfo> depth = std::nullopt;
    std::optional<TargetInfo> stencil = std::nullopt;
};

enum class Present : int32_t {
    eImmediate,
    eFifo,
    eFifoRelaxed,
    eMailbox,
};

struct RenderingDeviceInfo final {
    void* window = nullptr;
    uint32_t width;
    uint32_t height;
    Format format = Format::eRGBA8Unorm;
    Present present = Present::eImmediate;
    uint32_t frames_in_flight = 1;
    bool validation = false;

    constexpr RenderingDeviceInfo& setWindow(void* value) {
        window = value;
        return *this;
    }

    constexpr RenderingDeviceInfo& setWidth(uint32_t value) {
        width = value;
        return *this;
    }

    constexpr RenderingDeviceInfo& setHeight(uint32_t value) {
        height = value;
        return *this;
    }

    constexpr RenderingDeviceInfo& setFormat(Format value) {
        format = value;
        return *this;
    }

    constexpr RenderingDeviceInfo& setPresent(Present value) {
        present = value;
        return *this;
    }

    constexpr RenderingDeviceInfo& setFramesInFlight(uint32_t value) {
        frames_in_flight = value;
        return *this;
    }

    constexpr RenderingDeviceInfo& setEnableValidation(bool value) {
        validation = value;
        return *this;
    }
};

class RenderingDevice final {
    RenderingDeviceInfo m_info;

    RenderingDevice_T* m_impl = nullptr;

    explicit RenderingDevice(const RenderingDeviceInfo& info,
                             RenderingDevice_T* impl) 
        : m_info(info)
        , m_impl(impl) {}

public:
    [[nodiscard]]
    static RenderingDevice* Create(const RenderingDeviceInfo& info);

    ~RenderingDevice();

    RenderingDevice(const RenderingDevice&) = delete;
    void operator=(const RenderingDevice&) = delete;

    RenderingDevice(RenderingDevice&&) noexcept = delete;
    void operator=(RenderingDevice&&) noexcept = delete;

    /// Returns the creation info used for this rendering device.
    const RenderingDeviceInfo& info() const { return m_info; }

    void waitIdle();

    ptr malloc(uint64_t size, MemoryType type = MemoryType::eDefault);
    void free(ptr p);

    void* deviceToHostAddress(ptr p);
    ptr hostToDeviceAddress(void* p);

    Texture acquireSwapchainTexture();
    void present();
    void resizeSwapchain(uint32_t width, uint32_t height);

    Texture createTexture(const TextureInfo& info);
    void freeTexture(Texture texture);

    TextureDescriptor getTextureDescriptor(
        Texture texture, 
        const TextureViewInfo& info);
    TextureDescriptor getRWTextureDescriptor(
        Texture texture, 
        const TextureViewInfo& info);
    SamplerDescriptor getSamplerDescriptor(
        const SamplerInfo& info);

    CommandList beginRecording(QueueType queue);
    void submit(QueueType queue, 
                const std::vector<CommandList>& lists,
                const std::vector<TimelinePair>& signals = {}, 
                const std::vector<TimelinePair>& wait = {});
    void submitAndPresent(CommandList cmd, Semaphore signal, uint64_t value);

    Semaphore createSemaphore(uint64_t value);
    void freeSemaphore(Semaphore sema);
    void waitSemaphore(Semaphore sema, uint64_t value);

    Pipeline createComputePipeline(
        std::string_view compute);
    Pipeline createGraphicsPipeline(
        std::string_view vertex, 
        std::string_view fragment, 
        const RasterInfo& info);

    void freePipeline(Pipeline pipeline);

    void copy(CommandList cmd, ptr src, ptr dst, uint32_t size);
    void copyToTexture(void* src, Texture dst, const TextureRegion& region);
    void copyFromTexture(Texture src, void* dst, const TextureRegion& region);

    void barrier(CommandList cmd, Stage before, Stage after);

    void beginRendering(CommandList cmd, const RenderingInfo& info);
    void endRendering(CommandList cmd);
    
    void setPipeline(CommandList cmd, Pipeline pipeline);
    void setViewport(CommandList cmd, Viewport viewport);
    void setScissor(CommandList cmd, Rect2D rect);
    void setDepthBias(CommandList cmd, float clamp, float slope, float constant);
    void setDepthCompareOp(CommandList cmd, CompareOp op);
    void setEnableDepthTest(CommandList cmd, bool value);
    void setEnableDepthWrite(CommandList cmd, bool value);

    void drawInstanced(
        CommandList cmd,
        ptr vertex, 
        ptr fragment, 
        uint32_t vertices, 
        uint32_t instances);
    void drawIndexedInstanced(
        CommandList cmd,
        ptr vertex,
        ptr fragment,
        ptr index,
        uint32_t indices,
        uint32_t instances);
    void dispatch(
        CommandList cmd, 
        ptr data, 
        uint32_t x, 
        uint32_t y, 
        uint32_t z);
};

} // namespace gpu

#endif // IGPU_H_
