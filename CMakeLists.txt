cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 11)
project(Logger)

file(GLOB_RECURSE LOGGER_SOURCES "src/*.cpp" )

# 创建动态链接库
add_library(logger STATIC ${LOGGER_SOURCES})

# 设置库的头文件包含路径
target_include_directories(logger PUBLIC include)

# 设置动态库的版本信息
set_target_properties(logger PROPERTIES
    VERSION 1.0
    SOVERSION 1
)

# 创建测试程序
file(GLOB_RECURSE LOGGER_TEST_SOURCES "test/*.cpp")
add_executable(logger_test ${LOGGER_TEST_SOURCES})

# 链接日志库到测试程序
target_link_libraries(logger_test PRIVATE logger)

# 设置测试程序的头文件包含路径
target_include_directories(logger_test PRIVATE include)
