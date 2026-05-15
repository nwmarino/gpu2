cmake_minimum_required(VERSION 3.10)

project(igpu VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON) Ideally not a req.

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

IF (WIN32)
    set(CMAKE_PREFIX_PATH "C:/Program Files (x86)/GLFW;${CMAKE_PREFIX_PATH}")
ENDIF ()

# Make dependent on a build time flag...
find_package(glfw3 REQUIRED)
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
        -D IGPU_GLFW
)

target_link_libraries(igpu
    PRIVATE
        glfw
        Vulkan::Vulkan
)
