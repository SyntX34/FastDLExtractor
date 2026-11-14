# FastDL Extractor `v1.3`

A fast, multi-threaded **Source Engine FastDL** downloader and `.bz2` extractor with both a **CLI** and a **PyQt6 GUI**.

---

## Features

- **Multi-threaded Downloads & Directory Crawling** — Crawls FastDL HTTP directory listings concurrently across multiple worker threads and downloads files in parallel.
- **Automatic `.bz2` Decompression** — Automatically detects `.bz2`-compressed assets on the FastDL server, downloads them, and extracts them to the target game directory.
- **Recursive Wildcard & Extension Filtering** — Supports full folder crawls (`maps/`), prefix matching (`maps/zm_*`), and extension/glob matching (`models/*.mdl`, `sound/*.wav`, `materials/*.vmt`).
- **Responsive Stop / Cancel** — Immediately aborts in-flight directory crawls and file downloads in the GUI.
- **Cross-Platform** — Uses **WinHTTP** on Windows (zero external HTTP dependencies) and **libcurl** on Linux/macOS.

---

## Supported Path Patterns

Both the **CLI** and **GUI** support the following path formats:

| Pattern | Behavior |
| :--- | :--- |
| `maps/de_dust2.bsp` | Downloads the exact file |
| `maps/` | Recursively downloads all matching resource types in `maps/` |
| `maps/zm_*` | Recursively downloads all files starting with `zm_` in `maps/` |
| `models/*.mdl` | Recursively downloads all `.mdl` files under `models/` |
| `sound/weapons/*.wav` | Recursively downloads all `.wav` files under `sound/weapons/` |
| `materials/*.vmt` | Recursively downloads all `.vmt` files under `materials/` |

---

## Usage

### CLI

```bash
# Download a single map
./fastdl_extractor -s 0 -d maps/de_dust2.bsp

# Download all .mdl files recursively from models/
./fastdl_extractor -s 0 -d "models/*.mdl"

# Download all zm_ maps with 8 threads
./fastdl_extractor -s 0 -t 8 -d "maps/zm_*"

# Show all CLI options
./fastdl_extractor --help
```

### GUI

```bash
python gui/fastdl_gui.py
```

1. Select or add a FastDL server configuration (URL, output game directory, resource types).
2. Enter one or more paths/patterns (e.g. `models/*.mdl`, `sound/*.wav`, `maps/`).
3. Click **Start Download** — click **Stop** at any time to cancel listing or downloading immediately.

---

## How to Compile

### Prerequisites

- **CMake** 3.16+
- **C++17 Compiler** (MSVC 2022 on Windows, GCC 9+ or Clang 10+ on Linux/macOS)
- **bzip2** (`libbz2-dev` on Linux, `bzip2` via Homebrew on macOS, or `vcpkg` on Windows)
- **libcurl** (Linux/macOS only; Windows uses native WinHTTP)
- **Python 3.10+** and **PyQt6** (for the GUI)

---

### Windows (MSVC + vcpkg)

#### 1. Build the CLI
```powershell
# Install bzip2 via vcpkg (if not already installed)
vcpkg install bzip2:x64-windows

# Configure and build
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:USERPROFILE\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```
The CLI binary will be at `build\Release\fastdl_extractor.exe`.

#### 2. Build the GUI Shared Library (`fastdl.dll`)
```powershell
.\gui\build_lib.ps1
```
Then run the GUI:
```powershell
pip install PyQt6
python gui\fastdl_gui.py
```

---

### Linux (Ubuntu / Debian)

#### 1. Install Dependencies
```bash
sudo apt update
sudo apt install -y build-essential cmake libcurl4-openssl-dev libbz2-dev python3-pip
pip install PyQt6
```

#### 2. Build the CLI
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

#### 3. Build the GUI Shared Library (`libfastdl.so`)
```bash
./gui/build_lib.sh
python3 gui/fastdl_gui.py
```

---

### macOS

#### 1. Install Dependencies
```bash
brew install cmake bzip2 curl
pip3 install PyQt6
```

#### 2. Build the CLI & GUI Library
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.ncpu)

./gui/build_lib.sh
python3 gui/fastdl_gui.py
```

---

## Community & Policies

- [Contributing Guidelines](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security Policy](SECURITY.md)
- [License](LICENSE)
