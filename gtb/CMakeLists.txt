# Set up package dependencies.
SET(BOOST_ROOT $ENV{PACKAGES_ROOT}/Boost)
SET(Boost_COMPILER "-mgw73") # Boost is built with MinGW GCC
FIND_PACKAGE(Boost REQUIRED COMPONENTS filesystem)

FIND_PACKAGE(Vulkan REQUIRED)

# Compile and link the gtb program.
SET(GTB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/gtb.cpp)
ADD_EXECUTABLE(gtb ${GTB_SOURCES})
TARGET_LINK_LIBRARIES(gtb glfw ${Boost_LIBRARIES} ${Vulkan_LIBRARIES} pthread)
TARGET_LINK_LIBRARIES(gtb -Wl,-subsystem,windows)
TARGET_INCLUDE_DIRECTORIES(gtb PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${Boost_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS(gtb PRIVATE GTB_ENABLE_VULKAN_DEBUG_LAYER)

# Compile shaders.
SET(GTB_SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.frag)

FOREACH(GLSL ${GTB_SHADERS})
    GET_FILENAME_COMPONENT(GLSL_NAME ${GLSL} NAME)
    SET(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/${GLSL_NAME}.spv")
    ADD_CUSTOM_COMMAND(OUTPUT ${SPIRV} DEPENDS ${GLSL}
        COMMAND "$ENV{VULKAN_SDK}/Bin/glslc.exe" -c -Werror --target-env=vulkan1.1 -o ${SPIRV} ${GLSL})
    LIST(APPEND GTB_SPIRV_FILES ${SPIRV})
ENDFOREACH(GLSL)

ADD_CUSTOM_TARGET(gtb_shaders DEPENDS ${GTB_SPIRV_FILES})
ADD_DEPENDENCIES(gtb gtb_shaders)

# Copy all of the required shared libraries next to the newly built binary.
GET_FILENAME_COMPONENT(GTB_BOOST_FILESYSTEM_DLL_NAME "${Boost_FILESYSTEM_LIBRARY_RELEASE}" NAME)
GET_FILENAME_COMPONENT(GTB_BOOST_SYSTEM_DLL_NAME "${Boost_SYSTEM_LIBRARY_RELEASE}" NAME)

ADD_CUSTOM_COMMAND(TARGET gtb POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$ENV{PACKAGES_ROOT}/MinGW/bin/libstdc++-6.dll"
        "${CMAKE_CURRENT_BINARY_DIR}/libstdc++-6.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$ENV{PACKAGES_ROOT}/MinGW/bin/libgcc_s_seh-1.dll"
        "${CMAKE_CURRENT_BINARY_DIR}/libgcc_s_seh-1.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$ENV{PACKAGES_ROOT}/MinGW/bin/libwinpthread-1.dll"
        "${CMAKE_CURRENT_BINARY_DIR}/libwinpthread-1.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLFW_BINARY_DIR}/src/glfw3.dll"
        "${CMAKE_CURRENT_BINARY_DIR}/glfw3.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Boost_FILESYSTEM_LIBRARY_RELEASE}"
        "${CMAKE_CURRENT_BINARY_DIR}/${GTB_BOOST_FILESYSTEM_DLL_NAME}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Boost_SYSTEM_LIBRARY_RELEASE}"
        "${CMAKE_CURRENT_BINARY_DIR}/${GTB_BOOST_SYSTEM_DLL_NAME}")
