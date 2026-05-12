#!/usr/bin/env bash
# build_gui.sh  –  Full GUI build for Linux / macOS
#
# What it does:
#   1. Builds libfastdl.so / libfastdl.dylib  (C++ backend)
#   2. Installs PyInstaller + PyQt6 into a local venv (no system pollution)
#   3. Runs PyInstaller with fastdl_gui.spec → dist/FastDLTool  (single file)
#
# Usage:
#   chmod +x build_gui.sh
#   ./build_gui.sh
#
# The resulting executable is at:   dist/FastDLTool
# Copy it anywhere – it carries the C++ library and all Python deps inside.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║   FastDL Tool  –  Build GUI executable (one-file)   ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

OS="$(uname -s)"
echo "  Platform : $OS"
echo ""


check_dep() {
    command -v "$1" &>/dev/null || {
        echo "  [ERROR] '$1' not found. Install it and re-run."
        exit 1
    }
}

check_dep cmake
check_dep g++
check_dep python3

if [ "$OS" = "Linux" ]; then
    echo "  Checking libbz2-dev and libcurl..."
    pkg-config --exists libbz2 2>/dev/null || {
        echo "  [ERROR] libbz2 not found."
        echo "  Install: sudo apt install libbz2-dev libcurl4-gnutls-dev"
        exit 1
    }
fi

echo "  ┌─ Step 1/3 : Building C++ backend ─────────────────────────────────┐"

THIRD="$SCRIPT_DIR/third_party/nlohmann"
if [ ! -f "$THIRD/json.hpp" ]; then
    echo "  Downloading nlohmann/json.hpp..."
    mkdir -p "$THIRD"
    curl -fsSL \
        "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp" \
        -o "$THIRD/json.hpp"
fi

GUI_SRC="$SCRIPT_DIR/gui/src"
mkdir -p "$GUI_SRC"
for f in FastDLDownloader.cpp FastDLDownloader.h \
          ConfigManager.cpp    ConfigManager.h \
          Utils.h ProgressBar.h fastdl_capi.cpp fastdl_capi.h; do
    src="$SCRIPT_DIR/src/$f"
    dst="$GUI_SRC/$f"
    if [ -f "$src" ] && [ ! -f "$dst" ]; then
        cp "$src" "$dst"
        echo "    Copied $f → gui/src/"
    fi
done

mkdir -p "$SCRIPT_DIR/gui/third_party"
cp -r "$THIRD" "$SCRIPT_DIR/gui/third_party/" 2>/dev/null || true

BUILD_DIR="$SCRIPT_DIR/gui/build"
mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR" > /dev/null

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_BUILD_SHARED_LIBS=ON \
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="$SCRIPT_DIR/gui"

cmake --build . --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
popd > /dev/null

if [ "$OS" = "Linux" ]; then
    LIB_OUT="$SCRIPT_DIR/gui/libfastdl.so"
elif [ "$OS" = "Darwin" ]; then
    LIB_OUT="$SCRIPT_DIR/gui/libfastdl.dylib"
else
    LIB_OUT="$SCRIPT_DIR/gui/fastdl.dll"
fi

if [ ! -f "$LIB_OUT" ]; then
    FOUND="$(find "$BUILD_DIR" -name "libfastdl.*" | head -1)"
    if [ -n "$FOUND" ]; then
        cp "$FOUND" "$LIB_OUT"
    else
        echo "  [ERROR] C++ library not found after build. Check CMake output above."
        exit 1
    fi
fi
echo "  └─ Library: $LIB_OUT"
echo ""

echo "  ┌─ Step 2/3 : Installing Python dependencies ───────────────────────┐"

VENV="$SCRIPT_DIR/.venv-build"
if [ ! -d "$VENV" ]; then
    python3 -m venv "$VENV"
fi

source "$VENV/bin/activate"

pip install --quiet --upgrade pip
pip install --quiet PyQt6 pyinstaller

echo "  └─ Python deps ready (venv: .venv-build)"
echo ""

echo "  ┌─ Step 3/3 : Compiling GUI → single-file executable ───────────────┐"

cp "$SCRIPT_DIR/fastdl_gui.spec" "$SCRIPT_DIR/gui/fastdl_gui.spec" 2>/dev/null || true

cd "$SCRIPT_DIR"

pyinstaller fastdl_gui.spec \
    --distpath "$SCRIPT_DIR/dist" \
    --workpath "$SCRIPT_DIR/build-pyinstaller" \
    --noconfirm \
    --clean

deactivate

DIST_EXE="$SCRIPT_DIR/dist/FastDLTool"
if [ "$OS" = "Darwin" ]; then
    DIST_EXE="$SCRIPT_DIR/dist/FastDLTool"
fi

echo "  └─ Executable: dist/FastDLTool"
echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║   ✓  Build complete!                                 ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "  Run the GUI:"
echo "    ./dist/FastDLTool"
echo ""
echo "  The executable is fully self-contained."
echo "  Copy  dist/FastDLTool  anywhere — no Python or .so files needed."
echo ""