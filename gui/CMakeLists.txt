cmake_minimum_required(VERSION 3.10)
project(ProphetGUI)

set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

# Add headers that use Q_OBJECT
set(HEADERS
    MainWindow.h
)

set(SOURCES
    main.cpp
    MainWindow.cpp
)

qt_wrap_cpp(MOC_SOURCES ${HEADERS}) # <- This line is for Qt5 style, optional in Qt6

add_executable(gui
    ${SOURCES}
    ${MOC_SOURCES}
    ${HEADERS}  # optional for IDE
)

target_link_libraries(gui Qt6::Widgets)
set_target_properties(gui PROPERTIES AUTOMOC TRUE) # <- This line is KEY
