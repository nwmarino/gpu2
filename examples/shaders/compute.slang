//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

struct ComputeData<T> {
    [[vk::offset(16)]]
    Ptr<T> data;

    [ForceInline]
    Ptr<T> get() {
        return data;
    }
};

[[vk::push_constant]]
ComputeData<float> cs;

[shader("compute")]
[numthreads(64)]
void computeMain(uint id: SV_DispatchThreadID) {
    float *p = cs.get();

    p[id] = 42.0;
}
