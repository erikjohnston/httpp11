include_directories(.)

add_executable(Test test_main.cpp httpp11.cpp deferred.cpp http_parser.cpp)
target_link_libraries(Test Httpp11 Matrix)