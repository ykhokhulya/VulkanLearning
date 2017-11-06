cmake_minimum_required(VERSION 3.7.2)
project(00-instance-and-physical-device-selection)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

find_package(PkgConfig REQUIRED)

pkg_search_module(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_search_module(Vulkan REQUIRED IMPORTED_TARGET vulkan)

file(GLOB_RECURSE PROJECT_SOURCES src/*.cpp)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE PkgConfig::Vulkan
    PRIVATE PkgConfig::GLFW)
