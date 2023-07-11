cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(watermark) #name of your project

add_executable(watermark main.cpp)

find_package( OpenCV 4.5.0 REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
target_compile_options(watermark PRIVATE -Wall -ggdb3)
target_link_libraries(watermark rt ${OpenCV_LIBS} pthread)
