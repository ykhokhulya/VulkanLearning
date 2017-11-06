/**
 * MIT License
 *
 * Copyright (c) 2017 Yuriy Khokhulya
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
#include <vulkan/vulkan.h>

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

    GLFWwindow* window = glfwCreateWindow(640, 480, "Vulkan", nullptr, nullptr);

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vulkan learning";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "no engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    uint32_t extensions_num = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_num);

    uint32_t available_extensions_num;
    VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
        const char* pLayerName,
        uint32_t* pPropertyCount,
        VkExtensionProperties* pProperties);

    uint32_t layers_num;
    vkEnumerateInstanceLayerProperties(&layers_num, nullptr);
    std::vector<VkLayerProperties> available_layers(layers_num);
    vkEnumerateInstanceLayerProperties(&layers_num, available_layers.data());

    for (auto&& layer : available_layers)
    {
        std::cout << "[" << layer.layerName << "] " << layer.description
                  << std::endl;
    }

    VkInstanceCreateInfo instance_cinfo = {};
    instance_cinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_cinfo.pApplicationInfo = &app_info;
    instance_cinfo.enabledLayerCount = 0;
    instance_cinfo.enabledExtensionCount = extensions_num;
    instance_cinfo.ppEnabledExtensionNames = extensions;

    VkInstance instance;
    if (VkResult result = vkCreateInstance(&instance_cinfo, nullptr, &instance);
        result != VK_SUCCESS)
    {
        std::cout << "Failed to create VK instance. result=" << result
                  << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
