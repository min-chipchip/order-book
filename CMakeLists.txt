
# Require a CMake version that supports the target-based commands used below.
cmake_minimum_required(VERSION 3.16)

# Define the project and restrict it to C++ sources.
project(order_book LANGUAGES CXX)

# Build the application's executable from its entry point.
add_executable(order_book src/main.cpp)

# Make project headers available while compiling this executable. PRIVATE means
# the include path is not inherited by targets that link against order_book.
target_include_directories(order_book PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Require C++17 for this target without setting a global compiler flag.
target_compile_features(order_book PRIVATE cxx_std_17)

# Provide the BUILD_TESTING option and CTest integration for registered tests.
include(CTest)

# Let developers opt in to runtime memory and undefined-behavior checks.
option(ORDER_BOOK_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Apply strict, compiler-appropriate warning settings to a target.
function(order_book_enable_warnings target)
    if(MSVC)
        # /W4 enables a high warning level; /permissive- uses stricter standard conformance.
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        # Enable common, extra, and standard-conformance warnings on GCC/Clang.
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# Enable AddressSanitizer and UndefinedBehaviorSanitizer where the compiler supports them.
function(order_book_enable_sanitizers target)
    if(ORDER_BOOK_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        # Preserve stack frames to improve sanitizer diagnostics.
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        # Link the sanitizer runtimes into the final executable.
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

# Apply the shared quality checks to the application target.
order_book_enable_warnings(order_book)
order_book_enable_sanitizers(order_book)

# Create and register tests only when testing is enabled (BUILD_TESTING is ON).
if(BUILD_TESTING)
    # Let the tests directory define its targets relative to itself.
    add_subdirectory(tests)
endif()
