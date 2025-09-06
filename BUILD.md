# nRF52 Image Uploader - Build Instructions

## Prerequisites

- Qt6 (6.5 or later)
- CMake (3.20 or later)
- C++ compiler (GCC, Clang, or MSVC)

### Install Qt6

**Ubuntu/Debian:**
```bash
sudo apt install qt6-base-dev qt6-tools-dev cmake build-essential
```

**Windows:**
- Download Qt6 from https://www.qt.io/download
- Or use vcpkg: `vcpkg install qt6[core,widgets,gui]`

**macOS:**
```bash
brew install qt6 cmake
```

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Running

```bash
# From build directory
./nrf52-image-uploader
```

## Deployment

### Static Linking (Recommended for distribution)

**Windows:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DQt6_DIR="C:/Qt/6.5.0/msvc2022_64/lib/cmake/Qt6" ..
cmake --build . --config Release
windeployqt.exe Release/nrf52-image-uploader.exe
```

**Linux:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
# Use linuxdeployqt or AppImage for distribution
```

**macOS:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
macdeployqt nrf52-image-uploader.app -dmg
```

## Features

- Drag & drop up to 10 images
- Strict 370x120 pixel validation
- Image preview with remove functionality
- Ready for nRF52 toolchain integration