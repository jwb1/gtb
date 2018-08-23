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

#define _CRT_SECURE_NO_WARNINGS

#if (GTB_BUILD_TYPE == 1)
#define GTB_BUILD_DEBUG
#elif (GTB_BUILD_TYPE == 2)
#define GTB_BUILD_TYPE_RELEASE
#else
#error Unknown GTB_BUILD_TYPE
#endif

#ifdef GTB_BUILD_DEBUG
#define GTB_ENABLE_VULKAN_DEBUG_LAYER
#endif

// Compiler intrinsics
#include <intrin.h>

// C++ Standard Library
#include <cstdlib>
#include <cmath>
#include <cfenv>
#include <exception>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <limits>

// Boost
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp>

// Vulkan
#define VULKAN_HPP_NO_SMART_HANDLE
#include <vulkan/vulkan.hpp>

// GLFW
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4201) // nonstandard extension used : nameless struct / union
#pragma warning (disable : 4458) // declaration hides class member
#pragma warning (disable : 4701) // potentially uninitialized local variable used
#endif

// GLM
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_AVX
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

// GLI
#include <gli/gli.hpp>

#if defined(_MSC_VER)
#pragma warning (pop)
#endif

// TinyGLTF
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

// Local helper classes
#include "gtb/glfw_dispatch_loader.hpp"
#include "gtb/dbg_out.hpp"

/*
~~ Math Conventions ~~
- Vectors are treated as column vectors
- Matrices are stored column major; eg m[col][row] aka m[col] returns a column
|m00 m10 m20 m30|         |m0 m4 m8  m12|
|m01 m11 m21 m31|         |m1 m5 m9  m13|
|m02 m12 m22 m32|         |m2 m6 m10 m14|
|m03 m13 m23 m33|         |m3 m7 m11 m15|
- A column vector is multiplied by a matrix on the right:
|m00 m10 m20 m30| |v0|   |(m00 * v0) + (m10 * v1) + (m20 * v2) + (m30 * v3)|
|m01 m11 m21 m31| |v1| = |(m01 * v0) + (m11 * v1) + (m21 * v2) + (m31 * v3)|
|m02 m12 m22 m32| |v2|   |(m02 * v0) + (m12 * v1) + (m22 * v2) + (m32 * v3)|
|m03 m13 m23 m33| |v3|   |(m03 * v0) + (m13 * v1) + (m23 * v2) + (m33 * v3)|

- Transformations on the right of a matrix product applied are first geometrically;
If T, S, and R and transformations, then the product M = T * S * R will apply
R first, then S, the T, geometrically.
- Translation lives in column 3 (m12, m13, m14) when transforming homogeneous vectors

- Eye space places the camera at (0, 0, 0), looking in the direction of the +Z axis,
positive X to the right and positive y down. This is right handed.
- Counter-clockwise winding in eye space as front facing
- Clip space dimensions [[-1, 1], [-1, 1], [0,1]], where geometry with (x,y)
normalized device coordinates of (-1,-1) correspond to the upper-left corner of the
viewport and the near and far planes correspond to z normalized device coordinates
of 0 and +1, respectively.
- https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
*/

namespace gtb {
    uint32_t align_up(uint32_t value, uint32_t align)
    {
        return ((value + align - 1) & ~(align - 1));
    }

    void open_log_stream(std::ofstream& log_stream, const std::string& file_name)
    {
        boost::filesystem::path log_file_path(std::getenv("LOCALAPPDATA"));
        log_file_path /= "gtb";
        boost::filesystem::create_directories(log_file_path);

        log_file_path /= file_name;
        log_stream.open(log_file_path.string(), std::ios_base::out | std::ios_base::trunc);
    }

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

        // File I/O failures throw these.
        class file_exception : public exception {};

        typedef boost::error_info<struct errinfo_file_exception_file_, const char*> errinfo_file_exception_file;
        typedef boost::error_info<struct errinfo_file_exception_file_, const char*> errinfo_file_exception_message;

        // Exception handlers; only intended to be called in catch blocks!
        int handle_exception(const gtb::error::exception& e)
        {
            std::ofstream log_stream;
            open_log_stream(log_stream, "exception.log");
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
            open_log_stream(log_stream, "exception.log");
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
            open_log_stream(log_stream, "exception.log");
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
            open_log_stream(log_stream, "exception.log");
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
            open_log_stream(log_stream, "exception.log");
            if (log_stream.is_open()) {
                log_stream
                    << "Exception caught!" << std::endl
                    << "Type: *unknown*" << std::endl;
                log_stream.close();
            }
            return (EXIT_FAILURE);
        }
    }

    union value_2_10_10_10_snorm {
        struct {
            uint32_t a : 2;
            uint32_t b : 10;
            uint32_t g : 10;
            uint32_t r : 10;
        } fields;
        uint32_t value;

        value_2_10_10_10_snorm() = default;

        value_2_10_10_10_snorm(float fr, float fg, float fb)
        {
            fields.a = 0;
            fields.b = convert_float(fb);
            fields.g = convert_float(fg);
            fields.r = convert_float(fr);
        }

        explicit value_2_10_10_10_snorm(uint32_t v)
            : value(v)
        {}

        explicit value_2_10_10_10_snorm(const glm::vec3& vector)
        {
            fields.a = 0;
            fields.b = convert_float(vector.z);
            fields.g = convert_float(vector.y);
            fields.r = convert_float(vector.x);
        }

        value_2_10_10_10_snorm& operator =(uint32_t v)
        {
            value = v;
            return (*this);
        }

        value_2_10_10_10_snorm& operator =(const glm::vec3& vector)
        {
            fields.a = 0;
            fields.b = convert_float(vector.z);
            fields.g = convert_float(vector.y);
            fields.r = convert_float(vector.x);
            return (*this);
        }

    private:
        static uint32_t convert_float(float v)
        {
            if (v >= 1.0f) {
                return (0x1FF);
            }
            else if (v <= -1.0f) {
                return (0x3FF);
            }
            else {
                v *= 511.0f; // 511 = (2 ^ (10 - 1)) - 1
                if (v >= 0.0f) {
                    v += 0.5f;
                }
                else {
                    v -= 0.5f;
                }
                std::fesetround(FE_TOWARDZERO);
                return (std::lrintf(v) & 0x3FF);
            }
        }
    };

    struct vertex {
        glm::vec3 position;
        glm::tvec3<value_2_10_10_10_snorm> tangent_space_basis; // x = normal, y = tangent, z = bitangent
        glm::vec2 tex_coord;
    };

    class application {
        static constexpr uint32_t per_frame_ubo_size = 65535;

        static const vk::MemoryPropertyFlags ubo_memory_properties;
        static const vk::MemoryPropertyFlags staging_memory_properties;
        static const vk::MemoryPropertyFlags optimized_memory_properties;

        struct device_buffer {
            vk::Buffer buffer;
            vk::DeviceMemory device_memory;
        };
        typedef std::vector<device_buffer> device_buffer_vector;

        struct device_image {
            vk::Image image;
            vk::DeviceMemory device_memory;
            vk::ImageView view;
        };
        typedef std::vector<device_image> device_image_vector;

        struct draw_record {
            glm::mat4 transform;

            uint32_t index_count;
            uint32_t first_index;
            int32_t vertex_offset;

            vk::DescriptorSet immutable_state;

            uint8_t vbo;
            uint8_t ibo;
        };
        typedef std::vector<draw_record> draw_vector;

        typedef std::unordered_map<uint32_t, uint8_t> loaded_buffer_map;

        struct gltf_load_state {
            explicit gltf_load_state(const tinygltf::Model& m)
                : model(m)
                , draw_index(0)
            {}

            const tinygltf::Model& model;
            loaded_buffer_map loaded_ibo;
            loaded_buffer_map loaded_vbo;
            draw_vector draws;
            std::vector<vk::DescriptorSet> simple_immutable_sets;
            uint32_t draw_index;
        };

        // Logging
        std::ofstream m_log_stream;

        // GLFW
        GLFWwindow* m_window;

        // Vulkan
        vk::Instance m_instance;
        vk::DebugReportCallbackEXT m_debug_report_callback;
        vk::PhysicalDevice m_physical_device;
        uint32_t m_queue_family_index;
        vk::Device m_device;
        vk::Queue m_queue;
        vk::DispatchLoaderDynamic m_dispatch;

        // Swap chain for display
        vk::SurfaceKHR m_surface;
        vk::Format m_swap_chain_color_format;
        vk::Format m_swap_chain_depth_format;
        vk::Extent2D m_swap_chain_extent;
        vk::SwapchainKHR m_swap_chain;
        device_image_vector m_swap_chain_color_images;
        device_image_vector m_swap_chain_depth_images;
        vk::Fence m_next_image_ready;

        // Graphics memory
        vk::PhysicalDeviceMemoryProperties m_memory_properties;

        // Shaders
        vk::ShaderModule m_simple_vert;
        vk::ShaderModule m_simple_frag;

        // Render pass and targets
        vk::RenderPass m_simple_render_pass;
        std::vector<vk::Framebuffer> m_simple_framebuffers;

        // Command buffers (normally per-rendering thread)
        vk::CommandPool m_command_pool;
        std::vector<vk::CommandBuffer> m_command_buffers;
        std::vector<vk::Fence> m_command_fences;

        // Samplers
        vk::Sampler m_bilinear_sampler;

        // Pipelines
        vk::PipelineLayout m_simple_pipeline_layout;
        vk::Pipeline m_simple_pipeline;

        // Uniform buffers
        uint32_t m_ubo_min_field_align;
        device_buffer_vector m_uniform_buffers;

        // Mutable bound state = One set per frame pointing at each per-frame uniform buffer.
        vk::DescriptorPool m_mutable_descriptor_pool;
        vk::DescriptorSetLayout m_simple_mutable_set_layout;
        std::vector<vk::DescriptorSet> m_simple_mutable_sets;

        // Immutable bound state = One set per draw.
        vk::DescriptorPool m_immutable_descriptor_pool;
        vk::DescriptorSetLayout m_simple_immutable_set_layout;

        // Static buffers
        device_buffer_vector m_static_buffers;

        // Textures
        device_image_vector m_textures;

        // Draw list
        draw_vector m_draws;

        // Camera
        glm::mat4 m_camera_transform;

    public:
        static application* get();

        int run(int argc, char* argv[]);

    private:
        application();
        ~application();

        void init(int argc, char* argv[]);
        void cleanup();

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

        // Graphics memory
        uint32_t get_memory_type(uint32_t allowed_types, vk::MemoryPropertyFlags desired_memory_properties);

        // Shaders
        void shaders_init();
        void shaders_cleanup();

        // Render pass and targets
        void render_pass_init();
        void render_pass_cleanup();

        // Samplers
        void sampler_init();
        void sampler_cleanup();

        // Pipelines
        void pipeline_init();
        void pipeline_cleanup();

        // Per-frame buffers
        void per_frame_init();
        void per_frame_cleanup();

        // Built-in objects
        void builtin_object_init();

        // Loaded objects
        void gltf_load(const std::string& file_name);
        void gltf_load_node(
            const tinygltf::Node& node,
            const glm::mat4& parent_transform,
            gltf_load_state& load_state); // Recursive!

        void gltf_load_ibo(
            draw_record& node_draw,
            const tinygltf::Primitive& primitive,
            gltf_load_state& load_state);

        void gltf_load_vbo(
            draw_record& node_draw,
            const tinygltf::Primitive& primitive,
            gltf_load_state& load_state);

        void gltf_load_immutable_state(
            const tinygltf::Node& node,
            gltf_load_state& load_state); // Recursive!

        static bool gltf_load_image_data(
            tinygltf::Image* image,
            std::string* load_error,
            int required_width,
            int required_height,
            const unsigned char* bytes,
            int size,
            void* user_data);

        static uint32_t gltf_component_size(int component);

        // Command buffers
        vk::CommandBuffer create_one_time_command_buffer();
        void finish_one_time_command_buffer(vk::CommandBuffer cmd_buffer);
        void cleanup_one_time_command_buffer(vk::CommandBuffer cmd_buffer);

        // Buffers
        device_buffer_vector::iterator create_static_buffer(
            vk::BufferUsageFlags flags,
            const void* data,
            size_t sizeof_data);
        void static_buffers_cleanup();

        device_buffer create_device_buffer(
            vk::BufferUsageFlags flags,
            size_t sizeof_data,
            vk::MemoryPropertyFlags memory_properties);
        void cleanup_device_buffer(device_buffer& b);

        // Textures
        device_image_vector::iterator create_texture(const std::string& file_name);
        void textures_cleanup();

        void cleanup_device_image(device_image& t);

        // Disallow some C++ operations.
        application(application&&) = delete;
        application(const application&) = delete;
        application& operator=(application&&) = delete;
        application& operator=(const application&) = delete;
    };

    // static
    const vk::MemoryPropertyFlags application::ubo_memory_properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    // static
    const vk::MemoryPropertyFlags application::staging_memory_properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    // static
    const vk::MemoryPropertyFlags application::optimized_memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

    // static
    application* application::get()
    {
        static application app;
        return (&app);
    }

    application::application()
        : m_window(nullptr)
        , m_queue_family_index(std::numeric_limits<uint32_t>::max())
        , m_ubo_min_field_align(0)
        , m_camera_transform(1.0f)
    {
        std::fexcept_t fe;
        std::fesetexceptflag(&fe, 0);
    }

    application::~application()
    {}

    int application::run(int argc, char* argv[])
    {
        init(argc, argv);

        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
            tick();
            draw();
        }

        cleanup();
        return (EXIT_SUCCESS);
    }

    void application::init(int argc, char* argv[])
    {
        std::string object_file("gtb.gltf");
        if (argc >= 2) {
            object_file = argv[1];
        }

        open_log_stream(m_log_stream, "runtime.log");

        glfw_init();
        vk_init();
        shaders_init();
        render_pass_init();
        sampler_init();
        per_frame_init();
        pipeline_init();

        // geometry buffers and textures are either built-in or loaded.
        builtin_object_init();
        gltf_load(object_file);
    }

    void application::cleanup()
    {
        if (m_device) {
            m_device.waitIdle(m_dispatch);
        }

        textures_cleanup();
        static_buffers_cleanup();
        pipeline_cleanup();
        per_frame_cleanup();
        sampler_cleanup();
        render_pass_cleanup();
        shaders_cleanup();
        vk_cleanup();
        glfw_cleanup();

        m_log_stream.close();
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
        if (m_next_image_ready) {
            m_device.destroyFence(m_next_image_ready, nullptr, m_dispatch);
        }

        for (device_image& di : m_swap_chain_depth_images) {
            cleanup_device_image(di);
        }

        for (device_image& ci : m_swap_chain_color_images) {
            m_device.destroyImageView(ci.view, nullptr, m_dispatch);
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
        application_info.apiVersion = VK_API_VERSION_1_0;

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
        debug_report_create_info.flags =
            vk::DebugReportFlagBitsEXT::eError
            | vk::DebugReportFlagBitsEXT::ePerformanceWarning
            | vk::DebugReportFlagBitsEXT::eWarning
            | vk::DebugReportFlagBitsEXT::eInformation
            // | vk::DebugReportFlagBitsEXT::eDebug
            ;
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
        uint32_t queue_family_index = std::numeric_limits<uint32_t>::max();

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
            m_swap_chain_color_format = vk::Format::eB8G8R8A8Unorm;
            m_swap_chain_depth_format = vk::Format::eD32Sfloat;

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
            m_queue_family_index = queue_family_index;
        }
        else {
            BOOST_THROW_EXCEPTION(error::capability_exception()
                << error::errinfo_capability_description("No Vulkan physical devices meets requirements."));
        }

        // Need some device-specific info.
        m_memory_properties = m_physical_device.getMemoryProperties(d);

        vk::PhysicalDeviceProperties device_props = m_physical_device.getProperties(d);
        m_ubo_min_field_align = static_cast<uint32_t>(device_props.limits.minUniformBufferOffsetAlignment);

        // Create the device.
        float queue_priority = 1.0f; // Priority is not important when there is only a single queue.
        vk::DeviceQueueCreateInfo queue_create_info;
        queue_create_info.queueFamilyIndex = m_queue_family_index;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        vk::PhysicalDeviceFeatures device_features;
        device_features.textureCompressionBC = VK_TRUE;

        vk::DeviceCreateInfo device_create_info;
        device_create_info.queueCreateInfoCount = 1;
        device_create_info.pQueueCreateInfos = &queue_create_info;
        device_create_info.enabledLayerCount = static_cast<uint32_t>(required_layers.size());
        device_create_info.ppEnabledLayerNames = required_layers.data();
        device_create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
        device_create_info.ppEnabledExtensionNames = required_extensions.data();
        device_create_info.pEnabledFeatures = &device_features;
        m_device = m_physical_device.createDevice(device_create_info, nullptr, d);

        // Init the dynamic dispatch. All future Vulkan functions will be called through this.
        m_dispatch.init(m_instance, m_device);

        // Get all of the queues in the family.
        m_queue = m_device.getQueue(m_queue_family_index, 0, m_dispatch);

        // Command pool is the allocator wrapper.
        vk::CommandPoolCreateInfo command_pool_create_info;
        command_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        command_pool_create_info.queueFamilyIndex = m_queue_family_index;
        m_command_pool = m_device.createCommandPool(command_pool_create_info, nullptr, m_dispatch);
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
        swap_chain_create_info.imageFormat = m_swap_chain_color_format;
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
        std::vector<vk::Image> swap_chain_color_images = m_device.getSwapchainImagesKHR(m_swap_chain, m_dispatch);

        // Templates of create info structs for each swap chain image
        vk::ImageViewCreateInfo color_view_create_info;
        color_view_create_info.viewType = vk::ImageViewType::e2D;
        color_view_create_info.format = m_swap_chain_color_format;
        color_view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        color_view_create_info.subresourceRange.levelCount = 1;
        color_view_create_info.subresourceRange.layerCount = 1;

        vk::ImageCreateInfo depth_create_info;
        depth_create_info.imageType = vk::ImageType::e2D;
        depth_create_info.extent.width = m_swap_chain_extent.width;
        depth_create_info.extent.height = m_swap_chain_extent.height;
        depth_create_info.extent.depth = 1;
        depth_create_info.mipLevels = 1;
        depth_create_info.arrayLayers = 1;
        depth_create_info.format = m_swap_chain_depth_format;
        depth_create_info.tiling = vk::ImageTiling::eOptimal;
        depth_create_info.initialLayout = vk::ImageLayout::eUndefined;
        depth_create_info.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        depth_create_info.sharingMode = vk::SharingMode::eExclusive;
        depth_create_info.samples = vk::SampleCountFlagBits::e1;

        vk::ImageSubresourceRange subresource_range;
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
        subresource_range.baseMipLevel = 0;
        subresource_range.levelCount = 1;
        subresource_range.baseArrayLayer = 0;
        subresource_range.layerCount = 1;

        vk::ImageViewCreateInfo depth_view_create_info;
        depth_view_create_info.viewType = vk::ImageViewType::e2D;
        depth_view_create_info.format = m_swap_chain_depth_format;
        depth_view_create_info.subresourceRange = subresource_range;

        // Record a command buffer to copy the staging buffer to the optimized image.
        vk::CommandBuffer layout_command_buffer = create_one_time_command_buffer();

        m_swap_chain_color_images.reserve(swap_chain_color_images.size());
        m_swap_chain_depth_images.reserve(swap_chain_color_images.size());
        for (vk::Image &image : swap_chain_color_images) {
            device_image color_image;

            color_image.image = color_view_create_info.image = image;
            color_image.view = m_device.createImageView(color_view_create_info, nullptr, m_dispatch);

            m_swap_chain_color_images.emplace_back(color_image);

            device_image depth_image;

            depth_image.image = m_device.createImage(depth_create_info, nullptr, m_dispatch);

            vk::MemoryRequirements image_mem_reqs = m_device.getImageMemoryRequirements(depth_image.image, m_dispatch);

            vk::MemoryAllocateInfo image_alloc_info;
            image_alloc_info.allocationSize = image_mem_reqs.size;
            image_alloc_info.memoryTypeIndex = get_memory_type(image_mem_reqs.memoryTypeBits, optimized_memory_properties);
            depth_image.device_memory = m_device.allocateMemory(image_alloc_info, nullptr, m_dispatch);
            m_device.bindImageMemory(depth_image.image, depth_image.device_memory, 0, m_dispatch);

            depth_view_create_info.image = depth_image.image;
            depth_image.view = m_device.createImageView(depth_view_create_info, nullptr, m_dispatch);

            vk::ImageMemoryBarrier layout_barrier;
            layout_barrier.srcAccessMask = vk::AccessFlags();
            layout_barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            layout_barrier.oldLayout = vk::ImageLayout::eUndefined;
            layout_barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            layout_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            layout_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            layout_barrier.image = depth_image.image;
            layout_barrier.subresourceRange = subresource_range;
            layout_command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &layout_barrier, m_dispatch);

            m_swap_chain_depth_images.emplace_back(depth_image);
        }

        // TODO: Rather than finish here, we could wait at the end of the application constructor; will need to
        // remember staging details to be freed there.
        finish_one_time_command_buffer(layout_command_buffer);

        // Need a fence to use when acquiring a texture from the swap chain.
        vk::FenceCreateInfo fence_create_info;
        fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;
        m_next_image_ready = m_device.createFence(fence_create_info, nullptr, m_dispatch);
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
        vk::AttachmentDescription attachments[2];
        attachments[0].format = m_swap_chain_color_format;
        attachments[0].samples = vk::SampleCountFlagBits::e1;
        attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
        attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
        attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachments[0].initialLayout = vk::ImageLayout::eUndefined;
        attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

        attachments[1].format = m_swap_chain_depth_format;
        attachments[1].samples = vk::SampleCountFlagBits::e1;
        attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
        attachments[1].storeOp = vk::AttachmentStoreOp::eDontCare;
        attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachments[1].initialLayout = vk::ImageLayout::eUndefined;
        attachments[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference color_reference;
        color_reference.attachment = 0;
        color_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference depth_reference;
        depth_reference.attachment = 1;
        depth_reference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::SubpassDescription simple_subpass;
        simple_subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        simple_subpass.colorAttachmentCount = 1;
        simple_subpass.pColorAttachments = &color_reference;
        simple_subpass.pDepthStencilAttachment = &depth_reference;

        vk::RenderPassCreateInfo render_pass_create_info;
        render_pass_create_info.attachmentCount = _countof(attachments);
        render_pass_create_info.pAttachments = attachments;
        render_pass_create_info.subpassCount = 1;
        render_pass_create_info.pSubpasses = &simple_subpass;
        m_simple_render_pass = m_device.createRenderPass(render_pass_create_info, nullptr, m_dispatch);

        // Each render pass needs it's own set of frame buffers for the swap chain.
        uint32_t frames_in_flight = static_cast<uint32_t>(m_swap_chain_color_images.size());
        vk::ImageView fb_attachments[2];

        m_simple_framebuffers.reserve(frames_in_flight);
        for (uint32_t i = 0; i < frames_in_flight; ++i) {
            fb_attachments[0] = m_swap_chain_color_images[i].view;
            fb_attachments[1] = m_swap_chain_depth_images[i].view;

            vk::FramebufferCreateInfo frame_buffer_create_info;
            frame_buffer_create_info.renderPass = m_simple_render_pass;
            frame_buffer_create_info.attachmentCount = _countof(fb_attachments);
            frame_buffer_create_info.pAttachments = fb_attachments;
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

    void application::sampler_init()
    {
        vk::SamplerCreateInfo sampler_create_info;
        sampler_create_info.magFilter = vk::Filter::eLinear;
        sampler_create_info.minFilter = vk::Filter::eLinear;
        sampler_create_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
        sampler_create_info.addressModeU = vk::SamplerAddressMode::eRepeat;
        sampler_create_info.addressModeV = vk::SamplerAddressMode::eRepeat;
        sampler_create_info.addressModeW = vk::SamplerAddressMode::eRepeat;
        m_bilinear_sampler = m_device.createSampler(sampler_create_info, nullptr, m_dispatch);
    }

    void application::sampler_cleanup()
    {
        if (m_bilinear_sampler) {
            m_device.destroySampler(m_bilinear_sampler, nullptr, m_dispatch);
        }
    }

    void application::per_frame_init()
    {
        // Need a command buffer per frame in flight.
        uint32_t frames_in_flight = static_cast<uint32_t>(m_swap_chain_color_images.size());

        vk::CommandBufferAllocateInfo command_buffer_allocate_info;
        command_buffer_allocate_info.commandPool = m_command_pool;
        command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;
        command_buffer_allocate_info.commandBufferCount = frames_in_flight;
        m_command_buffers = m_device.allocateCommandBuffers(command_buffer_allocate_info, m_dispatch);

        // Need a uniform buffer per frame in flight as well.
        m_uniform_buffers.reserve(frames_in_flight);
        for (uint32_t i = 0; i < frames_in_flight; ++i) {
            m_uniform_buffers.emplace_back(create_device_buffer(vk::BufferUsageFlagBits::eUniformBuffer, per_frame_ubo_size, ubo_memory_properties));
        }

        // Descriptors help us bind uniform buffer memory. Need a pool object to allocate them.
        vk::DescriptorPoolSize descriptor_pool_size;
        descriptor_pool_size.type = vk::DescriptorType::eUniformBufferDynamic;
        descriptor_pool_size.descriptorCount = frames_in_flight * 1; // 1 == number of uniform buffers

        vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
        descriptor_pool_create_info.maxSets = frames_in_flight;
        descriptor_pool_create_info.poolSizeCount = 1;
        descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;

        m_mutable_descriptor_pool = m_device.createDescriptorPool(descriptor_pool_create_info, nullptr, m_dispatch);

        // Need a fence per command buffer / uniform buffer.
        vk::FenceCreateInfo fence_create_info;
        fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;

        m_command_fences.reserve(frames_in_flight);
        for (uint32_t i = 0; i < frames_in_flight; ++i) {
            m_command_fences.emplace_back(m_device.createFence(fence_create_info, nullptr, m_dispatch));
        }
    }

    void application::per_frame_cleanup()
    {
        for (vk::Fence& fence : m_command_fences) {
            m_device.destroyFence(fence, nullptr, m_dispatch);
        }

        if (m_mutable_descriptor_pool) {
            m_device.destroyDescriptorPool(m_mutable_descriptor_pool, nullptr, m_dispatch);
        }

        for (device_buffer& b : m_uniform_buffers) {
            cleanup_device_buffer(b);
        }

        if (!m_command_buffers.empty()) {
            m_device.freeCommandBuffers(m_command_pool, m_command_buffers, m_dispatch);
        }

        if (m_command_pool) {
            m_device.destroyCommandPool(m_command_pool, nullptr, m_dispatch);
        }
    }

    void application::pipeline_init()
    {
        // Create pipelines for each rendering method.

        // Shaders.
        vk::PipelineShaderStageCreateInfo shader_stage_create_info[2];

        shader_stage_create_info[0].stage = vk::ShaderStageFlagBits::eVertex;
        shader_stage_create_info[0].module = m_simple_vert;
        shader_stage_create_info[0].pName = "main";

        shader_stage_create_info[1].stage = vk::ShaderStageFlagBits::eFragment;
        shader_stage_create_info[1].module = m_simple_frag;
        shader_stage_create_info[1].pName = "main";

        // Vertex attribute layout.
        vk::VertexInputBindingDescription input_binding_vbo;
        input_binding_vbo.binding = 0;
        input_binding_vbo.stride = sizeof(gtb::vertex);
        input_binding_vbo.inputRate = vk::VertexInputRate::eVertex;

        vk::VertexInputAttributeDescription vertex_attrib_descriptions[3];
        vertex_attrib_descriptions[0].location = 0;
        vertex_attrib_descriptions[0].binding = 0;
        vertex_attrib_descriptions[0].format = vk::Format::eR32G32B32Sfloat;
        vertex_attrib_descriptions[0].offset = offsetof(gtb::vertex, position);

        vertex_attrib_descriptions[1].location = 1;
        vertex_attrib_descriptions[1].binding = 0;
        vertex_attrib_descriptions[1].format = vk::Format::eR32G32B32Uint;
        vertex_attrib_descriptions[1].offset = offsetof(gtb::vertex, tangent_space_basis);

        vertex_attrib_descriptions[2].location = 2;
        vertex_attrib_descriptions[2].binding = 0;
        vertex_attrib_descriptions[2].format = vk::Format::eR32G32Sfloat;
        vertex_attrib_descriptions[2].offset = offsetof(gtb::vertex, tex_coord);

        vk::PipelineVertexInputStateCreateInfo vertex_input_create_info;
        vertex_input_create_info.vertexBindingDescriptionCount = 1;
        vertex_input_create_info.pVertexBindingDescriptions = &input_binding_vbo;
        vertex_input_create_info.vertexAttributeDescriptionCount = _countof(vertex_attrib_descriptions);
        vertex_input_create_info.pVertexAttributeDescriptions = vertex_attrib_descriptions;

        // Input assembly.
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info;
        input_assembly_create_info.topology = vk::PrimitiveTopology::eTriangleList;

        // Viewport and scissor.
        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swap_chain_extent.width);
        viewport.height = static_cast<float>(m_swap_chain_extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = m_swap_chain_extent;

        vk::PipelineViewportStateCreateInfo viewport_create_info;
        viewport_create_info.viewportCount = 1;
        viewport_create_info.pViewports = &viewport;
        viewport_create_info.scissorCount = 1;
        viewport_create_info.pScissors = &scissor;

        // Rasterizer.
        vk::PipelineRasterizationStateCreateInfo rasterization_create_info;
        rasterization_create_info.depthClampEnable = VK_FALSE;
        rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_create_info.polygonMode = vk::PolygonMode::eFill;
        rasterization_create_info.cullMode = vk::CullModeFlagBits::eBack; //vk::CullModeFlagBits::eNone;
        rasterization_create_info.frontFace = vk::FrontFace::eCounterClockwise;
        rasterization_create_info.depthBiasEnable = VK_FALSE;
        rasterization_create_info.lineWidth = 1.0f;

        // Depth buffer.
        vk::PipelineDepthStencilStateCreateInfo depth_create_info;
        depth_create_info.depthTestEnable = VK_TRUE;
        depth_create_info.depthWriteEnable = VK_TRUE;
        depth_create_info.depthCompareOp = vk::CompareOp::eLess;

        // Multisampling.
        vk::PipelineMultisampleStateCreateInfo multisample_create_info;
        multisample_create_info.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisample_create_info.sampleShadingEnable = VK_FALSE;

        // Blending.
        vk::PipelineColorBlendAttachmentState color_blend_attachment;
        color_blend_attachment.blendEnable = VK_FALSE;
        color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo blend_create_info;
        blend_create_info.logicOpEnable = VK_FALSE;
        blend_create_info.attachmentCount = 1;
        blend_create_info.pAttachments = &color_blend_attachment;

        // Binding layout.
        vk::DescriptorSetLayoutBinding mutable_set_layout_bindings[1];
        mutable_set_layout_bindings[0].binding = 0;
        mutable_set_layout_bindings[0].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
        mutable_set_layout_bindings[0].descriptorCount = 1;
        mutable_set_layout_bindings[0].stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::DescriptorSetLayoutCreateInfo mutable_set_layout_create_info;
        mutable_set_layout_create_info.bindingCount = _countof(mutable_set_layout_bindings);
        mutable_set_layout_create_info.pBindings = mutable_set_layout_bindings;
        m_simple_mutable_set_layout = m_device.createDescriptorSetLayout(mutable_set_layout_create_info, nullptr, m_dispatch);

        vk::DescriptorSetLayoutBinding immutable_set_layout_bindings[1];
        immutable_set_layout_bindings[0].binding = 1;
        immutable_set_layout_bindings[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        immutable_set_layout_bindings[0].descriptorCount = 1;
        immutable_set_layout_bindings[0].stageFlags = vk::ShaderStageFlagBits::eFragment;

        vk::DescriptorSetLayoutCreateInfo immutable_set_layout_create_info;
        immutable_set_layout_create_info.bindingCount = _countof(immutable_set_layout_bindings);
        immutable_set_layout_create_info.pBindings = immutable_set_layout_bindings;
        m_simple_immutable_set_layout = m_device.createDescriptorSetLayout(immutable_set_layout_create_info, nullptr, m_dispatch);

        vk::DescriptorSetLayout set_layouts[] = {
            m_simple_mutable_set_layout, m_simple_immutable_set_layout
        };

        vk::PipelineLayoutCreateInfo layout_create_info;
        layout_create_info.setLayoutCount = _countof(set_layouts);
        layout_create_info.pSetLayouts = set_layouts;
        m_simple_pipeline_layout = m_device.createPipelineLayout(layout_create_info, nullptr, m_dispatch);

        // UBO descriptor setup.
        uint32_t frames_in_flight = static_cast<uint32_t>(m_swap_chain_color_images.size());

        std::vector<vk::DescriptorSetLayout> replicated_set_layouts(frames_in_flight, m_simple_mutable_set_layout);
        vk::DescriptorSetAllocateInfo set_allocate_info;
        set_allocate_info.descriptorPool = m_mutable_descriptor_pool;
        set_allocate_info.descriptorSetCount = frames_in_flight;
        set_allocate_info.pSetLayouts = replicated_set_layouts.data();

        m_simple_mutable_sets = m_device.allocateDescriptorSets(set_allocate_info, m_dispatch);

        for (uint32_t i = 0; i < frames_in_flight; ++i) {
            vk::DescriptorBufferInfo descriptor_buffer_info[1];
            vk::WriteDescriptorSet write_descriptor_set[1];

            descriptor_buffer_info[0].buffer = m_uniform_buffers[i].buffer;
            descriptor_buffer_info[0].offset = 0; // Offsets will be set dynamically at bind time.
            descriptor_buffer_info[0].range = sizeof(glm::mat4);

            write_descriptor_set[0].dstSet = m_simple_mutable_sets[i];
            write_descriptor_set[0].dstBinding = 0;
            write_descriptor_set[0].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
            write_descriptor_set[0].descriptorCount = 1;
            write_descriptor_set[0].pBufferInfo = &descriptor_buffer_info[0];

            m_device.updateDescriptorSets(_countof(write_descriptor_set), write_descriptor_set, 0, nullptr, m_dispatch);
        }

        // Combine the pipeline.
        vk::GraphicsPipelineCreateInfo pipeline_create_info;
        pipeline_create_info.stageCount = _countof(shader_stage_create_info);
        pipeline_create_info.pStages = shader_stage_create_info;
        pipeline_create_info.pVertexInputState = &vertex_input_create_info;
        pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
        pipeline_create_info.pViewportState = &viewport_create_info;
        pipeline_create_info.pRasterizationState = &rasterization_create_info;
        pipeline_create_info.pMultisampleState = &multisample_create_info;
        pipeline_create_info.pDepthStencilState = &depth_create_info;
        pipeline_create_info.pColorBlendState = &blend_create_info;
        pipeline_create_info.layout = m_simple_pipeline_layout;
        pipeline_create_info.renderPass = m_simple_render_pass;
        pipeline_create_info.subpass = 0;

        m_simple_pipeline = m_device.createGraphicsPipeline(vk::PipelineCache(), pipeline_create_info, nullptr, m_dispatch);
    }
    
    void application::pipeline_cleanup()
    {
        if (m_simple_pipeline) {
            m_device.destroyPipeline(m_simple_pipeline, nullptr, m_dispatch);
        }

        if (m_simple_pipeline_layout) {
            m_device.destroyPipelineLayout(m_simple_pipeline_layout, nullptr, m_dispatch);
        }

        if (m_simple_mutable_set_layout) {
            m_device.destroyDescriptorSetLayout(m_simple_mutable_set_layout, nullptr, m_dispatch);
        }

        if (m_simple_immutable_set_layout) {
            m_device.destroyDescriptorSetLayout(m_simple_immutable_set_layout, nullptr, m_dispatch);
        }
    }

    void application::builtin_object_init()
    {
        // A Textured Quad (No quad primitive in Vulkan, so two tris and eat the helpers.)
        static const gtb::vertex quad_verts[] = {
            //  position                normal                tangent               bitangent               texcoord
            { { 0.0f, 0.0f, 0.0f }, { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }, { 0.0f, 0.0f } },
            { { 0.0f, 1.0f, 0.0f }, { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }, { 0.0f, 1.0f } },
            { { 1.0f, 1.0f, 0.0f }, { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }, { 1.0f, 1.0f } },
            { { 1.0f, 0.0f, 0.0f }, { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }, { 1.0f, 0.0f } }
        };
        create_static_buffer(vk::BufferUsageFlagBits::eVertexBuffer, quad_verts, sizeof(quad_verts));

        static const uint16_t quad_indices[] = {
            0, 1, 3,
            1, 2, 3
        };
        create_static_buffer(vk::BufferUsageFlagBits::eIndexBuffer, quad_indices, sizeof(quad_indices));
    }

    void application::gltf_load(const std::string& file_name)
    {
        tinygltf::TinyGLTF loader;
        loader.SetImageLoader(gltf_load_image_data, this);

        tinygltf::Model model;
        std::string loader_error;

        if (!loader.LoadASCIIFromFile(&model, &loader_error, file_name)) {
            if (!loader_error.empty()) {
                BOOST_THROW_EXCEPTION(error::file_exception()
                    << error::errinfo_file_exception_file(file_name.c_str())
                    << error::errinfo_file_exception_message(loader_error.c_str()));
            }
            else {
                BOOST_THROW_EXCEPTION(error::file_exception()
                    << error::errinfo_file_exception_file(file_name.c_str()));
            }
        }

        // First pass over the model to load ibo /vbo as well as count draws.
        const tinygltf::Scene& scene(model.scenes.at(model.defaultScene));
        glm::mat4 scene_transform(1.0f);
        gltf_load_state load_state(model);

        for (int node_index : scene.nodes) {
            const tinygltf::Node& scene_node(model.nodes.at(node_index));
            gltf_load_node(scene_node, scene_transform, load_state);
        }

        uint32_t draw_count = static_cast<uint32_t>(load_state.draws.size());

        // Immutable state needs a pool and one set per draw.
        vk::DescriptorPoolSize descriptor_pool_sizes[1];
        descriptor_pool_sizes[0].type = vk::DescriptorType::eCombinedImageSampler;
        descriptor_pool_sizes[0].descriptorCount = draw_count; // Each draw needs a single sampler.

        vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
        descriptor_pool_create_info.maxSets = draw_count;
        descriptor_pool_create_info.poolSizeCount = _countof(descriptor_pool_sizes);
        descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes;

        m_immutable_descriptor_pool = m_device.createDescriptorPool(descriptor_pool_create_info, nullptr, m_dispatch);

        std::vector<vk::DescriptorSetLayout> replicated_set_layouts(draw_count, m_simple_immutable_set_layout);
        vk::DescriptorSetAllocateInfo set_allocate_info;
        set_allocate_info.descriptorPool = m_immutable_descriptor_pool;
        set_allocate_info.descriptorSetCount = draw_count;
        set_allocate_info.pSetLayouts = replicated_set_layouts.data();

        load_state.simple_immutable_sets = m_device.allocateDescriptorSets(set_allocate_info, m_dispatch);

        // Second pass over the model to load textures and set up immutable descriptor sets.
        for (int node_index : scene.nodes) {
            const tinygltf::Node& scene_node(model.nodes.at(node_index));
            gltf_load_immutable_state(scene_node, load_state);
        }

        // This might be an append later on.
        m_draws = load_state.draws;
    }

    void application::gltf_load_node(
        const tinygltf::Node& node,
        const glm::mat4& parent_transform,
        gltf_load_state& load_state)
    {
        glm::mat4 node_transform(1.0f);
        if (node.matrix.size() == 16) {
            for (int col = 0; col < 4; ++col) {
                for (int row = 0; row < 4; ++row) {
                    node_transform[col][row] = static_cast<glm::mat4::value_type>(node.matrix[(col * 4) + row]);
                }
            }
        }
        else {
            glm::vec3 node_translation(0.0f, 0.0f, 0.0f);
            if (node.translation.size() == 3) {
                for (int i = 0; i < 3; ++i) {
                    node_translation[i] = static_cast<glm::vec3::value_type>(node.translation[i]);
                }
            }

            glm::vec3 node_scale(1.0f, 1.0f, 1.0f);
            if (node.scale.size() == 3) {
                for (int i = 0; i < 3; ++i) {
                    node_scale[i] = static_cast<glm::vec3::value_type>(node.scale[i]);
                }
            }

            glm::quat node_rotation(1.0f, 0.0f, 0.0f, 0.0f); // constructor params are (w,x,y,z) not (x,y,z,w)!
            if (node.rotation.size() == 4) {
                for (int i = 0; i < 4; ++i) {
                    node_rotation[i] = static_cast<glm::quat::value_type>(node.rotation[i]);
                }
            }

            node_transform =
                glm::translate(node_translation) * // Translate to final position
                glm::scale(node_scale) *
                glm::mat4_cast(node_rotation);
        }
        node_transform *= parent_transform;

        if (node.mesh != -1) {
            const tinygltf::Mesh& mesh = load_state.model.meshes.at(node.mesh);
            for (const tinygltf::Primitive& primitive : mesh.primitives) {
                draw_record node_draw;
                node_draw.transform = node_transform;

                gltf_load_ibo(node_draw, primitive, load_state);
                gltf_load_vbo(node_draw, primitive, load_state);

                load_state.draws.push_back(node_draw);
            }
        }
        if (node.camera != -1) {
            glm::mat4 projection_transform(1.0f);

            const tinygltf::Camera& camera = load_state.model.cameras.at(node.camera);
            if (camera.type == "perspective") {
                if (camera.perspective.zfar == 0.0f) {
                    projection_transform = glm::infinitePerspectiveRH(
                        camera.perspective.yfov,
                        camera.perspective.aspectRatio,
                        camera.perspective.znear);
                }
                else {
                    projection_transform = glm::perspectiveRH(
                        camera.perspective.yfov,
                        camera.perspective.aspectRatio,
                        camera.perspective.znear,
                        camera.perspective.zfar);
                }
            }
            else if (camera.type == "orthographic") {
                projection_transform = glm::orthoRH(
                    0.0f, 2.0f * camera.orthographic.xmag,
                    0.0f, 2.0f * camera.orthographic.ymag,
                    camera.orthographic.znear,
                    camera.orthographic.zfar);
            }

            m_camera_transform = projection_transform * node_transform;
        }

        for (int node_index : node.children) {
            const tinygltf::Node& child_node(load_state.model.nodes.at(node_index));
            gltf_load_node(child_node, node_transform, load_state);
        }
    }

    void application::gltf_load_ibo(
        draw_record& node_draw,
        const tinygltf::Primitive& primitive,
        gltf_load_state& load_state)
    {
        // Index buffer
        const tinygltf::Accessor& index_accessor = load_state.model.accessors.at(primitive.indices);

        loaded_buffer_map::iterator loaded_buffer(load_state.loaded_ibo.find(index_accessor.bufferView));
        if (loaded_buffer != load_state.loaded_ibo.end()) {
            node_draw.ibo = loaded_buffer->second;
        }
        else {
            const tinygltf::BufferView& index_buffer_view = load_state.model.bufferViews.at(index_accessor.bufferView);
            const tinygltf::Buffer& index_buffer = load_state.model.buffers.at(index_buffer_view.buffer);

            device_buffer_vector::iterator device_buffer(create_static_buffer(
                vk::BufferUsageFlagBits::eIndexBuffer,
                index_buffer.data.data() + index_buffer_view.byteOffset,
                index_buffer_view.byteLength));
            uint8_t device_buffer_index = static_cast<uint8_t>(device_buffer - m_static_buffers.begin());

            load_state.loaded_ibo.emplace(index_accessor.bufferView, device_buffer_index);
            node_draw.ibo = device_buffer_index;
        }

        // Draw parameters also come from index information.
        node_draw.first_index = static_cast<uint32_t>(index_accessor.byteOffset / gltf_component_size(index_accessor.componentType));
        node_draw.index_count = static_cast<uint32_t>(index_accessor.count);
        node_draw.vertex_offset = 0;
    }

    void application::gltf_load_vbo(
        draw_record& node_draw,
        const tinygltf::Primitive& primitive,
        gltf_load_state& load_state)
    {
        const tinygltf::Accessor& index_accessor = load_state.model.accessors.at(primitive.indices);
        uint32_t count = static_cast<uint32_t>(index_accessor.count);

        std::vector<vertex> vbo_data;
        vbo_data.reserve(count);

        // gltf supports flexible attrib arrays that need to be packed into the vbo
        uint32_t position_accessor_index = primitive.attributes.at("POSITION");
        uint32_t normal_accessor_index = primitive.attributes.at("NORMAL");
        uint32_t texcoord_accessor_index = primitive.attributes.at("TEXCOORD_0");
        uint32_t tangent_accessor_index = primitive.attributes.at("TANGENT");

        const tinygltf::Accessor& position_accessor = load_state.model.accessors.at(position_accessor_index);
        const tinygltf::Accessor& normal_accessor = load_state.model.accessors.at(normal_accessor_index);
        const tinygltf::Accessor& texcoord_accessor = load_state.model.accessors.at(texcoord_accessor_index);
        const tinygltf::Accessor& tangent_accessor = load_state.model.accessors.at(tangent_accessor_index);

        const tinygltf::BufferView& position_buffer_view = load_state.model.bufferViews.at(position_accessor.bufferView);
        const tinygltf::BufferView& normal_buffer_view = load_state.model.bufferViews.at(normal_accessor.bufferView);
        const tinygltf::BufferView& texcoord_buffer_view = load_state.model.bufferViews.at(texcoord_accessor.bufferView);
        const tinygltf::BufferView& tangent_buffer_view = load_state.model.bufferViews.at(tangent_accessor.bufferView);

        const tinygltf::Buffer& position_buffer = load_state.model.buffers.at(position_buffer_view.buffer);
        const tinygltf::Buffer& normal_buffer = load_state.model.buffers.at(normal_buffer_view.buffer);
        const tinygltf::Buffer& texcoord_buffer = load_state.model.buffers.at(texcoord_buffer_view.buffer);
        const tinygltf::Buffer& tangent_buffer = load_state.model.buffers.at(tangent_buffer_view.buffer);

        const unsigned char* position_base_pointer = position_buffer.data.data() + position_buffer_view.byteOffset + position_accessor.byteOffset;
        const unsigned char* normal_base_pointer = normal_buffer.data.data() + normal_buffer_view.byteOffset + normal_accessor.byteOffset;
        const unsigned char* tangent_base_pointer = tangent_buffer.data.data() + tangent_buffer_view.byteOffset + tangent_accessor.byteOffset;
        const unsigned char* texcoord_base_pointer = texcoord_buffer.data.data() + texcoord_buffer_view.byteOffset + texcoord_accessor.byteOffset;

        uint32_t position_stride = static_cast<uint32_t>((position_buffer_view.byteStride == 0) ? sizeof(glm::vec3) : position_buffer_view.byteStride);
        uint32_t normal_stride = static_cast<uint32_t>((normal_buffer_view.byteStride == 0) ? sizeof(glm::vec3) : normal_buffer_view.byteStride);
        uint32_t tangent_stride = static_cast<uint32_t>((tangent_buffer_view.byteStride == 0) ? sizeof(glm::vec4) : tangent_buffer_view.byteStride);

        uint32_t texcoord_stride = 0;
        if (texcoord_buffer_view.byteStride == 0) {
            switch (texcoord_accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                texcoord_stride = sizeof(float) * 2;
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                texcoord_stride = sizeof(unsigned char) * 2;
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                texcoord_stride = sizeof(unsigned short) * 2;
                break;
            }
        }
        else {
            texcoord_stride = static_cast<uint32_t>(texcoord_buffer_view.byteStride);
        }

        for (uint32_t v = 0; v < count; ++v) {
            vertex vert;

            vert.position = *reinterpret_cast<const glm::vec3*>(position_base_pointer + (position_stride * v));
            vert.tangent_space_basis.x = *reinterpret_cast<const glm::vec3*>(normal_base_pointer + (normal_stride * v));
            vert.tangent_space_basis.y = (*reinterpret_cast<const glm::vec4*>(tangent_base_pointer + (tangent_stride * v))).xyz;
            // TODO: Work out the rest of the tangent space basis

            switch (texcoord_accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                vert.tex_coord = *reinterpret_cast<const glm::vec2*>(texcoord_base_pointer + (texcoord_stride * v));
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                vert.tex_coord.x = static_cast<float>(*(texcoord_base_pointer + (texcoord_stride * v))) / 255.0f;
                vert.tex_coord.y = static_cast<float>(*(texcoord_base_pointer + (texcoord_stride * v) + sizeof(unsigned char))) / 255.0f;
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                vert.tex_coord.x = static_cast<float>(*reinterpret_cast<const unsigned short*>(texcoord_base_pointer + (texcoord_stride * v))) / 65536.0f;
                vert.tex_coord.y = static_cast<float>(*reinterpret_cast<const unsigned short*>(texcoord_base_pointer + (texcoord_stride * v) + sizeof(unsigned short))) / 65536.0f;
                break;
            }

            vbo_data.push_back(vert);
        }

        device_buffer_vector::iterator device_buffer(create_static_buffer(
            vk::BufferUsageFlagBits::eVertexBuffer,
            vbo_data.data(),
            vbo_data.size() * sizeof(vertex)));
        uint8_t device_buffer_index = static_cast<uint8_t>(device_buffer - m_static_buffers.begin());

        node_draw.vbo = device_buffer_index;
    }

    void application::gltf_load_immutable_state(
        const tinygltf::Node& node,
        gltf_load_state& load_state)
    {
        if (node.mesh == -1) {
            return;
        }

        const tinygltf::Mesh& mesh = load_state.model.meshes.at(node.mesh);
        for (const tinygltf::Primitive& primitive : mesh.primitives) {
            const tinygltf::Material& material = load_state.model.materials.at(primitive.material);

            int color_texture_index = material.values.at("baseColorTexture").TextureIndex();
            const tinygltf::Texture& color_texture = load_state.model.textures.at(color_texture_index);
            const tinygltf::Image& color_texture_image = load_state.model.images.at(color_texture.source);

            device_image_vector::iterator texture_image(create_texture(color_texture_image.uri));

            // Write the immutable state binding information into the set for this draw.
            vk::DescriptorSet immutable_state_set = load_state.simple_immutable_sets[load_state.draw_index];
            vk::DescriptorImageInfo descriptor_image_info[1];
            vk::WriteDescriptorSet write_descriptor_set[1];

            descriptor_image_info[0].sampler = m_bilinear_sampler;
            descriptor_image_info[0].imageView = texture_image->view;
            descriptor_image_info[0].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            write_descriptor_set[0].dstSet = immutable_state_set;
            write_descriptor_set[0].dstBinding = 1;
            write_descriptor_set[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            write_descriptor_set[0].descriptorCount = 1;
            write_descriptor_set[0].pImageInfo = &descriptor_image_info[0];

            m_device.updateDescriptorSets(_countof(write_descriptor_set), write_descriptor_set, 0, nullptr, m_dispatch);

            load_state.draws.at(load_state.draw_index).immutable_state = immutable_state_set;
            load_state.draw_index++;
        }

        for (int node_index : node.children) {
            const tinygltf::Node& child_node(load_state.model.nodes.at(node_index));
            gltf_load_immutable_state(child_node, load_state);
        }
    }

    // static
    bool application::gltf_load_image_data(
        tinygltf::Image* /*image*/,
        std::string* /*load_error*/,
        int /*required_width*/,
        int /*required_height*/,
        const unsigned char* /*bytes*/,
        int /*size*/,
        void* user_data)
    {
        application* app = static_cast<application*>(user_data);
        app;

        return (true);
    }

    // static
    uint32_t application::gltf_component_size(int component)
    {
        switch (component) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return (1);
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return (2);
        case TINYGLTF_COMPONENT_TYPE_INT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return (4);
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
            return (8);
        }
        return (0);
    }

    vk::CommandBuffer application::create_one_time_command_buffer()
    {
        vk::CommandBufferAllocateInfo command_buffer_allocate_info;
        command_buffer_allocate_info.commandPool = m_command_pool;
        command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;
        command_buffer_allocate_info.commandBufferCount = 1;
        vk::CommandBuffer one_time_command_buffer = m_device.allocateCommandBuffers(command_buffer_allocate_info, m_dispatch)[0];

        vk::CommandBufferBeginInfo begin_info;
        begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        one_time_command_buffer.begin(begin_info, m_dispatch);

        return (one_time_command_buffer);
    }

    void application::finish_one_time_command_buffer(vk::CommandBuffer cmd_buffer)
    {
        cmd_buffer.end(m_dispatch);

        // Submit the command buffer.
        vk::SubmitInfo submit_info;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd_buffer;
        m_queue.submit(submit_info, nullptr, m_dispatch);

        // Finish it by waiting for queue idle.
        m_queue.waitIdle(m_dispatch);
    }

    void application::cleanup_one_time_command_buffer(vk::CommandBuffer cmd_buffer)
    {
        m_device.freeCommandBuffers(m_command_pool, cmd_buffer, m_dispatch);
    }

    application::device_buffer_vector::iterator application::create_static_buffer(
        vk::BufferUsageFlags flags,
        const void* data,
        size_t sizeof_data)
    {
        // Need a staging buffer to upload the data from.
        device_buffer staging_buffer = create_device_buffer(vk::BufferUsageFlagBits::eTransferSrc, sizeof_data, staging_memory_properties);

        // Write the data to the staging buffer.
        void* mapped_staging_memory = m_device.mapMemory(staging_buffer.device_memory, 0, sizeof_data, vk::MemoryMapFlags(), m_dispatch);
        memcpy(mapped_staging_memory, data, sizeof_data);
        m_device.unmapMemory(staging_buffer.device_memory, m_dispatch);

        // Optimized buffer for actual GPU use.
        device_buffer optimized_buffer = create_device_buffer(flags | vk::BufferUsageFlagBits::eTransferDst, sizeof_data, optimized_memory_properties);

        // Record a command buffer to copy the staging buffer to the optimized buffer.
        vk::CommandBuffer copy_command_buffer = create_one_time_command_buffer();

        vk::BufferCopy copy_region;
        copy_region.size = sizeof_data;
        copy_command_buffer.copyBuffer(staging_buffer.buffer, optimized_buffer.buffer, copy_region, m_dispatch);

        // TODO: Rather than finish here, we could wait at the end of the application constructor; will need to
        // remember staging details to be freed there.
        finish_one_time_command_buffer(copy_command_buffer);

        // Cleanup.
        cleanup_one_time_command_buffer(copy_command_buffer);
        cleanup_device_buffer(staging_buffer);

        return (m_static_buffers.emplace(m_static_buffers.end(), optimized_buffer));
    }

    void application::static_buffers_cleanup()
    {
        for (device_buffer& b : m_static_buffers) {
            cleanup_device_buffer(b);
        }
    }

    application::device_buffer application::create_device_buffer(
        vk::BufferUsageFlags flags,
        size_t sizeof_data,
        vk::MemoryPropertyFlags memory_properties)
    {
        device_buffer b;

        // Create buffer object
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = sizeof_data;
        buffer_create_info.usage = flags;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;
        b.buffer = m_device.createBuffer(buffer_create_info, nullptr, m_dispatch);

        // Allocate memory for the buffer object.
        vk::MemoryRequirements buffer_mem_reqs = m_device.getBufferMemoryRequirements(b.buffer, m_dispatch);

        vk::MemoryAllocateInfo buffer_alloc_info;
        buffer_alloc_info.allocationSize = buffer_mem_reqs.size;
        buffer_alloc_info.memoryTypeIndex = get_memory_type(buffer_mem_reqs.memoryTypeBits, memory_properties);
        b.device_memory = m_device.allocateMemory(buffer_alloc_info, nullptr, m_dispatch);
        m_device.bindBufferMemory(b.buffer, b.device_memory, 0, m_dispatch);

        return (b);
    }

    void application::cleanup_device_buffer(device_buffer& b)
    {
        m_device.destroyBuffer(b.buffer, nullptr, m_dispatch);
        m_device.freeMemory(b.device_memory, nullptr, m_dispatch);
    }

    application::device_image_vector::iterator application::create_texture(const std::string& file_name)
    {
        gli::texture gli_texture(gli::load(file_name));
        if (gli_texture.empty()) {
            BOOST_THROW_EXCEPTION(error::file_exception()
                << error::errinfo_file_exception_file(file_name.c_str()));
        }

        // Need a staging buffer to upload the data from.
        device_buffer staging_buffer = create_device_buffer(vk::BufferUsageFlagBits::eTransferSrc, gli_texture.size(), staging_memory_properties);

        // Write the data to the staging buffer.
        void* mapped_staging_memory = m_device.mapMemory(staging_buffer.device_memory, 0, gli_texture.size(), vk::MemoryMapFlags(), m_dispatch);
        memcpy(mapped_staging_memory, gli_texture.data(), gli_texture.size());
        m_device.unmapMemory(staging_buffer.device_memory, m_dispatch);

        // Create a backing image.
        device_image optimized_texture;
        gli::extent3d gli_texture_extent(gli_texture.extent());

        vk::ImageCreateInfo image_create_info;
        switch (gli_texture.target()) {
        case gli::TARGET_1D:
        case gli::TARGET_1D_ARRAY:
            break;

        case gli::TARGET_2D:
            image_create_info.imageType = vk::ImageType::e2D;
            image_create_info.extent.width = gli_texture_extent.x;
            image_create_info.extent.height = gli_texture_extent.y;
            image_create_info.extent.depth = 1;
            image_create_info.mipLevels = static_cast<uint32_t>(gli_texture.levels());
            image_create_info.arrayLayers = 1;
            break;

        case gli::TARGET_2D_ARRAY:
        case gli::TARGET_3D:
        case gli::TARGET_RECT:
        case gli::TARGET_RECT_ARRAY:
        case gli::TARGET_CUBE:
        case gli::TARGET_CUBE_ARRAY:
            break;
        }

        image_create_info.format = static_cast<vk::Format>(gli_texture.format());
        image_create_info.tiling = vk::ImageTiling::eOptimal;
        image_create_info.initialLayout = vk::ImageLayout::eUndefined;
        image_create_info.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
        image_create_info.sharingMode = vk::SharingMode::eExclusive;
        image_create_info.samples = vk::SampleCountFlagBits::e1;

        optimized_texture.image = m_device.createImage(image_create_info, nullptr, m_dispatch);

        // Allocate memory for the image object.
        vk::MemoryRequirements image_mem_reqs = m_device.getImageMemoryRequirements(optimized_texture.image, m_dispatch);

        vk::MemoryAllocateInfo image_alloc_info;
        image_alloc_info.allocationSize = image_mem_reqs.size;
        image_alloc_info.memoryTypeIndex = get_memory_type(image_mem_reqs.memoryTypeBits, optimized_memory_properties);
        optimized_texture.device_memory = m_device.allocateMemory(image_alloc_info, nullptr, m_dispatch);
        m_device.bindImageMemory(optimized_texture.image, optimized_texture.device_memory, 0, m_dispatch);

        // Create a image view to access the image through.
        vk::ImageSubresourceRange subresource_range;
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        subresource_range.baseMipLevel = 0;
        subresource_range.levelCount = static_cast<uint32_t>(gli_texture.levels());
        subresource_range.baseArrayLayer = 0;
        subresource_range.layerCount = static_cast<uint32_t>(gli_texture.layers());

        vk::ImageViewCreateInfo image_view_create_info;
        image_view_create_info.image = optimized_texture.image;
        image_view_create_info.viewType = vk::ImageViewType::e2D;
        image_view_create_info.format = image_create_info.format;
        image_view_create_info.subresourceRange = subresource_range;

        optimized_texture.view = m_device.createImageView(image_view_create_info, nullptr, m_dispatch);

        // Record a command buffer to copy the staging buffer to the optimized image.
        vk::CommandBuffer copy_command_buffer = create_one_time_command_buffer();

        vk::ImageMemoryBarrier start_barrier;
        start_barrier.srcAccessMask = vk::AccessFlags();
        start_barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        start_barrier.oldLayout = vk::ImageLayout::eUndefined;
        start_barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
        start_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        start_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        start_barrier.image = optimized_texture.image;
        start_barrier.subresourceRange = subresource_range;
        copy_command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &start_barrier, m_dispatch);

        vk::BufferImageCopy copy_image_region;
        copy_image_region.bufferOffset = 0;
        copy_image_region.bufferRowLength = 0;
        copy_image_region.bufferImageHeight = 0;
        copy_image_region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        copy_image_region.imageSubresource.mipLevel = 0;
        copy_image_region.imageSubresource.baseArrayLayer = 0;
        copy_image_region.imageSubresource.layerCount = static_cast<uint32_t>(gli_texture.layers());
        copy_image_region.imageOffset = { 0, 0, 0 };
        copy_image_region.imageExtent = image_create_info.extent;
        copy_command_buffer.copyBufferToImage(staging_buffer.buffer, optimized_texture.image, vk::ImageLayout::eTransferDstOptimal, copy_image_region, m_dispatch);

        vk::ImageMemoryBarrier end_barrier;
        end_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        end_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        end_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        end_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        end_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        end_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        end_barrier.image = optimized_texture.image;
        end_barrier.subresourceRange = subresource_range;
        copy_command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &end_barrier, m_dispatch);

        // TODO: Rather than finish here, we could wait at the end of the application constructor; will need to
        // remember staging details to be freed there.
        finish_one_time_command_buffer(copy_command_buffer);

        // Cleanup.
        cleanup_one_time_command_buffer(copy_command_buffer);
        cleanup_device_buffer(staging_buffer);

        return (m_textures.emplace(m_textures.end(), optimized_texture));
    }

    void application::textures_cleanup()
    {
        // TODO: Immutable state is currently textures, cleanup should probably be elsewhere.
        if (m_immutable_descriptor_pool) {
            m_device.destroyDescriptorPool(m_immutable_descriptor_pool, nullptr, m_dispatch);
        }

        for (device_image& t : m_textures) {
            cleanup_device_image(t);
        }
    }

    void application::cleanup_device_image(device_image& t)
    {
        m_device.destroyImageView(t.view, nullptr, m_dispatch);
        m_device.destroyImage(t.image, nullptr, m_dispatch);
        m_device.freeMemory(t.device_memory, nullptr, m_dispatch);
    }

    uint32_t application::get_memory_type(uint32_t allowed_types, vk::MemoryPropertyFlags desired_memory_properties)
    {
        unsigned long mem_type = 0;

        while (_BitScanForward(&mem_type, allowed_types)) {
            if ((m_memory_properties.memoryTypes[mem_type].propertyFlags & desired_memory_properties) == desired_memory_properties) {
                return (mem_type);
            }
            else {
                allowed_types &= ~(1 << mem_type);
            }
        }

        BOOST_THROW_EXCEPTION(error::capability_exception()
            << error::errinfo_capability_description("Could not find needed memory type."));
    }

    void application::tick()
    {

    }

    void application::draw()
    {
        constexpr uint64_t infinite_wait = std::numeric_limits<uint64_t>::max();

        // Get the next image to render to from the swap chain.
        m_device.resetFences(m_next_image_ready, m_dispatch);
        uint32_t acquired_image = m_device.acquireNextImageKHR(m_swap_chain, infinite_wait, vk::Semaphore(), m_next_image_ready, m_dispatch).value;
        // TODO: Deal with suboptimal or out-of-date swapchains

        // Get command buffers objects associated with this image.
        vk::CommandBuffer& command_buffer(m_command_buffers[acquired_image]);
        vk::Fence& command_fence(m_command_fences[acquired_image]);
        device_buffer& uniform_buffer(m_uniform_buffers[acquired_image]);
        vk::DescriptorSet& descriptor_set(m_simple_mutable_sets[acquired_image]);

        // Wait for the commands complete fence in order to record.
        m_device.waitForFences(command_fence, VK_FALSE, infinite_wait, m_dispatch);
        m_device.resetFences(command_fence, m_dispatch);

        // Now we can reset and record a new command buffer for this frame.
        command_buffer.reset(vk::CommandBufferResetFlags(), m_dispatch);

        vk::CommandBufferBeginInfo begin_info;
        begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        command_buffer.begin(begin_info, m_dispatch);

        vk::ClearValue clear_values[2];
        clear_values[0].color.setFloat32( {{0.0f, 0.0f, 0.0f, 1.0f}} );
        clear_values[1].depthStencil.setDepth(1.0f);
        clear_values[1].depthStencil.setStencil(0);

        vk::RenderPassBeginInfo pass_begin_info;
        pass_begin_info.renderPass = m_simple_render_pass;
        pass_begin_info.framebuffer = m_simple_framebuffers[acquired_image];
        pass_begin_info.renderArea.offset = vk::Offset2D(0, 0);
        pass_begin_info.renderArea.extent = m_swap_chain_extent;
        pass_begin_info.clearValueCount = _countof(clear_values);
        pass_begin_info.pClearValues = clear_values;
        command_buffer.beginRenderPass(pass_begin_info, vk::SubpassContents::eInline, m_dispatch);

        // Generate commands for per-frame but not per-draw work.
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_simple_pipeline, m_dispatch);

        // Setup to write the uniform buffer.
        // TODO: Keep the memory mapped all of the time.
        uint8_t* ubo_data = reinterpret_cast<uint8_t*>(m_device.mapMemory(uniform_buffer.device_memory, 0, per_frame_ubo_size, vk::MemoryMapFlags(), m_dispatch));
        uint32_t ubo_offset = 0;

        // Do all of the per-draw work.
        for (draw_record& d : m_draws) {
            // Bind geometry
            vk::DeviceSize zero_offset = 0;
            command_buffer.bindVertexBuffers(0, m_static_buffers[d.vbo].buffer, zero_offset, m_dispatch);
            command_buffer.bindIndexBuffer(m_static_buffers[d.ibo].buffer, zero_offset, vk::IndexType::eUint16, m_dispatch);

            // Fill out and bind dynamic state uniform buffer.
            uint32_t dynamic_ubo_offsets[1];

            // Update transform UBO field.
            *reinterpret_cast<glm::mat4*>(ubo_data + ubo_offset) = m_camera_transform * d.transform;
            dynamic_ubo_offsets[0] = ubo_offset;
            ubo_offset = align_up(ubo_offset + sizeof(glm::mat4), m_ubo_min_field_align);

            // TODO: Collapse this into a single bind call.
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_simple_pipeline_layout, 0, 1, &descriptor_set, _countof(dynamic_ubo_offsets), dynamic_ubo_offsets, m_dispatch);

            // Bind the immutable state.
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_simple_pipeline_layout, 1, 1, &d.immutable_state, 0, nullptr, m_dispatch);

            // Draw
            command_buffer.drawIndexed(d.index_count, 1, d.first_index, d.vertex_offset, 0, m_dispatch);
        }

        // Finish command buffer recording.
        m_device.unmapMemory(uniform_buffer.device_memory, m_dispatch);

        command_buffer.endRenderPass(m_dispatch);
        command_buffer.end(m_dispatch);

        // Now we need our aquired image to actually be ready.
        m_device.waitForFences(m_next_image_ready, VK_FALSE, infinite_wait, m_dispatch);

        // Submit work.
        vk::SubmitInfo submit_info;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        m_queue.submit(submit_info, command_fence, m_dispatch);

        // Present the texture.
        vk::PresentInfoKHR present_info;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &m_swap_chain;
        present_info.pImageIndices = &acquired_image;
        m_queue.presentKHR(present_info, m_dispatch);
    }

    // static
    void application::glfw_key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
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

        app; //app->m_log_stream

        error::dbg_out
            << "Vulkan debug report:" << std::endl
            << " Flags = " << vk::to_string(vk::DebugReportFlagBitsEXT(flags)) << std::endl
            << " Object Type = " << vk::to_string(vk::ObjectType(object_type)) << std::endl
            << " Layer Prefix = " << layer_prefix << std::endl
            << " Message = " << message << std::endl;

        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            BOOST_THROW_EXCEPTION(error::vk_debug_report_error()
                << error::errinfo_vk_debug_report_error_object_type(vk::to_string(vk::ObjectType(object_type)).c_str())
                << error::errinfo_vk_debug_report_error_object(object)
                << error::errinfo_vk_debug_report_error_location(location)
                << error::errinfo_vk_debug_report_error_message_code(message_code)
                << error::errinfo_vk_debug_report_error_layer_prefix(layer_prefix)
                << error::errinfo_vk_debug_report_error_message(message));
        }

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

#if defined(__clang__)
extern "C" {
    __attribute__((dllexport)) __attribute__((selectany)) uint32_t NvOptimusEnablement = 0x00000001;
}
#elif defined(_MSC_VER)
extern "C" {
    __declspec(dllexport, selectany) uint32_t NvOptimusEnablement = 0x00000001;
}
#else
#error How to declare NvOptimusEnablement with other compilers?
#endif
