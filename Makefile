# -----------------------------
# CMake-driven build (macOS)
# -----------------------------

BUILD_TYPE ?= Debug

# Use clang++ to avoid libc++/libstdc++ mismatch with vcpkg-built deps
# When using GCC, ensure vcpkg is also using GCC to build dependencies to avoid ABI issues
CXX ?= /usr/bin/clang++

# CMake build directories
BUILD_DIR := build
CMAKE_BUILD_DIR := $(BUILD_DIR)/$(shell echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]')

# vcpkg toolchain (expects VCPKG_ROOT exported in your shell)
VCPKG_TOOLCHAIN := $(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake

# App target name from CMakeLists.txt
APP_TARGET := app

.PHONY: configure build run test clean format lint

# Configure step (generates build files)
configure:
	@mkdir -p $(CMAKE_BUILD_DIR)
	cmake -S . -B $(CMAKE_BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DCMAKE_TOOLCHAIN_FILE="$(VCPKG_TOOLCHAIN)"

# Build step
build: configure
	cmake --build $(CMAKE_BUILD_DIR) -j

# Build and run the app executable
run: build
	@./$(CMAKE_BUILD_DIR)/$(APP_TARGET)

# Build and run tests
test: build
	ctest --test-dir $(CMAKE_BUILD_DIR) --output-on-failure

# Clean build artifacts from build directory
clean:
	rm -rf $(BUILD_DIR)

# Formatting based on clang-format
format:
	find . -type f \( -name "*.cpp" -o -name "*.h" \) -exec clang-format -i {} +

# Linting based on clang-tidy
TIDY_FLAGS = -- -std=c++20
lint:
	find . -type f -name "*.cpp" -exec clang-tidy {} $(TIDY_FLAGS) \;
