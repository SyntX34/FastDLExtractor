# build_gui.ps1  –  Full GUI build for Windows (MSVC/vcpkg or MinGW)
#
# What it does:
#   1. Builds fastdl.dll  (C++ backend, via vcpkg + MSVC or MinGW)
#   2. Creates a local Python venv and installs PyQt6 + PyInstaller
#   3. Runs PyInstaller → dist\FastDLTool.exe  (single-file, no dependencies)
#
# Usage (in Developer PowerShell for VS 2022, or any shell with cmake/g++ in PATH):
#   .\build_gui.ps1
#
# Optional parameters:
#   -VcpkgRoot   Path to vcpkg root          (default: %USERPROFILE%\vcpkg)
#   -UseMinGW    Use MinGW instead of MSVC   (default: $false)
#   -MinGWPath   Path to MinGW64 bin dir     (default: C:\msys64\mingw64\bin)
#
# The resulting executable is at:  dist\FastDLTool.exe
# It carries the DLL and all Python deps inside – no installer needed.

param(
    [string]$VcpkgRoot  = "$env:USERPROFILE\vcpkg",
    [switch]$UseMinGW   = $false,
    [string]$MinGWPath  = "C:\msys64\mingw64\bin"
)

$ErrorActionPreference = "Stop"
$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$GuiDir     = Join-Path $ScriptDir "gui"
$GuiSrc     = Join-Path $GuiDir    "src"
$ThirdParty = Join-Path $ScriptDir "third_party\nlohmann"
$VenvDir    = Join-Path $ScriptDir ".venv-build"

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════════╗"
Write-Host "║   FastDL Tool  –  Build GUI executable (one-file)   ║"
Write-Host "╚══════════════════════════════════════════════════════╝"
Write-Host ""

function Require-Command($cmd) {
    if (-not (Get-Command $cmd -ErrorAction SilentlyContinue)) {
        Write-Host "  [ERROR] '$cmd' not found in PATH. Install it and re-run."
        exit 1
    }
}

Require-Command "cmake"
Require-Command "python"

Write-Host "  ┌─ Step 1/3 : Building C++ backend (fastdl.dll) ─────────────┐"

if (-not (Test-Path "$ThirdParty\json.hpp")) {
    Write-Host "  Downloading nlohmann/json.hpp..."
    New-Item -ItemType Directory -Force -Path $ThirdParty | Out-Null
    Invoke-WebRequest `
        "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp" `
        -OutFile "$ThirdParty\json.hpp"
}

New-Item -ItemType Directory -Force -Path $GuiSrc | Out-Null
$sourceFiles = @(
    "FastDLDownloader.cpp","FastDLDownloader.h",
    "ConfigManager.cpp","ConfigManager.h",
    "Utils.h","ProgressBar.h",
    "fastdl_capi.cpp","fastdl_capi.h"
)
foreach ($f in $sourceFiles) {
    $src = Join-Path $ScriptDir "src\$f"
    $dst = Join-Path $GuiSrc     $f
    if ((Test-Path $src) -and -not (Test-Path $dst)) {
        Copy-Item $src $dst
        Write-Host "    Copied $f -> gui\src\"
    }
}

New-Item -ItemType Directory -Force -Path "$GuiDir\third_party" | Out-Null
Copy-Item -Recurse -Force "$ScriptDir\third_party\nlohmann" `
          "$GuiDir\third_party\nlohmann"

if ($UseMinGW) {
    Write-Host "  Using MinGW compiler at: $MinGWPath"
    $gcc   = Join-Path $MinGWPath "gcc.exe"
    $gxx   = Join-Path $MinGWPath "g++.exe"
    if (-not (Test-Path $gcc)) {
        Write-Host "  [ERROR] MinGW gcc not found at $gcc"
        exit 1
    }

    $BuildDir = Join-Path $GuiDir "build-mingw"
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    Push-Location $BuildDir

    cmake ..\  `
        -G "MinGW Makefiles" `
        -DCMAKE_BUILD_TYPE=Release `
        "-DCMAKE_C_COMPILER=$gcc" `
        "-DCMAKE_CXX_COMPILER=$gxx" `
        -DCMAKE_BUILD_SHARED_LIBS=ON

    cmake --build . -j4
    Pop-Location

} else {
    if (-not (Test-Path "$VcpkgRoot\vcpkg.exe")) {
        Write-Host "  Cloning vcpkg to $VcpkgRoot ..."
        git clone --depth=1 https://github.com/Microsoft/vcpkg.git $VcpkgRoot
        & "$VcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics
    }

    Write-Host "  Installing bzip2 via vcpkg..."
    & "$VcpkgRoot\vcpkg.exe" install bzip2:x64-windows --classic --recurse 2>&1 | Out-Null

    $BuildDir = Join-Path $GuiDir "build-win"
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    Push-Location $BuildDir

    cmake ..\  `
        -DCMAKE_BUILD_TYPE=Release `
        "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
        -DVCPKG_TARGET_TRIPLET=x64-windows `
        -DCMAKE_BUILD_SHARED_LIBS=ON `
        -A x64

    cmake --build . --config Release
    Pop-Location
}

$dll = Get-ChildItem -Path $GuiDir -Filter "fastdl.dll" -Recurse | Select-Object -First 1
if (-not $dll) {
    Write-Host "  [ERROR] fastdl.dll not found after build. Check CMake output above."
    exit 1
}
$DllDest = Join-Path $GuiDir "fastdl.dll"
if ($dll.FullName -ne $DllDest) {
    Copy-Item $dll.FullName $DllDest -Force
}
Write-Host "  └─ DLL ready: gui\fastdl.dll"

# ── Bundle bzip2 DLL alongside fastdl.dll so PyInstaller includes it ──────────
if (-not $UseMinGW) {
    # vcpkg may install bzip2 under various names/paths — search broadly
    $Bz2Patterns = @('bz2.dll', 'bzip2.dll', 'libbz2.dll')
    $Bz2Found = $false
    foreach ($pattern in $Bz2Patterns) {
        $Bz2Dll = Get-ChildItem -Path "$VcpkgRoot\installed" -Filter $pattern -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($Bz2Dll) {
            Copy-Item $Bz2Dll.FullName $GuiDir -Force
            Write-Host "  └─ Copied $($Bz2Dll.Name) → gui\ (runtime dependency)"
            $Bz2Found = $true
            break
        }
    }
    if (-not $Bz2Found) {
        Write-Host "  [WARNING] bzip2 DLL not found in vcpkg install — GUI may fail at runtime"
    }
} else {
    # MinGW: bzip2.dll is in the MinGW bin directory (try several names)
    $MinGWBz2 = Join-Path $MinGWPath "bz2.dll"
    if (-not (Test-Path $MinGWBz2)) {
        $MinGWBz2 = Join-Path $MinGWPath "bzip2.dll"
    }
    if (Test-Path $MinGWBz2) {
        Copy-Item $MinGWBz2 $GuiDir -Force
        Write-Host "  └─ Copied $(Split-Path $MinGWBz2 -Leaf) → gui\ (MinGW)"
    } else {
        Write-Host "  [WARNING] bzip2 DLL not found in MinGW — GUI may fail at runtime"
    }
}
} else {
    # MinGW: bzip2.dll is in the MinGW bin directory
    $MinGWBz2 = Join-Path $MinGWPath "bzip2.dll"
    if (Test-Path $MinGWBz2) {
        Copy-Item $MinGWBz2 $GuiDir -Force
        Write-Host "  └─ Copied bz2.dll → gui\ (MinGW)"
    }
}
Write-Host ""

Write-Host "  ┌─ Step 2/3 : Installing Python dependencies ─────────────────┐"

if (-not (Test-Path $VenvDir)) {
    python -m venv $VenvDir
}

$pip = Join-Path $VenvDir "Scripts\pip.exe"
$pyinstaller = Join-Path $VenvDir "Scripts\pyinstaller.exe"

& $pip install --quiet --upgrade pip
& $pip install --quiet PyQt6 pyinstaller

Write-Host "  └─ Python deps ready (venv: .venv-build)"
Write-Host ""

Write-Host "  ┌─ Step 3/3 : Compiling GUI → FastDLTool.exe ────────────────┐"

Set-Location $ScriptDir

& $pyinstaller fastdl_gui.spec `
    --distpath "$ScriptDir\dist" `
    --workpath "$ScriptDir\build-pyinstaller" `
    --noconfirm `
    --clean

$ExePath = Join-Path $ScriptDir "dist\FastDLTool.exe"
if (Test-Path $ExePath) {
    Write-Host "  └─ Executable: dist\FastDLTool.exe"
} else {
    Write-Host "  [WARNING] FastDLTool.exe not found in dist\ — check PyInstaller output above."
}

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════════╗"
Write-Host "║   ✓  Build complete!                                 ║"
Write-Host "╚══════════════════════════════════════════════════════╝"
Write-Host ""
Write-Host "  Run the GUI:"
Write-Host "    dist\FastDLTool.exe"
Write-Host ""
Write-Host "  The executable is fully self-contained."
Write-Host "  Copy  dist\FastDLTool.exe  anywhere — no Python, no DLLs needed."
Write-Host ""

Set-Location $ScriptDir