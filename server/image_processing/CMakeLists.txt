# 添加源文件
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 添加 image_processing 模块的头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(imgProcessor
    imageProcessor.cpp
    base64.cpp
)



target_link_libraries(imgProcessor ${OpenCV_LIBS})

# 链接 socket 库（Linux 下必需）
target_link_libraries(imgProcessor pthread)
