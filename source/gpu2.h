//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#ifndef GPU2_H_
#define GPU2_H_

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace gpu {

using HandleType = uintptr_t;

#define GPU_DEFINE_HANDLE(T) \
    using T = HandleType;

#define GPU_DEFINE_ENUM_FLAGS(T) \
    inline T operator&(T x, T y) { return T(uint32_t(x) & uint32_t(y)); } \
    inline T operator|(T x, T y) { return T(uint32_t(x) | uint32_t(y)); } \
    inline T operator~(T x) { return T(~uint32_t(x)); }

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
    Default,
    Device,
    Readback,
};

enum class IndexType {
    U8,
    U16,
    U32,
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
};

enum class FrontFace {
    Clockwise,
    CounterClockwise,
};

enum class FillMode {
    Solid,
    Wireframe,
};

enum class Format {
    Undefined,
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    R32_FLOAT,
    R32G32_FLOAT,
    R32G32B32A32_FLOAT,
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

enum class TextureState {
    Undefined,
    General,
    ShaderRead,
    ShaderReadWrite,
    DepthRead,
    DepthWrite,
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
    ComputeRead    = 1 << 4,
    ComputeWrite   = 1 << 5,
};

enum class ShaderStageFlags : uint32_t {
    None     = 0,
    Vertex   = 1 << 0,
    Fragment = 1 << 1,
    Compute  = 1 << 2,
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
    float x = 0;
    float y = 0;
    float width;
    float height;
    float min_depth = 0.0f;
    float max_depth = 1.0f;
};

struct Scissor {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width;
    uint32_t height;
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
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t buffers = 3;
    Format format = Format::R8G8B8A8_UNORM;
    PresentMode present = PresentMode::Immediate;
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
    std::span<const uint32_t> bytecode;
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
GPU_DEFINE_HANDLE(Fence)

struct CopyRegion {
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
    bool depth_test = false;
    bool depth_write = false;
    CompareOp depth_compare = CompareOp::LessEqual;
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

struct DeviceInfo {
    Backend backend = {};
    bool validation = false;
};

class Device {
protected:
    const DeviceInfo m_info = {};

    explicit Device(const DeviceInfo& info) : m_info(info) {}

public:
    /// Returns the information used to create this device.
    const DeviceInfo& info() const { return m_info; }

    virtual void destroy();

    virtual void waitIdle();
    virtual void waitIdle(QueueType);

    virtual AllocResult malloc(uint64_t size, MemoryType type);
    virtual AllocResult malloc(uint64_t size, uint64_t align, MemoryType type);
    virtual void free(ptr);
    virtual void* deviceToHostPointer(ptr);

    virtual Texture createTexture(const TextureInfo&);
    virtual void freeTexture(Texture);

    virtual Sampler createSampler(const SamplerInfo&);
    virtual void freeSampler(Sampler);

    virtual Shader createShader(const ShaderInfo&);
    virtual void freeShader(Shader);

    virtual Pipeline createGraphicsPipeline(Shader vertex, Shader fragment, const RasterInfo& info);
    virtual Pipeline createComputePipeline(Shader compute);
    void freePipeline(Pipeline);

    virtual Swapchain createSwapchain(const SwapchainInfo&);
    virtual void freeSwapchain(Swapchain);
    virtual void resizeSwapchain(Swapchain, uint64_t width, uint64_t height);
    virtual Texture acquireTexture(Swapchain, Fence = null);
    virtual void present(Swapchain);

    virtual Semaphore createSemaphore();
    virtual void freeSemaphore(Semaphore);

    virtual Fence createFence();
    virtual void freeFence(Fence);
    virtual void waitFence(Fence);
    virtual void resetFence(Fence);

    virtual Descriptor getTextureDescriptor(Texture, TextureViewInfo);
    virtual Descriptor getRWTextureDescriptor(Texture, TextureViewInfo);
    virtual Descriptor getSamplerDescriptor(Sampler);
    virtual void releaseTextureDescriptor(Descriptor);
    virtual void releaseRWTextureDescriptor(Descriptor);
    virtual void releaseSamplerDescriptor(Descriptor);

    virtual CommandList beginRecording(QueueType);
    virtual void submit(QueueType, 
                        CommandList, 
                        Semaphore signal = null, 
                        Semaphore wait = null);

    virtual void copy(CommandList, ptr src, ptr dst, uint64_t size);
    virtual void copyToTexture(CommandList, void* src, Texture dst, const CopyRegion& region);
    virtual void copyFromTexture(CommandList, Texture src, void* dst, const CopyRegion& region);
    virtual void clearTextureColor(CommandList, Texture, ClearColor);
    virtual void clearTextureDepth(CommandList, Texture, ClearDepth);

    virtual void barrier(CommandList, StageFlags before, StageFlags after);
    virtual void barrier(CommandList, Texture texture, TextureState prev, TextureState next);

    virtual void beginRendering(CommandList, const RenderInfo&);
    virtual void endRendering(CommandList);

    virtual void setPipeline(CommandList, Pipeline);
    virtual void setViewport(CommandList, Viewport);
    virtual void setScissor(CommandList, Scissor);

    virtual void drawInstanced(
        CommandList, 
        ptr vertex, 
        ptr fragment, 
        uint32_t vertices, 
        uint32_t instances);
    virtual void drawIndexedInstances(
        CommandList,
        ptr vertex, 
        ptr fragment, 
        ptr index,
        IndexType type,
        uint32_t indices, 
        uint32_t instances);
    virtual void dispatch(
        CommandList, 
        ptr compute, 
        uint32_t x, 
        uint32_t y, 
        uint32_t z);
};

Device* createDevice(const DeviceInfo&);

} // namespace gpu

#endif // GPU2_H_
