cmake_minimum_required(VERSION 3.20)

project(prophet_core VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
  ${PROJECT_SOURCE_DIR}/core
  ${PROJECT_SOURCE_DIR}/include
)

set(SOURCES
  core/main.cpp
  core/modules.cpp
  core/ipc_server.cpp
  core/daemon_api.cpp
)

set(HEADERS
  core/modules.hpp
  core/ipc_server.hpp
  core/daemon_api.hpp
)

add_executable(prophet_core
  ${SOURCES}
  ${HEADERS}
)
