cmake_minimum_required(VERSION 3.1)

find_path(HTTP_PARSER_INCLUDE_DIR
    NAMES http_parser.h
    PATHS ${HTTP_PARSER_DIR}/include
)

find_library(HTTP_PARSER_LIBRARY
    NAMES libhttp_parser.so
    PATHS ${HTTP_PARSER_DIR}/lib
)

find_library(HTTP_PARSER_STATIC_LIBRARY
    NAMES libhttp_parser.a
    PATHS ${HTTP_PARSER_DIR}/lib
)

MESSAGE(STATUS "http_parser: ${HTTP_PARSER_INCLUDE_DIR} ${HTTP_PARSER_LIBRARY}")

mark_as_advanced(HTTP_PARSER_LIBRARY HTTP_PARSER_INCLUDE_DIR)

if (HTTP_PARSER_INCLUDE_DIR AND HTTP_PARSER_LIBRARY)
    add_library(http_parser SHARED IMPORTED GLOBAL)
    set_target_properties(http_parser PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${HTTP_PARSER_INCLUDE_DIR}"
        IMPORTED_LOCATION "${HTTP_PARSER_LIBRARY}"
    )

    add_library(http_parser_static STATIC IMPORTED GLOBAL)
    set_target_properties(http_parser_static PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${HTTP_PARSER_INCLUDE_DIR}"
        IMPORTED_LOCATION "${HTTP_PARSER_STATIC_LIBRARY}"
    )

endif(HTTP_PARSER_INCLUDE_DIR AND HTTP_PARSER_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HttpParser
    HTTP_PARSER_INCLUDE_DIR HTTP_PARSER_LIBRARY
)
