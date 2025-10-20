#!/bin/bash
# configure.sh - Cross-platform CMake configuration script for Linux/macOS

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Convert relative paths to absolute paths
ARM_GCC_PATH="$(cd "$SCRIPT_DIR/../libraries/arm-gnu-toolchain" 2>/dev/null && pwd)" || {
    echo "Error: ARM GCC toolchain not found at ../libraries/arm-gnu-toolchain"
    exit 1
}

NRF_SDK_PATH="$(cd "$SCRIPT_DIR/../libraries/nrf5_sdk" 2>/dev/null && pwd)" || {
    echo "Error: nRF5 SDK not found at ../libraries/nrf5_sdk"
    exit 1
}

LVGL_PATH="$(cd "$SCRIPT_DIR/../libraries/lvgl" 2>/dev/null && pwd)" || {
    echo "Error: LVGL not found at ../libraries/lvgl"
    exit 1
}

CMAKE_PATH="$(cd "$SCRIPT_DIR/../libraries/cmake/bin" 2>/dev/null && pwd)" || {
    echo "Error: CMake not found at ../libraries/cmake/bin"
    exit 1
}

NINJA_PATH="$(cd "$SCRIPT_DIR/../libraries/ninja" 2>/dev/null && pwd)" || {
    echo "Error: Ninja not found at ../libraries/ninja"
    exit 1
}

# Check if ninja executable exists
if [ ! -f "$NINJA_PATH/ninja" ]; then
    echo "Error: Ninja executable not found at $NINJA_PATH/ninja"
    exit 1
fi

SOURCE_PATH="$(cd "$SCRIPT_DIR/../libraries/nrf52-lcd-tester-fw" 2>/dev/null && pwd)" || {
    echo "Error: Source not found at ../libraries/nrf52-lcd-tester-fw"
    exit 1
}

# Set build type (default to Debug if not specified)
BUILD_TYPE="${1:-Debug}"

# Convert images path to absolute path
IMAGES_PATH="$(cd "$SCRIPT_DIR/../generated" 2>/dev/null && pwd)" || {
    echo "Error: Images directory not found at ../generated"
    exit 1
}

echo "Configuring with:"
echo "  ARM_GCC_PATH:  $ARM_GCC_PATH"
echo "  NRF_SDK_PATH:  $NRF_SDK_PATH"
echo "  LVGL_PATH:     $LVGL_PATH"
echo "  IMAGES_PATH:   $IMAGES_PATH"
echo "  BUILD_TYPE:    $BUILD_TYPE"
echo "  CMAKE:         $CMAKE_PATH/cmake"
echo "  NINJA:         $NINJA_PATH/ninja"
echo "  SOURCE:        $SOURCE_PATH"
echo ""

# Always clean previous build files to ensure fresh configuration
echo "Cleaning previous build files..."
rm -f CMakeCache.txt cmake_install.cmake Makefile
rm -rf CMakeFiles/

# Add Ninja to PATH
export PATH="$NINJA_PATH:$PATH"

# Export paths as environment variables so they're available during CMake's try_compile phase
export ARM_GCC_PATH
export NRF_SDK_PATH
export LVGL_PATH
export IMAGES_PATH

# Set toolchain file path
TOOLCHAIN_FILE="$SCRIPT_DIR/toolchain-arm-none-eabi.cmake"

# Run CMake with absolute paths and Ninja generator
"$CMAKE_PATH/cmake" \
    -G "Ninja" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DARM_GCC_PATH:STRING="$ARM_GCC_PATH" \
    -DNRF_SDK_PATH:STRING="$NRF_SDK_PATH" \
    -DLVGL_PATH:STRING="$LVGL_PATH" \
    -DIMAGES_PATH:STRING="$IMAGES_PATH" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    "$SOURCE_PATH"

if [ $? -ne 0 ]; then
    echo ""
    echo "Configuration failed!"
    exit 1
fi

"$CMAKE_PATH/cmake" --build .
