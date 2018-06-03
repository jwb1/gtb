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
#pragma once
namespace gtb {
    class window {
        class impl;
        std::experimental::propagate_const<std::unique_ptr<impl>> m_pimpl;
    public:
        window(int w, int h, std::string title,
            std::function<void()> tick,
            std::function<void()> draw,
            std::function<void(int, int)> resize,
            std::function<void(char)> key_in);
        ~window();

        int run();
    private:
        window(window&&) = delete;
        window(const window&) = delete;
        window& operator=(window&&) = delete;
        window& operator=(const window&) = delete;
    };
}
