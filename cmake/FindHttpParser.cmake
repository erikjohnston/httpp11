cmake_minimum_required(VERSION 3.1)


if (NOT HTTPPARSER_FIND_VERSION)
    set(HTTPPARSER_FIND_VERSION "2.3")
endif (NOT HTTPPARSER_FIND_VERSION)

include(ExternalProject)

if (NOT HTTPPARSER_EXTERNAL_PREFIX)
    set (HTTPPARSER_EXTERNAL_PREFIX ${CMAKE_SOURCE_DIR}/externals/http_parser)
endif (NOT HTTPPARSER_EXTERNAL_PREFIX)

# Right, lets download it then!
ExternalProject_Add (
    http-parser-proj
    GIT_REPOSITORY "git@github.com:joyent/http-parser.git"
    GIT_TAG "v${HTTPPARSER_FIND_VERSION}"
    INSTALL_DIR ${HTTPPARSER_EXTERNAL_PREFIX}
    UPDATE_COMMAND ""
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND cmake -E make_directory <INSTALL_DIR>/lib
        COMMAND cmake -E make_directory <INSTALL_DIR>/include
    BUILD_COMMAND make package library
    INSTALL_COMMAND cmake -E copy libhttp_parser.a <INSTALL_DIR>/lib/
        COMMAND cmake -E copy libhttp_parser.so.${HTTPPARSER_FIND_VERSION} <INSTALL_DIR>/lib/
        COMMAND cmake -E copy http_parser.h <INSTALL_DIR>/include/
)

# Set up all the different variables pkg_check_modules would
set(HTTPPARSER_INCLUDE_DIRS ${HTTPPARSER_EXTERNAL_PREFIX}/include)
set(HTTPPARSER_LIBRARY_DIRS ${HTTPPARSER_EXTERNAL_PREFIX}/lib)
set(HTTPPARSER_LIBRARIES httpparser)
set(HTTPPARSER_LDFLAGS)
set(HTTPPARSER_LDFLAGS_OTHER)
set(HTTPPARSER_CFLAGS)
set(HTTPPARSER_CFLAGS_OTHER)
set(HTTPPARSER_VERSION ${HTTPPARSER_FIND_VERSION})
set(HTTPPARSER_DEPENDENCY http-parser-proj)
set(HTTPPARSER_FOUND True)
set(HTTPPARSER_LIBDIR ${HTTPPARSER_EXTERNAL_PREFIX}/lib)

set(extra_libs ${HTTPPARSER_LIBRARIES})
list(REMOVE_ITEM extra_libs uv)
MESSAGE(STATUS ${extra_libs})

if (HTTPPARSER_FOUND)
    add_library(HttpParser SHARED IMPORTED GLOBAL)
    set(
        HTTPPARSER_LIBRARY
        "${HTTPPARSER_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}http_parser${CMAKE_SHARED_LIBRARY_SUFFIX}.${HTTPPARSER_FIND_VERSION}"
    )
    set_target_properties(HttpParser PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${HTTPPARSER_INCLUDE_DIRS}"
        IMPORTED_LOCATION "${HTTPPARSER_LIBRARY}"
    )

    add_library(HttpParserStatic STATIC IMPORTED GLOBAL)
    set(
        HTTPPARSER_STATIC_LIBRARY
        "${HTTPPARSER_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}http_parser${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )
    set_target_properties(HttpParserStatic PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${HTTPPARSER_STATIC_INCLUDE_DIRS}"
        IMPORTED_LOCATION "${HTTPPARSER_STATIC_LIBRARY}"
    )
endif(HTTPPARSER_FOUND)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    HttpParser DEFAULT_MSG
    HTTPPARSER_LIBRARIES HTTPPARSER_INCLUDE_DIRS
)
