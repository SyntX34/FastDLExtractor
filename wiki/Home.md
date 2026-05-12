# FastDL Tool Wiki

Welcome to the FastDL Tool documentation wiki. This tool helps you sync game files from FastDL servers to your local game installation.

## Available Languages

| Flag | Language | Link |
|------|----------|------|
| 🇬🇧 | **English** | [Full Documentation](#english) |
| 🇪🇸 | **Español** | [Documentación en español](#español) |
| 🇨🇳 | **中文** | [中文文档](#中文) |
| 🇷🇺 | **Русский** | [Документация на русском](#русский) |
| 🇩🇰 | **Dansk** | [Dansk dokumentation](#dansk) |
| 🇩🇪 | **Deutsch** | [Deutsche Dokumentation](#deutsch) |
| 🇫🇷 | **Français** | [Documentation française](#français) |
| 🇵🇱 | **Polski** | [Dokumentacja po polsku](#polski) |
| 🇵🇹 | **Português** | [Documentação em português](#português) |
| 🇹🇷 | **Türkçe** | [Türkçe belgelendirme](#türkçe) |
| 🇯🇵 | **日本語** | [日本語ドキュメント](#日本語) |
| 🇰🇷 | **한국어** | [한국어 문서](#한국어) |

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

## Русский

### Обзор

FastDL Tool — кроссплатформенное приложение для синхронизации игровых файлов с FastDL сервера.

### Особенности

| Функция | Описание |
|---------|---------|
| **Умная синхронизация** | Сравнивает файлы сервера с локальными — скачивает только недостающие |
| **Парсинг папок** | Рекурсивно сканирует и скачивает папки с сервера |
| **Фильтр префиксов** | Скачивает только файлы с определенным префиксом (например, `maps/zm_*`) |
| **Многопоточность** | Настраиваемые рабочие потоки (по умолчанию 4, максимум 16) |

### Установка

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Использование

```bash
# Интерактивный режим
FastDLTool

# Использовать сервер 0
FastDLTool -s 0

# Принудительная загрузка заново
FastDLTool -s 0 -f
```

---

## Dansk

### Oversigt

FastDL Tool er et tværkørende CLI og GUI-program til synkronisering af spilfiler fra en FastDL-server.

### Funktioner

| Funktion | Beskrivelse |
|----------|-------------|
| **Smart sync** | Sammenligner serverens filer med lokale filer — downloader kun hvad der mangler |
| **Mappe-scan** | Gennemløber og downloader mapper rekursivt fra FastDL-serveren |
| **Præfiksfilter** | Download kun filer med bestemt præfiks (f.eks. `maps/zm_*`) |

### Installation

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## Deutsch

### Übersicht

FastDL Tool ist eine plattformübergreifende CLI- und GUI-Anwendung zum Synchronisieren von Spieldateien von einem FastDL-Server.

### Funktionen

| Funktion | Beschreibung |
|----------|--------------|
| **Intelligente Synchronisation** | Vergleicht Serverdateien mit lokalen Dateien — lädt nur fehlende herunter |
| **Ordner-Durchsuchen** | Durchsucht und lädt Ordner rekursiv vom FastDL-Server |
| **Präfixfilter** | Lädt nur Dateien mit bestimmtem Präfix herunter (z.B. `maps/zm_*`) |

### Installation

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## Français

### Aperçu

FastDL Tool est une application CLI et GUI multiplateforme pour synchroniser les fichiers de jeu depuis un serveur FastDL.

### Fonctionnalités

| Fonction | Description |
|----------|-------------|
| **Synchronisation intelligente** | Compare les fichiers serveur avec les fichiers locaux — ne télécharge que ce qui manque |
| **Parcours de dossiers** | Liste et télécharge les dossiers de manière récursive depuis le serveur FastDL |
| **Filtre de préfixe** | Télécharge uniquement les fichiers correspondant au préfixe (ex: `maps/zm_*`) |

### Installation

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## Polski

### Przegląd

FastDL Tool to wieloplatformowe narzędzie CLI i GUI do synchronizacji plików gry z serwera FastDL.

### Funkcje

| Funkcja | Opis |
|---------|------|
| **Inteligentna synchronizacja** | Porównuje pliki serwera z lokalnymi — pobiera tylko brakujące |
| **Przeszukiwanie folderów** | Rekurencyjne przeszukiwanie i pobieranie folderów z serwera |
| **Filtr prefiksów** | Pobiera tylko pliki z określonym prefiksem (np. `maps/zm_*`) |

### Instalacja

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## Português

### Visão Geral

FastDL Tool é um aplicativo CLI e GUI multiplataforma para sincronizar arquivos de jogos de um servidor FastDL.

### Recursos

| Recurso | Descrição |
|---------|-----------|
| **Sincronização inteligente** | Compara arquivos do servidor com arquivos locais — baixa apenas o que falta |
| **Varredura de pastas** | Lista e baixa pastas recursivamente do servidor FastDL |
| **Filtro de prefixo** | Baixa apenas arquivos com determinado prefixo (ex: `maps/zm_*`) |

### Instalação

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## Türkçe

### Genel Bakış

FastDL Tool, bir FastDL sunucusundan yerel oyun kurulumunuza oyun dosyalarını senkronize etmek için çapraz platform bir CLI ve GUI uygulamasıdır.

### Özellikler

| Özellik | Açıklama |
|---------|----------|
| **Akıllı senkronizasyon** | Sunucu dosyalarını yerel dosyalarla karşılaştırır — sadece eksik olanları indirir |
| **Klasör tarama** | FastDL sunucusundan klasörleri tekrarlayarak tarar ve indirir |
| **Önek filtresi** | Sadece belirli önekle başlayan dosyaları indirir (örn: `maps/zm_*`) |

### Kurulum

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## 日本語

### 概要

FastDL Toolは、FastDLサーバーからローカルゲームインストールにゲームファイルを同期するためのクロスプラットフォームのCLIおよびGUIアプリケーションです。

### 機能

| 機能 | 説明 |
|------|------|
| **スマート同期** | サーバーファイルとローカルファイルを比較し、不足しているファイルのみをダウンロード |
| **フォルダクロール** | FastDLサーバーからフォルダを再帰的にリストしダウンロード |
| **プレフィックスフィルター** | 指定したプレフィックス（例：`maps/zm_*`）のファイルのみをダウンロード |

### インストール

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## 한국어

### 개요

FastDL Tool은 FastDL 서버에서 로컬 게임 설치로 게임 파일을 동기화하기 위한 크로스 플랫폼 CLI 및 GUI 애플리케이션입니다.

### 기능

| 기능 | 설명 |
|------|------|
| **스마트 동기화** | 서버 파일과 로컬 파일을 비교하여 누락된 파일만 다운로드 |
| **폴더 크롤링** | FastDL 서버에서 폴더를 재귀적으로 나열하고 다운로드 |
| **접두사 필터** | 지정된 접두사(`maps/zm_*` 등)가 있는 파일만 다운로드 |

### 설치

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev libbz2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## License

MIT License