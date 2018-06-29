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

// C++ Standard Library
#include <cstdlib>
#include <exception>
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>

// Boost
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>

// Vulkan
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>

// GLFW
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Local helper classes
#include "gtb/glfw_dispatch_loader.hpp"

namespace gtb {
    namespace error {
        // Base class for gtb internal error handling.
        class exception : public virtual std::exception, public virtual boost::exception {};

        // GLFW function failures or error callbacks throw these.
        class glfw_exception : public exception {};

        typedef boost::error_info<struct errinfo_glfw_failed_function_, const char*> errinfo_glfw_failed_function;
        typedef boost::error_info<struct errinfo_glfw_error_callback_error_, int> errinfo_glfw_error_callback_error;
        typedef boost::error_info<struct errinfo_glfw_error_callback_description_, const char*> errinfo_glfw_error_callback_description;

        // System capability failures throw these.
        class capability_exception : public exception {};

        typedef boost::error_info<struct errinfo_capability_description_, const char*> errinfo_capability_description;

        // Vulkan debug reports with the error bit set throw these.
        class vk_debug_report_error : public exception {};

        typedef boost::error_info<struct errinfo_vk_debug_report_error_object_type_, const char*> errinfo_vk_debug_report_error_object_type;
        typedef boost::error_info<struct errinfo_vk_debug_report_error_object_, uint64_t> errinfo_vk_debug_report_error_object;
        typedef boost::error_info<struct errinfo_vk_debug_report_error_location_, size_t> errinfo_vk_debug_report_error_location;
        typedef boost::error_info<struct errinfo_vk_debug_report_error_message_code_, int32_t> errinfo_vk_debug_report_error_message_code;
        typedef boost::error_info<struct errinfo_vk_debug_report_error_layer_prefix_, const char*> errinfo_vk_debug_report_error_layer_prefix;
        typedef boost::error_info<struct errinfo_vk_debug_report_error_message_, const char*> errinfo_vk_debug_report_error_message;

        // File i/o failures throw these.
        class file_exception : public exception {};

        typedef boost::error_info<struct errinfo_file_exception_file_, const char*> errinfo_file_exception_file;

        // Exception handlers; only intended to be called in catch blocks!
        void open_exception_log_stream(std::ofstream& log_stream)
        {
            boost::filesystem::path log_file_path(std::getenv("LOCALAPPDATA"));
            log_file_path /= "gtb";
            boost::filesystem::create_directories(log_file_path);

            log_file_path /= "exception.log";
            log_stream.open(log_file_path.string(), std::ios_base::out | std::ios_base::trunc);
        }

        int handle_exception(const gtb::error::exception& e)
        {
            std::ofstream log_stream;
            open_exception_log_stream(log_stream);
            if (log_stream.is_open()) {
                log_stream
                    << "Exception caught!" << std::endl
                    << "Type: gtb::error:exception" << std::endl
                    << boost::diagnostic_information(e);
                log_stream.close();
            }
            return (EXIT_FAILURE);
        }

        int handle_exception(const boost::exception& e)
        {
            std::ofstream log_stream;
            open_exception_log_stream(log_stream);
            if (log_stream.is_open()) {
                log_stream
                    << "Exception caught!" << std::endl
                    << "Type: boost::exception" << std::endl
                    << boost::diagnostic_information(e);
                log_stream.close();
            }
            return (EXIT_FAILURE);
        }

        int handle_exception(const vk::Error& e)
        {
            std::ofstream log_stream;
            open_exception_log_stream(log_stream);
            if (log_stream.is_open()) {
                log_stream
                    << "Exception caught!" << std::endl
                    << "Type: vk::Error" << std::endl
                    << "What: " << e.what() << std::endl;
                log_stream.close();
            }
            return (EXIT_FAILURE);
        }

        int handle_exception(const std::exception& e)
        {
            std::ofstream log_stream;
            open_exception_log_stream(log_stream);
            if (log_stream.is_open()) {
                log_stream
                    << "Exception caught!" << std::endl
                    << "Type: std::exception" << std::endl
                    << "What: " << e.what() << std::endl;
                log_stream.close();
            }
            return (EXIT_FAILURE);
        }

        int handle_exception()
        {
            std::ofstream log_stream;
            open_exception_log_stream(log_stream);
            if (log_stream.is_open()) {
                log_stream
                    << "Exception caught!" << std::endl
                    << "Type: *unknown*" << std::endl;
                log_stream.close();
            }
            return (EXIT_FAILURE);
        }
    }

    class application {
        // GLFW
        GLFWwindow* m_window;

        // Vulkan
        vk::Instance m_instance;
        vk::DebugReportCallbackEXT m_debug_report_callback;
        vk::PhysicalDevice m_physical_device;
        vk::Device m_device;
        vk::Queue m_queue;
        vk::DispatchLoaderDynamic m_dispatch;
        vk::SurfaceKHR m_surface;
        vk::Format m_swap_chain_format;
        vk::Extent2D m_swap_chain_extent;
        vk::SwapchainKHR m_swap_chain;
        std::vector<vk::Image> m_swap_chain_images;
        std::vector<vk::ImageView> m_swap_chain_views;

        // Shaders
        vk::ShaderModule m_simple_vert;
        vk::ShaderModule m_simple_frag;

        // Render pass and targets
        vk::RenderPass m_simple_render_pass;
        std::vector<vk::Framebuffer> m_simple_framebuffers;
    public:
        static application* get();

        int run(int argc, char* argv[]);

    private:
        application();
        ~application();

        void tick();
        void draw();

        // GLFW
        void glfw_init();
        void glfw_cleanup();

        // GLFW uses C-style callbacks
        static void glfw_error_callback(int error, const char* description);
        static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void glfw_refresh_callback(GLFWwindow* window);

        // Vulkan
        void vk_init();
        void vk_cleanup();

        void vk_create_instance(
            const std::vector<const char*>& required_layers,
            const std::vector<const char*>& required_extensions);

        void vk_create_device_objects(
            const std::vector<const char*>& required_layers,
            const std::vector<const char*>& required_extensions);

        void vk_create_swap_chain();

        static VkBool32 vk_debug_report(
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT object_type,
            uint64_t object,
            size_t location,
            int32_t message_code,
            const char* layer_prefix,
            const char* message,
            void* user_data);

        // Shaders
        void shaders_init();
        void shaders_cleanup();

        // Render pass and targets
        void render_pass_init();
        void render_pass_cleanup();

        // Disallow some C++ operations.
        application(application&&) = delete;
        application(const application&) = delete;
        application& operator=(application&&) = delete;
        application& operator=(const application&) = delete;
    };

    // static
    application* application::get()
    {
        static application app;
        return (&app);
    }

    application::application()
        : m_window(nullptr)
    {
        glfw_init();
        vk_init();
        shaders_init();
        render_pass_init();
    }

    application::~application()
    {
        render_pass_init();
        shaders_cleanup();
        vk_cleanup();
        glfw_cleanup();
    }

    void application::glfw_init()
    {
        glfwSetErrorCallback(glfw_error_callback);

        if (!glfwInit()) {
            BOOST_THROW_EXCEPTION(error::glfw_exception()
                << error::errinfo_glfw_failed_function("glfwInit"));
        }

        if (!glfwVulkanSupported()) {
            BOOST_THROW_EXCEPTION(error::capability_exception()
                << error::errinfo_capability_description("Could not find vulkan runtime or driver."));
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(1024, 768, "gtb", nullptr, nullptr);
        if (!m_window) {
            BOOST_THROW_EXCEPTION(error::glfw_exception()
                << error::errinfo_glfw_failed_function("glfwCreateWindow"));
        }

        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowRefreshCallback(m_window, glfw_refresh_callback);
        glfwSetKeyCallback(m_window, glfw_key_callback);
    }

    void application::glfw_cleanup()
    {
        if (m_window) {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
    }

    void application::vk_init()
    {
        // Build lists of required layers and extensions.
        std::vector<const char*> required_layers;
#if defined(GTB_ENABLE_VULKAN_DEBUG_LAYER)
        required_layers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif

        std::vector<const char*> required_instance_extensions;

        uint32_t glfw_extensions_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
        required_instance_extensions.assign(glfw_extensions, glfw_extensions + glfw_extensions_count);

#if defined(GTB_ENABLE_VULKAN_DEBUG_LAYER)
        required_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

        std::vector<const char*> required_device_extensions;
        required_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // Create Vulkan objects.
        vk_create_instance(required_layers, required_instance_extensions);
        vk_create_device_objects(required_layers, required_device_extensions);
        vk_create_swap_chain();
    }

    void application::vk_cleanup()
    {
        for (vk::ImageView &view : m_swap_chain_views) {
            m_device.destroyImageView(view, nullptr, m_dispatch);
        }

        if (m_swap_chain) {
            m_device.destroySwapchainKHR(m_swap_chain, nullptr, m_dispatch);
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

    void application::vk_create_instance(
        const std::vector<const char*>& required_layers,
        const std::vector<const char*>& required_extensions)
    {
        glfw_dispatch_loader d;

        // Check for required instance layers.
        std::vector<vk::LayerProperties> supported_layers(vk::enumerateInstanceLayerProperties(d));
        size_t found_layers = 0;
        std::string layer_name;
        for (vk::LayerProperties& layer : supported_layers) {
            layer_name = layer.layerName;
            found_layers += std::count(required_layers.begin(), required_layers.end(), layer_name);
        }
        if (found_layers != required_layers.size()) {
            BOOST_THROW_EXCEPTION(error::capability_exception()
                << error::errinfo_capability_description("Not all required vulkan instance layers found."));
        }

        // Check for required instance extensions.
        std::vector<vk::ExtensionProperties> supported_extensions(vk::enumerateInstanceExtensionProperties(nullptr, d));
        size_t found_extensions = 0;
        std::string extension_name;
        for (vk::ExtensionProperties& extension : supported_extensions) {
            extension_name = extension.extensionName;
            found_extensions += std::count(required_extensions.begin(), required_extensions.end(), extension_name);
        }
        if (found_extensions != required_extensions.size()) {
            BOOST_THROW_EXCEPTION(error::capability_exception()
                << error::errinfo_capability_description("Not all required vulkan instance extensions found."));
        }

        // Create the vulkan instance.
        vk::ApplicationInfo application_info;
        application_info.pApplicationName = "gtb";
        application_info.apiVersion = VK_API_VERSION_1_1;

        vk::InstanceCreateInfo instance_create_info;
        instance_create_info.pApplicationInfo = &application_info;
        instance_create_info.enabledLayerCount = static_cast<uint32_t>(required_layers.size());
        instance_create_info.ppEnabledLayerNames = required_layers.data();
        instance_create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
        instance_create_info.ppEnabledExtensionNames = required_extensions.data();

        m_instance = vk::createInstance(instance_create_info, nullptr, d);
    }

    void application::vk_create_device_objects(
        const std::vector<const char*>& required_layers,
        const std::vector<const char*>& required_extensions)
    {
        glfw_dispatch_loader d(m_instance);

#if defined(GTB_ENABLE_VULKAN_DEBUG_LAYER)
        // Register the debug callback.
        vk::DebugReportCallbackCreateInfoEXT debug_report_create_info;
        debug_report_create_info.flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eWarning;
        debug_report_create_info.pfnCallback = &application::vk_debug_report;
        debug_report_create_info.pUserData = this;

        m_debug_report_callback = m_instance.createDebugReportCallbackEXT(debug_report_create_info, nullptr, d);
#endif

        // Hook to the window via a surface.
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &surface) != VK_SUCCESS) {
            BOOST_THROW_EXCEPTION(error::glfw_exception()
                << error::errinfo_glfw_failed_function("glfwCreateWindowSurface"));
        }
        m_surface = surface;

        // Find a physical device and queue family index.
        vk::PhysicalDevice found_physical_device;
        uint32_t queue_family_index;

        // Enumerate all installed physical devices.
        std::vector<vk::PhysicalDevice> physical_devices = m_instance.enumeratePhysicalDevices(d);
        if (physical_devices.empty()) {
            BOOST_THROW_EXCEPTION(error::capability_exception()
                << error::errinfo_capability_description("No Vulkan physical devices enumerated."));
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
            for (queue_family_index = 0; queue_family_index < queue_family_properties.size(); ++queue_family_index) {
                bool surface_support = physical_device.getSurfaceSupportKHR(queue_family_index, m_surface, d);
                bool has_graphics_and_compute = ((queue_family_properties[queue_family_index].queueFlags & graphics_and_compute) == graphics_and_compute);

                if (has_graphics_and_compute && surface_support) {
                    break;
                }
            }
            if (queue_family_index == queue_family_properties.size()) {
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
            m_swap_chain_format = vk::Format::eB8G8R8A8Unorm;

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
        }
        else {
            BOOST_THROW_EXCEPTION(error::capability_exception()
                << error::errinfo_capability_description("No Vulkan physical devices meets requirements."));
        }

        // Create the device.
        float queue_priority = 1.0f; // Priority is not important when there is only a single queue.
        vk::DeviceQueueCreateInfo queue_create_info;
        queue_create_info.queueFamilyIndex = queue_family_index;
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

        // Get all of the queues in the family.
        m_queue = m_device.getQueue(queue_family_index, 0, m_dispatch);
    }

    void application::vk_create_swap_chain()
    {
        // This call still has to be dispatched by instance only. So no dynamic dispatch.
        glfw_dispatch_loader d(m_instance);
        vk::SurfaceCapabilitiesKHR surface_capabilities = m_physical_device.getSurfaceCapabilitiesKHR(m_surface, d);
        m_swap_chain_extent = surface_capabilities.currentExtent;

        // Create the whole swap chain.
        vk::SwapchainCreateInfoKHR swap_chain_create_info;
        swap_chain_create_info.surface = m_surface;
        swap_chain_create_info.minImageCount = 3;
        swap_chain_create_info.imageFormat = m_swap_chain_format;
        swap_chain_create_info.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        swap_chain_create_info.imageExtent = m_swap_chain_extent;
        swap_chain_create_info.imageArrayLayers = 1;
        swap_chain_create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        swap_chain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
        swap_chain_create_info.preTransform = surface_capabilities.currentTransform;
        swap_chain_create_info.presentMode = vk::PresentModeKHR::eMailbox;
        swap_chain_create_info.clipped = VK_TRUE;

        m_swap_chain = m_device.createSwapchainKHR(swap_chain_create_info, nullptr, m_dispatch);

        // Get the images from the swap chain and create views to each.
        m_swap_chain_images = m_device.getSwapchainImagesKHR(m_swap_chain, m_dispatch);

        vk::ImageViewCreateInfo image_view_create_info;
        image_view_create_info.viewType = vk::ImageViewType::e2D;
        image_view_create_info.format = m_swap_chain_format;
        image_view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.layerCount = 1;

        m_swap_chain_views.reserve(m_swap_chain_images.size());
        for (vk::Image &image : m_swap_chain_images) {
            image_view_create_info.image = image;
            m_swap_chain_views.emplace_back(m_device.createImageView(image_view_create_info, nullptr, m_dispatch));
        }
    }

    void application::shaders_init()
    {
        std::ifstream spirv_stream;
        std::vector<char> spirv_buffer;
        vk::ShaderModuleCreateInfo shader_module_create_info;

        struct shader_to_init {
            std::string file_name;
            vk::ShaderModule& module;
        } init_list[] = {
            { "simple.vert.spv", m_simple_vert },
            { "simple.frag.spv", m_simple_frag }
        };

        for (shader_to_init& init_this : init_list) {
            spirv_stream.open(init_this.file_name, std::ios::ate | std::ios::binary);

            if (!spirv_stream.is_open()) {
                BOOST_THROW_EXCEPTION(error::file_exception()
                    << error::errinfo_file_exception_file(init_this.file_name.c_str()));
            }

            size_t spirv_stream_size = spirv_stream.tellg();
            if (spirv_stream_size == size_t(-1)) {
                BOOST_THROW_EXCEPTION(error::file_exception()
                    << error::errinfo_file_exception_file(init_this.file_name.c_str()));
            }

            spirv_buffer.reserve(spirv_stream_size);
            spirv_stream.seekg(0);
            spirv_stream.read(spirv_buffer.data(), spirv_stream_size);

            shader_module_create_info.codeSize = spirv_stream_size;
            shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(spirv_buffer.data());

            init_this.module = m_device.createShaderModule(shader_module_create_info, nullptr, m_dispatch);

            spirv_stream.close();
            spirv_buffer.clear();
        }
    }

    void application::shaders_cleanup()
    {
        if (m_simple_frag) {
            m_device.destroyShaderModule(m_simple_frag, nullptr, m_dispatch);
        }

        if (m_simple_vert) {
            m_device.destroyShaderModule(m_simple_vert, nullptr, m_dispatch);
        }
    }

    void application::render_pass_init()
    {
        // Need a render pass setup for each rendering method.
        vk::AttachmentDescription color_attachment;
        color_attachment.format = m_swap_chain_format;
        color_attachment.samples = vk::SampleCountFlagBits::e1;
        color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
        color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
        color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        color_attachment.initialLayout = vk::ImageLayout::eUndefined;
        color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference color_reference;
        color_reference.attachment = 0;
        color_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription simple_subpass;
        simple_subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        simple_subpass.colorAttachmentCount = 1;
        simple_subpass.pColorAttachments = &color_reference;

        vk::RenderPassCreateInfo render_pass_create_info;
        render_pass_create_info.attachmentCount = 1;
        render_pass_create_info.pAttachments = &color_attachment;
        render_pass_create_info.subpassCount = 1;
        render_pass_create_info.pSubpasses = &simple_subpass;

        m_simple_render_pass = m_device.createRenderPass(render_pass_create_info, nullptr, m_dispatch);

        // Each render pass needs it's own set of frame buffers for the swap chain.
        m_simple_framebuffers.reserve(m_swap_chain_views.size());
        for (vk::ImageView& image_view : m_swap_chain_views) {
            vk::FramebufferCreateInfo frame_buffer_create_info;
            frame_buffer_create_info.renderPass = m_simple_render_pass;
            frame_buffer_create_info.attachmentCount = 1;
            frame_buffer_create_info.pAttachments = &image_view;
            frame_buffer_create_info.width = m_swap_chain_extent.width;
            frame_buffer_create_info.height = m_swap_chain_extent.height;
            frame_buffer_create_info.layers = 1;

            m_simple_framebuffers.emplace_back(m_device.createFramebuffer(frame_buffer_create_info, nullptr, m_dispatch));
        }
    }

    void application::render_pass_cleanup()
    {
        for (vk::Framebuffer& frame_buffer : m_simple_framebuffers) {
            m_device.destroyFramebuffer(frame_buffer, nullptr, m_dispatch);
        }

        if (m_simple_render_pass) {
            m_device.destroyRenderPass(m_simple_render_pass, nullptr, m_dispatch);
        }
    }

    int application::run(int, char*[])
    {
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
            tick();
            draw();
        }
        return (EXIT_SUCCESS);
    }

    void application::tick()
    {

    }

    void application::draw()
    {

    }

    // static
    void application::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    // static
    void application::glfw_refresh_callback(GLFWwindow* window)
    {
        application* app = static_cast<application*>(glfwGetWindowUserPointer(window));
        app->draw();
    }

    // static
    void application::glfw_error_callback(int error, const char* description)
    {
        BOOST_THROW_EXCEPTION(error::glfw_exception()
            << error::errinfo_glfw_error_callback_error(error)
            << error::errinfo_glfw_error_callback_description(description));
    }

    // static
    VkBool32 application::vk_debug_report(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT object_type,
        uint64_t object,
        size_t location,
        int32_t message_code,
        const char* layer_prefix,
        const char* message,
        void* user_data)
    {
        application* app = static_cast<application*>(user_data);

        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            BOOST_THROW_EXCEPTION(error::vk_debug_report_error()
                << error::errinfo_vk_debug_report_error_object_type(vk::to_string(vk::ObjectType(object_type)).c_str())
                << error::errinfo_vk_debug_report_error_object(object)
                << error::errinfo_vk_debug_report_error_location(location)
                << error::errinfo_vk_debug_report_error_message_code(message_code)
                << error::errinfo_vk_debug_report_error_layer_prefix(layer_prefix)
                << error::errinfo_vk_debug_report_error_message(message));
        }
        // TODO: How to deal with other debug report types?

        return (VK_FALSE);
    }
}

int main(int argc, char* argv[])
{
    int ret_val = EXIT_FAILURE;
    try {
        ret_val = gtb::application::get()->run(argc, argv);
    }
    catch (const gtb::error::exception& e) {
        ret_val = gtb::error::handle_exception(e);
    }
    catch (const boost::exception& e) {
        ret_val = gtb::error::handle_exception(e);
    }
    catch (const vk::Error& e) {
        ret_val = gtb::error::handle_exception(e);
    }
    catch (const std::exception& e) {
        ret_val = gtb::error::handle_exception(e);
    }
    catch (...) {
        ret_val = gtb::error::handle_exception();
    }

    return (ret_val);
}

extern "C" {
    __attribute__((dllexport)) __attribute__((selectany)) uint32_t NvOptimusEnablement = 0x00000001;
}
