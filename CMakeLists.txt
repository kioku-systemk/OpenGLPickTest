cmake_minimum_required(VERSION 3.4.1)

project(OpenGLPickTest)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -Wall -fno-exceptions")  

add_subdirectory(third_party/glfw)
include_directories(third_party/glfw/include)

find_package(OpenGL REQUIRED)

include_directories(${GLFW_INCLUDE_DIR})
add_executable(picktest main.cpp glad/glad.c)
target_link_libraries(picktest glfw ${OPENGL_LIBRARY})
