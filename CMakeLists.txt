cmake_minimum_required(VERSION 3.22)
project(6610P7)

set(CMAKE_CXX_STANDARD 17)
add_executable(6610P7 main.cpp)
target_link_libraries(6610P7 -lglut -lGL -lGLEW)
