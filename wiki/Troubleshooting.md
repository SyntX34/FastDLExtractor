# Troubleshooting

Common issues and solutions for FastDL Tool.

## Table of Contents

1. [Configuration Errors](#configuration-errors)
2. [Download Errors](#download-errors)
3. [Build Errors](#build-errors)
4. [GUI Issues](#gui-issues)
5. [Platform-Specific Issues](#platform-specific-issues)

---

## Configuration Errors

### "Config not found" on startup

**Cause:** No configuration file exists at the default location.

**Solution:**
1. The tool creates an example config automatically on first run
2. Edit `configs/servers.json` with your server details
3. Or specify a custom config: `FastDLTool -c /path/to/config.json`

### "No servers found in config"

**Cause:** The configuration file exists but has no servers defined.

**Solution:** Add at least one server to the `servers` array:
```json
{
    "servers": [
        {
            "id": "my_server",
            "name": "My Game Server",
            "fastdl_url": "http://fastdl.example.com/game/",
            "game_path": "/path/to/game",
            "resource_types": [".bsp", ".mdl"]
        }
    ]
}
```

### "No download paths configured"

**Cause:** Server is selected but no `download_paths` defined.

**Solution:**
1. Add paths to config:
```json
"download_paths": {
    "my_server": ["maps/", "materials/"]
}
```
2. Or use CLI: `FastDLTool -s 0 -d maps/`

---

## Download Errors

### "Failed after N attempts"

**Causes and Solutions:**

1. **FastDL URL inaccessible**
   - Verify URL works in browser
   - Check network/firewall settings
   - Try `curl` or `wget` to test connectivity

2. **Directory listing disabled**
   - Folder mode requires `nginx: autoindex on;` or `Apache: Options +Indexes`
   - Use specific file mode instead

3. **Wrong file path**
   - Verify paths match server structure
   - Check for case sensitivity on Linux

4. **bzip2 not installed**
   - Windows (MSYS2): `pacman -S mingw-w64-x86_64-bzip2`
   - Linux: `sudo apt install libbz2-dev`

### Files downloading but 0 bytes

**Causes:**
- Empty directory on server
- Permission denied on server (403)
- Incorrect resource_types filtering

**Solution:**
- Check the URL in browser
- Remove or expand `resource_types` in config

### ".bz2 download failed, falling back to plain file"

This is **normal behavior** when compressed files don't exist. The tool tries `.bz2` first (common for FastDL), then falls back to the uncompressed file.

---

## Build Errors

### "bzip2 not found" (Windows MinGW)

```
CMake Error: bzlib.h not found
```

**Solution:**
```bash
# In MSYS2 MinGW 64-bit terminal:
pacman -S mingw-w64-x86_64-bzip2
```

### "libcurl not found" (Linux)

**Solution:**
```bash
# Debian/Ubuntu:
sudo apt install libcurl4-openssl-dev

# Fedora/RHEL:
sudo dnf install libcurl-devel
```

### "Could not find bz2" (macOS)

```bash
brew install bzip2
```

If still failing, you may need:
```bash
export CFLAGS="-I$(brew --prefix bzip2)/include"
export LDFLAGS="-L$(brew --prefix bzip2)/lib"
cmake -B build
```

### PyInstaller "No module named 'PyQt6'"

**Solution:**
```bash
pip install PyQt6
# Or create a virtual environment first:
python -m venv venv
source venv/bin/activate  # Linux/macOS
# or: venv\Scripts\activate  # Windows
pip install PyQt6 pyinstaller
```

---

## GUI Issues

### "Backend not loaded" or "libfastdl.so not found"

**Cause:** The C++ library hasn't been built.

**Solution:**
1. Build the C++ library first:
   ```bash
   # Linux/macOS
   ./build_lib.sh
   
   # Windows
   .\build_lib.ps1
   ```
2. Verify the library exists:
   - Linux: `gui/libfastdl.so`
   - Windows: `gui/fastdl.dll`
   - macOS: `gui/libfastdl.dylib`

### GUI opens but buttons are disabled

**Cause:** Backend library failed to load.

**Solution:**
1. Check the library file exists in the GUI directory
2. Rebuild the library
3. Check error output in terminal when running GUI

### "Permission denied" saving config

**Solution:**
- Run with appropriate permissions
- Check that `~/.fastdltool/` directory is writable
- On Linux/macOS: `chmod 755 ~/.fastdltool`

---

## Platform-Specific Issues

### Windows Path Issues

**Problem:** Paths with spaces cause issues.

**Solution:** The tool handles spaces automatically. Just use quotes in config:
```json
"game_path": "C:/Program Files (x86)/Steam/steamapps/common/Counter-Strike Source/cstrike"
```

### Windows UTF-8 Console

The executable is built with UTF-8 support embedded. If you see garbled characters:
1. Ensure you're using the built executable (not running from Visual Studio)
2. Check Windows locale settings

### Linux Permission Denied

```bash
# If you get permission errors:
chmod +x build/FastDLTool
```

### macOS "Library not loaded"

```bash
# If the GUI fails to start:
export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH
./dist/FastDLTool
```

---

## Getting More Help

1. Check the existing [GitHub Issues](https://github.com/SyntX34/FastDLExtractor/issues)
2. Include your OS, build method, and full error message when reporting issues
3. Run with maximum verbosity if available

---

## Español

### Errores de Configuración

**"Config not found" al iniciar:**
- Edite el archivo `configs/servers.json` generado automáticamente

**Los archivos no se descargan:**
- Verifique que la URL del FastDL es accesible
- Asegúrese de que `resource_types` incluye las extensiones correctas

### Errores de Compilación

**Error bzip2 (Windows MinGW):**
```bash
pacman -S mingw-w64-x86_64-bzip2
```

---

## 中文

### 配置错误

**配置文件未找到：**
- 编辑 `configs/servers.json`

### 下载错误

**下载失败：**
- 检查 FastDL URL 是否可访问
- 确认 `resource_types` 包含正确的副檔名

### 构建错误

**bzip2 错误：**
```bash
# Windows: pacman -S mingw-w64-x86_64-bzip2
# Linux: sudo apt install libbz2-dev
```

---

## Русский

### Ошибки конфигурации

**"Config not found":**
- Отредактируйте файл `configs/servers.json`

### Ошибки загрузки

**Загрузка не удалась:**
- Проверьте доступность FastDL URL
- Убедитесь в правильности путей

---

## License

MIT License