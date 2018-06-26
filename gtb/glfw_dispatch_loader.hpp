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
        VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR( VkPhysicalDevice pd, VkSurfaceKHR s, VkSurfaceCapabilitiesKHR* cap) const
        {
            PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR get_physical_device_surface_capabilities = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
                glfwGetInstanceProcAddress(m_instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
            return (get_physical_device_surface_capabilities(pd, s, cap));
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
}
