# This file lives beside matching_engine_test.cpp, so the source needs no path.
add_executable(order_book_tests matching_engine_test.cpp)

target_include_directories(order_book_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(order_book_tests PRIVATE cxx_std_17)

order_book_enable_warnings(order_book_tests)
order_book_enable_sanitizers(order_book_tests)

add_test(NAME matching_engine COMMAND order_book_tests)
