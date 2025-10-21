cmake_minimum_required(VERSION 3.20)

project(prophet_core)
set(CMAKE_CXX_STANDARD 23)

include_directories(include)

file(GLOB SOURCES "core/*.cpp")

add_executable(prophet_core ${SOURCES})
