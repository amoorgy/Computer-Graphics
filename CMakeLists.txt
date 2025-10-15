cmake_minimum_required(VERSION 3.31)
project(IcyTower)

set(CMAKE_CXX_STANDARD 20)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find GLFW
find_package(glfw3 REQUIRED)

# Find GLEW
find_package(GLEW REQUIRED)

# Add executable
add_executable(IcyTower main.cpp)

# Link libraries
target_link_libraries(IcyTower
    OpenGL::GL
    glfw
    GLEW::GLEW
)
