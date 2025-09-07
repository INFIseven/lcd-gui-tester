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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

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

target_link_libraries(lcd-gui-tester PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network)

# For deployment
set_target_properties(lcd-gui-tester PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)