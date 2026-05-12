# FastDL Tool Wiki

[![en](https://img.shields.io/badge/lang-en-blue.svg)](#fastdl-tool-wiki) [![es](https://img.shields.io/badge/lang-es-green.svg)](#fastdl-tool-wiki-es) [![zh](https://img.shields.io/badge/lang-zh-red.svg)](#fastdl-tool-wiki-zh)

---

## FastDL Tool Wiki (English)

### Table of Contents
1. [Overview](#overview)
2. [Features](#features)
3. [Installation](#installation)
4. [Configuration](#configuration)
5. [Usage](#usage)
6. [Download Modes](#download-modes)
7. [GUI Application](#gui-application)
8. [Troubleshooting](#troubleshooting)
9. [FAQ](#faq)

---

### Overview

FastDL Tool is a cross-platform CLI and GUI application for syncing game files from a FastDL server to your local game installation. It supports Source Engine games including Counter-Strike, Garry's Mod, Team Fortress 2, and more.

**Key Capabilities:**
- Smart sync: Only downloads missing files
- Multi-threaded downloads (1-16 threads)
- Automatic .bz2 decompression
- Fallback logic for compressed/uncompressed files
- Extension filtering
- Multiple server profiles

---

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

---

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

---

### Configuration

The configuration file is a JSON file typically located at `configs/servers.json`.

**Config Structure:**
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

**Field Reference:**

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique identifier for this server configuration |
| `name` | string | Display name shown in menus and logs |
| `fastdl_url` | string | Base URL of your FastDL HTTP server (trailing `/` optional) |
| `game_path` | string | Local path where downloaded files are written |
| `resource_types` | array | File extensions to allow (empty = all) |
| `download_paths` | object | Per-server list of paths to download |

---

### Usage

#### CLI Usage

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

# Download specific file
FastDLTool -s 0 -d maps/de_dust2.bsp

# Sync entire folder
FastDLTool -s 0 -d maps/

# Prefix filter (only zm_ maps)
FastDLTool -s 0 -d maps/zm_*

# Force re-download
FastDLTool -s 0 -f
```

#### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | All files downloaded successfully |
| 1 | Config error or bad arguments |
| 2 | One or more files failed after retries |

---

### Download Modes

#### Mode 1: Folder Sync (`"folder/"`)

Downloads all files in a directory recursively.

```json
"download_paths": {
    "server_id": ["maps/", "materials/", "models/", "sound/"]
}
```

**Requirements:** FastDL server must have directory listing enabled (`nginx autoindex on;` or Apache `Options +Indexes`).

#### Mode 2: Prefix Filter (`"folder/prefix*"`)

Downloads only files starting with the specified prefix.

```json
"download_paths": {
    "server_id": ["maps/zm_*", "models/v_*"]
}
```

#### Mode 3: Specific Files (`"path/to/file.ext"`)

Downloads exact file paths only.

```json
"download_paths": {
    "server_id": ["maps/de_dust2.bsp", "materials/custom/texture.vtf"]
}
```

#### Mode 4: Mixed

Combine different modes in the same list.

```json
"download_paths": {
    "server_id": ["maps/", "maps/de_dust2.bsp", "sound/ui/"]
}
```

---

### GUI Application

The GUI provides a user-friendly interface for managing servers and downloads.

**Features:**
- Server manager: Add, edit, remove server profiles
- Download tab: Select server, set threads, specify path
- Live log with color-coded status
- Dual progress bars (current file and overall)
- Settings tab: Configure library path and default options
- Dark theme (Catppuccin-inspired)

---

### Troubleshooting

#### Common Issues

**1. "Config not found" on startup**
- Solution: Edit the generated example config at `configs/servers.json` with your server details

**2. "No download paths configured"**
- Solution: Add paths to `download_paths` in your config or use `-d` flag to specify a path

**3. Download fails with "Failed after N attempts"**
- Check FastDL URL is accessible in browser
- Verify directory listing is enabled on server
- Ensure bzip2 is installed (Windows: `pacman -S mingw-w64-x86_64-bzip2`)

**4. Files not decompressing**
- Ensure bzip2 development libraries are installed during build
- Check the downloaded .bz2 files exist before extraction

**5. "libfastdl.so not found" (Linux)**
- Build the C++ library first: `./build_lib.sh`
- Or compile manually: `cmake -B build && cmake --build build`

**6. SSL/TLS errors on Windows**
- Ensure Windows is updated
- Check proxy settings if behind corporate firewall

**7. "Permission denied" saving config**
- Run with appropriate permissions
- Check directory write access

**8. GUI shows "Backend not loaded"**
- Build the C++ library first
- Verify `libfastdl.so`/`fastdl.dll` is in the GUI directory

---

### FAQ

**Q: What games are supported?**
A: Any Source Engine game with FastDL support: CS:Source, CS:GO, Garry's Mod, TF2, L4D2, etc.

**Q: How does the .bz2 fallback work?**
A: The tool first tries to download `file.ext.bz2`. If that fails (404), it tries `file.ext` directly.

**Q: Can I use this with a password-protected FastDL?**
A: Currently, the tool does not support authentication. The FastDL URL must be publicly accessible.

**Q: What file extensions should I use?**
A: Common Source Engine extensions:
- `.bsp` - Maps
- `.nav` - Navigation meshes
- `.mdl` - Models
- `.vtx` - Vertex data
- `.vvd` - Valve vertex data
- `.phy` - Physics data
- `.vtf` - Texture format
- `.vmt` - Valve material
- `.wav`, `.mp3` - Sounds

**Q: Why are some files skipped?**
A: Files are skipped if their extension isn't in `resource_types`. Empty `resource_types` = allow all.

---

## FastDL Tool Wiki (Español)

### Tabla de Contenidos
1. [Descripción General](#descripción-general)
2. [Características](#características)
3. [Instalación](#instalación)
4. [Configuración](#configuración)
5. [Uso](#uso)
6. [Modos de Descarga](#modos-de-descarga)
7. [Solución de Problemas](#solución-de-problemas)

---

### Descripción General

FastDL Tool es una aplicación multiplataforma para sincronizar archivos de juegos desde un servidor FastDL a tu instalación local. Soporta juegos Source Engine como Counter-Strike, Garry's Mod, Team Fortress 2, etc.

---

### Características

| Característica | Descripción |
|---------------|-------------|
| **Sincronización inteligente** | Solo descarga archivos que faltan localmente |
| **Rascado de carpetas** | Lista y descarga carpetas recursivamente |
| **Filtro de prefijo** | Solo descarga archivos que coinciden con prefijo |
| **Multihilo** | Trabajadores paralelos configurables (1-16) |
| **Auto-descompresión BZ2** | Descarga, descomprime y elimina el archivo .bz2 |
| **Lógica de respaldo** | Intenta .bz2 primero, luego archivo sin comprimir |
| **Filtro de extensiones** | Solo descarga tipos de archivo específicos |

---

### Instalación

**Windows:**
- CMake 3.16+
- Visual Studio 2019/2022 con carga de trabajo C++ o MinGW-w64

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

### Configuración

```json
{
    "global_game_path": "",
    "servers": [
        {
            "id": "mi_servidor",
            "name": "Servidor CS:Source",
            "fastdl_url": "http://fastdl.ejemplo.com/cstrike/",
            "game_path": "/ruta/al/juego/cstrike",
            "resource_types": [".bsp", ".mdl", ".wav", ".vtf"]
        }
    ],
    "download_paths": {
        "mi_servidor": ["maps/", "materials/"]
    }
}
```

---

### Uso

```bash
# Modo interactivo
FastDLTool

# Descargar usando servidor 0
FastDLTool -s 0 -t 8

# Forzar redescarga
FastDLTool -s 0 -f
```

---

### Solución de Problemas

**Los archivos no se descargan:**
- Verifica que la URL del FastDL es accesible
- Asegúrate de que `resource_types` incluye las extensiones correctas
- Comprueba permisos de escritura en `game_path`

**Error de bzip2:**
- Windows: `pacman -S mingw-w64-x86_64-bzip2` en MSYS2
- Linux: `sudo apt install libbz2-dev`

---

## FastDL Tool Wiki (中文)

### 目录
1. [概述](#概述)
2. [功能](#功能)
3. [安装](#安装)
4. [配置](#配置)
5. [使用方法](#使用方法)
6. [故障排除](#故障排除)

---

### 概述

FastDL Tool 是一個跨平台的 CLI 和 GUI 應用程式，用於從 FastDL 伺服器同步遊戲文件到本地安裝目錄。支援 Source 引擎遊戲，包括 Counter-Strike、Garry's Mod、Team Fortress 2 等。

---

### 功能

| 功能 | 描述 |
|------|------|
| **智能同步** | 只下載缺失的文件 |
| **多執行緒** | 可配置的並行下載 (1-16 執行緒) |
| **BZ2 自動解壓** | 下載 .bz2 後自動解壓 |
| **後備邏輯** | 優先嘗試 .bz2，如果失敗則下載原始檔案 |
| **副檔名過濾** | 只下載指定的檔案類型 |

---

### 安装

**Windows:**
- CMake 3.16+
- Visual Studio 2019/2022 或 MinGW-w64

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

### 配置

```json
{
    "servers": [
        {
            "id": "cs_server",
            "name": "CS 伺服器",
            "fastdl_url": "http://fastdl.example.com/cstrike/",
            "game_path": "C:/Steam/steamapps/common/Counter-Strike Source/cstrike",
            "resource_types": [".bsp", ".mdl", ".vtf", ".vmt"]
        }
    ],
    "download_paths": {
        "cs_server": ["maps/", "materials/", "models/"]
    }
}
```

---

### 使用方法

```bash
# 互動模式
FastDLTool

# 使用伺服器 0 下載
FastDLTool -s 0

# 使用 8 個執行緒下載
FastDLTool -s 0 -t 8

# 強制重新下載
FastDLTool -s 0 -f
```

---

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

MIT License - see [LICENSE](LICENSE) for details.

*Made by [SyntX](https://github.com/SyntX34)*