cmake_minimum_required(VERSION 3.15)
project(DLS2SF2 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(dls2sf2
    src/src/dls2sf2.cpp
    src/src/dls2sf2_internal.cpp
    src/src/dls_parser.cpp
    src/src/dls_converter.cpp
    src/src/sf2_writer.cpp
)

target_include_directories(dls2sf2
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
        $<INSTALL_INTERFACE:include>
)

add_executable(dls2sf2_test
    test/test.cpp
)

target_link_libraries(dls2sf2_test
    PRIVATE
        dls2sf2
)

target_include_directories(dls2sf2_test
    PRIVATE
        src/include
)

enable_testing()
add_test(NAME dls2sf2_test COMMAND dls2sf2_test)
