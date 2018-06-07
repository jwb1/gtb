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
#include "gtb/graphics.hpp"
#include "gtb/exception.hpp"

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>

namespace gtb {
    class glfw_dispatch_loader {
    public:
        VkResult vkCreateInstance(const VkInstanceCreateInfo* ci, const VkAllocationCallbacks* a, VkInstance* i) const
        {
            PFN_vkCreateInstance create_instance = reinterpret_cast<PFN_vkCreateInstance>(
                glfwGetInstanceProcAddress(nullptr, "vkCreateInstance"));
            return (create_instance(ci, a, i));
        }
        void vkDestroyInstance(VkInstance i, const VkAllocationCallbacks* a) const
        {
            PFN_vkDestroyInstance destroy_instance = reinterpret_cast<PFN_vkDestroyInstance>(
                glfwGetInstanceProcAddress(nullptr, "vkDestroyInstance"));
            destroy_instance(i, a);
        }

        VkResult vkCreateDebugReportCallbackEXT(VkInstance i, const VkDebugReportCallbackCreateInfoEXT* ci, const VkAllocationCallbacks* a, VkDebugReportCallbackEXT* c) const
        {
            PFN_vkCreateDebugReportCallbackEXT create_debug_report_callback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
                glfwGetInstanceProcAddress(nullptr, "vkCreateDebugReportCallbackEXT"));
            return (create_debug_report_callback(i, ci, a, c));
        }
        void vkDestroyDebugReportCallbackEXT(VkInstance i, VkDebugReportCallbackEXT c, const VkAllocationCallbacks* a) const
        {
            PFN_vkDestroyDebugReportCallbackEXT destroy_debug_report_callback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
                glfwGetInstanceProcAddress(nullptr, "vkDestroyDebugReportCallbackEXT"));
            destroy_debug_report_callback(i, c, a);
        }
    };

    class graphics::impl {
        vk::Instance m_instance;
        vk::DebugReportCallbackEXT m_debug_report_callback;
    public:
        impl(std::string application_name, bool enable_debug_layer);
        ~impl();

    private:
        static VkBool32 debug_report_stub(
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT object_type,
            uint64_t object,
            size_t location,
            int32_t message_code,
            const char* layer_prefix,
            const char* message,
            void* user_data);

        void debug_report(
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT object_type,
            uint64_t object,
            size_t location,
            int32_t message_code,
            const char* layer_prefix,
            const char* message);
    };

    graphics::impl::impl(std::string application_name, bool enable_debug_layer)
    {
        // Check for required instance layers.
        std::vector<const char*> required_layers;
        if (enable_debug_layer) {
            required_layers.push_back("VK_LAYER_LUNARG_standard_validation");
        }

        std::vector<vk::LayerProperties> supported_layers(vk::enumerateInstanceLayerProperties());
        size_t found_layers = 0;
        for (vk::LayerProperties& layer : supported_layers) {
            std::string layer_name(layer.layerName);
            found_layers += std::count(required_layers.begin(), required_layers.end(), layer_name);
        }
        if (found_layers != required_layers.size()) {
            // !TODO! throw
        }

        // Check for required instance extensions, including the glfw ones.
        std::vector<const char*> required_extensions;

        uint32_t glfw_extensions_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
        required_extensions.assign(glfw_extensions, glfw_extensions + glfw_extensions_count);

        if (enable_debug_layer) {
            required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        std::vector<vk::ExtensionProperties> supported_extensions(vk::enumerateInstanceExtensionProperties());
        size_t found_extensions = 0;
        for (vk::ExtensionProperties& extension : supported_extensions) {
            std::string extension_name(extension.extensionName);
            found_extensions += std::count(required_extensions.begin(), required_extensions.end(), extension_name);
        }
        if (found_extensions != required_extensions.size()) {
            // !TODO throw
        }

        // Now, to start calling vulkan, we first use glfw to dispatch until we can create
        // the real dispatch loader (right after the device creation.)
        glfw_dispatch_loader glfw_dispatch;

        // Create the vulkan instance.
        vk::ApplicationInfo application_info;
        application_info.pApplicationName = application_name.c_str();
        application_info.apiVersion = VK_API_VERSION_1_1;

        vk::InstanceCreateInfo instance_create_info;
        instance_create_info.pApplicationInfo = &application_info;
        instance_create_info.enabledLayerCount = static_cast<uint32_t>(required_layers.size());
        instance_create_info.ppEnabledLayerNames = required_layers.data();
        instance_create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
        instance_create_info.ppEnabledExtensionNames = required_extensions.data();

        m_instance = vk::createInstance(instance_create_info, nullptr, glfw_dispatch);

        // Register the debug callback.
        if (enable_debug_layer) {
            vk::DebugReportCallbackCreateInfoEXT debug_report_create_info;
            debug_report_create_info.flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eWarning;
            debug_report_create_info.pfnCallback = &graphics::impl::debug_report_stub;
            debug_report_create_info.pUserData = this;

            m_debug_report_callback = m_instance.createDebugReportCallbackEXT(debug_report_create_info, nullptr, glfw_dispatch);
        }
    }

    graphics::impl::~impl() = default;

    // static
    VkBool32 graphics::impl::debug_report_stub(
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT object_type,
            uint64_t object,
            size_t location,
            int32_t message_code,
            const char* layer_prefix,
            const char* message,
            void* user_data)
    {
        graphics::impl* impl = static_cast<graphics::impl*>(user_data);
        impl->debug_report(flags, object_type, object, location, message_code, layer_prefix, message);
        return (VK_FALSE);
    }

    void graphics::impl::debug_report(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT object_type,
        uint64_t object,
        size_t location,
        int32_t message_code,
        const char* layer_prefix,
        const char* message)
    {
        // TODO: throw maybe?
    }

    graphics::graphics(std::string application_name, bool enable_debug_layer)
        : m_pimpl(std::make_unique<impl>(application_name, enable_debug_layer))
    {}
    graphics::~graphics() = default;
}
