# fastdl_gui.spec
# PyInstaller spec file for FastDL Tool GUI
#
# How to build:
#   Windows : run build_gui.ps1  (handles C++ lib + PyInstaller automatically)
#   Linux   : run build_gui.sh   (handles C++ lib + PyInstaller automatically)
#   macOS   : run build_gui.sh   (handles C++ lib + PyInstaller automatically)
#
# Or manually (after the C++ lib is already built):
#   pip install pyinstaller PyQt6
#   pyinstaller fastdl_gui.spec

import sys
import os
from pathlib import Path

# ─── Locate the compiled C++ backend ──────────────────────────────────────────
_here = Path(SPECPATH)  # directory that contains this .spec file

def _find_lib():
    """Return (src_path, dest_name) for the shared library, or raise."""
    candidates = {
        "Windows": [
            _here / "fastdl.dll",
            _here / "build-win" / "Release" / "fastdl.dll",
        ],
        "Linux": [
            _here / "libfastdl.so",
            _here / "build" / "libfastdl.so",
        ],
        "Darwin": [
            _here / "libfastdl.dylib",
            _here / "build" / "libfastdl.dylib",
        ],
    }
    platform_name = {
        "win32":  "Windows",
        "linux":  "Linux",
        "darwin": "Darwin",
    }.get(sys.platform, "Linux")

    for p in candidates.get(platform_name, []):
        if p.exists():
            return str(p), p.name
    raise FileNotFoundError(
        f"C++ backend library not found for {platform_name}.\n"
        f"Run build_lib.sh (Linux/macOS) or build_lib.ps1 (Windows) first.\n"
        f"Looked in: {[str(c) for c in candidates.get(platform_name, [])]}"
    )

lib_src, lib_name = _find_lib()

# ─── Analysis ─────────────────────────────────────────────────────────────────
a = Analysis(
    [str(_here / "fastdl_gui.py")],
    pathex=[str(_here)],
    binaries=[
        # Bundle the shared library; PyInstaller copies it next to the exe
        (lib_src, "."),
    ],
    datas=[],
    hiddenimports=[
        # PyQt6 sub-modules that PyInstaller sometimes misses
        "PyQt6.QtCore",
        "PyQt6.QtGui",
        "PyQt6.QtWidgets",
        "PyQt6.sip",
        # stdlib modules used at runtime
        "ctypes",
        "ctypes.util",
        "json",
        "threading",
        "pathlib",
        "platform",
        "time",
    ],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[
        # Trim unused heavy packages to keep the exe smaller
        "matplotlib",
        "numpy",
        "pandas",
        "PIL",
        "scipy",
        "tkinter",
        "unittest",
        "email",
        "html",
        "http",
        "urllib",
        "xml",
        "xmlrpc",
    ],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=None,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=None)

# ─── EXE / bundle ─────────────────────────────────────────────────────────────
exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name="FastDLTool",          # output executable name (no extension needed)
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,                   # compress with UPX if available (optional)
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,              # False = no console window (GUI-only app)
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    # Windows-only: embed a manifest and set the app icon
    uac_admin=False,
    # icon="gui/icon.ico",      # uncomment and provide an icon file if desired
)

# ─── macOS .app bundle (ignored on other platforms) ───────────────────────────
# Uncomment if you want a proper .app on macOS:
# app = BUNDLE(
#     exe,
#     name="FastDLTool.app",
#     icon=None,
#     bundle_identifier="com.syntx34.fastdltool",
# )
