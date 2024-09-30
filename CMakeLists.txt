cmake_minimum_required(VERSION 3.12)

project(mqtt)

ADD_DEFINITIONS( -O3 )

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_PREFIX_PATH "~/work/mqtt/x86")

find_package(eclipse-paho-mqtt-c REQUIRED)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(mqtt ${SOURCES})

target_include_directories(mqtt PRIVATE ${Mqtt_INCLUDE_DIRS})

target_link_libraries(mqtt PRIVATE ${Mqtt_LIBRARIES})