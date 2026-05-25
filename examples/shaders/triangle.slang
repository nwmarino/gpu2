//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

struct VertexData<T> {
    [[vk::offset(0)]]
    Ptr<T, Access.Immutable> data;

    [ForceInline]
    Ptr<T, Access.Immutable> get() {
        return data;
    }
};

struct FragmentData<T> {
    [[vk::offset(8)]]
    Ptr<T, Access.Immutable> data;

    [ForceInline]
    Ptr<T, Access.Immutable> get() {
        return data;
    }
};

struct VertexOutput {
    float4 position : SV_Position;
};

[[vk::push_constant]]
VertexData<float2> vs;

[shader("vertex")]
VertexOutput vertexMain(uint id: SV_VertexID) {
    VertexOutput out;
    out.position = float4(vs.get()[id], 0.0, 1.0);
    return out;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target0 {
    return float4(1.0, 0.0, 0.0, 1.0);
}
