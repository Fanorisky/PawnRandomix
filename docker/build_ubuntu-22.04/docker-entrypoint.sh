#!/bin/sh
set -e

# Configuration
config="${CONFIG:-Release}"
build_samp="${BUILD_SAMP_PLUGIN:-0}"

# Determine build type name
if [ "$build_samp" = "1" ]; then
    build_type="SA-MP"
else
    build_type="open.mp"
fi

echo "========================================"
echo "Building Randomix for $build_type"
echo "Configuration: $config"
echo "========================================"

# Configure CMake
cmake \
    -S . \
    -B build \
    -G Ninja \
    -DCMAKE_C_FLAGS=-m32 \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DCMAKE_BUILD_TYPE="$config" \
    -DBUILD_SAMP_PLUGIN="$build_samp" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# Build
cmake \
    --build build \
    --config "$config" \
    --parallel $(nproc)

echo "========================================"
echo "Build completed successfully!"
echo "Output: build/Randomix.so"
echo "========================================"
