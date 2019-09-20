# cmake_minimum_required(VERSION <specify CMake version here>)
project(test)
cmake_minimum_required(VERSION 2.8)
set(SOURCE_FILES main.cpp)
set(BOOST_ROOT "/usr/local/include/boost")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX FLAGS -fexec-charset=UTF-8)

set(BOOST_MIN_VERSION "1.71.0")

#include_directories(H:\\boost_1_71_0)
#[[include_directories(\\home\\boost_1_71_0)
#link_directories(H:\\boost_1_71_0\\libs)
link_directories(\\home\\boost_1_71_0\\libs)]]
aux_source_directory(. DIR_SRCS)
add_executable(test ${DIR_SRCS})#[[
target_link_libraries(test boost_thread-mt boost_system)]]