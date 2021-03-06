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
#include <shaderc/shaderc.hpp>
#include <unordered_map>
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

/**
 *  #version 400
 *  #extension GL_ARB_separate_shader_objects : enable
 *  #extension GL_ARB_shading_language_420pack : enable
 *  layout (std140, binding = 0) uniform bufferVals {
 *      mat4 mvp;
 *  } u_buffer_vals;
 *  layout (location = 0) in vec4 in_pos;
 *  layout (location = 1) in vec4 in_color;
 *  layout (location = 0) out vec4 out_color;
 *  void main() {
 *     out_color = in_color;
 *     gl_Position = u_buffer_vals.mvp * in_pos;
 *  }
 */
const std::vector<uint8_t> c_vert_shader = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x08, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x47, 0x4C, 0x53, 0x4C, 0x2E, 0x73, 0x74, 0x64, 0x2E, 0x34, 0x35, 0x30,
    0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x6D, 0x61, 0x69, 0x6E, 0x00, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
    0x1C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x90, 0x01, 0x00, 0x00, 0x04, 0x00, 0x09, 0x00, 0x47, 0x4C, 0x5F, 0x41,
    0x52, 0x42, 0x5F, 0x73, 0x65, 0x70, 0x61, 0x72, 0x61, 0x74, 0x65, 0x5F,
    0x73, 0x68, 0x61, 0x64, 0x65, 0x72, 0x5F, 0x6F, 0x62, 0x6A, 0x65, 0x63,
    0x74, 0x73, 0x00, 0x00, 0x04, 0x00, 0x09, 0x00, 0x47, 0x4C, 0x5F, 0x41,
    0x52, 0x42, 0x5F, 0x73, 0x68, 0x61, 0x64, 0x69, 0x6E, 0x67, 0x5F, 0x6C,
    0x61, 0x6E, 0x67, 0x75, 0x61, 0x67, 0x65, 0x5F, 0x34, 0x32, 0x30, 0x70,
    0x61, 0x63, 0x6B, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x6D, 0x61, 0x69, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x6F, 0x75, 0x74, 0x5F, 0x63, 0x6F, 0x6C, 0x6F,
    0x72, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x0B, 0x00, 0x00, 0x00,
    0x69, 0x6E, 0x5F, 0x63, 0x6F, 0x6C, 0x6F, 0x72, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x00, 0x67, 0x6C, 0x5F, 0x50,
    0x65, 0x72, 0x56, 0x65, 0x72, 0x74, 0x65, 0x78, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x67, 0x6C, 0x5F, 0x50, 0x6F, 0x73, 0x69, 0x74, 0x69, 0x6F, 0x6E, 0x00,
    0x06, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x67, 0x6C, 0x5F, 0x50, 0x6F, 0x69, 0x6E, 0x74, 0x53, 0x69, 0x7A, 0x65,
    0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x67, 0x6C, 0x5F, 0x43, 0x6C, 0x69, 0x70, 0x44,
    0x69, 0x73, 0x74, 0x61, 0x6E, 0x63, 0x65, 0x00, 0x05, 0x00, 0x03, 0x00,
    0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
    0x16, 0x00, 0x00, 0x00, 0x62, 0x75, 0x66, 0x66, 0x65, 0x72, 0x56, 0x61,
    0x6C, 0x73, 0x00, 0x00, 0x06, 0x00, 0x04, 0x00, 0x16, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x6D, 0x76, 0x70, 0x00, 0x05, 0x00, 0x06, 0x00,
    0x18, 0x00, 0x00, 0x00, 0x75, 0x5F, 0x62, 0x75, 0x66, 0x66, 0x65, 0x72,
    0x5F, 0x76, 0x61, 0x6C, 0x73, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
    0x1C, 0x00, 0x00, 0x00, 0x69, 0x6E, 0x5F, 0x70, 0x6F, 0x73, 0x00, 0x00,
    0x47, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0B, 0x00, 0x00, 0x00,
    0x1E, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x48, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x0B, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x48, 0x00, 0x04, 0x00,
    0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x48, 0x00, 0x05, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
    0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
    0x18, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x47, 0x00, 0x04, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x17, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x04, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00, 0x0A, 0x00, 0x00, 0x00,
    0x0B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
    0x0D, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2B, 0x00, 0x04, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x04, 0x00, 0x0F, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x05, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x0F, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00,
    0x11, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x15, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x04, 0x00,
    0x15, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x1E, 0x00, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x04, 0x00, 0x17, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x16, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00, 0x17, 0x00, 0x00, 0x00,
    0x18, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
    0x19, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
    0x3B, 0x00, 0x04, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0xF8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3D, 0x00, 0x04, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00,
    0x3E, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x05, 0x00, 0x19, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00,
    0x18, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x3D, 0x00, 0x04, 0x00,
    0x15, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00,
    0x3D, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00,
    0x1C, 0x00, 0x00, 0x00, 0x91, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x1E, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x05, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00,
    0x12, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x03, 0x00,
    0x1F, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFD, 0x00, 0x01, 0x00,
    0x38, 0x00, 0x01, 0x00,
};

/**
 *  #version 400
 *  #extension GL_ARB_separate_shader_objects : enable
 *  #extension GL_ARB_shading_language_420pack : enable
 *  layout (location = 0) in vec4 in_color;
 *  layout (location = 0) out vec4 out_color;
 *  void main() {
 *     out_color = in_color;
 *  }
 */
const std::vector<uint8_t> c_frag_shader = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x08, 0x00,
    0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x47, 0x4C, 0x53, 0x4C, 0x2E, 0x73, 0x74, 0x64, 0x2E, 0x34, 0x35, 0x30,
    0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x6D, 0x61, 0x69, 0x6E, 0x00, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x90, 0x01, 0x00, 0x00, 0x04, 0x00, 0x09, 0x00,
    0x47, 0x4C, 0x5F, 0x41, 0x52, 0x42, 0x5F, 0x73, 0x65, 0x70, 0x61, 0x72,
    0x61, 0x74, 0x65, 0x5F, 0x73, 0x68, 0x61, 0x64, 0x65, 0x72, 0x5F, 0x6F,
    0x62, 0x6A, 0x65, 0x63, 0x74, 0x73, 0x00, 0x00, 0x04, 0x00, 0x09, 0x00,
    0x47, 0x4C, 0x5F, 0x41, 0x52, 0x42, 0x5F, 0x73, 0x68, 0x61, 0x64, 0x69,
    0x6E, 0x67, 0x5F, 0x6C, 0x61, 0x6E, 0x67, 0x75, 0x61, 0x67, 0x65, 0x5F,
    0x34, 0x32, 0x30, 0x70, 0x61, 0x63, 0x6B, 0x00, 0x05, 0x00, 0x04, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x6D, 0x61, 0x69, 0x6E, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x05, 0x00, 0x09, 0x00, 0x00, 0x00, 0x6F, 0x75, 0x74, 0x5F,
    0x63, 0x6F, 0x6C, 0x6F, 0x72, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
    0x0B, 0x00, 0x00, 0x00, 0x69, 0x6E, 0x5F, 0x63, 0x6F, 0x6C, 0x6F, 0x72,
    0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
    0x0B, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
    0x0A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x3B, 0x00, 0x04, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0xF8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3D, 0x00, 0x04, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00,
    0x3E, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
    0xFD, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00,
};

const float c_cube_vertices[][6] = {
    // red face
    {-1, -1, 1, 1, 0, 0},
    {-1, 1, 1, 1, 0, 0},
    {1, -1, 1, 1, 0, 0},
    {1, -1, 1, 1, 0, 0},
    {-1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 0, 0},
    // green face
    {-1, -1, -1, 0, 1, 0},
    {1, -1, -1, 0, 1, 0},
    {-1, 1, -1, 0, 1, 0},
    {-1, 1, -1, 0, 1, 0},
    {1, -1, -1, 0, 1, 0},
    {1, 1, -1, 0, 1, 0},
    // blue face
    {-1, 1, 1, 0, 0, 1},
    {-1, -1, 1, 0, 0, 1},
    {-1, 1, -1, 0, 0, 1},
    {-1, 1, -1, 0, 0, 1},
    {-1, -1, 1, 0, 0, 1},
    {-1, -1, -1, 0, 0, 1},
    // yellow face
    {1, 1, 1, 1, 1, 0},
    {1, 1, -1, 1, 1, 0},
    {1, -1, 1, 1, 1, 0},
    {1, -1, 1, 1, 1, 0},
    {1, 1, -1, 1, 1, 0},
    {1, -1, -1, 1, 1, 0},
    // magenta face
    {1, 1, 1, 1, 0, 1},
    {-1, 1, 1, 1, 0, 1},
    {1, 1, -1, 1, 0, 1},
    {1, 1, -1, 1, 0, 1},
    {-1, 1, 1, 1, 0, 1},
    {-1, 1, -1, 1, 0, 1},
    // cyan face
    {1, -1, 1, 0, 1, 1},
    {1, -1, -1, 0, 1, 1},
    {-1, -1, 1, 0, 1, 1},
    {-1, -1, 1, 0, 1, 1},
    {1, -1, -1, 0, 1, 1},
    {-1, -1, -1, 0, 1, 1},
};

namespace {

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

} // namespace

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

    uint32_t glfw_extensions_num = 0;
    const char** glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extensions_num);

    std::vector<const char*> extensions(
        glfw_extensions, glfw_extensions + glfw_extensions_num);

    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    const std::vector<const char*> validation_layers = {
        "VK_LAYER_LUNARG_standard_validation",
    };

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();
    instance_info.enabledLayerCount =
        static_cast<uint32_t>(validation_layers.size());
    instance_info.ppEnabledLayerNames = validation_layers.data();

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
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceFeatures features = {};
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
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

    std::vector<const char*> device_extensions;
    device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	VkPhysicalDeviceFeatures device_features = {};
    device_features.depthClamp = VK_TRUE;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
    device_info.pQueueCreateInfos = queue_infos.data();
    device_info.enabledExtensionCount =
        static_cast<uint32_t>(device_extensions.size());
    device_info.ppEnabledExtensionNames = device_extensions.data();
    device_info.pEnabledFeatures = &device_features;

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

    VkAttachmentDescription attachment_descs[2] = {};
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
        vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass),
        "CreateRenderPass");

    VkPipelineShaderStageCreateInfo shader_stages[2] = {};
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].pName = "main";

    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].pName = "main";

    VkShaderModuleCreateInfo vert_shader_module_info = {};
    vert_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_shader_module_info.codeSize = c_vert_shader.size();
    vert_shader_module_info.pCode =
        reinterpret_cast<const uint32_t*>(c_vert_shader.data());
    FAIL_IF_NOT_SUCCESS(
        vkCreateShaderModule(
            device, &vert_shader_module_info, nullptr, &shader_stages[0].module),
        "CreateShaderModule");

    VkShaderModuleCreateInfo frag_shader_module_info = {};
    frag_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_shader_module_info.codeSize = c_frag_shader.size();
    frag_shader_module_info.pCode =
        reinterpret_cast<const uint32_t*>(c_frag_shader.data());
    FAIL_IF_NOT_SUCCESS(
        vkCreateShaderModule(
            device, &frag_shader_module_info, nullptr, &shader_stages[1].module),
        "CreateShaderModule");

    VkImageView attachments[2] = {};
    attachments[1] = depth_imageview;

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.renderPass = render_pass;
    fb_info.attachmentCount = 2;
    fb_info.pAttachments = attachments;
    fb_info.width = c_width;
    fb_info.height = c_height;
    fb_info.layers = 1;

    std::vector<VkFramebuffer> framebuffers(swapchain_imageviews.size());
    for (std::size_t i = 0; i < swapchain_imageviews.size(); i++)
    {
        attachments[0] = swapchain_imageviews[i];
        FAIL_IF_NOT_SUCCESS(
            vkCreateFramebuffer(device, &fb_info, nullptr, &framebuffers[i]),
            "CreateFramebuffer");
    }

    VkBufferCreateInfo vertex_buf_info = {};
    vertex_buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertex_buf_info.size = sizeof(c_cube_vertices);
    vertex_buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer vertex_buf = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateBuffer(device, &vertex_buf_info, nullptr, &vertex_buf),
        "CreateBuffer");

    VkMemoryRequirements vertex_buf_mem_reqs = {};
    vkGetBufferMemoryRequirements(device, vertex_buf, &vertex_buf_mem_reqs);

    auto[vertex_buf_mem_type_index_found, vertex_buf_mem_type_index] =
        findMemoryTypeIndex(
            physical_device_mem_prop,
            vertex_buf_mem_reqs,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (!vertex_buf_mem_type_index_found)
    {
        std::cerr << "Couldn't find vertex buffer memory type." << std::endl;
        return EXIT_FAILURE;
    }

    VkMemoryAllocateInfo vertex_buf_mem_alloc_info = {};
    vertex_buf_mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vertex_buf_mem_alloc_info.memoryTypeIndex = vertex_buf_mem_type_index;
    vertex_buf_mem_alloc_info.allocationSize = vertex_buf_mem_reqs.size;

    VkDeviceMemory vertex_buf_mem = {};
    FAIL_IF_NOT_SUCCESS(
        vkAllocateMemory(
            device, &vertex_buf_mem_alloc_info, nullptr, &vertex_buf_mem),
        "AllocateMemory");

    void* vertex_buf_data_ptr = nullptr;
    FAIL_IF_NOT_SUCCESS(
        vkMapMemory(
            device, vertex_buf_mem, 0, vertex_buf_mem_reqs.size, 0, &vertex_buf_data_ptr),
        "MapMemory");
    std::memcpy(vertex_buf_data_ptr, c_cube_vertices, sizeof(c_cube_vertices));
    vkUnmapMemory(device, vertex_buf_mem);

    FAIL_IF_NOT_SUCCESS(
        vkBindBufferMemory(device, vertex_buf, vertex_buf_mem, 0),
        "BindBufferMemory");

    VkVertexInputBindingDescription vi_binding = {};
    vi_binding.binding = 0;
    vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vi_binding.stride = sizeof(c_cube_vertices[0]);

    VkVertexInputAttributeDescription vi_attribs[2] = {};
    vi_attribs[0].binding = 0;
    vi_attribs[0].location = 0;
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vi_attribs[0].offset = 0;
    vi_attribs[1].binding = 0;
    vi_attribs[1].location = 1;
    vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vi_attribs[1].offset = 16;

    VkDynamicState dynamic_state_enables[VK_DYNAMIC_STATE_RANGE_SIZE] = {};
    uint32_t dynamic_state_num = 0;

    dynamic_state_enables[dynamic_state_num++] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_state_enables[dynamic_state_num++] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo dyn_state_info = {};
    dyn_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn_state_info.pDynamicStates = dynamic_state_enables;
    dyn_state_info.dynamicStateCount = dynamic_state_num;

    VkPipelineVertexInputStateCreateInfo vi_state_info = {};
    vi_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi_state_info.vertexBindingDescriptionCount = 1;
    vi_state_info.pVertexBindingDescriptions = &vi_binding;
    vi_state_info.vertexAttributeDescriptionCount = 2;
    vi_state_info.pVertexAttributeDescriptions = vi_attribs;

    VkPipelineInputAssemblyStateCreateInfo ia_state_info = {};
    ia_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_state_info.primitiveRestartEnable = VK_FALSE;
    ia_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo rs_state_info = {};
    rs_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rs_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rs_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rs_state_info.depthClampEnable = VK_TRUE;
    rs_state_info.rasterizerDiscardEnable = VK_FALSE;
    rs_state_info.depthBiasEnable = VK_FALSE;
    rs_state_info.depthBiasConstantFactor = 0;
    rs_state_info.depthBiasClamp = 0;
    rs_state_info.depthBiasSlopeFactor = 0;
    rs_state_info.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState cb_att_state[1] = {};
    cb_att_state[0].colorWriteMask = 0xf;
    cb_att_state[0].blendEnable = VK_FALSE;
    cb_att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
    cb_att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
    cb_att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    cb_att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    cb_att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cb_att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    VkPipelineColorBlendStateCreateInfo cb_state_info = {};
    cb_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb_state_info.attachmentCount = 1;
    cb_state_info.pAttachments = cb_att_state;
    cb_state_info.logicOpEnable = VK_FALSE;
    cb_state_info.logicOp = VK_LOGIC_OP_NO_OP;
    cb_state_info.blendConstants[0] = 1.0f;
    cb_state_info.blendConstants[1] = 1.0f;
    cb_state_info.blendConstants[2] = 1.0f;
    cb_state_info.blendConstants[3] = 1.0f;

    VkPipelineViewportStateCreateInfo vp_state_info = {};
    vp_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_state_info.viewportCount = 1;
    vp_state_info.scissorCount = 1;

    VkPipelineDepthStencilStateCreateInfo ds_state_info = {};
    ds_state_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds_state_info.depthTestEnable = VK_TRUE;
    ds_state_info.depthWriteEnable = VK_TRUE;
    ds_state_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    ds_state_info.depthBoundsTestEnable = VK_FALSE;
    ds_state_info.minDepthBounds = 0;
    ds_state_info.maxDepthBounds = 0;
    ds_state_info.stencilTestEnable = VK_FALSE;
    ds_state_info.back.failOp = VK_STENCIL_OP_KEEP;
    ds_state_info.back.passOp = VK_STENCIL_OP_KEEP;
    ds_state_info.back.compareOp = VK_COMPARE_OP_ALWAYS;
    ds_state_info.back.compareMask = 0;
    ds_state_info.back.reference = 0;
    ds_state_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
    ds_state_info.back.writeMask = 0;
    ds_state_info.front = ds_state_info.back;

    VkPipelineMultisampleStateCreateInfo ms_state_info = {};
    ms_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    ms_state_info.sampleShadingEnable = VK_FALSE;
    ms_state_info.alphaToCoverageEnable = VK_FALSE;
    ms_state_info.alphaToOneEnable = VK_FALSE;
    ms_state_info.minSampleShading = 0.0;

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = 0;
    pipeline_info.pVertexInputState = &vi_state_info;
    pipeline_info.pInputAssemblyState = &ia_state_info;
    pipeline_info.pRasterizationState = &rs_state_info;
    pipeline_info.pColorBlendState = &cb_state_info;
    pipeline_info.pTessellationState = nullptr;
    pipeline_info.pMultisampleState = &ms_state_info;
    pipeline_info.pDynamicState = &dyn_state_info;
    pipeline_info.pViewportState = &vp_state_info;
    pipeline_info.pDepthStencilState = &ds_state_info;
    pipeline_info.pStages = shader_stages;
    pipeline_info.stageCount = 2;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;

    VkPipeline pipeline = {};
    FAIL_IF_NOT_SUCCESS(
        vkCreateGraphicsPipelines(device, 0, 1, &pipeline_info, nullptr, &pipeline),
        "CreateGraphicsPipelines");

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}
