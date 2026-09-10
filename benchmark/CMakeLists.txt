# 压测程序
add_executable(bench_log bench_log.cc)

target_link_libraries(bench_log PRIVATE LouisLogLib)
target_include_directories(bench_log PRIVATE ${PROJECT_SOURCE_DIR}/src)

find_package(Threads REQUIRED)
target_link_libraries(bench_log PRIVATE Threads::Threads)