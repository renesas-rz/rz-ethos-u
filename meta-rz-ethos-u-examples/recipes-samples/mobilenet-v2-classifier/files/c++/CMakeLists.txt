cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
project(mobilenet-v2-classifier C CXX)

SET(EXE_NAME "mobilenet-v2-classifier")

file(GLOB SRC
	"main.cpp"
	"processing.cpp"
	"processing.h"
	"wayland_utils.h"
	"wayland_utils.cpp"
)

add_executable(${EXE_NAME} ${SRC})

# OpenCV
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENCV REQUIRED opencv4)
include_directories(${OPENCV_INCLUDE_DIRS})
link_directories(${OPENCV_LIBRARY_DIRS})

# XDG shell
find_library(XDG_SHELL_PROTOCOL xdg-shell-client-protocol REQUIRED)

# Link against TensorFlow Lite, OpenCV and wayland-client libraries
target_link_libraries(${EXE_NAME}
	tensorflow-lite
	${OPENCV_LIBRARIES}
	${XDG_SHELL_PROTOCOL}
	wayland-client
)

install(TARGETS ${EXE_NAME} DESTINATION "bin")
