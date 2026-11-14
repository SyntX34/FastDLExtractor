# Contributing to FastDL Extractor

Thank you for your interest in contributing to **FastDL Extractor**!

## How to Contribute

1. **Fork the repository** and create a feature branch from `main`:
   ```bash
   git checkout -b fix/your-issue-or-feature
   ```
2. **Make your changes**:
   - Keep C++ code compatible with C++17 (MSVC, GCC, Clang).
   - Keep Python GUI code compatible with Python 3.10+ and PyQt6.
   - Follow the existing code style and naming conventions.
3. **Test your changes**:
   - Build both the CLI (`cmake --build build`) and the GUI shared library (`gui/build_lib.ps1` or `gui/build_lib.sh`).
   - Verify downloads, wildcard patterns (`models/*.mdl`, `maps/zm_*`), and cancellation/stop behavior.
4. **Commit & Open a Pull Request**:
   - Write clear, descriptive commit messages.
   - Reference any related issues in your PR description.

## Reporting Bugs

When opening an issue, please include:
- OS and version (Windows / Linux / macOS)
- Whether you are using the CLI or GUI
- Steps to reproduce, the pattern/URL used, and any error output
