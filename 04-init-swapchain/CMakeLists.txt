cmake_minimum_required(VERSION 3.7.2)
project(04-init-swapchain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE PROJECT_SOURCES src/*.cpp)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_compile_definitions(
    ${PROJECT_NAME}
	PRIVATE -DGLFW_INCLUDE_VULKAN)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE Vulkan::Vulkan
    PRIVATE glfw)
