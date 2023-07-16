cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(enhancer) #name of your project

add_executable(enhancer main.cpp)

find_package( OpenCV 4.5.0 REQUIRED )

set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda-10.2) 
find_package(CUDA REQUIRED)

#inclue cuda
include_directories(${CUDA_INCLUDE_DIRS})
target_link_libraries(enhancer ${CUDA_LIBRARIES})

include_directories( ${OpenCV_INCLUDE_DIRS} )
target_compile_options(enhancer PRIVATE -Wall -ggdb3)
target_link_libraries(enhancer rt ${OpenCV_LIBS} pthread)
