#pragma once

#include <switch.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace gl {

static inline GLFWwindow *init_glfw(int width, int height) {
    if (!glfwInit())
        return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    auto *window = glfwCreateWindow(width, height, "", NULL, NULL);

    if (window) {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync
    }

    return window;
}

static inline void exit_glfw(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

static inline Result init_glad() {
    return !gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
}

} // namespace gl
