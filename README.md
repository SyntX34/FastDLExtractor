# FastDL Tool

A cross-platform CLI tool for downloading files from a FastDL server (commonly used with Source Engine games like Garry's Mod, CS:Source, TF2, etc.). Supports `.bz2` decompression, multi-threaded downloads, multiple server configs, and live progress display.

---

## Features

- **Multi-threaded downloads** — configurable parallel workers (default: 4)
- **BZ2 decompression** — automatically detects and extracts `.bz2` files (FastDL convention)
- **Fallback logic** — tries `.bz2` first, falls back to plain file if not found
- **Multiple server profiles** — config file holds as many servers as you want
- **Interactive server selection** — prompts if you have multiple servers
- **Live progress bar** — per-file progress, speed, ETA, overall bar
- **Extension filtering** — only download the file types you want
- **Pre-configured download lists** — per-server lists of paths in the config
- **Single static binary** on Windows (MinGW `-static` flags, no DLL dependencies)

---

## Building

### Windows (MinGW-w64)

**Prerequisites:**
- [MinGW-w64](https://github.com/niXman/mingw-builds-binaries/releases) installed at `C:\mingw64`
- [CMake](https://cmake.org/download/) in your PATH
- `libbz2` — included in most MinGW-w64 distributions. If missing:
  - MSYS2: `pacman -S mingw-w64-x86_64-bzip2`
  - Or download from [bzip2.org](https://sourceware.org/bzip2/)

```bat
cmake -B build_win -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_C_COMPILER=C:\mingw64\bin\gcc.exe ^
      -DCMAKE_CXX_COMPILER=C:\mingw64\bin\g++.exe
cmake --build build_win -j4
```

### Linux

**Prerequisites:**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev   # Debian/Ubuntu
sudo dnf install cmake gcc-c++ libcurl-devel bzip2-devel     # Fedora/RHEL
sudo pacman -S cmake gcc curl bzip2                           # Arch
```


---

## Configuration

On first run without a config, an example `configs/servers.json` is created:

```json
{
    "global_game_path": "",
    "servers": [
        {
            "id": "gmod_server1",
            "name": "Garry's Mod — My Server",
            "fastdl_url": "http://fastdl.example.com/garrysmod/",
            "game_path": "C:/Program Files (x86)/Steam/steamapps/common/GarrysMod/garrysmod",
            "resource_types": [".bsp", ".mdl", ".vtx", ".vvd", ".phy", ".wav", ".mp3", ".vtf", ".vmt"]
        }
    ],
    "download_paths": {
        "gmod_server1": [
            "maps/gm_flatgrass.bsp",
            "materials/myserver/logo.vtf"
        ]
    }
}
```

### Config fields

| Field | Description |
|-------|-------------|
| `id` | Unique ID for this server (used as key in `download_paths`) |
| `name` | Human-readable display name |
| `fastdl_url` | Base URL of your FastDL server |
| `game_path` | Where to copy downloaded files (informational — the tool tells you) |
| `resource_types` | Extensions to allow (empty = allow everything) |
| `download_paths` | Per-server list of relative paths to download |

---

## Usage

```
FastDLTool [OPTIONS]

  -c, --config  <path>   Config file  (default: configs/servers.json)
  -s, --server  <index>  Server index, 0-based (skips interactive prompt)
  -d, --download <path>  Download a specific relative path
  -o, --output  <dir>    Output directory (default: downloads/)
  -t, --threads <n>      Parallel download threads (default: 4)
  -h, --help             This help text
  -v, --version          Version info
```

### Examples

```bash
# Interactive — choose server from list, uses pre-configured download_paths
FastDLTool

# Download one specific file from server 0, 8 threads
FastDLTool -s 0 -d maps/de_dust2.bsp -t 8

# Use a different config, download to a specific directory
FastDLTool -c ~/myserver.json -s 1 -o ~/Downloads/fastdl

# Download a whole directory of maps (requires paths in config)
FastDLTool -s 0 -o C:/Games/gmod/garrysmod
```

### Exit codes

| Code | Meaning |
|------|---------|
| `0`  | All files downloaded successfully |
| `1`  | Config error / bad arguments |
| `2`  | One or more files failed to download |

---

## How BZ2 works

FastDL servers typically compress files with bzip2 (`.mdl` → `.mdl.bz2`).

1. Tool requests `file.bz2` from the server
2. If the server returns 200 OK → downloads and decompresses to `file`
3. If the server returns 404 → retries the plain `file`
4. Decompression is **streaming** (no memory limit on large files)

---

## Project structure

```
FastDLTool/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp              # CLI entry point, progress display
│   ├── FastDLDownloader.h/cpp # Thread pool, HTTP (WinHTTP/curl), BZ2
│   ├── ConfigManager.h/cpp   # JSON config load/save
│   ├── ProgressBar.h         # Animated progress bar
│   └── Utils.h               # formatBytes, formatSpeed, formatDuration
└── third_party/
    └── nlohmann/
        └── json.hpp          # Bundled minimal JSON (no external dep)
```
