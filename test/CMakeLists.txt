# 查找GTest库
find_package(GTest REQUIRED)

# 创建测试程序
add_executable(log_lib_test test_log.cc)

# 链接日志库到测试程序
target_link_libraries(log_lib_test PRIVATE LouisLogLib GTest::GTest GTest::Main)

# 设置测试程序的头文件包含路径
target_include_directories(log_lib_test PRIVATE ${PROJECT_SOURCE_DIR}/src)