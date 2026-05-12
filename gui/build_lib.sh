#!/usr/bin/env bash
# build_lib.sh  –  builds libfastdl.so (Linux) or libfastdl.dylib (macOS)
# Run from the project root (same level as src/ and gui/)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo ""
echo "╔══════════════════════════════════════════╗"
echo "║   FastDL Tool  –  Build shared library   ║"
echo "╚══════════════════════════════════════════╝"
echo ""

OS="$(uname -s)"
echo "  Platform : $OS"

check_dep() {
    command -v "$1" &>/dev/null || {
        echo "  [ERROR] '$1' not found. Install it first."
        exit 1
    }
}

check_dep cmake
check_dep g++

if [ "$OS" = "Linux" ]; then
    echo "  Checking libbz2-dev and libcurl..."
    pkg-config --exists libbz2 2>/dev/null || {
        echo "  Install: sudo apt install libbz2-dev libcurl4-gnutls-dev"
        exit 1
    }
fi

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
          Utils.h ProgressBar.h; do
    src="$SCRIPT_DIR/src/$f"
    dst="$GUI_SRC/$f"
    if [ -f "$src" ] && [ ! -f "$dst" ]; then
        cp "$src" "$dst"
        echo "  Copied $f → gui/src/"
    fi
done

mkdir -p "$SCRIPT_DIR/gui/third_party"
cp -r "$SCRIPT_DIR/third_party/nlohmann" "$SCRIPT_DIR/gui/third_party/" 2>/dev/null || true

BUILD_DIR="$SCRIPT_DIR/gui/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_BUILD_SHARED_LIBS=ON

cmake --build . --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

cd "$SCRIPT_DIR/gui"

if [ "$OS" = "Linux" ]; then
    OUT="libfastdl.so"
elif [ "$OS" = "Darwin" ]; then
    OUT="libfastdl.dylib"
else
    OUT="fastdl.dll"
fi

find build -name "libfastdl.*" | head -1 | xargs -I{} cp {} "$SCRIPT_DIR/gui/$OUT" 2>/dev/null || true

echo ""
echo "  ✓  Library built: gui/$OUT"
echo ""
echo "  Run the GUI:"
echo "    pip install PyQt6"
echo "    python gui/fastdl_gui.py"
echo ""