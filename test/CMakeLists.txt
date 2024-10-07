cmake_minimum_required(VERSION 3.12)

project(mqttTest)

ADD_DEFINITIONS( -O3 )

set(CMAKE_CXX_STANDARD 17)


include_directories("/home/wujiaxiang/work/mqtt/x86/include") 

link_directories("../build" "/home/wujiaxiang/work/mqtt/x86/lib/")

link_libraries(mqtt paho-mqtt3a  paho-mqtt3c pthread)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(mqttTest ${SOURCES})