cmake_minimum_required (VERSION 3.7)

set (TARGET vkguide)

project (${TARGET})

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (EXECUTABLE_OUTPUT_PATH "../bin/")

file (GLOB INCLUDES "./include/*.h")
file (GLOB SOURCES "./src/*.cpp")

find_package (Vulkan REQUIRED)
find_package (fmt REQUIRED)
find_package (glm REQUIRED)
find_package (SDL2 REQUIRED)

add_executable (${TARGET} ${SOURCES} ${INCLUDES})

target_include_directories (${TARGET} PRIVATE ./include/)

target_link_libraries (${TARGET} PRIVATE Vulkan::Vulkan)
target_link_libraries (${TARGET} PRIVATE fmt::fmt)
target_link_libraries (${TARGET} PRIVATE glm::glm)
target_link_libraries (${TARGET} PRIVATE SDL2::SDL2)
