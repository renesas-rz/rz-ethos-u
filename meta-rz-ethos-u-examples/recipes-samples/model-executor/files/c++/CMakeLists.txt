cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 17)
project(model-executor C CXX)

SET(EXE_NAME "model-executor")

file(GLOB SRC
	"main.cpp"
	"yaml_parser.h"
	"yaml_parser.cpp"
)

add_executable(${EXE_NAME} ${SRC})

# libtensorflow-lite.so and its headers are provided by the
# ethos-u-linux-driver-stack recipe (DEPENDS).
find_library(TENSORFLOW_LITE_LIBRARY NAMES tensorflow-lite REQUIRED)

target_link_libraries(${EXE_NAME} PRIVATE yaml-cpp ${TENSORFLOW_LITE_LIBRARY})

install(TARGETS ${EXE_NAME} DESTINATION "bin")
