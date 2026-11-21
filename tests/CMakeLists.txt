include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(googletest)

add_executable(bminor_tests
    examples_test.cpp
    highlighter_test.cpp
    lexer_test.cpp
    parser_test.cpp)
target_link_libraries(bminor_tests PRIVATE bminor_core GTest::gtest_main)
target_compile_definitions(bminor_tests PRIVATE BMINOR_EXAMPLES_DIR="${PROJECT_SOURCE_DIR}/examples")

include(GoogleTest)
gtest_discover_tests(bminor_tests)

add_test(NAME cli-ast COMMAND bminor ast ${PROJECT_SOURCE_DIR}/examples/factorial.bminor)
add_test(NAME cli-html COMMAND bminor highlight --html ${PROJECT_SOURCE_DIR}/examples/showcase.bminor)
