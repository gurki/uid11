cmake_minimum_required( VERSION 3.28 FATAL_ERROR )

project( uid11-tests
    VERSION 0.1.0
    DESCRIPTION "uid11 C++ test suite"
    LANGUAGES CXX
)

include( FetchContent )

# Catch2 v3 (test framework)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE
)

# nlohmann/json (for parsing the shared test-vectors.json)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable( Catch2 nlohmann_json )

add_executable( ${PROJECT_NAME}
    test_uid11.cpp
)

target_compile_features( ${PROJECT_NAME}
    PRIVATE
        cxx_std_23
)

# Resolve the path to the shared test-vectors.json (repo root, two levels up)
set( UID11_TEST_VECTORS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../test-vectors.json" )

target_compile_definitions( ${PROJECT_NAME}
    PRIVATE
        UID11_TEST_VECTORS_PATH="${UID11_TEST_VECTORS_PATH}"
)

target_link_libraries( ${PROJECT_NAME}
    PRIVATE
        uid11
        Catch2::Catch2WithMain
        nlohmann_json::nlohmann_json
)

include( CTest )
include( Catch )
catch_discover_tests( ${PROJECT_NAME} )
