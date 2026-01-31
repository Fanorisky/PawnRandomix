#!/bin/bash
set -e

# Build configuration
config="${CONFIG:-Release}"
build_samp="${BUILD_SAMP_PLUGIN:-0}"

# Docker image settings
image_name="randomix/build"
ubuntu_version="22.04"
image_tag="${image_name}:ubuntu-${ubuntu_version}"

echo "Building Docker image: $image_tag"
docker build \
    -t "$image_tag" \
    "build_ubuntu-${ubuntu_version}/" \
|| exit 1

# Prepare build directories
folders=('build')
for folder in "${folders[@]}"; do
    if [[ ! -d "./${folder}" ]]; then
        mkdir -p "${folder}" &&
        chown 1000:1000 "${folder}" || exit 1
    fi
done

# Run build in container
echo "Starting build..."
docker run \
    --rm \
    -t \
    -w /code \
    -v "$PWD:/code" \
    -v "$PWD/docker/build:/code/build" \
    -e CONFIG="${config}" \
    -e BUILD_SAMP_PLUGIN="${build_samp}" \
    "$image_tag"
