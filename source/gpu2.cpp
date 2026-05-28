//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "gpu2.h"

using namespace gpu;

Device* gpu::createDevice(const DeviceInfo& info) {
    Device* device = nullptr;
    
    switch (info.backend) {
        case Backend::Vulkan:
            break;
    }

    return device;
}
