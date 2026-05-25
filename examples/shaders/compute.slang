//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

struct ComputeData<T> {
    [[vk::offset(16)]]
    Ptr<T, Access.Immutable> data;

    [ForceInline]
    Ptr<T, Access.Immutable> get() {
        return data;
    }
};

[shader("compute")]
void computeMain() {

}
