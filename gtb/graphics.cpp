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
#include "gtb/application.hpp"
#include "gtb/window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace gtb {
    // To initialize vulkan, we first use glfw to dispatch until we can create
    // the real dispatch loader (right after the device creation.)
    class glfw_dispatch_loader {
        VkInstance m_instance;
    public:
        glfw_dispatch_loader() : m_instance(nullptr) {}
        explicit glfw_dispatch_loader(VkInstance instance) : m_instance(instance) {}

        VkResult vkEnumerateInstanceLayerProperties(uint32_t* c, VkLayerProperties* p) const
        {
            PFN_vkEnumerateInstanceLayerProperties enumerate_instance_layer_properties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
                glfwGetInstanceProcAddress(m_instance, "vkEnumerateInstanceLayerProperties"));
            return (enumerate_instance_layer_properties(c, p));
        }
        VkResult vkEnumerateInstanceExtensionProperties(const char* l, uint32_t* c, VkExtensionProperties* p) const
        {
            PFN_vkEnumerateInstanceExtensionProperties enumerate_instance_extension_properties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
                glfwGetInstanceProcAddress(m_instance, "vkEnumerateInstanceExtensionProperties"));
            return (enumerate_instance_extension_properties(l, c, p));
        }

        VkResult vkCreateInstance(const VkInstanceCreateInfo* ci, const VkAllocationCallbacks* a, VkInstance* i) const
        {
            PFN_vkCreateInstance create_instance = reinterpret_cast<PFN_vkCreateInstance>(
                glfwGetInstanceProcAddress(m_instance, "vkCreateInstance"));
            return (create_instance(ci, a, i));
        }
        void vkDestroyInstance(VkInstance i, const VkAllocationCallbacks* a) const
        {
            PFN_vkDestroyInstance destroy_instance = reinterpret_cast<PFN_vkDestroyInstance>(
                glfwGetInstanceProcAddress(m_instance, "vkDestroyInstance"));
            destroy_instance(i, a);
        }

        VkResult vkCreateDebugReportCallbackEXT(VkInstance i, const VkDebugReportCallbackCreateInfoEXT* ci, const VkAllocationCallbacks* a, VkDebugReportCallbackEXT* c) const
        {
            PFN_vkCreateDebugReportCallbackEXT create_debug_report_callback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
                glfwGetInstanceProcAddress(m_instance, "vkCreateDebugReportCallbackEXT"));
            return (create_debug_report_callback(i, ci, a, c));
        }
        void vkDestroyDebugReportCallbackEXT(VkInstance i, VkDebugReportCallbackEXT c, const VkAllocationCallbacks* a) const
        {
            PFN_vkDestroyDebugReportCallbackEXT destroy_debug_report_callback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
                glfwGetInstanceProcAddress(m_instance, "vkDestroyDebugReportCallbackEXT"));
            destroy_debug_report_callback(i, c, a);
        }

        VkResult vkEnumeratePhysicalDevices(VkInstance i, uint32_t* c, VkPhysicalDevice* pd) const
        {
            PFN_vkEnumeratePhysicalDevices enumerate_physical_devices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
                glfwGetInstanceProcAddress(m_instance, "vkEnumeratePhysicalDevices"));
            return (enumerate_physical_devices(i, c, pd));
        }
        void vkGetPhysicalDeviceProperties(VkPhysicalDevice pd, VkPhysicalDeviceProperties* p) const
        {
            PFN_vkGetPhysicalDeviceProperties get_physical_device_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
                glfwGetInstanceProcAddress(m_instance, "vkGetPhysicalDeviceProperties"));
            get_physical_device_properties(pd, p);
        }
        void vkGetPhysicalDeviceFeatures(VkPhysicalDevice pd, VkPhysicalDeviceFeatures* f) const
        {
            PFN_vkGetPhysicalDeviceFeatures get_physical_device_features = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(
                glfwGetInstanceProcAddress(m_instance, "vkGetPhysicalDeviceFeatures"));
            get_physical_device_features(pd, f);
        }
        void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice pd, uint32_t* c, VkQueueFamilyProperties* qfp) const
        {
            PFN_vkGetPhysicalDeviceQueueFamilyProperties get_physical_device_queue_family_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
                glfwGetInstanceProcAddress(m_instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
            get_physical_device_queue_family_properties(pd, c, qfp);
        }
        VkResult vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice pd, uint32_t qf, VkSurfaceKHR s, VkBool32* b) const
        {
            PFN_vkGetPhysicalDeviceSurfaceSupportKHR get_physical_device_surface_support = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
                glfwGetInstanceProcAddress(m_instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
            return (get_physical_device_surface_support(pd, qf, s, b));
        }
        VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice pd, VkSurfaceKHR s, uint32_t* c, VkSurfaceFormatKHR* sf) const
        {
            PFN_vkGetPhysicalDeviceSurfaceFormatsKHR get_physical_device_surface_formats = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
                glfwGetInstanceProcAddress(m_instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
            return (get_physical_device_surface_formats(pd, s, c, sf));
        }
        VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice pd, VkSurfaceKHR s, uint32_t* c, VkPresentModeKHR* pm) const
        {
            PFN_vkGetPhysicalDeviceSurfacePresentModesKHR get_physical_device_surface_present_modes = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
                glfwGetInstanceProcAddress(m_instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
            return (get_physical_device_surface_present_modes(pd, s, c, pm));
        }
        VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice pd, const char* l, uint32_t* c, VkExtensionProperties* p) const
        {
            PFN_vkEnumerateDeviceExtensionProperties enumerate_device_extension_properties = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
                glfwGetInstanceProcAddress(m_instance, "vkEnumerateDeviceExtensionProperties"));
            return (enumerate_device_extension_properties(pd, l, c, p));
        }

        VkResult vkCreateDevice(VkPhysicalDevice pd, const VkDeviceCreateInfo* ci, const VkAllocationCallbacks* a, VkDevice* d) const
        {
            PFN_vkCreateDevice create_device = reinterpret_cast<PFN_vkCreateDevice>(
                glfwGetInstanceProcAddress(m_instance, "vkCreateDevice"));
            return (create_device(pd, ci, a, d));
        }
        void vkDestroyDevice(VkDevice d, const VkAllocationCallbacks* a) const
        {
            PFN_vkDestroyDevice destroy_device = reinterpret_cast<PFN_vkDestroyDevice>(
                glfwGetInstanceProcAddress(m_instance, "vkDestroyDevice"));
            destroy_device(d, a);
        }

        void vkDestroySurfaceKHR(VkInstance i, VkSurfaceKHR s, const VkAllocationCallbacks* a) const
        {
            PFN_vkDestroySurfaceKHR destroy_surface = reinterpret_cast<PFN_vkDestroySurfaceKHR>(
                glfwGetInstanceProcAddress(m_instance, "vkDestroySurfaceKHR"));
            destroy_surface(i, s, a);
        }
    };

    class graphics::impl {
        vk::Instance m_instance;
        vk::DebugReportCallbackEXT m_debug_report_callback;
        vk::PhysicalDevice m_physical_device;
        uint32_t m_queue_index;
        vk::Device m_device;
        vk::DispatchLoaderDynamic m_dispatch;
        vk::SurfaceKHR m_surface;
        vk::SwapChainKHR m_swap_chain;
    public:
        impl(std::string application_name, bool enable_debug_layer, window* w);
        ~impl();

    private:
        void create_instance(
            std::string application_name,
            const std::vector<const char*>& required_layers,
            const std::vector<const char*>& required_extensions);

        void create_device_objects(
            bool enable_debug_layer,
            window* w,
            const std::vector<const char*>& required_layers,
            const std::vector<const char*>& required_extensions);

        void create_swap_chain();

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

    graphics::impl::impl(std::string application_name, bool enable_debug_layer, window* w)
    {
        // Build lists of required layers and extensions.
        std::vector<const char*> required_layers;
        if (enable_debug_layer) {
            required_layers.push_back("VK_LAYER_LUNARG_standard_validation");
        }

        std::vector<const char*> required_instance_extensions;

        uint32_t glfw_extensions_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
        required_instance_extensions.assign(glfw_extensions, glfw_extensions + glfw_extensions_count);

        if (enable_debug_layer) {
            required_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        std::vector<const char*> required_device_extensions;
        required_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // Create Vulkan objects.
        create_instance(application_name, required_layers, required_instance_extensions);
        create_device_objects(enable_debug_layer, w, required_layers, required_device_extensions);
        create_swap_chain();
    }

    graphics::impl::~impl()
    {
        // Destroy things with the same dispatch they were created with. Might not matter?
        if (m_swap_chain) {
            m_device.destroySwapChainKHR(m_swap_chain, m_dispatch);
        }

        glfw_dispatch_loader d(m_instance);

        if (m_device) {
            m_device.destroy(nullptr, d);
        }

        if (m_surface) {
            m_instance.destroySurfaceKHR(m_surface, nullptr, d);
        }

        if (m_debug_report_callback) {
            m_instance.destroyDebugReportCallbackEXT(m_debug_report_callback, nullptr, d);
        }

        if (m_instance) {
            m_instance.destroy(nullptr, d);
        }
    }

    void graphics::impl::create_instance(
        std::string application_name,
        const std::vector<const char*>& required_layers,
        const std::vector<const char*>& required_extensions)
    {
        glfw_dispatch_loader d;

        // Check for required instance layers.
        std::vector<vk::LayerProperties> supported_layers(vk::enumerateInstanceLayerProperties(d));
        size_t found_layers = 0;
        for (vk::LayerProperties& layer : supported_layers) {
            std::string layer_name(layer.layerName);
            found_layers += std::count(required_layers.begin(), required_layers.end(), layer_name);
        }
        if (found_layers != required_layers.size()) {
            BOOST_THROW_EXCEPTION(capability_exception()
                << errinfo_capability_description("Not all required vulkan instance layers found."));
        }

        // Check for required instance extensions.
        std::vector<vk::ExtensionProperties> supported_extensions(vk::enumerateInstanceExtensionProperties(nullptr, d));
        size_t found_extensions = 0;
        for (vk::ExtensionProperties& extension : supported_extensions) {
            std::string extension_name(extension.extensionName);
            found_extensions += std::count(required_extensions.begin(), required_extensions.end(), extension_name);
        }
        if (found_extensions != required_extensions.size()) {
            BOOST_THROW_EXCEPTION(capability_exception()
                << errinfo_capability_description("Not all required vulkan instance extensions found."));
        }

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

        m_instance = vk::createInstance(instance_create_info, nullptr, d);
    }

    void graphics::impl::create_device_objects(
        bool enable_debug_layer,
        window* w,
        const std::vector<const char*>& required_layers,
        const std::vector<const char*>& required_extensions)
    {
        glfw_dispatch_loader d(m_instance);

        // Register the debug callback.
        if (enable_debug_layer) {
            vk::DebugReportCallbackCreateInfoEXT debug_report_create_info;
            debug_report_create_info.flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eWarning;
            debug_report_create_info.pfnCallback = &graphics::impl::debug_report_stub;
            debug_report_create_info.pUserData = this;

            m_debug_report_callback = m_instance.createDebugReportCallbackEXT(debug_report_create_info, nullptr, d);
        }

        // Hook to the window via a surface.
        m_surface = w->create_vk_surface(m_instance);

        // Find a physical device and queue family index.
        vk::PhysicalDevice found_physical_device;
        uint32_t qf;

        // Enumerate all installed physical devices.
        std::vector<vk::PhysicalDevice> physical_devices = m_instance.enumeratePhysicalDevices(d);
        if (physical_devices.empty()) {
            BOOST_THROW_EXCEPTION(capability_exception()
                << errinfo_capability_description("No Vulkan physical devices enumerated."));
        }

        // Store details about each physical device enumerated here.
        std::vector<vk::QueueFamilyProperties> queue_family_properties;
        std::vector<vk::ExtensionProperties> device_extensions;
        std::vector<vk::SurfaceFormatKHR> surface_formats;
        std::vector<vk::PresentModeKHR> present_modes;

        for (vk::PhysicalDevice& physical_device : physical_devices) {
            // See if the physical device supports the required extensions.
            device_extensions = physical_device.enumerateDeviceExtensionProperties(nullptr, d);
            if (device_extensions.empty()) {
                continue;
            }

            size_t found_extensions = 0;
            for (vk::ExtensionProperties& extension : device_extensions) {
                std::string extension_name(extension.extensionName);
                found_extensions += std::count(required_extensions.begin(), required_extensions.end(), extension_name);
            }
            if (found_extensions != required_extensions.size()) {
                continue;
            }

            // See if there is a queue family on the selected physical device that meets our requirements.
            queue_family_properties = physical_device.getQueueFamilyProperties(d);
            if (queue_family_properties.empty()) {
                continue;
            }

            const vk::QueueFlags graphics_and_compute = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;
            for (qf = 0; qf < queue_family_properties.size(); ++qf) {
                bool surface_support = physical_device.getSurfaceSupportKHR(qf, m_surface, d);
                bool has_graphics_and_compute = ((queue_family_properties[qf].queueFlags & graphics_and_compute) == graphics_and_compute);

                if (has_graphics_and_compute && surface_support) {
                    break;
                }
            }
            if (qf == queue_family_properties.size()) {
                continue;
            }

            // See if the physical device is compatible with the window surface.
            // Looking for BGRA 8888 UNORM / SRGB
            surface_formats = physical_device.getSurfaceFormatsKHR(m_surface, d);
            if (surface_formats.empty()) {
                continue;
            }

            bool found_surface_format = ((surface_formats.size() == 1) && (surface_formats[0].format == vk::Format::eUndefined));
            if (!found_surface_format) {
                for (vk::SurfaceFormatKHR& surface_format : surface_formats) {
                    if ((surface_format.format == vk::Format::eB8G8R8A8Unorm) &&
                        (surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)) {
                        found_surface_format = true;
                        break;
                    }
                }
            }
            if (!found_surface_format) {
                continue;
            }

            // Looking for mailbox presentation for triple buffering.
            present_modes = physical_device.getSurfacePresentModesKHR(m_surface, d);
            if (present_modes.empty()) {
                continue;
            }

            bool found_present_mode = false;
            for (vk::PresentModeKHR& present_mode : present_modes) {
                if (present_mode == vk::PresentModeKHR::eMailbox) {
                    found_present_mode = true;
                    break;
                }
            }
            if (!found_present_mode) {
                continue;
            }

            // This physical device could work.
            found_physical_device = physical_device;

            // At this point, this physical device meets all of our checks. We culd use it. If
            // it is ALSO a discreet GPU, we quit searching now.
            vk::PhysicalDeviceProperties device_props = physical_device.getProperties(d);
            if (device_props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                break;
            }
        }

        if (found_physical_device) {
            m_physical_device = found_physical_device;
            m_queue_index = qf;
        }
        else {
            BOOST_THROW_EXCEPTION(capability_exception()
                << errinfo_capability_description("No Vulkan physical devices meets requirements."));
        }

        // Create the device.
        float queue_priority = 1.0f; // Priority is not important when there is only a single queue.
        vk::DeviceQueueCreateInfo queue_create_info;
        queue_create_info.queueFamilyIndex = m_queue_index;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        vk::DeviceCreateInfo device_create_info;
        device_create_info.queueCreateInfoCount = 1;
        device_create_info.pQueueCreateInfos = &queue_create_info;
        device_create_info.enabledLayerCount = static_cast<uint32_t>(required_layers.size());
        device_create_info.ppEnabledLayerNames = required_layers.data();
        device_create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
        device_create_info.ppEnabledExtensionNames = required_extensions.data();

        m_device = m_physical_device.createDevice(device_create_info, nullptr, d);

        // Init the dynamic dispatch. All future Vulkan functions will be called through this.
        m_dispatch.init(m_instance, m_device);
    }

    void graphics::impl::create_swap_chain()
    {
        vk::SurfaceCapabilitiesKHR surface_capabilities = m_physical_device.getSurfaceCapabilitiesKHR(m_surface, m_dispatch);

        vk::SwapchainCreateInfoKHR swap_chain_create_info;
        swap_chain_create_info.surface = m_surface;
        swap_chain_create_info.minImageCount = 3;
        swap_chain_create_info.imageFormat = vk::Format::eB8G8R8A8Unorm;
        swap_chain_create_info.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        swap_chain_create_info.imageExtent = surface_capabilities.currentExtent;
        swap_chain_create_info.imageArrayLayers = 1;
        swap_chain_create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        swap_chain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
        swap_chain_create_info.preTransform = swap_chain_create_info.currentTransform;
        swap_chain_create_info.presentMode = vk::PresentModeKHR::eMailbox;
        swap_chain_create_info.clipped = VK_TRUE;

        m_swap_chain = m_device.createSwapChainKHR(swap_chain_create_info, nullptr, m_dispatch);
    }

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
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            BOOST_THROW_EXCEPTION(vk_debug_report_error()
                << errinfo_vk_debug_report_error_object_type(vk::to_string(vk::ObjectType(object_type)).c_str())
                << errinfo_vk_debug_report_error_object(object)
                << errinfo_vk_debug_report_error_location(location)
                << errinfo_vk_debug_report_error_message_code(message_code)
                << errinfo_vk_debug_report_error_layer_prefix(layer_prefix)
                << errinfo_vk_debug_report_error_message(message));
        }
        // TODO: How to deal with other debug report types?
    }

    graphics::graphics(std::string application_name, bool enable_debug_layer, window* w)
        : m_pimpl(std::make_unique<impl>(application_name, enable_debug_layer, w))
    {}
    graphics::~graphics() = default;
}
