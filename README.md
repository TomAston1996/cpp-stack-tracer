# CPP Stack Tracer

## Prerequisites and Initial Setup

- Install a `C++20` compatible compiler (GCC 10+, Clang 10+, MSVC 2019+).
- Install `CMake` version 3.20 or higher.
- Install `vcpkg` for dependency management:

```bash
cd ~
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
```

- Add `vcpkg` to your shell configuration (e.g., `.zshrc` or `.bashrc`):

```bash
export VCPKG_ROOT="<path_to_where_you_cloned_the_repo>/vcpkg"
export PATH="$VCPKG_ROOT:$PATH"
```

- Configure the project with `make`:
```bash
make configure
```

## Running the Application for MacOS

Build the application using:
```bash
make build
# OR make build BUILD_TYPE=Release
```

Build and run the application using:
```bash
make run
# OR make run BUILD_TYPE=Release
```

Test the application using:
```bash
make test
# OR make test BUILD_TYPE=Release
```