//
//  common.h
//
//  Copyright (c) 2026 Nick Marino.
//  All rights reserved.
//

#ifndef GPU2_EXAMPLES_COMMON_H_
#define GPU2_EXAMPLES_COMMON_H_

#include <string>
#include <fstream>

static inline std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    std::streamsize size = file.tellg();
    std::string contents = "";
    contents.resize(size);
    file.seekg(0);
    file.read(contents.data(), size);
    file.close();
    return contents;
}

#endif // GPU2_EXAMPLES_COMMON_H_
