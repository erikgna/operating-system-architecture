cmake_minimum_required(VERSION 3.0.0)
project(EXT3 VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)

include(CTest)
enable_testing()

find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)
pkg_search_module(GTEST REQUIRED gtest)

if( OPENSSL_FOUND )
    include_directories(${OPENSSL_INCLUDE_DIRS})
    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
endif()

if( GTest_FOUND )
    include_directories(${GTEST_INCLUDE_DIRS})
    message(STATUS "Using GTest ${GTEST_VERSION}")
endif()

add_executable(EXT3 main.cpp fs.cpp sha256.cpp)
target_link_libraries(EXT3 gtest crypto pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
include(CPack)