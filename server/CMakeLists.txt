# cmake_minimum_required(VERSION 3.10)
# project(cv_project)




# add_executable(my_project main.cpp)

cmake_minimum_required(VERSION 3.10)
project(TcpImageServer)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)

# 包含 server 目录
add_subdirectory(server)
add_subdirectory(image_processing)
