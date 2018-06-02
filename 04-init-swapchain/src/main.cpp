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
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>

#define FAIL_IF_NOT_SUCCESS(FunctionCall, ActionName)                     \
    if (VkResult result = (FunctionCall); result != VK_SUCCESS)           \
    {                                                                     \
        std::cerr << "'" << (ActionName) << "' failed. result=" << result \
                  << std::endl;                                           \
        return EXIT_FAILURE;                                              \
    }

constexpr uint32_t c_width = 640;
constexpr uint32_t c_height = 480;

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

    VkSurfaceKHR surface = {};
    FAIL_IF_NOT_SUCCESS(
        glfwCreateWindowSurface(instance, window, nullptr, &surface),
        "CreateWindowSurface");

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

    VkPhysicalDevice& physical_device = physical_devices[physical_device_index];

    uint32_t queue_family_num = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_num, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_num);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_num, queue_families.data());

    uint32_t graphics_queue_family_index = std::numeric_limits<uint32_t>::max();
    uint32_t present_queue_family_index = std::numeric_limits<uint32_t>::max();
    for (std::size_t i = 0; i < queue_families.size(); ++i)
    {
        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            physical_device, i, surface, &present_support);

        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            if (present_support == VK_TRUE)
            {
                graphics_queue_family_index = static_cast<uint32_t>(i);
                present_queue_family_index = static_cast<uint32_t>(i);
                break;
            }

            if (graphics_queue_family_index == std::numeric_limits<uint32_t>::max())
            {
                graphics_queue_family_index = static_cast<uint32_t>(i);
            }
        }

        if (present_queue_family_index == std::numeric_limits<uint32_t>::max() &&
            present_support == VK_TRUE)
        {
            present_queue_family_index = static_cast<uint32_t>(i);
        }
    }

    if (graphics_queue_family_index == std::numeric_limits<uint32_t>::max())
    {
        std::cerr << "Suitable graphic queue family not found." << std::endl;
        return EXIT_FAILURE;
    }

    if (present_queue_family_index == std::numeric_limits<uint32_t>::max())
    {
        std::cerr << "Suitable present queue family not found." << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    std::set<uint32_t> unique_queue_family_indices = {
        graphics_queue_family_index,
        present_queue_family_index,
    };

    for (auto queue_family_index : unique_queue_family_indices)
    {
        float queue_priority = 1.0f;
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = queue_family_index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        queue_infos.push_back(queue_info);
    }

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
    device_info.pQueueCreateInfos = queue_infos.data();

    VkDevice device = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateDevice(physical_device, &device_info, nullptr, &device),
        "CreateDevice");

    VkQueue present_queue = {};
    vkGetDeviceQueue(device, present_queue_family_index, 0, &present_queue);

    VkQueue graphic_queue = {};
    vkGetDeviceQueue(device, graphics_queue_family_index, 0, &graphic_queue);

    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.queueFamilyIndex = graphics_queue_family_index;

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

    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    FAIL_IF_NOT_SUCCESS(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            physical_device, surface, &surface_capabilities),
        "GetPhysicalDeviceSurfaceCapabilities");

    uint32_t surface_format_num;
    FAIL_IF_NOT_SUCCESS(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device, surface, &surface_format_num, nullptr),
        "GetPhysicalDeviceSurfaceFormats");
    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_num);
    FAIL_IF_NOT_SUCCESS(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device, surface, &surface_format_num, surface_formats.data()),
        "GetPhysicalDeviceSurfaceFormats");

    if (surface_formats.empty())
    {
        std::cerr << "Suitable surface format not found." << std::endl;
        return EXIT_FAILURE;
    }

    VkSurfaceFormatKHR surface_format = [&surface_formats]() {
        if (surface_formats.size() == 1 &&
            surface_formats.front().format == VK_FORMAT_UNDEFINED)
        {
            return VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM,
                                      VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        }
        for (const auto& format : surface_formats)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return format;
            }
        }
        return surface_formats.front();

    }();

    uint32_t present_mode_num;
    FAIL_IF_NOT_SUCCESS(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_device, surface, &present_mode_num, nullptr),
        "GetPhysicalDeviceSurfacePresentModes");
    std::vector<VkPresentModeKHR> present_modes(present_mode_num);
    FAIL_IF_NOT_SUCCESS(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_device, surface, &present_mode_num, present_modes.data()),
        "GetPhysicalDeviceSurfacePresentModes");

    VkPresentModeKHR present_mode = [&present_modes]() {
        VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& mode : present_modes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return mode;
            }
            else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                best_mode = mode;
            }
        }
        return best_mode;
    }();

    VkExtent2D extent = [&surface_capabilities]() {
        if (surface_capabilities.currentExtent.width !=
            std::numeric_limits<uint32_t>::max())
        {
            return surface_capabilities.currentExtent;
        }
        VkExtent2D actual_extent = {};
        actual_extent.width = std::clamp(
            c_width,
            surface_capabilities.minImageExtent.width,
            surface_capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(
            c_height,
            surface_capabilities.minImageExtent.height,
            surface_capabilities.maxImageExtent.height);
        return actual_extent;
    }();

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_info.imageFormat = surface_format.format;
    swapchain_info.imageColorSpace = surface_format.colorSpace;
    swapchain_info.imageExtent = extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (graphics_queue_family_index != present_queue_family_index)
    {
        uint32_t queue_family_indices[] = {
            graphics_queue_family_index,
            present_queue_family_index,
        };
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = queue_family_indices;
    }
    swapchain_info.preTransform = surface_capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain),
        "CreateSwapChain");

    uint32_t swapchain_image_num;
    FAIL_IF_NOT_SUCCESS(
        vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_num, nullptr),
        "GetSwapchainImages");
    std::vector<VkImage> swapchain_images(swapchain_image_num);
    FAIL_IF_NOT_SUCCESS(
        vkGetSwapchainImagesKHR(
            device, swapchain, &swapchain_image_num, swapchain_images.data()),
        "GetSwapchainImages");

    std::vector<VkImageView> swapchain_imageviews(swapchain_images.size());
    for (std::size_t i = 0; i < swapchain_imageviews.size(); ++i)
    {
        VkImageViewCreateInfo imageview_info = {};
        imageview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageview_info.flags;
        imageview_info.image = swapchain_images[i];
        imageview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageview_info.format = surface_format.format;
        imageview_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageview_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageview_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageview_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageview_info.subresourceRange.baseMipLevel = 0;
        imageview_info.subresourceRange.levelCount = 1;
        imageview_info.subresourceRange.baseArrayLayer = 0;
        imageview_info.subresourceRange.layerCount = 1;

        FAIL_IF_NOT_SUCCESS(
            vkCreateImageView(
                device, &imageview_info, nullptr, &swapchain_imageviews[i]),
            "CreateImageView");
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}
