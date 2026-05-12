# build_lib.bat  –  Windows (MSVC + vcpkg) build for fastdl.dll
# Run from the project root in a Developer PowerShell for VS 2022

param(
    [string]$VcpkgRoot = "$env:USERPROFILE\vcpkg"
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "╔══════════════════════════════════════════╗"
Write-Host "║   FastDL Tool  –  Build shared library   ║"
Write-Host "╚══════════════════════════════════════════╝"
Write-Host ""

$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$GuiDir     = Join-Path $ScriptDir "gui"
$GuiSrc     = Join-Path $GuiDir    "src"
$BuildDir   = Join-Path $GuiDir    "build-win"
$ThirdParty = Join-Path $ScriptDir "third_party\nlohmann"


if (-not (Test-Path "$VcpkgRoot\vcpkg.exe")) {
    Write-Host "  Cloning vcpkg..."
    git clone --depth=1 https://github.com/Microsoft/vcpkg.git $VcpkgRoot
    & "$VcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics
}

Write-Host "  Installing bzip2 via vcpkg..."
& "$VcpkgRoot\vcpkg.exe" install bzip2:x64-windows --classic --recurse 2>&1 | Out-Null


if (-not (Test-Path "$ThirdParty\json.hpp")) {
    Write-Host "  Downloading nlohmann/json.hpp..."
    New-Item -ItemType Directory -Force -Path $ThirdParty | Out-Null
    Invoke-WebRequest `
        "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp" `
        -OutFile "$ThirdParty\json.hpp"
}


New-Item -ItemType Directory -Force -Path $GuiSrc | Out-Null
foreach ($f in @("FastDLDownloader.cpp","FastDLDownloader.h",
                  "ConfigManager.cpp","ConfigManager.h",
                  "Utils.h","ProgressBar.h")) {
    $src = Join-Path $ScriptDir "src\$f"
    $dst = Join-Path $GuiSrc     $f
    if ((Test-Path $src) -and -not (Test-Path $dst)) {
        Copy-Item $src $dst
        Write-Host "  Copied $f -> gui\src\"
    }
}

New-Item -ItemType Directory -Force -Path "$GuiDir\third_party" | Out-Null
Copy-Item -Recurse -Force "$ScriptDir\third_party\nlohmann" `
          "$GuiDir\third_party\nlohmann"


New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Set-Location $BuildDir

cmake .. `
    -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows `
    -A x64

cmake --build . --config Release

$dll = Get-ChildItem -Path $BuildDir -Filter "fastdl.dll" -Recurse | Select-Object -First 1
if ($dll) {
    Copy-Item $dll.FullName (Join-Path $GuiDir "fastdl.dll")
    Write-Host ""
    Write-Host "  ✓  Built: gui\fastdl.dll"
} else {
    Write-Host "  [WARNING] DLL not found after build."
}

Write-Host ""
Write-Host "  Run the GUI:"
Write-Host "    pip install PyQt6"
Write-Host "    python gui\fastdl_gui.py"
Write-Host ""

Set-Location $ScriptDir