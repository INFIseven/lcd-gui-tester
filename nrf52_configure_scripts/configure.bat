@echo off
REM configure.bat - Windows CMake configuration script

setlocal enabledelayedexpansion

REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

REM Convert relative paths to absolute paths
pushd "%SCRIPT_DIR%\..\libraries\arm-gnu-toolchain" 2>nul
if errorlevel 1 (
    echo Error: ARM GCC toolchain not found at ..\libraries\arm-gnu-toolchain
    exit /b 1
)
set "ARM_GCC_PATH=%CD%"
popd

pushd "%SCRIPT_DIR%\..\libraries\nrf5_sdk" 2>nul
if errorlevel 1 (
    echo Error: nRF5 SDK not found at ..\libraries\nrf5_sdk
    exit /b 1
)
set "NRF_SDK_PATH=%CD%"
popd

pushd "%SCRIPT_DIR%\..\libraries\lvgl" 2>nul
if errorlevel 1 (
    echo Error: LVGL not found at ..\libraries\lvgl
    exit /b 1
)
set "LVGL_PATH=%CD%"
popd

pushd "%SCRIPT_DIR%\..\libraries\cmake\bin" 2>nul
if errorlevel 1 (
    echo Error: CMake not found at ..\libraries\cmake\bin
    exit /b 1
)
set "CMAKE_PATH=%CD%\cmake.exe"
popd

pushd "%SCRIPT_DIR%\..\libraries" 2>nul
if errorlevel 1 (
    echo Error: Libraries directory not found at ..\libraries
    exit /b 1
)
set "NINJA_PATH=%CD%\ninja\ninja.exe"
if not exist "%NINJA_PATH%" (
    echo Error: Ninja not found at ..\libraries\ninja\ninja.exe
    exit /b 1
)
popd

pushd "%SCRIPT_DIR%\..\libraries\nrf52-lcd-tester-fw" 2>nul
if errorlevel 1 (
    echo Error: Source not found at ..\libraries\nrf52-lcd-tester-fw
    exit /b 1
)
set "SOURCE_PATH=%CD%"
popd

REM Set build type (default to Debug if not specified)
set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Debug"

REM Convert images path to absolute path
pushd "%SCRIPT_DIR%\..\generated" 2>nul
if errorlevel 1 (
    echo Error: Images directory not found at ..\generated
    exit /b 1
)
set "IMAGES_PATH=%CD%"
popd

echo Configuring with:
echo   ARM_GCC_PATH:  %ARM_GCC_PATH%
echo   NRF_SDK_PATH:  %NRF_SDK_PATH%
echo   LVGL_PATH:     %LVGL_PATH%
echo   IMAGES_PATH:   %IMAGES_PATH%
echo   BUILD_TYPE:    %BUILD_TYPE%
echo   CMAKE:         %CMAKE_PATH%
echo   NINJA:         %NINJA_PATH%
echo   SOURCE:        %SOURCE_PATH%
echo.

REM Always clean CMake cache to ensure fresh configuration
echo Cleaning previous build files...
if exist CMakeCache.txt del /F /Q CMakeCache.txt
if exist CMakeFiles rd /S /Q CMakeFiles
if exist cmake_install.cmake del /F /Q cmake_install.cmake
if exist Makefile del /F /Q Makefile

REM Change to script directory
cd /d "%SCRIPT_DIR%"

REM Add Ninja to PATH
for %%i in ("%NINJA_PATH%") do set "NINJA_DIR=%%~dpi"
set "NINJA_DIR=%NINJA_DIR:~0,-1%"
set "PATH=%NINJA_DIR%;%PATH%"

REM Run CMake with absolute paths (use forward slashes for CMake compatibility)
set "ARM_GCC_PATH=%ARM_GCC_PATH:\=/%"
set "NRF_SDK_PATH=%NRF_SDK_PATH:\=/%"
set "LVGL_PATH=%LVGL_PATH:\=/%"
set "IMAGES_PATH=%IMAGES_PATH:\=/%"
set "SOURCE_PATH=%SOURCE_PATH:\=/%"

REM Use toolchain file to set compilers (must be before source directory)
set "TOOLCHAIN_FILE=%SCRIPT_DIR%\toolchain-arm-none-eabi.cmake"
set "TOOLCHAIN_FILE=%TOOLCHAIN_FILE:\=/%"

"%CMAKE_PATH%" ^
    -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN_FILE%" ^
    -DARM_GCC_PATH:STRING="%ARM_GCC_PATH%" ^
    -DNRF_SDK_PATH:STRING="%NRF_SDK_PATH%" ^
    -DLVGL_PATH:STRING="%LVGL_PATH%" ^
    -DIMAGES_PATH:STRING="%IMAGES_PATH%" ^
    -DCMAKE_BUILD_TYPE="%BUILD_TYPE%" ^
    "%SOURCE_PATH%"

if errorlevel 1 (
    echo.
    echo Configuration failed!
    exit /b 1
)

"%CMAKE_PATH%" --build .

endlocal
