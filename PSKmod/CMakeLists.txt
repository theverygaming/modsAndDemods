cmake_minimum_required(VERSION 3.1)

project(PSKmod)

add_executable(PSKmod main.cpp)
find_package(PkgConfig)
pkg_check_modules(VOLK REQUIRED volk)
target_include_directories(PSKmod PUBLIC ${VOLK_INCLUDE_DIRS})
link_directories(${VOLK_LIBRARY_DIRS})
target_link_libraries(PSKmod PUBLIC ${VOLK_LIBRARIES})
target_compile_options(PSKmod PUBLIC ${VOLK_CFLAGS_OTHER})