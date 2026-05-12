# FastDL Tool Wiki

Welcome to the FastDL Tool documentation wiki. This tool helps you sync game files from FastDL servers to your local game installation.

## Available Languages

- **[English](#english)** - Full documentation
- **[Español](#español)** - Documentación en español
- **[中文](#中文)** - 中文文档

---

## English

### Overview

FastDL Tool is a cross-platform CLI and GUI application for syncing game files from a FastDL server to your local game installation. It supports Source Engine games including Counter-Strike, Garry's Mod, Team Fortress 2, and more.

### Features

| Feature | Description |
|---------|-------------|
| **Smart sync** | Compares server file list against local files — only downloads what's missing |
| **Folder crawl** | Recursively lists and downloads folders from FastDL server |
| **Prefix filter** | Download only files matching a prefix (e.g., `maps/zm_*`) |
| **Multi-threaded** | Configurable parallel workers (default: 4, max: 16) |
| **BZ2 auto-extract** | Downloads .bz2, decompresses, deletes archive — transparent |
| **Fallback logic** | Tries .bz2 first, falls back to plain file if not found |
| **Extension filter** | Only downloads specified file types |
| **Multiple servers** | One config file, unlimited server profiles |
| **Live progress** | Per-file speed, ETA, overall progress bar |
| **Static binary** | Windows build links statically — no DLL hell |
| **GUI frontend** | PyQt6 dark-themed desktop app, compiles to single executable |

### Installation

#### Requirements

**Windows:**
- CMake 3.16+
- Visual Studio 2019/2022 with C++ workload OR MinGW-w64 via MSYS2
- bzip2 development package

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf install cmake gcc-c++ libcurl-devel bzip2-devel python3
```

**macOS (Homebrew):**
```bash
brew install cmake bzip2 curl python
```

#### Building the CLI Tool

```bash
# Linux/macOS
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Windows (MinGW)
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

#### Building the GUI Application

**Linux/macOS:**
```bash
chmod +x build_gui.sh
./build_gui.sh
```

**Windows (PowerShell):**
```powershell
.\build_gui.ps1
```

Or with MinGW:
```powershell
.\build_gui.ps1 -UseMinGW
```

### Configuration

The configuration file is a JSON file typically located at `configs/servers.json`.

```json
{
    "global_game_path": "",
    "servers": [
        {
            "id": "css_server1",
            "name": "CS:Source - My Server",
            "fastdl_url": "https://fastdl.example.com/cstrike/",
            "game_path": "C:/Program Files (x86)/Steam/steamapps/common/Counter-Strike Source/cstrike/download",
            "resource_types": [".bsp", ".nav", ".mdl", ".vtf", ".vmt", ".wav", ".mp3"]
        }
    ],
    "download_paths": {
        "css_server1": ["maps/", "materials/", "models/", "sound/"]
    }
}
```

### Usage

```
FastDLTool [OPTIONS]

  -c, --config  <path>   Config file (default: configs/servers.json)
  -s, --server  <index>  Server index 0-N (skips interactive selection)
  -d, --download <path>  Single file or folder to download (overrides config)
  -o, --output  <dir>    Output directory (default: game_path from config)
  -t, --threads <n>      Download threads (default: 4, max: 16)
  -f, --force            Force re-download even if file exists
  -h, --help             Show help
  -v, --version          Version info
```

**Examples:**
```bash
# Interactive mode
FastDLTool

# Sync missing files for server 0
FastDLTool -s 0

# Download with 8 threads
FastDLTool -s 0 -t 8

# Specific file
FastDLTool -s 0 -d maps/de_dust2.bsp

# Folder sync
FastDLTool -s 0 -d maps/

# Prefix filter (only zm_ maps)
FastDLTool -s 0 -d maps/zm_*

# Force re-download
FastDLTool -s 0 -f
```

### Download Modes

1. **Folder sync** (`"maps/"`) - Recursively download folder contents
2. **Prefix filter** (`"maps/zm_*"`) - Only files starting with prefix
3. **Specific files** (`"maps/de_dust2.bsp"`) - Exact file paths
4. **Mixed** - Combine modes in same list

### Troubleshooting

**1. "Config not found" on startup**
- Edit the generated example config at `configs/servers.json`

**2. "No download paths configured"**
- Add paths to `download_paths` in config or use `-d` flag

**3. Download fails with "Failed after N attempts"**
- Check FastDL URL is accessible in browser
- Verify directory listing is enabled on server
- Ensure bzip2 is installed

**4. GUI shows "Backend not loaded"**
- Build the C++ library first
- Verify `libfastdl.so`/`fastdl.dll` is in the GUI directory

### FAQ

**Q: What games are supported?**
A: Any Source Engine game: CS:Source, CS:GO, Garry's Mod, TF2, L4D2, etc.

**Q: How does the .bz2 fallback work?**
A: Tries `file.ext.bz2` first, if 404 then tries `file.ext` directly.

**Q: What file extensions should I use?**
A: `.bsp`, `.nav`, `.mdl`, `.vtx`, `.vvd`, `.phy`, `.vtf`, `.vmt`, `.wav`, `.mp3`

---

## Español

### Descripción General

FastDL Tool es una aplicación multiplataforma para sincronizar archivos de juegos desde un servidor FastDL.

### Características

| Característica | Descripción |
|---------------|-------------|
| **Sincronización inteligente** | Solo descarga archivos que faltan localmente |
| **Rascado de carpetas** | Lista y descarga carpetas recursivamente |
| **Filtro de prefijo** | Solo descarga archivos que coinciden con prefijo |
| **Multihilo** | Trabajadores paralelos configurables (1-16) |
| **Auto-descompresión BZ2** | Descarga, descomprime y elimina el archivo .bz2 |

### Instalación

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Uso

```bash
# Modo interactivo
FastDLTool

# Usar servidor 0
FastDLTool -s 0

# Forzar redescarga
FastDLTool -s 0 -f
```

### Solución de Problemas

**Los archivos no se descargan:**
- Verifica que la URL del FastDL es accesible
- Asegúrate de que `resource_types` incluye las extensiones correctas
- Comprueba permisos de escritura en `game_path`

**Error de bzip2:**
- Windows: `pacman -S mingw-w64-x86_64-bzip2` en MSYS2
- Linux: `sudo apt install libbz2-dev`

---

## 中文

### 概述

FastDL Tool 是一個跨平台的應用程式，用於從 FastDL 伺服器同步遊戲文件。

### 功能

| 功能 | 描述 |
|------|------|
| **智能同步** | 只下載缺失的文件 |
| **多執行緒** | 可配置的並行下載 (1-16) |
| **BZ2 自動解壓** | 下載 .bz2 後自動解壓 |

### 安装

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### 使用方法

```bash
# 互動模式
FastDLTool

# 使用伺服器 0
FastDLTool -s 0

# 強制重新下載
FastDLTool -s 0 -f
```

### 故障排除

**無法下載文件：**
- 檢查 FastDL URL 是否可訪問
- 確認 `resource_types` 包含正確的副檔名
- 檢查 `game_path` 的寫入權限

**bzip2 錯誤：**
- Windows: 在 MSYS2 中執行 `pacman -S mingw-w64-x86_64-bzip2`
- Linux: `sudo apt install libbz2-dev`

**配置文件找不到：**
- 首次運行會自動生成範例配置
- 請編輯 `configs/servers.json` 填入您的伺服器信息

---

## License

MIT License