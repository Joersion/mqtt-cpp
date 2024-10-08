cmake_minimum_required(VERSION 3.12)

project(mqtt)

ADD_DEFINITIONS( -O3 )

set(CMAKE_CXX_STANDARD 17)

include_directories("~/work/mqtt/x86/include") 

link_directories("~/work/mqtt/x86/lib/")

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(mqtt SHARED ${SOURCES})

target_link_libraries(mqtt PRIVATE paho-mqtt3a-static)