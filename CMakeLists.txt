cmake_minimum_required(VERSION 3.10)

project(igpu VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON) Ideally not a req.

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

find_package(Vulkan REQUIRED)

add_library(igpu
    source/igpu.cpp
)

target_include_directories(igpu
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

target_compile_options(igpu
    PRIVATE
        -g
        -D IGPU_VULKAN
)

target_link_libraries(igpu
    PRIVATE
        Vulkan::Vulkan
)
