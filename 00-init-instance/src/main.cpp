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

    VkInstance instance = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateInstance(&instance_info, nullptr, &instance), "CreateInstance");

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}
