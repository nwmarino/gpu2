//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#ifndef GPU2_H_
#define GPU2_H_

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace gpu {

using HandleType = uintptr_t;

#define GPU_DEFINE_HANDLE(T) \
    using T = HandleType;

#define GPU_DEFINE_ENUM_FLAGS(T) \
    inline bool operator&(T x, T y) { return bool(uint32_t(x) & uint32_t(y)); } \
    inline T operator|(T x, T y) { return T(uint32_t(x) | uint32_t(y)); } \
    inline T operator~(T x) { return T(~uint32_t(x)); }

#define VERTEX_STAGE 0
#define FRAGMENT_STAGE 1
#define COMPUTE_STAGE 2

using ptr = uintptr_t;

using Descriptor = uint32_t;

static constexpr uintptr_t null = 0;

enum class Backend {
    Vulkan,
    /* D3D12, */
};

enum class QueueType : uint32_t {
    Graphics = 0,
    Compute = 1,
    Transfer = 2,
};

enum class MemoryType {
    Upload,
    Device,
    Readback,
};

enum class PresentMode {
    Immediate,
    Fifo,
    FifoRelaxed,
    Mailbox,
};

enum class Topology {
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan,
};

enum class CullMode {
    None,
    Front,
    Back,
    Both,
};

enum class FillMode {
    Solid,
    Wireframe,
};

enum class FrontFace {
    Clockwise,
    CounterClockwise,
};

enum class Format {
    Undefined,
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    R16G16_SNORM,
    R32_FLOAT,
    R32G32_FLOAT,
    R32G32B32A32_FLOAT,
    R8_UNORM,
    R16_UINT,
    R32_UINT,
    D32_FLOAT,
    D24_UNORM_S8_UINT,
};

enum class LoadOp {
    Unknown,
    Load,
    Clear,
};

enum class StoreOp {
    Unknown,
    Store,
};

enum class CompareOp {
    Never,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,
    Always,
};

enum class BlendOp {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum class StencilOp {
    Keep,
    Zero,
    Replace,
    IncrementAndClamp,
    DecrementAndClamp,
    Invert,
    IncrementAndWrap,
    DecrementAndWrap,
};

enum class Factor {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate,
};

enum class Filter {
    Nearest,
    Linear,
};

enum class AddressMode {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge, 
};

enum class BorderColor {
    FloatTransparentBlack,
    IntTransparentBlack,
    FloatOpaqueBlack,
    IntOpaqueBlack,
    FloatOpaqueWhite,
    IntOpaqueWhite,  
};

enum class TextureType {
    D1,
    D2,
    D3,
};

enum class PipelineType {
    Graphics,
    Compute,
};

enum class TextureLayout {
    Undefined,
    General,
    ShaderRead,
    ColorTarget,
    DepthStencilTarget,
    TransferSrc,
    TransferDst,
    Present,
};

enum class StageFlags : uint32_t {
    None           = 0,
    Transfer       = 1 << 0,
    RasterColorOut = 1 << 1,
    Vertex         = 1 << 2,
    Fragment       = 1 << 3,
    Compute        = 1 << 4,
};

enum class ShaderStageFlags : uint32_t {
    None     = 0,
    Vertex   = 1 << VERTEX_STAGE,
    Fragment = 1 << FRAGMENT_STAGE,
    Compute  = 1 << COMPUTE_STAGE,
    All      = Vertex | Fragment | Compute,
};

enum class UsageFlags : uint32_t {
    None                   = 0,
    TransferSrc            = 1 << 0,
    TransferDst            = 1 << 1,
    Sampled                = 1 << 2,
    Storage                = 1 << 3,
    ColorAttachment        = 1 << 4,
    DepthStencilAttachment = 1 << 5,
};

GPU_DEFINE_ENUM_FLAGS(StageFlags)
GPU_DEFINE_ENUM_FLAGS(ShaderStageFlags)
GPU_DEFINE_ENUM_FLAGS(UsageFlags)

struct Color {
    float r;
    float g;
    float b;
    float a;
};

using ClearColor = Color;

struct ClearDepth {
    float depth = 1.0f;
    uint8_t stencil = 0;
};

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float min_depth = 0.0f;
    float max_depth = 1.0f;
};

struct Scissor {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct Offset3D {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
};

struct Dimension3D {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;
};

struct SwapchainInfo {
    void* window = nullptr;
    Format format = Format::R8G8B8A8_UNORM;
    PresentMode present = PresentMode::Immediate;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t frames_in_flight = 1;
};

struct TextureInfo {
    TextureType type = TextureType::D2;
    Format format = Format::Undefined;
    UsageFlags usage = UsageFlags::None;
    Dimension3D area = {};
    uint8_t mip_count = 1;
    uint8_t sample_count = 1;
};

struct TextureViewInfo {
    Format format = Format::Undefined;
    uint8_t base_mip = 0;
    uint8_t mip_count = 1;
    uint8_t base_layer = 0;
    uint8_t layer_count = 1;
};

struct SamplerInfo {
    Filter min = Filter::Linear;
    Filter mag = Filter::Linear;
    Filter mip = Filter::Linear;
    AddressMode u = AddressMode::Repeat;
    AddressMode v = AddressMode::Repeat;
    AddressMode w = AddressMode::Repeat;
    float min_lod = 0.0f;
    float max_lod = 1000.0f;
    bool enable_anisotropy = false;
    float max_anisotropy = 1.0f;
    bool enable_compare = false;
    CompareOp compare_op = CompareOp::Always;
};

struct ShaderInfo {
    std::string_view bytecode;
    std::string_view entry = "main";
    ShaderStageFlags stage = ShaderStageFlags::None;
};

/// The result of a device memory allocation.
struct AllocResult {
    ptr device = null;
    void* host = nullptr;

    /// Returns true if this allocation is valid.
    bool valid() const {
        return device != null;
    }
};

GPU_DEFINE_HANDLE(Texture)
GPU_DEFINE_HANDLE(Sampler)
GPU_DEFINE_HANDLE(Shader)
GPU_DEFINE_HANDLE(Swapchain)
GPU_DEFINE_HANDLE(CommandList)
GPU_DEFINE_HANDLE(Pipeline)
GPU_DEFINE_HANDLE(Semaphore)

struct CopyRegion {
    Offset3D offset = {};
    Dimension3D area = {};
    uint8_t mip_level = 0;
    uint8_t base_layer = 0;
    uint8_t layer_count = 1;
};

struct AttachmentInfo {
    Format format = Format::Undefined;
    bool blend_enable = false;
    BlendOp color_op = BlendOp::Add;
    Factor src_color_factor = Factor::One;
    Factor dst_color_factor = Factor::Zero;
    BlendOp alpha_op = BlendOp::Add;
    Factor src_alpha_factor = Factor::One;
    Factor dst_alpha_factor = Factor::Zero;
    uint32_t color_write_mask = 0xFF;
};

struct RasterInfo {
    Topology topology = Topology::TriangleList;
    FillMode fill = FillMode::Solid;
    CullMode cull = CullMode::None;
    FrontFace face = FrontFace::CounterClockwise;
    uint8_t sample_count = 1;
    std::vector<AttachmentInfo> attachments = {};
    Format depth = Format::Undefined;
    CompareOp depth_compare = CompareOp::LessEqual;
    bool depth_test = false;
    bool depth_write = false;
    bool depth_bias_enable = false;
    float depth_bias_clamp = 0.0f;
    float depth_bias_slope = 0.0f;
    float depth_bias_constant = 0.0f;
};

struct TargetInfo {
    Texture texture = null;
    TextureViewInfo view = {};
    LoadOp load = LoadOp::Unknown;
    StoreOp store = StoreOp::Unknown;

    union {
        ClearColor clear_color;
        ClearDepth clear_depth;
    };
};

struct RenderInfo {
    Scissor area = {};
    std::vector<TargetInfo> targets = {};
    std::optional<TargetInfo> depth = {};
};

struct Capabilities {

};

struct DeviceInfo {
    Backend backend = {};
    bool validation = false;
};

class Device {
protected:
    const DeviceInfo info = {};

    explicit Device(const DeviceInfo& info) : info(info) {}

public:
    /// Returns the information used to create this device.
    const DeviceInfo& getInfo() const { return info; }

    virtual void destroy() = 0;

    virtual Capabilities getCapabilities() = 0;

    virtual void waitIdle() = 0;
    virtual void waitIdle(QueueType) = 0;

    virtual AllocResult malloc(uint32_t size, MemoryType type) = 0;
    virtual void free(ptr) = 0;
    virtual void* deviceToHostPointer(ptr) = 0;

    virtual Texture createTexture(const TextureInfo&) = 0;
    virtual void freeTexture(Texture) = 0;

    virtual void copyToTexture(void* src, Texture dst, const CopyRegion& region) = 0;
    virtual void copyFromTexture(Texture src, void* dst, const CopyRegion& region) = 0;

    virtual Sampler createSampler(const SamplerInfo&) = 0;
    virtual void freeSampler(Sampler) = 0;

    virtual Shader createShader(const ShaderInfo&) = 0;
    virtual void freeShader(Shader) = 0;

    virtual Pipeline createGraphicsPipeline(Shader vertex, Shader fragment, const RasterInfo& info) = 0;
    virtual Pipeline createComputePipeline(Shader compute) = 0;
    virtual void freePipeline(Pipeline) = 0;

    virtual Semaphore createSemaphore(uint64_t value) = 0;
    virtual void freeSemaphore(Semaphore) = 0;
    virtual void waitSemaphore(Semaphore, uint64_t value) = 0;

    virtual Swapchain createSwapchain(const SwapchainInfo&) = 0;
    virtual void freeSwapchain(Swapchain) = 0;
    virtual void resizeSwapchain(Swapchain, uint32_t width, uint32_t height) = 0;
    virtual Texture acquireSwapchainTexture(Swapchain) = 0;
    virtual void present(Swapchain, CommandList, Semaphore, uint64_t value) = 0;

    virtual Descriptor getSamplerDescriptor(Sampler) = 0;
    virtual Descriptor getTextureDescriptor(Texture, TextureViewInfo) = 0;
    virtual Descriptor getRWTextureDescriptor(Texture, TextureViewInfo) = 0;
    virtual void releaseSamplerDescriptor(Descriptor) = 0;
    virtual void releaseTextureDescriptor(Descriptor) = 0;
    virtual void releaseRWTextureDescriptor(Descriptor) = 0;

    virtual CommandList beginRecording(QueueType) = 0;
    virtual void submit(
        QueueType, 
        CommandList, 
        Semaphore signal = null, 
        Semaphore wait = null) = 0;

    virtual void copy(CommandList, ptr src, ptr dst, uint64_t size) = 0;

    virtual void barrier(CommandList, StageFlags before, StageFlags after) = 0;
    virtual void barrier(CommandList, Texture, TextureLayout before, TextureLayout after) = 0;

    virtual void beginRendering(CommandList, const RenderInfo&) = 0;
    virtual void endRendering(CommandList) = 0;

    virtual void setPipeline(CommandList, Pipeline) = 0;
    virtual void setViewport(CommandList, Viewport) = 0;
    virtual void setScissor(CommandList, Scissor) = 0;

    virtual void drawInstanced(
        CommandList, 
        ptr vertex, 
        ptr fragment, 
        uint32_t vertices, 
        uint32_t instances) = 0;
    virtual void drawIndexedInstanced(
        CommandList,
        ptr vertex, 
        ptr fragment, 
        ptr index,
        uint32_t indices,
        uint32_t offset,
        uint32_t instances) = 0;
    virtual void dispatch(
        CommandList, 
        ptr compute, 
        uint32_t x, 
        uint32_t y, 
        uint32_t z) = 0;
};

Device* createDevice(const DeviceInfo&);

} // namespace gpu

#endif // GPU2_H_
