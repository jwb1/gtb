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
#include "gtb/pch.hpp"
#include "gtb/window.hpp"
#include "gtb/exception.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace gtb {
    class window::impl {
        // Callbacks
        std::function<void()> m_tick;
        std::function<void()> m_draw;
        std::function<void(char)> m_key_in;
        // GLFW
        GLFWwindow* m_window;
    public:
        impl(int w, int h, std::string title,
            std::function<void()> tick,
            std::function<void()> draw,
            std::function<void(char)> key_in);
        ~impl();

        int run();

        vk::SurfaceKHR create_vk_surface(vk::Instance instance) const;
    private:
        // GLFW uses C-style callbacks
        static void error_callback(int error, const char* description);
        static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void refresh_callback(GLFWwindow* window);
    };

    window::impl::impl(int w, int h, std::string title,
        std::function<void()> tick,
        std::function<void()> draw,
        std::function<void(char)> key_in)
        : m_tick(std::move(tick))
        , m_draw(std::move(draw))
        , m_key_in(std::move(key_in))
        , m_window(nullptr)
    {
        glfwSetErrorCallback(error_callback);

        if (!glfwInit()) {
            BOOST_THROW_EXCEPTION(glfw_exception()
                << errinfo_glfw_failed_function("glfwInit"));
        }

        if (!glfwVulkanSupported()) {
            BOOST_THROW_EXCEPTION(capability_exception()
                << errinfo_capability_description("Could not find vulkan runtime or driver."));
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
        if (!m_window) {
            BOOST_THROW_EXCEPTION(glfw_exception()
                << errinfo_glfw_failed_function("glfwCreateWindow"));
        }

        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowRefreshCallback(m_window, refresh_callback);
        glfwSetKeyCallback(m_window, key_callback);
    }

    window::impl::~impl()
    {
        if (m_window) {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
    }

    int window::impl::run()
    {
        while (!glfwWindowShouldClose(m_window)) {
            m_tick();
            glfwPollEvents();
            m_draw();
        }
        return (EXIT_SUCCESS);
    }

    vk::SurfaceKHR window::impl::create_vk_surface(vk::Instance instance) const
    {
        VkSurfaceKHR surface; // Create C++ wrapper on way out of this function.
        if (glfwCreateWindowSurface(instance, m_window, nullptr, &surface) != VK_SUCCESS) {
            BOOST_THROW_EXCEPTION(glfw_exception()
                << errinfo_glfw_failed_function("glfwCreateWindowSurface"));
        }
        return (surface);
    }

    // static
    void window::impl::error_callback(int error, const char* description)
    {
        BOOST_THROW_EXCEPTION(glfw_exception()
            << errinfo_glfw_error_callback_error(error)
            << errinfo_glfw_error_callback_description(description));
    }

    // static
    void window::impl::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        /* TODO
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        */
    }

    // static
    void window::impl::refresh_callback(GLFWwindow* window)
    {
        window::impl* pimpl = static_cast<window::impl*>(glfwGetWindowUserPointer(window));
        pimpl->m_draw();
    }

    window::window(int w, int h, std::string title,
        std::function<void()> tick,
        std::function<void()> draw,
        std::function<void(char)> key_in)
        : m_pimpl(std::make_unique<impl>(w, h, title, tick, draw, key_in))
    {}
    window::~window() = default;

    int window::run() { return (m_pimpl->run()); }
    vk::SurfaceKHR window::create_vk_surface(vk::Instance instance) const { return (m_pimpl->create_vk_surface(instance)); }
}
