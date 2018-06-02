/**
 * MIT License
 *
 * Copyright (c) 2018 Yuriy Khokhulya
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <vector>

#define FAIL_IF_NOT_SUCCESS(FunctionCall, ActionName)                     \
    if (VkResult result = (FunctionCall); result != VK_SUCCESS)           \
    {                                                                     \
        std::cerr << "'" << (ActionName) << "' failed. result=" << result \
                  << std::endl;                                           \
        return EXIT_FAILURE;                                              \
    }

constexpr int c_width = 640;
constexpr int c_height = 480;

int main(int argc, char** argv)
{
    glfwSetErrorCallback([](int err, const char* msg) {
        std::cerr << "[GLFW](" << std::hex << err << ") " << msg << std::endl;
    });

    if (GLFW_TRUE != glfwInit())
    {
        std::cerr << "Failed to init GLFW." << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window =
        glfwCreateWindow(c_width, c_height, "Vulkan", nullptr, nullptr);

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vulkan learning";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "no engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    uint32_t extensions_num = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_num);

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = extensions_num;
    instance_info.ppEnabledExtensionNames = extensions;

    VkInstance instance;
    FAIL_IF_NOT_SUCCESS(
        vkCreateInstance(&instance_info, nullptr, &instance), "CreateInstance");

    uint32_t physical_devices_num;
    FAIL_IF_NOT_SUCCESS(
        vkEnumeratePhysicalDevices(instance, &physical_devices_num, nullptr),
        "EnumeratePhysicalDevices");
    std::vector<VkPhysicalDevice> physical_devices(physical_devices_num);
    FAIL_IF_NOT_SUCCESS(
        vkEnumeratePhysicalDevices(
            instance, &physical_devices_num, physical_devices.data()),
        "EnumeratePhysicalDevices");

    uint32_t physical_device_index = std::numeric_limits<uint32_t>::max();
    for (std::size_t i = 0; i < physical_devices.size(); ++i)
    {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physical_device_index = static_cast<uint32_t>(i);
            break;
        }
    }
    if (physical_device_index == std::numeric_limits<uint32_t>::max())
    {
        std::cerr << "Suitable device not found." << std::endl;
        return EXIT_FAILURE;
    }

    VkPhysicalDevice physical_device = physical_devices[physical_device_index];

    uint32_t queue_family_num = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_num, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_num);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_num, queue_families.data());

    uint32_t queue_family_index = std::numeric_limits<uint32_t>::max();
    for (std::size_t i = 0; i < queue_families.size(); ++i)
    {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queue_family_index = static_cast<uint32_t>(i);
            break;
        }
    }

    if (queue_family_index == std::numeric_limits<uint32_t>::max())
    {
        std::cerr << "Suitable family queue not found." << std::endl;
        return EXIT_FAILURE;
    }

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = queue_family_index;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;

    VkDevice device = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateDevice(physical_device, &device_info, nullptr, &device),
        "CreateDevice");

    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.queueFamilyIndex = queue_family_index;

    VkCommandPool cmd_pool = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateCommandPool(device, &cmd_pool_info, nullptr, &cmd_pool),
        "CreateCommandPool");

    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {};
    cmd_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buffer_alloc_info.commandPool = cmd_pool;
    cmd_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buffer = {};
    FAIL_IF_NOT_SUCCESS(
        vkAllocateCommandBuffers(device, &cmd_buffer_alloc_info, &cmd_buffer),
        "AllocateCommandBuffers");

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}
