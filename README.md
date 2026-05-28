# Hex Editor

A simple hex editor with SDL3 to inspect and modify binary files.

<img width="960" height="540" alt="demo1" src="https://github.com/user-attachments/assets/2fc95558-33be-475c-b0a1-9841a9efef81" />

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



