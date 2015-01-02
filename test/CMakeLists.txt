include_directories(.)

add_executable(Test test_main.cpp httpp11.cpp collators.cpp utils.cpp)
target_link_libraries(Test httplib)