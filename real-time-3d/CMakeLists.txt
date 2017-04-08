project(3d)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#########################################################
# FIND OPENCV
#########################################################
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})
add_definitions(${OpenCV_DEFINITIONS})
if(NOT OpenCV_FOUND)
    message(ERROR " OpenCV not found!")
endif(NOT OpenCV_FOUND)


target_link_libraries( ${PROJECT_NAME} ${OpenCV_LIBS} )
