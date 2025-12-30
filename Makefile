# Compiler type and version, has been set up using brew install gcc
CXX := g++-15

# Build type (debug by default)
BUILD_TYPE ?= debug

# Compiler flags
# -Wall warns about many common issues
# -Wextra enables additional warnings
# -Wpedantic enforces strict compliance to the C++ standard
# -Weffc++ enables warnings based on Scott Meyers' "Effective C++" guidelines
# -Wconversion warns about implicit type conversions that may alter a value
# -Wsign-conversion warns about implicit conversions between signed and unsigned types
# -Werror turns all warnings into errors (NOT included here, but can be added if desired)
CXXFLAGS_COMMON := -std=c++20  -Wall -Weffc++ -Wextra -Wconversion -Wsign-conversion -Wpedantic -Werror

# Main source file (fixed)
MAIN := src/main.cpp

# Additional translation units to always compile and link (ADD HERE i.e. src/foo.cpp src/bar.cpp)
SOURCES :=

# Output directory
BUILD_DIR := build

# Debug flags - this includes debug symbols and disables optimizations
CXXFLAGS_DEBUG := -g -O0 -DDEBUG

# Release flags - this enables optimizations and disables debug symbols
CXXFLAGS_RELEASE := -O3 -DNDEBUG

# Select flags based on build type
ifeq ($(BUILD_TYPE),release)
	CXXFLAGS := $(CXXFLAGS_COMMON) $(CXXFLAGS_RELEASE)
	BUILD_DIR := build/release
else
	CXXFLAGS := $(CXXFLAGS_COMMON) $(CXXFLAGS_DEBUG)
	BUILD_DIR := build/debug
endif

EXE := $(notdir $(basename $(MAIN)))

# Build the program
.PHONY: build
build:
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(MAIN) $(SOURCES) -o $(BUILD_DIR)/$(EXE)


# Build and run the program
.PHONY: run
run:
	@$(MAKE) build BUILD_TYPE=$(BUILD_TYPE)
	@./$(BUILD_DIR)/$(EXE)


# Clean build artifacts and remove build directory. Run using: make clean
.PHONY: clean
clean:
	rm -rf build


.PHONY: format
format:
	find . -type f \( -name "*.cpp" -o -name "*.h" \) -exec clang-format -i {} +


TIDY_FLAGS = -- -std=c++20

.PHONY: lint
lint:
	find . -type f -name "*.cpp" -exec clang-tidy {} $(TIDY_FLAGS) \;

