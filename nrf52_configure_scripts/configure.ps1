#!/usr/bin/env pwsh
# configure.ps1 - PowerShell CMake configuration script for Windows/Linux/macOS

param(
    [string]$BuildType = "Debug",
    [switch]$Clean
)

# Stop on errors
$ErrorActionPreference = "Stop"

# Get the directory where this script is located
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Function to resolve absolute path
function Resolve-AbsolutePath {
    param([string]$RelativePath)

    $FullPath = Join-Path $ScriptDir $RelativePath
    if (Test-Path $FullPath) {
        return (Resolve-Path $FullPath).Path
    } else {
        throw "Path not found: $RelativePath (resolved to: $FullPath)"
    }
}

try {
    # Convert relative paths to absolute paths
    Write-Host "Resolving paths..." -ForegroundColor Cyan

    $ArmGccPath = Resolve-AbsolutePath "..\libraries\arm-gnu-toolchain"
    $NrfSdkPath = Resolve-AbsolutePath "..\libraries\nrf5_sdk"
    $LvglPath = Resolve-AbsolutePath "..\libraries\lvgl"
    $CmakePath = Resolve-AbsolutePath "..\libraries\cmake\bin"
    $SourcePath = Resolve-AbsolutePath "..\libraries\nrf52-lcd-tester-fw"

    # Determine CMake executable name based on platform
    if ($IsWindows -or $env:OS -match "Windows") {
        $CmakeExe = Join-Path $CmakePath "cmake.exe"
    } else {
        $CmakeExe = Join-Path $CmakePath "cmake"
    }

    if (-not (Test-Path $CmakeExe)) {
        throw "CMake executable not found at: $CmakeExe"
    }

    # Convert images path to absolute path
    $ImagesPath = Resolve-AbsolutePath "..\generated"

    Write-Host ""
    Write-Host "Configuring with:" -ForegroundColor Green
    Write-Host "  ARM_GCC_PATH:  $ArmGccPath"
    Write-Host "  NRF_SDK_PATH:  $NrfSdkPath"
    Write-Host "  LVGL_PATH:     $LvglPath"
    Write-Host "  IMAGES_PATH:   $ImagesPath"
    Write-Host "  BUILD_TYPE:    $BuildType"
    Write-Host "  CMAKE:         $CmakeExe"
    Write-Host "  SOURCE:        $SourcePath"
    Write-Host ""

    # Change to script directory
    Push-Location $ScriptDir

    try {
        # Always clean CMake cache to ensure fresh configuration
        Write-Host "Cleaning previous build files..." -ForegroundColor Yellow
        if (Test-Path "CMakeCache.txt") { Remove-Item "CMakeCache.txt" -Force }
        if (Test-Path "CMakeFiles") { Remove-Item "CMakeFiles" -Recurse -Force }
        if (Test-Path "cmake_install.cmake") { Remove-Item "cmake_install.cmake" -Force }
        if (Test-Path "Makefile") { Remove-Item "Makefile" -Force }

        # Convert paths to use forward slashes for CMake compatibility
        $ArmGccPath = $ArmGccPath -replace '\\', '/'
        $NrfSdkPath = $NrfSdkPath -replace '\\', '/'
        $LvglPath = $LvglPath -replace '\\', '/'
        $ImagesPath = $ImagesPath -replace '\\', '/'
        $SourcePath = $SourcePath -replace '\\', '/'

        # Use toolchain file to set compilers
        $ToolchainFile = Join-Path $ScriptDir "toolchain-arm-none-eabi.cmake"
        $ToolchainFile = $ToolchainFile -replace '\\', '/'

        # Build CMake arguments (use STRING type for cache variables)
        $CmakeArgs = @(
            "-DCMAKE_TOOLCHAIN_FILE=$ToolchainFile",
            "-DARM_GCC_PATH:STRING=$ArmGccPath",
            "-DNRF_SDK_PATH:STRING=$NrfSdkPath",
            "-DLVGL_PATH:STRING=$LvglPath",
            "-DIMAGES_PATH:STRING=$ImagesPath",
            "-DCMAKE_BUILD_TYPE=$BuildType",
            "$SourcePath"
        )

        # Add generator for Windows
        if ($IsWindows -or $env:OS -match "Windows") {
            $CmakeArgs = @("-G", "MinGW Makefiles") + $CmakeArgs
        }

        # Run CMake
        Write-Host "Running CMake..." -ForegroundColor Cyan
        & $CmakeExe @CmakeArgs

        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed with exit code $LASTEXITCODE"
        }

        # Build the project
        Write-Host ""
        Write-Host "Building project..." -ForegroundColor Cyan
        & $CmakeExe --build .

        if ($LASTEXITCODE -ne 0) {
            throw "Build failed with exit code $LASTEXITCODE"
        }

    } finally {
        Pop-Location
    }

} catch {
    Write-Host ""
    Write-Host "Error: $_" -ForegroundColor Red
    exit 1
}
