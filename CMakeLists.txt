CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(gtb)

SET(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries")

# GLFW for Window Management
SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
SET(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
SET(GLFW_VULKAN_STATIC OFF CACHE BOOL "Use the Vulkan loader statically linked into application")
SET(GLFW_USE_HYBRID_HPG ON CACHE BOOL "Force use of high-performance GPU on hybrid systems")
ADD_SUBDIRECTORY(modules/glfw)

# Build the gtb program
ADD_SUBDIRECTORY(gtb)
