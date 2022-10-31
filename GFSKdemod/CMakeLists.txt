cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 17)

add_subdirectory(libdsp)

project(GFSKdemod)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

#set(CMAKE_CXX_FLAGS "-pthread -Wall -Wextra")
#set(CMAKE_CXX_FLAGS_DEBUG "-g")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(GFSKdemod main.cpp)
find_package(PkgConfig)
pkg_check_modules(VOLK REQUIRED volk)
target_include_directories(GFSKdemod PUBLIC ${VOLK_INCLUDE_DIRS})
link_directories(${VOLK_LIBRARY_DIRS})
target_link_libraries(GFSKdemod PUBLIC ${VOLK_LIBRARIES})
target_compile_options(GFSKdemod PUBLIC ${VOLK_CFLAGS_OTHER})

target_link_libraries(GFSKdemod PUBLIC dsp)
target_include_directories(GFSKdemod PUBLIC libdsp/src)