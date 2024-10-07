cmake_minimum_required(VERSION 3.12)

project(mqtt)

ADD_DEFINITIONS( -O3 )

set(CMAKE_CXX_STANDARD 17)

# set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_PREFIX_PATH "~/work/mqtt/x86")

set(eclipse-paho-mqtt-c_INCLUDE_DIRS ~/work/mqtt/x86/include)
set(eclipse-paho-mqtt-c_LIBRARIES ~/work/mqtt/x86/lib/libpaho-mqtt3a.so) 

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(mqtt SHARED ${SOURCES})

target_include_directories(mqtt PRIVATE ${eclipse-paho-mqtt-c_INCLUDE_DIRS})

target_link_libraries(mqtt PRIVATE ${eclipse-paho-mqtt-c_LIBRARIES})