//
//  Copyright (c) 2026 Nick Marino
//  All rights reserved.
//

#include "gpu2.h"
#include "gpu2.vk.h"

using namespace gpu;

Device* gpu::createDevice(const DeviceInfo& info) {
    Device* device = nullptr;
    
    switch (info.backend) {
        case Backend::Vulkan:
            device = new (std::nothrow) VulkanDevice(info);
            break;
    }

    return device;
}
