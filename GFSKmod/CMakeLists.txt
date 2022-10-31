cmake_minimum_required(VERSION 3.1)

project(GFSKmod)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(GFSKmod main.cpp)
find_package(PkgConfig)
pkg_check_modules(VOLK REQUIRED volk)
target_include_directories(GFSKmod PUBLIC ${VOLK_INCLUDE_DIRS})
link_directories(${VOLK_LIBRARY_DIRS})
target_link_libraries(GFSKmod PUBLIC ${VOLK_LIBRARIES})
target_compile_options(GFSKmod PUBLIC ${VOLK_CFLAGS_OTHER})