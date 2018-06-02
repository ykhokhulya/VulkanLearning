cmake_minimum_required(VERSION 3.7.2)
project(VulkanLearning)

if(MSVC)
  list(APPEND CMAKE_PREFIX_PATH C:/Glfw3)
  set(VULKAN_SDK C:/VulkanSDK/1.1.73.0)
endif()

add_subdirectory(00-init-instance)
add_subdirectory(01-enumarate-devices)
add_subdirectory(02-init-device)
