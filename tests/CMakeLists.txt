include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(googletest)

add_executable(bminor_tests
    lexer_test.cpp)
target_link_libraries(bminor_tests PRIVATE bminor_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(bminor_tests)
