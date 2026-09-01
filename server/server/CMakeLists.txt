# 添加源文件
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 添加 server 模块的头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(server
    serverMain.cpp
    tcp_server.cpp
)
target_link_libraries(server ${OpenCV_LIBS})
target_link_libraries(server imgProcessor)
target_link_libraries(server pthread)