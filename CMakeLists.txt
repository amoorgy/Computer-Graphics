cmake_minimum_required(VERSION 3.31)
project(IcyTower)

set(CMAKE_CXX_STANDARD 20)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find GLUT
if(APPLE)
    find_library(GLUT_LIBRARY GLUT)
    find_path(GLUT_INCLUDE_DIR GLUT/glut.h)
else()
    find_package(GLUT REQUIRED)
endif()

# Add executable
add_executable(IcyTower main.cpp)

# Link libraries
if(APPLE)
    target_link_libraries(IcyTower
        ${OPENGL_LIBRARIES}
        ${GLUT_LIBRARY}
    )
    target_include_directories(IcyTower PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIR})
else()
    target_link_libraries(IcyTower
        ${OPENGL_LIBRARIES}
        ${GLUT_LIBRARY}
    )
    target_include_directories(IcyTower PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
endif()
