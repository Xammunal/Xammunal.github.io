cmake_minimum_required(VERSION 3.2)
project(TestTinyArkanoid)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "C:/Users/pa6o4/CLionProjects/arcan")

set(SOURCE_FILES main.cpp)
add_executable(TestTinyArkanoid ${SOURCE_FILES})

# target_link_libraries(${EXECUTABLE_NAME} ${MY_LIBRARIES})
