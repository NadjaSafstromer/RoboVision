cmake_minimum_required(VERSION 3.10)
project(RoboVision)


set(CMAKE_CXX_STANDARD 17)
set(OpenCV_DIR "D:/tools/opencv/build")
set(CMAKE_TOOLCHAIN_FILE "D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")


find_package(OpenCV REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

# ZMQ
include_directories("D:/tools/vcpkg/installed/x64-windows/include")
include_directories("D:/tools/libzmq_mingw/include")

# Lägg till rätt plats för lib-filer
link_directories("D:/tools/libzmq_msvc_build/lib/Release")

add_executable(ColourIdentify ColourIdentify.cpp)
target_include_directories(ColourIdentify PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(ColourIdentify ${OpenCV_LIBS})

add_executable(VievVideo VievVideo.cpp)
target_include_directories(VievVideo PRIVATE ${OpenCV_INCLUDE_DIRS})
# ZMQ rätt namn enligt din mapp: libzmq-v143-mt-4_3_6.lib
target_link_libraries(VievVideo ${OpenCV_LIBS} libzmq-v143-mt-4_3_6 nlohmann_json::nlohmann_json)



#add_executable(Client client.cpp)
#target_link_libraries(Client zmq)
