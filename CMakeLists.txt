cmake_minimum_required(VERSION 3.1)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(http-parser-version "v2.3")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -Wno-c++98-compat -Wno-padded -Wno-exit-time-destructors -Wno-global-constructors")

find_package(HttpParser REQUIRED)
include_directories(SYSTEM ${HTTPPARSER_INCLUDE_DIRS})
include_directories(include)

add_library(httpp11Objects OBJECT src/httpp11.cpp src/collator.cpp src/buffer_view.cpp)
add_dependencies(httpp11Objects ${HTTPPARSER_DEPENDENCY})
target_compile_options(httpp11Objects PRIVATE "-std=c++11" "-fPIC")


add_library(httpp11 SHARED $<TARGET_OBJECTS:httpp11Objects>)
target_link_libraries(httpp11 HttpParser)
target_include_directories(httpp11 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_options(httpp11 PUBLIC "-std=c++11")


add_library(httpp11Static STATIC $<TARGET_OBJECTS:httpp11Objects>)
target_link_libraries(httpp11Static HttpParser)
target_include_directories(httpp11Static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_options(httpp11Static PUBLIC "-std=c++11")


add_subdirectory(test)

enable_testing ()
add_test (NAME MyTest COMMAND Test)

install(TARGETS httpp11 EXPORT httpp11 ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(EXPORT httpp11 DESTINATION lib/cmake/httpp11)
install(DIRECTORY include DESTINATION include)

