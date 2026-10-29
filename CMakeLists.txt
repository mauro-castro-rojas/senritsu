cmake_minimum_required(VERSION 3.25)

project(senritsu VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

add_library(senritsu_core STATIC
    src/senritsu/core/transport.cpp
)

target_include_directories(senritsu_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_features(senritsu_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(senritsu_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(senritsu_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(senritsu_transport_tests
        tests/transport_tests.cpp
    )

    target_link_libraries(senritsu_transport_tests PRIVATE senritsu_core)

    add_test(
        NAME senritsu.transport
        COMMAND senritsu_transport_tests
    )
endif()
