# Installation Guide

Detailed installation instructions for FastDL Tool.

## System Requirements

### Windows

- **CMake** 3.16 or later
- **Visual Studio** 2019/2022 with C++ workload, or **MinGW-w64** via MSYS2
- **bzip2** development package

#### Installing with Visual Studio (Recommended)

1. Install Visual Studio 2019/2022 with "Desktop development with C++" workload
2. Install CMake from https://cmake.org/download/
3. Ensure CMake is in your PATH

#### Installing with MinGW

1. Install MSYS2 from https://www.msys2.org/
2. Open MSYS2 MinGW 64-bit terminal
3. Run:
```bash
pacman -S mingw-w64-x86_64-bzip2 mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc
```

### Linux

#### Debian/Ubuntu

```bash
sudo apt update
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
```

#### Fedora/RHEL

```bash
sudo dnf install cmake gcc-c++ libcurl-devel bzip2-devel
```

#### Arch Linux

```bash
sudo pacman -S cmake gcc curl bzip2
```

### macOS

```bash
brew install cmake bzip2 curl python
```

## Building the CLI Tool

### Quick Build (Linux/macOS)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Windows (MinGW)

```cmd
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_C_COMPILER=C:\msys64\mingw64\bin\gcc.exe ^
      -DCMAKE_CXX_COMPILER=C:\msys64\mingw64\bin\g++.exe
cmake --build build -j4
```

### Windows (MSVC)

From Developer PowerShell:
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Building the GUI Application

### Linux/macOS

```bash
chmod +x build_gui.sh
./build_gui.sh
```

Output: `dist/FastDLTool`

### Windows

```powershell
.\build_gui.ps1
```

Output: `dist\FastDLTool.exe`

### GUI with MinGW (Windows)

```powershell
.\build_gui.ps1 -UseMinGW
```

## Manual Build Steps

If you prefer to compile each step manually:

### Step 1: Build the C++ library

```bash
# Linux/macOS
./build_lib.sh

# Windows
.\build_lib.ps1
```

### Step 2: Install Python dependencies

```bash
pip install PyQt6 pyinstaller
```

### Step 3: Run PyInstaller

```bash
pyinstaller fastdl_gui.spec --distpath dist --noconfirm --clean
```

## Verifying Installation

After building, verify the executable works:

```bash
# CLI
./build/FastDLTool --version

# GUI
./dist/FastDLTool
```

## Troubleshooting Build Issues

### "bzip2 not found" (Windows MinGW)

Make sure you installed the package in MSYS2:
```bash
pacman -S mingw-w64-x86_64-bzip2
```

### CMake configuration errors

1. Ensure you have write permissions to the build directory
2. Try deleting the `build` directory and running cmake again
3. Check that all dependencies are installed

### PyInstaller build fails

1. Ensure Python 3.10+ is installed
2. Run `pip install --upgrade PyQt6 pyinstaller`
3. Check that the C++ library was built successfully first

---

## Русский (Russian)

### Системные требования

**Windows:**
- CMake 3.16+
- Visual Studio 2019/2022 с C++ или MinGW-w64 через MSYS2

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
```

**macOS:**
```bash
brew install cmake bzip2 curl python
```

---

## Deutsch

### Systemanforderungen

**Windows:**
- CMake 3.16+
- Visual Studio 2019/2022 mit C++ oder MinGW-w64 über MSYS2

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
```

**macOS:**
```bash
brew install cmake bzip2 curl python
```

---

## Français

### Configuration requise

**Windows:**
- CMake 3.16+
- Visual Studio 2019/2022 avec C++ ou MinGW-w64 via MSYS2

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
```

**macOS:**
```bash
brew install cmake bzip2 curl python
```

---

## Español

### Requisitos del Sistema

**Windows:**
- CMake 3.16+
- Visual Studio 2019/2022 con C++ o MinGW-w64 vía MSYS2

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
```

**macOS:**
```bash
brew install cmake bzip2 curl python
```

---

## 中文

### 系统要求

**Windows:**
- CMake 3.16+
- Visual Studio 2019/2022 或 MinGW-w64 通过 MSYS2

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
```

**macOS:**
```bash
brew install cmake bzip2 curl python
```

---

## License

MIT License