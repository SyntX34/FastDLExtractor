# Frequently Asked Questions

## General Questions

### Q: What games are supported?

**A:** FastDL Tool works with any Source Engine game that uses FastDL:
- Counter-Strike 1.6 / Source / Global Offensive
- Team Fortress 2
- Garry's Mod
- Day of Defeat: Source
- Half-Life 2: Deathmatch
- Left 4 Dead / Left 4 Dead 2
- Portal / Portal 2
- Any community mod using FastDL

### Q: What operating systems are supported?

**A:**
- **Windows:** 10/11 (MSVC or MinGW builds)
- **Linux:** Most distributions (Ubuntu, Fedora, Arch, etc.)
- **macOS:** 10.15+

---

## Configuration Questions

### Q: What file extensions should I use?

**A:** Common Source Engine extensions:

| Extension | Type | Description |
|-----------|------|-------------|
| `.bsp` | Maps | Compiled map files |
| `.nav` | Navigation | AI navigation meshes |
| `.ain` | Navigation | Updated navigation format |
| `.mdl` | Model | Model meshes |
| `.vtx` | Model | Vertex data |
| `.vvd` | Model | Valve vertex data |
| `.phy` | Model | Physics collision data |
| `.vtf` | Texture | Valve texture format |
| `.vmt` | Material | Valve material definitions |
| `.pcf` | Particles | Particle system files |
| `.wav` | Sound | Audio files |
| `.mp3` | Sound | Audio files |

### Q: Can I use relative paths in config?

**A:** Yes! Both `game_path` and `download_paths` support relative paths:
```json
{
    "game_path": "./csgo",
    "fastdl_url": "http://server.com/csgo/"
}
```

### Q: Can I use environment variables in paths?

**A:** Not directly. You would need to use the CLI `-o` option:
```bash
FastDLTool -s 0 -o $GAME_PATH
```

---

## Download Questions

### Q: How does the .bz2 fallback work?

**A:** The tool uses this logic for each file:
1. Try to download `filename.ext.bz2`
   - If 200 OK → download, decompress, done
   - If 404 → try step 2
2. Try to download `filename.ext`
   - If 200 OK → download as-is
   - If 404 → error after retries

This means you don't need to know which files are compressed on the server.

### Q: Can I download individual files only?

**A:** Yes, use the specific file mode or `-d` flag:
```bash
FastDLTool -s 0 -d maps/de_dust2.bsp
FastDLTool -s 0 -d materials/models/weapons/v_models/v_knife.mdl
```

### Q: Can I exclude certain file types?

**A:** Yes, remove them from `resource_types`:
```json
{
    "resource_types": [".bsp", ".nav", ".mdl"],  // Only these types
    "resource_types": []  // All types (default)
}
```

### Q: Why are my files being skipped?

**A:** Files are skipped when:
1. Their extension isn't in `resource_types` (if list is not empty)
2. They already exist locally (without `-f` flag)
3. They failed the prefix filter

---

## Building Questions

### Q: Do I need Python to run the CLI version?

**A:** No! The CLI tool is pure C++ and doesn't require Python. Python is only needed for:
- Building the GUI application
- Running the GUI source directly (without PyInstaller)

### Q: Can I build a portable executable?

**A:** Yes! Use the provided build scripts:
- `build_gui.sh` / `build_gui.ps1` creates a single-file executable
- The executable bundles the C++ library and all Python dependencies

### Q: How do I build with static linking (Windows)?

**A:** The provided CMakeLists.txt already configures static linking for MinGW. The resulting `.exe` won't need any DLLs.

---

## Network Questions

### Q: Can I use this with a password-protected FastDL?

**A:** Currently not supported. The FastDL URL must be publicly accessible without authentication.

### Q: Does it support HTTPS?

**A:** Yes, both HTTP and HTTPS are supported.

### Q: Can I use a proxy?

**A:** System proxy settings may work, but explicit proxy configuration is not currently supported.

### Q: What about rate limiting?

**A:** The tool uses standard HTTP requests. If your server has rate limiting, you may need to:
- Reduce thread count: `FastDLTool -s 0 -t 2`
- Contact your server administrator

---

## Troubleshooting Questions

### Q: Where are logs stored?

**A:** CLI output goes to stdout/stderr. For GUI, logs appear in the application's log window.

### Q: How can I get more verbose output?

**A:** The tool shows file-by-file progress by default. For very large file lists, only summary is shown.

### Q: What do the exit codes mean?

| Code | Meaning |
|------|---------|
| 0 | All files downloaded successfully |
| 1 | Configuration error or bad arguments |
| 2 | One or more files failed after all retries |

---

## Contributing

### Q: How can I report a bug?

**A:** Open an issue at [GitHub Issues](https://github.com/SyntX34/FastDLExtractor/issues) with:
- Your OS and version
- Build method used
- Full error message
- Steps to reproduce

### Q: Can I contribute translations?

**A:** Yes! Pull requests for additional language documentation are welcome.