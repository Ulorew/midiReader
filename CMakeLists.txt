cmake_minimum_required(VERSION 3.22)
project(midiReader)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finput-charset=CP1251 -fexec-charset=CP866")
add_executable(midiReader main.cpp)
