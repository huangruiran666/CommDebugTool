cmake_minimum_required(VERSION 3.16)
project(CommDebugTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find Qt
find_package(Qt6 COMPONENTS Widgets SerialPort Network QUIET)

if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets SerialPort Network REQUIRED)
    set(QT_LIBRARIES Qt5::Widgets Qt5::SerialPort Qt5::Network)
else()
    qt_standard_project_setup()
    set(QT_LIBRARIES Qt6::Widgets Qt6::SerialPort Qt6::Network)
endif()

# File Discovery
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/CommDebugTool/*.h")
file(GLOB_RECURSE FORMS "ui/*.ui")

add_executable(CommDebugTool MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

target_include_directories(CommDebugTool PRIVATE 
    "include"
    "include/CommDebugTool"
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(CommDebugTool PRIVATE ${QT_LIBRARIES})
