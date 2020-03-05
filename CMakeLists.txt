cmake_minimum_required(VERSION 3.14)
project(WaterSimulator)

set(CMAKE_CXX_STANDARD 11)

include_directories(/home/zsmeton/Dropbox/Coding/CS444/include)
include_directories(/home/zsmeton/Dropbox/Coding/CS444/SemesterProject/include)
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(WaterSimulator main.cpp ParticleSystem.h)
target_link_libraries(WaterSimulator SOIL3 glfw GLEW GL freetype)
