#!/bin/bash

slangc triangle.slang -profile glsl_450 -stage vertex -entry vertexMain -o triangle.vert.spv
slangc triangle.slang -profile glsl_450 -stage fragment -entry fragmentMain -o triangle.frag.spv
