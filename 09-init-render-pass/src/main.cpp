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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

const glm::mat4 c_clip(
    glm::vec4(1.f, 0.f, 0.f, 0.f),
    glm::vec4(0.f, -1.f, 0.f, 0.f),
    glm::vec4(0.f, 0.f, .5f, 0.f),
    glm::vec4(0.f, 0.f, .5f, 1.f));

const glm::mat4 c_mvp = c_clip *
                        glm::perspective(glm::radians(45.f), 1.f, .1f, 100.f) *
                        glm::lookAt(
                            glm::vec3(-5.f, 3.f, -10.f),
                            glm::vec3(0.f, 0.f, 0.f),
                            glm::vec3(0.f, -1.f, 0.f)) *
                        glm::mat4(1.f);

std::pair<bool, uint32_t> findMemoryTypeIndex(
    const VkPhysicalDeviceMemoryProperties& physical_device_mem_props,
    const VkMemoryRequirements& mem_reqs,
    VkMemoryPropertyFlags prop_flags)
{
    uint32_t type_index = std::numeric_limits<uint32_t>::max();
    uint32_t mem_type_bits = mem_reqs.memoryTypeBits;

    for (uint32_t i = 0; i < physical_device_mem_props.memoryTypeCount; ++i)
    {
        if ((mem_type_bits & 1) == 1 &&
            (physical_device_mem_props.memoryTypes[i].propertyFlags &
             prop_flags) == prop_flags)
        {
            type_index = i;
            break;
        }
        mem_type_bits >>= 1;
    }

    return {
        type_index != std::numeric_limits<uint32_t>::max(),
        type_index,
    };
}

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

    VkPhysicalDeviceMemoryProperties physical_device_mem_prop = {};
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_mem_prop);

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

    VkFormat depth_image_format = VK_FORMAT_D16_UNORM;
    VkImageTiling depth_image_tiling = VK_IMAGE_TILING_MAX_ENUM;

    VkFormatProperties depth_image_format_properties = {};
    vkGetPhysicalDeviceFormatProperties(
        physical_device, depth_image_format, &depth_image_format_properties);
    if (depth_image_format_properties.linearTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        depth_image_tiling = VK_IMAGE_TILING_LINEAR;
    }
    else if (depth_image_format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        depth_image_tiling = VK_IMAGE_TILING_OPTIMAL;
    }

    if (depth_image_tiling == VK_IMAGE_TILING_MAX_ENUM)
    {
        std::cerr << "Depth image format " << depth_image_format
                  << " not supported." << std::endl;
        return EXIT_FAILURE;
    }

    VkImageCreateInfo depth_image_info = {};
    depth_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image_info.imageType = VK_IMAGE_TYPE_2D;
    depth_image_info.format = depth_image_format;
    depth_image_info.extent.width = c_width;
    depth_image_info.extent.height = c_height;
    depth_image_info.extent.depth = 1;
    depth_image_info.mipLevels = 1;
    depth_image_info.arrayLayers = 1;
    depth_image_info.tiling = depth_image_tiling;
    depth_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImage depth_image = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateImage(device, &depth_image_info, nullptr, &depth_image),
        "CreateImage");

    VkMemoryRequirements depth_image_mem_reqs = {};
    vkGetImageMemoryRequirements(device, depth_image, &depth_image_mem_reqs);

    auto[depth_image_mem_type_index_found, depth_image_mem_type_index] =
        findMemoryTypeIndex(
            physical_device_mem_prop,
            depth_image_mem_reqs,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (!depth_image_mem_type_index_found)
    {
        std::cerr << "Couldn't find depth image memory type." << std::endl;
        return EXIT_FAILURE;
    }

    VkMemoryAllocateInfo depth_image_mem_alloc = {};
    depth_image_mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depth_image_mem_alloc.allocationSize = depth_image_mem_reqs.size;
    depth_image_mem_alloc.memoryTypeIndex = depth_image_mem_type_index;

    VkDeviceMemory depth_image_mem = VK_NULL_HANDLE;
    vkAllocateMemory(device, &depth_image_mem_alloc, nullptr, &depth_image_mem);

    FAIL_IF_NOT_SUCCESS(
        vkBindImageMemory(device, depth_image, depth_image_mem, 0),
        "BindImageMemory");

    VkImageViewCreateInfo depth_imageview_info = {};
    depth_imageview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_imageview_info.image = depth_image;
    depth_imageview_info.format = VK_FORMAT_D16_UNORM;
    depth_imageview_info.components.r = VK_COMPONENT_SWIZZLE_R;
    depth_imageview_info.components.g = VK_COMPONENT_SWIZZLE_G;
    depth_imageview_info.components.b = VK_COMPONENT_SWIZZLE_B;
    depth_imageview_info.components.a = VK_COMPONENT_SWIZZLE_A;
    depth_imageview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_imageview_info.subresourceRange.baseMipLevel = 0;
    depth_imageview_info.subresourceRange.levelCount = 1;
    depth_imageview_info.subresourceRange.baseArrayLayer = 0;
    depth_imageview_info.subresourceRange.layerCount = 1;
    depth_imageview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

    VkImageView depth_imageview = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateImageView(device, &depth_imageview_info, nullptr, &depth_imageview),
        "CreateImageView");

    VkBufferCreateInfo uniform_buf_info = {};
    uniform_buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniform_buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniform_buf_info.size = sizeof(c_mvp);
    uniform_buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer uniform_buf = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateBuffer(device, &uniform_buf_info, nullptr, &uniform_buf),
        "CreateBuffer");

    VkMemoryRequirements uniform_buf_mem_reqs = {};
    vkGetBufferMemoryRequirements(device, uniform_buf, &uniform_buf_mem_reqs);

    auto[uniform_buf_mem_type_index_found, uniform_buf_mem_type_index] =
        findMemoryTypeIndex(
            physical_device_mem_prop,
            uniform_buf_mem_reqs,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (!uniform_buf_mem_type_index_found)
    {
        std::cerr << "Couldn't find depth image memory type." << std::endl;
        return EXIT_FAILURE;
    }

    VkMemoryAllocateInfo uniform_buf_mem_alloc_info = {};
    uniform_buf_mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    uniform_buf_mem_alloc_info.memoryTypeIndex = uniform_buf_mem_type_index;
    uniform_buf_mem_alloc_info.allocationSize = uniform_buf_mem_reqs.size;

    VkDeviceMemory uniform_buf_mem = {};
    FAIL_IF_NOT_SUCCESS(
        vkAllocateMemory(
            device, &uniform_buf_mem_alloc_info, nullptr, &uniform_buf_mem),
        "AllocateMemory");

    void* uniform_buf_data_ptr = nullptr;
    FAIL_IF_NOT_SUCCESS(
        vkMapMemory(
            device, uniform_buf_mem, 0, uniform_buf_mem_reqs.size, 0, &uniform_buf_data_ptr),
        "MapMemory");
    std::memcpy(uniform_buf_data_ptr, &c_mvp, sizeof(c_mvp));
    vkUnmapMemory(device, uniform_buf_mem);

    FAIL_IF_NOT_SUCCESS(
        vkBindBufferMemory(device, uniform_buf, uniform_buf_mem, 0),
        "BindBufferMemory");

    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = 0;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.bindingCount = 1;
    descriptor_layout.pBindings = &layout_binding;

    std::vector<VkDescriptorSetLayout> layout_desc_set(1);
    FAIL_IF_NOT_SUCCESS(
        vkCreateDescriptorSetLayout(
            device, &descriptor_layout, nullptr, layout_desc_set.data()),
        "CreateDescriptorSetLayout");

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = layout_desc_set.size();
    pipeline_layout_info.pSetLayouts = layout_desc_set.data();

    VkPipelineLayout pipeline_layout = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreatePipelineLayout(
            device, &pipeline_layout_info, nullptr, &pipeline_layout),
        "CreatePipelineLayout");

    VkAttachmentDescription attachment_descs[2];
    attachment_descs[0].format = surface_format.format;
    attachment_descs[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_descs[1].format = depth_image_format;
    attachment_descs[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[1].finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_reference = {};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_reference = {};
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pDepthStencilAttachment = &depth_reference;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = attachment_descs;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    VkRenderPass render_pass = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateRenderPass(device, &render_pass_info, NULL, &render_pass),
        "CreateRenderPass");

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}
