#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include <chrono>

#include "SmartWatchApp.hpp"

static const int TARGET_FPS = 75;
static const double FRAME_TIME = 1.0 / TARGET_FPS;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW init failed!\n";
        return -1;
    }

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    int screenWidth  = mode->width;
    int screenHeight = mode->height;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Pametni Sat", primary, nullptr);
    if (!window) {
        std::cerr << "Window creation failed!\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed!\n";
        glfwTerminate();
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SmartWatchApp app;
    if (!app.init(window, screenWidth, screenHeight)) {
        glfwTerminate();
        return -1;
    }

    glfwSetWindowUserPointer(window, &app);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    while (!glfwWindowShouldClose(window)) {
        double frameStart = glfwGetTime();

        glfwPollEvents();

        double currentTime = glfwGetTime();
        app.update(currentTime);
        app.render();

        glfwSwapBuffers(window);

        // Frame limiter
        double frameEnd = glfwGetTime();
        double elapsed = frameEnd - frameStart;
        double toWait  = FRAME_TIME - elapsed;

        if (toWait > 0.0) {
            auto sleepMs = static_cast<int>((toWait - 0.001) * 1000.0);
            if (sleepMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            }
            while ((glfwGetTime() - frameStart) < FRAME_TIME) {
                // busy wait
            }
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* app = static_cast<SmartWatchApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->onKey(key, scancode, action, mods);
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto* app = static_cast<SmartWatchApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->onMouseButton(button, action, mods);
    }
}