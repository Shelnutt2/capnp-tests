cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(addressbook)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES addressbook.c++ addressbook.capnp.c++)
add_executable(addressbook ${SOURCE_FILES})

find_package(CapnProto REQUIRED)
include_directories(${CAPNP_INCLUDE_DIRS})
target_link_libraries(addressbook ${CAPNP_LIBRARIES})


set(SOURCE_FILES dynamic_test.cpp)
add_executable(dynamic_test ${SOURCE_FILES})

target_link_libraries(dynamic_test ${CAPNP_LIBRARIES})

set(SOURCE_FILES dynamic_json_test.cpp)
add_executable(dynamic_json_test ${SOURCE_FILES})
target_link_libraries(dynamic_json_test ${CAPNP_LIBRARIES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCE_FILES dynamic_write_test.cpp)
add_executable(dynamic_write_test ${SOURCE_FILES})
target_link_libraries(dynamic_write_test ${CAPNP_LIBRARIES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCE_FILES dynamic_mmap_test.cpp)
add_executable(dynamic_mmap_test ${SOURCE_FILES})
target_link_libraries(dynamic_mmap_test ${CAPNP_LIBRARIES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})