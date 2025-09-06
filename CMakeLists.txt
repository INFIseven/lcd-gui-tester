cmake_minimum_required(VERSION 3.20)

project(lcd-gui-tester VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

qt_standard_project_setup()

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

qt_add_executable(lcd-gui-tester
    ${SOURCES}
    ${HEADERS}
)

qt_add_resources(lcd-gui-tester "resources"
    PREFIX "/"
    FILES
)

target_link_libraries(lcd-gui-tester PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network)

# For deployment
set_target_properties(lcd-gui-tester PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)