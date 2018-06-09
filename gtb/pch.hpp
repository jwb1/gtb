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
// C++ Standard Library
#include <exception>
#include <memory>
#include <functional>
#include <utility>
#include <algorithm>
#include <experimental/propagate_const>
#include <string>
#include <vector>
// Boost
#include <boost/exception/all.hpp>
// Vulkan
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>
