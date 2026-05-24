# Currently in development

A simple hex editor with SDL3 to inspect and modify binary files.
## Requirements

- CMake
- A C++17 compiler
- [vcpkg](https://github.com/microsoft/vcpkg)
- SDL3
- SDL3_ttf

## Install Dependencies

After installing vcpkg, install the required libraries:

## macOS/Linux

```bash
./vcpkg install sdl3 sdl3-ttf
```

## Windows

```powershell
.\vcpkg install sdl3 sdl3-ttf
```

## Build on macOS/Linux

```bash
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build
```

Run:

```bash
./build/hexeditor
```

## Build on Windows

```powershell
cmake -B build `
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build
```

Run:

```powershell
.\build\Debug\hexeditor.exe
```

<img width="752" height="624" alt="Screenshot 2026-05-23 at 4 38 04 PM" src="https://github.com/user-attachments/assets/bb0ba570-c4be-44a4-a73f-893c1117f66a" />
