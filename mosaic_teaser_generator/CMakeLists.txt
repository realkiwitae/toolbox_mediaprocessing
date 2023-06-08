cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(mosaic-gen) #name of your project

add_executable(mosaic_gen main.cpp)

find_package( OpenCV 4.5.0 REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
target_compile_options(mosaic_gen PRIVATE -Wall -ggdb3)
target_link_libraries(mosaic_gen rt ${OpenCV_LIBS} pthread)
