cmake_minimum_required(VERSION 3.20)
project(cpp_stack_tracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add GCC/Clang warning flags here
# Compiler flags
# -Wall warns about many common issues
# -Wextra enables additional warnings
# -Wpedantic enforces strict compliance to the C++ standard
# -Weffc++ enables warnings based on Scott Meyers' "Effective C++" guidelines
# -Wconversion warns about implicit type conversions that may alter a value
# -Wsign-conversion warns about implicit conversions between signed and unsigned types
# -Werror turns all warnings into errors (NOT included here, but can be added if desired)
add_compile_options(
  -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
)

# Debug/Release flags
add_compile_definitions(
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

add_executable(app
  src/main.cpp
)

# Header files are in the include/ directory
target_include_directories(app PRIVATE include)

# Tests (GoogleTest via vcpkg)
enable_testing()
find_package(GTest CONFIG REQUIRED)

add_executable(tests
  tests/main_test.cpp
  tests/instrumentor_test.cpp
)
target_include_directories(tests PRIVATE include)
target_link_libraries(tests PRIVATE GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)
