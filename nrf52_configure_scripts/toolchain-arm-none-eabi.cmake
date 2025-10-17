# ARM GCC Toolchain file for cross-compilation
# This file must be specified with -DCMAKE_TOOLCHAIN_FILE before the source directory

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Specify the cross compiler (will be set by configure script)
# Note: During try_compile, CMAKE variables from command line are passed as cache variables
# Check if ARM_GCC_PATH is already defined (either as cache or normal variable)
if(NOT ARM_GCC_PATH)
    # Try to get from environment variable as fallback
    if(DEFINED ENV{ARM_GCC_PATH})
        set(ARM_GCC_PATH $ENV{ARM_GCC_PATH} CACHE STRING "ARM GCC Path" FORCE)
    else()
        message(FATAL_ERROR "ARM_GCC_PATH must be defined via -DARM_GCC_PATH:STRING=<path> or environment variable")
    endif()
endif()

# Set compilers
if(WIN32)
    set(CMAKE_C_COMPILER "${ARM_GCC_PATH}/bin/arm-none-eabi-gcc.exe")
    set(CMAKE_CXX_COMPILER "${ARM_GCC_PATH}/bin/arm-none-eabi-g++.exe")
    set(CMAKE_ASM_COMPILER "${ARM_GCC_PATH}/bin/arm-none-eabi-gcc.exe")
    set(CMAKE_OBJCOPY "${ARM_GCC_PATH}/bin/arm-none-eabi-objcopy.exe")
    set(CMAKE_SIZE "${ARM_GCC_PATH}/bin/arm-none-eabi-size.exe")
else()
    set(CMAKE_C_COMPILER "${ARM_GCC_PATH}/bin/arm-none-eabi-gcc")
    set(CMAKE_CXX_COMPILER "${ARM_GCC_PATH}/bin/arm-none-eabi-g++")
    set(CMAKE_ASM_COMPILER "${ARM_GCC_PATH}/bin/arm-none-eabi-gcc")
    set(CMAKE_OBJCOPY "${ARM_GCC_PATH}/bin/arm-none-eabi-objcopy")
    set(CMAKE_SIZE "${ARM_GCC_PATH}/bin/arm-none-eabi-size")
endif()

# Skip compiler tests for cross-compilation
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
