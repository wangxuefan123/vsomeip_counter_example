cmake_minimum_required(VERSION 2.8.7)

project(vSomeIPServiceExample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11")
find_package(Threads  REQUIRED)
#include_directories("/home/ml/car/vsomeip")
#link_directories("/usr/local/lib")
include_directories(${VSOMEIP_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})
find_package(vsomeip3 REQUIRED)
add_executable(service-example  ../service-example.cpp)
target_link_libraries(service-example  vsomeip3 pthread)

add_executable(request-example ../request-example.cpp)
target_link_libraries(request-example  vsomeip3 pthread)

add_executable(subscribe-example ../subscribe-example.cpp)
target_link_libraries(subscribe-example  vsomeip3 pthread)
#add_executable  (client-example ../client-example.cpp)
#target_link_libraries(client-example  vsomeip ${Boost_LIBRARIES})
