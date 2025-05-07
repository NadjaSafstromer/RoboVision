cmake_minimum_required(VERSION 3.10)
project(ColourIdentify)

set(CMAKE_CXX_STANDARD 17)
set(OpenCV_DIR "D:/tools/opencv/build")

find_package(OpenCV REQUIRED)

add_executable(ColourIdentify ColourIdentify.cpp)
target_include_directories(ColourIdentify PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(ColourIdentify ${OpenCV_LIBS})
