add_executable(order_book_tests matching_engine_test.cpp matching_engine_replay.cpp)

target_include_directories(order_book_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(order_book_tests PRIVATE cxx_std_17)

order_book_enable_warnings(order_book_tests)
order_book_enable_sanitizers(order_book_tests)

add_test(NAME matching_engine COMMAND order_book_tests)
add_test(NAME order_book_cli COMMAND ${CMAKE_COMMAND}
    "-DORDER_BOOK=$<TARGET_FILE:order_book>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/order_book_cli_test.cmake")
