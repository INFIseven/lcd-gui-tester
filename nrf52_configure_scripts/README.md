# CMake Configuration Scripts

This directory contains cross-platform scripts to configure the nRF52 LCD Tester firmware project with CMake. These scripts automatically resolve relative paths to absolute paths, which is required by CMake for cross-compilation toolchains.

## Available Scripts

### Linux/macOS: `configure.sh`

**Usage:**
```bash
./configure.sh [BuildType] [clean]
```

**Examples:**
```bash
# Configure with Debug build type (default)
./configure.sh

# Configure with Release build type
./configure.sh Release

# Clean and reconfigure with Debug
./configure.sh Debug clean

# Clean and reconfigure with Release
./configure.sh Release clean
```

### Windows (Command Prompt): `configure.bat`

**Usage:**
```cmd
configure.bat [BuildType] [clean]
```

**Examples:**
```cmd
REM Configure with Debug build type (default)
configure.bat

REM Configure with Release build type
configure.bat Release

REM Clean and reconfigure with Debug
configure.bat Debug clean

REM Clean and reconfigure with Release
configure.bat Release clean
```

### Windows/Linux/macOS (PowerShell): `configure.ps1`

**Usage:**
```powershell
./configure.ps1 [-BuildType <type>] [-Clean]
```

**Examples:**
```powershell
# Configure with Debug build type (default)
./configure.ps1

# Configure with Release build type
./configure.ps1 -BuildType Release

# Clean and reconfigure with Debug
./configure.ps1 -Clean

# Clean and reconfigure with Release
./configure.ps1 -BuildType Release -Clean
```

## What the Scripts Do

1. **Resolve Paths**: Convert relative paths to absolute paths for:
   - ARM GCC Toolchain (`../libraries/arm-gnu-toolchain`)
   - nRF5 SDK (`../libraries/nrf5_sdk`)
   - LVGL Library (`../libraries/lvgl`)
   - CMake (`../libraries/cmake/bin`)
   - Source Code (`../libraries/nrf52-lcd-tester-fw`)

2. **Validate**: Check that all required directories exist

3. **Display Configuration**: Show all paths and settings being used

4. **Clean (Optional)**: Remove previous CMake cache files if `clean` parameter is provided

5. **Run CMake**: Execute CMake with the correct absolute paths

## Building After Configuration

Once configuration is complete, you can build the project using:

**Linux/macOS:**
```bash
make
# or
cmake --build .
```

**Windows:**
```cmd
mingw32-make
REM or
cmake --build .
```

**PowerShell (any platform):**
```powershell
cmake --build .
```

## Troubleshooting

### Error: "Path not found"
Make sure all required libraries are present in the `../libraries/` directory:
- `arm-gnu-toolchain/`
- `nrf5_sdk/`
- `lvgl/`
- `cmake/`
- `nrf52-lcd-tester-fw/`

### Error: "CMake configuration failed"
1. Check that the ARM GCC toolchain is properly installed
2. Verify that the nRF5 SDK is the correct version
3. Try running with the `clean` parameter to remove cached configuration

## Manual CMake Command

If you need to run CMake manually with absolute paths:

```bash
cmake \
  -DARM_GCC_PATH=/absolute/path/to/arm-gnu-toolchain \
  -DNRF_SDK_PATH=/absolute/path/to/nrf5_sdk \
  -DLVGL_PATH=/absolute/path/to/lvgl \
  -DIMAGES_PATH=../generated \
  -DCMAKE_BUILD_TYPE=Debug \
  /absolute/path/to/nrf52-lcd-tester-fw
```

## Why Absolute Paths?

CMake requires absolute paths for compiler executables when they are set before the `project()` command in cross-compilation scenarios. The CMakeLists.txt file sets the ARM GCC compiler paths using the `ARM_GCC_PATH` variable, so this path must be absolute, not relative.
