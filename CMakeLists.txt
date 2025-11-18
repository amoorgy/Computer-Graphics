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

# Copy asset files to build directory
configure_file(${CMAKE_SOURCE_DIR}/IcyTowerLogo.png ${CMAKE_BINARY_DIR}/IcyTowerLogo.png COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/game-over-417465.mp3 ${CMAKE_BINARY_DIR}/game-over-417465.mp3 COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/game-bonus-02-294436.mp3 ${CMAKE_BINARY_DIR}/game-bonus-02-294436.mp3 COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/you-win-sequence-1-183948.mp3 ${CMAKE_BINARY_DIR}/you-win-sequence-1-183948.mp3 COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/game-start-6104.mp3 ${CMAKE_BINARY_DIR}/game-start-6104.mp3 COPYONLY)
