//  gtb: Graphics Test Bench
//  Copyright 2018 Joshua Buckman
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#include <cstdio>
#include <cstdlib>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static void refresh_callback(GLFWwindow* window)
{
    /*
    app* a = glfwGetWindowUserPointer(window);
    a->draw();
    */
}

static void resize_callback(GLFWwindow* window, int width, int height)
{
    /*
    app* a = glfwGetWindowUserPointer(window);
    a->resize(width, height);
    */
}

int main(void)
{
    GLFWwindow* window = nullptr;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        fprintf(stderr, "Cannot initialize GLFW.\nExiting ...\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    if (!glfwVulkanSupported()) {
        glfwTerminate();
        fprintf(stderr, "GLFW failed to find the Vulkan loader.\nExiting ...\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(640, 480, "gtb", NULL, NULL);
    if (!window) {
        glfwTerminate();
        fprintf(stderr, "GLFW failed to create a window.\nExiting ...\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    //app* a;
    //glfwSetWindowUserPointer(window, a);
    glfwSetWindowRefreshCallback(window, refresh_callback);
    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwSetKeyCallback(window, key_callback);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        //a->tick();
        //a->draw();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
