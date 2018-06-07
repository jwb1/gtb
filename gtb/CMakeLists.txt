SET(BOOST_ROOT $ENV{PACKAGES_ROOT}/Boost)
SET(Boost_COMPILER "-mgw73") # Boost is built with MinGW GCC
FIND_PACKAGE(Boost REQUIRED COMPONENTS filesystem)

FIND_PACKAGE(Vulkan REQUIRED)

SET(GTB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/exception.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics.cpp
)
ADD_EXECUTABLE(gtb ${GTB_SOURCES})
TARGET_LINK_LIBRARIES(gtb glfw ${Boost_LIBRARIES} ${Vulkan_LIBRARIES})
TARGET_LINK_LIBRARIES(gtb -Wl,-subsystem,windows)
TARGET_INCLUDE_DIRECTORIES(gtb PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${Boost_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
)

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
