include_directories(.)

add_executable(Test test_main.cpp httpp11.cpp http_response_parser.cpp utils.cpp)
target_link_libraries(Test httplib)