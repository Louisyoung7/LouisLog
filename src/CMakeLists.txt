# 创建静态链接库
add_library(LouisLogLib STATIC LouisLog.cc)

target_include_directories(LouisLogLib PRIVATE ${PROJECT_SOURCE_DIR}/src)

