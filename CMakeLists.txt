cmake_minimum_required(VERSION 3.5)

project(tmidi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tmidi main.cpp ../stv/os.cpp)

target_link_libraries(tmidi PRIVATE asound)
