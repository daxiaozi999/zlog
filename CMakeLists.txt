cmake_minimum_required(VERSION 3.10)

# 项目信息
project(zlogging 
    VERSION 1.0.0
    DESCRIPTION "A simple logging library"
    LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)    # 可执行文件 -> bin/
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)    # 静态库 -> lib/
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)    # 共享库 -> lib/

# 添加源文件
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# 创建库
add_library(zlogging STATIC ${SOURCES})

# 设置头文件包含目录
target_include_directories(zlogging PUBLIC src)

# 编译测试程序
add_executable(example tests/example.cpp)
target_link_libraries(example zlogging)

add_executable(performance_test tests/performance_test.cpp)
target_link_libraries(performance_test zlogging)

# 设置编译选项
if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(zlogging PRIVATE -Wall -Wextra)
endif()

# 安装配置（可选）
install(TARGETS zlogging
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)

install(FILES src/zlogging.h
    DESTINATION include)