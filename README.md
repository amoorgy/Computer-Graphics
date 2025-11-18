# IcyTower (OpenGL/GLUT)

This project builds and runs via CMake. Assets (PNG/MP3) are loaded using relative paths from the workspace root.

## Prerequisites (macOS)
- Xcode Command Line Tools (clang, lldb)
- CMake (e.g., `brew install cmake`)
- System OpenGL + GLUT frameworks (preinstalled on macOS)

## Build & Run in VS Code
- Build+Run task:
  - Run: Terminal → Run Task… → `Run: IcyTower`
  - This configures CMake, builds to `build/`, and runs the app with `cwd` set to the workspace root so assets resolve.
- Debug:
  - Start debugging (F5) with `Run IcyTower` launch config. It builds first, then launches `build/IcyTower` with the correct cwd.

## Build & Run from Terminal
```bash
# From the repository root
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
# Run with cwd at repo root so assets resolve
./build/IcyTower
```

## Notes
- If CMake complains about version, update CMake via Homebrew.
- If audio doesn’t play, ensure macOS can run `afplay` (it’s built-in) and volume is on.
- If the window doesn’t appear when using remote terminals, run locally in VS Code or Terminal.app.
