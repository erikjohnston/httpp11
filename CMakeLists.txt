cmake_minimum_required(VERSION 2.8)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include(ExternalProject)

ExternalProject_Add (
    http-parser-proj
    GIT_REPOSITORY "git@github.com:joyent/http-parser.git"
    GIT_TAG "master"
#    DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/download/http_parser
#    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals/http_parser
#    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/build/http_parser
    INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals/http_parser
    BUILD_IN_SOURCE 1
#    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/http-parser
    CONFIGURE_COMMAND cmake -E make_directory <INSTALL_DIR>/lib 
        COMMAND cmake -E make_directory <INSTALL_DIR>/include
    BUILD_COMMAND make package
    INSTALL_COMMAND cmake -E copy libhttp_parser.a <INSTALL_DIR>/lib/
        COMMAND cmake -E copy http_parser.h <INSTALL_DIR>/include/
)

ExternalProject_Get_Property(http-parser-proj install_dir)

add_library(http-parser STATIC IMPORTED)
set_property(TARGET http-parser PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libhttp_parser.a)
add_dependencies(http-parser http-parser-proj)

set(HTTP_PARSER_INCLUDE_DIR ${install_dir}/include)
include_directories(${HTTP_PARSER_INCLUDE_DIR})

include_directories(include/)
add_library(httpp11 src/httpp11.cpp src/http_response_parser.cpp)
target_link_libraries(httpp11 http-parser)

add_executable(myapp src/main.cpp)
target_link_libraries(myapp httpp11)

include_directories(${HTTP_PARSER_INCLUDE_DIR})
