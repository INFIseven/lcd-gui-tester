cmake_minimum_required(VERSION 3.20)

project(lcd-gui-tester VERSION 1.0.0 LANGUAGES CXX)

# Get git tag for version display
execute_process(
    COMMAND git describe --tags --exact-match HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# If no exact tag, get version with commit info and dirty status
if(NOT GIT_TAG_VERSION)
    execute_process(
        COMMAND git describe --tags --always --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

# Fallback to project version if git is not available
if(NOT GIT_TAG_VERSION)
    set(GIT_TAG_VERSION "v${PROJECT_VERSION}")
endif()

message(STATUS "Application version: ${GIT_TAG_VERSION}")

# Pass version to the application
add_compile_definitions(APP_VERSION="${GIT_TAG_VERSION}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Auto-locate a Qt6 SDK installed under ~/Qt (e.g. ~/Qt/6.11.1/gcc_64) so a
# bare `cmake ..` works without passing -DCMAKE_PREFIX_PATH. An explicitly
# provided Qt6_DIR or CMAKE_PREFIX_PATH still takes precedence.
if(NOT Qt6_DIR AND NOT CMAKE_PREFIX_PATH)
    file(GLOB _qt_candidates "$ENV{HOME}/Qt/*/gcc_64")
    if(_qt_candidates)
        list(SORT _qt_candidates)
        list(REVERSE _qt_candidates) # prefer the highest version
        list(GET _qt_candidates 0 _qt_prefix)
        set(CMAKE_PREFIX_PATH "${_qt_prefix}")
        message(STATUS "Auto-detected Qt6 at: ${_qt_prefix}")
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network Concurrent)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/imagepreviewwidget.cpp
    src/imagedropwidget.cpp
    src/librarychecker.cpp
    src/embeddedpython.cpp
    src/lvglscriptrunner.cpp
    src/startupchecker.cpp
)

set(HEADERS
    src/mainwindow.h
    src/imagepreviewwidget.h
    src/imagedropwidget.h
    src/librarychecker.h
    src/embeddedpython.h
    src/lvglscriptrunner.h
    src/startupchecker.h
)

add_executable(lcd-gui-tester
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(lcd-gui-tester PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network Qt6::Concurrent)

# Copy nRF52 configure scripts to build_mcu folder
file(GLOB NRF52_CONFIGURE_SCRIPTS "${CMAKE_SOURCE_DIR}/nrf52_configure_scripts/*")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/build_mcu")

foreach(SCRIPT_FILE ${NRF52_CONFIGURE_SCRIPTS})
    get_filename_component(SCRIPT_NAME ${SCRIPT_FILE} NAME)
    configure_file(
        ${SCRIPT_FILE}
        "${CMAKE_BINARY_DIR}/build_mcu/${SCRIPT_NAME}"
        COPYONLY
    )
endforeach()

# Make sure shell scripts are executable
if(UNIX)
    file(GLOB SHELL_SCRIPTS "${CMAKE_BINARY_DIR}/build_mcu/*.sh" "${CMAKE_BINARY_DIR}/build_mcu/*.ps1")
    foreach(SHELL_SCRIPT ${SHELL_SCRIPTS})
        execute_process(COMMAND chmod +x ${SHELL_SCRIPT})
    endforeach()
endif()

# For deployment
# Comment out WIN32_EXECUTABLE for Debug builds to see console output
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(lcd-gui-tester PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
    )
endif()