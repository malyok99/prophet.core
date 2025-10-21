cmake_minimum_required(VERSION 3.10)
project(ProphetGUI)

set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

set(HEADERS
  MainWindow.hpp
  ModuleDelegate.hpp
)

set(SOURCES
  main.cpp
  MainWindow.cpp
  ModuleDelegate.cpp
)

qt_wrap_cpp(MOC_SOURCES ${HEADERS}) 

add_executable(gui
  ${SOURCES}
  ${MOC_SOURCES}
  ${HEADERS}
)

target_link_libraries(gui Qt6::Widgets)
set_target_properties(gui PROPERTIES AUTOMOC TRUE)
