cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(slowmotion) #name of your project

add_executable(slowmo main.cpp)

find_package( OpenCV 4.5.0 REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
target_compile_options(slowmo PRIVATE -Wall -ggdb3)
target_link_libraries(slowmo rt ${OpenCV_LIBS} pthread)
