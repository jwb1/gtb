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
#include "gtb/application.hpp"
#include "gtb/window.hpp"
#include "gtb/graphics.hpp"

namespace gtb {
    class application::impl {
        window m_window;
        graphics m_graphics;
    public:
        impl();
        ~impl();

        int run(int argc, char* argv[]);

        window* get_window() { return (&m_window); }
        graphics* get_graphics() { return (&m_graphics); }

        void tick();
        void draw();
        void resize(int w, int h);
        void key_in(char c);
    };

    application::impl::impl()
        : m_window(640, 480, "gtb",
            std::bind(&impl::tick, this),
            std::bind(&impl::draw, this),
            std::bind(&impl::resize, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&impl::key_in, this, std::placeholders::_1))
        , m_graphics(
            "gtb",
#if defined(GTB_ENABLE_VULKAN_DEBUG_LAYER)
            true
#else
            false
#endif
        )
    {}
    application::impl::~impl() = default;

    int application::impl::run(int argc, char* argv[])
    {
        return (m_window.run());
    }

    void application::impl::tick()
    {

    }

    void application::impl::draw()
    {

    }

    void application::impl::resize(int w, int h)
    {

    }

    void application::impl::key_in(char c)
    {

    }

    // static
    application* application::get()
    {
        static application app;
        return (&app);
    }

    application::application()
        : m_pimpl(std::make_unique<impl>())
    {}
    application::~application() = default;

    int application::run(int argc, char* argv[]) { return (m_pimpl->run(argc, argv)); }

    window* application::get_window() { return (m_pimpl->get_window()); }
    graphics* application::get_graphics() { return (m_pimpl->get_graphics()); }
}
