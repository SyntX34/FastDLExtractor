# Configuration Guide

Understanding and configuring FastDL Tool.

## Configuration File Location

The configuration file is typically located at:
- **Default:** `configs/servers.json`
- **GUI:** `~/.fastdltool/servers.json`

## Basic Configuration Structure

```json
{
    "global_game_path": "./game",
    "servers": [],
    "download_paths": {}
}
```

## Server Configuration

Each server in the `servers` array requires:

```json
{
    "id": "unique_identifier",
    "name": "Display Name",
    "fastdl_url": "http://fastdl.example.com/game/",
    "game_path": "/local/game/path",
    "resource_types": [".bsp", ".mdl", ".vtf"]
}
```

### Field Reference

| Field | Required | Description |
|-------|----------|-------------|
| `id` | Yes | Unique identifier; must match key in `download_paths` |
| `name` | Yes | Display name in menus and logs |
| `fastdl_url` | Yes | Base URL of FastDL server (trailing `/` optional) |
| `game_path` | Yes | Local directory for downloaded files |
| `resource_types` | No | File extensions to download (empty = all) |

## Download Paths

Configure which files to download for each server:

```json
{
    "download_paths": {
        "server_id": ["maps/", "materials/", "models/weapons/v_knife.mdl"]
    }
}
```

## Complete Example

```json
{
    "global_game_path": "",
    "servers": [
        {
            "id": "csgo_public",
            "name": "CS:GO Public Server",
            "fastdl_url": "https://fastdl.example.com/csgo/",
            "game_path": "C:/Steam/steamapps/common/csgo/csgo",
            "resource_types": [
                ".bsp", ".nav", ".ain",
                ".mdl", ".vtx", ".vvd", ".phy",
                ".vtf", ".vmt", ".pcf",
                ".wav", ".mp3"
            ]
        },
        {
            "id": "tf2_event",
            "name": "TF2 Event Server",
            "fastdl_url": "http://tf2.fastdl.com/server1/",
            "game_path": "/home/user/tf2/tf",
            "resource_types": [".bsp", ".nav", ".mdl", ".vtf", ".vmt"]
        }
    ],
    "download_paths": {
        "csgo_public": [
            "maps/",
            "maps/awp_*",
            "materials/models/weapons/",
            "sound/"
        ],
        "tf2_event": [
            "maps/pl_badwater.bsp",
            "maps/cp_dustbowl.bsp",
            "materials/"
        ]
    }
}
```

## Common Resource Types

### Counter-Strike / Source Games
```
.bsp      - Maps
.nav      - Navigation meshes
.ain      - AI navigation
.mdl      - Models
.vtx      - Vertex data
.vvd      - Valve vertex data
.phy      - Physics data
.vtf      - Texture format
.vmt      - Valve material
.pcf      - Particle systems
.wav      - Sound files
.mp3      - Sound files
```

### Garry's Mod
```
.bsp      - Maps
.nav      - Navigation
.gma      - Addons (when available)
```

## Download Modes Explained

### 1. Folder Sync
```
"maps/"           - All files in maps directory
"materials/"      - All materials recursively
```

### 2. Prefix Filter
```
"maps/zm_*"       - Only zombie maps
"models/v_*"      - Only view models
```

### 3. Specific Files
```
"maps/de_dust2.bsp"
"sound/buttons/button1.wav"
```

### 4. Mixed Mode
```
"maps/",            - All maps
"maps/de_inferno.bsp", - Plus specific map
"sound/ui/"          - Plus UI sounds
```

## Using Global Game Path

Set a default output directory:

```json
{
    "global_game_path": "C:/Steam/steamapps/common/Counter-Strike Source/cstrike",
    "servers": [
        {
            "id": "server1",
            "game_path": ""  // Uses global_game_path
        }
    ]
}
```

## Environment Variables

The GUI stores settings in:
- **Windows:** Registry (`HKEY_CURRENT_USER\Software\SyntX34\FastDLTool`)
- **Linux/macOS:** `~/.config/SyntX34/FastDLTool.conf` (QSettings)