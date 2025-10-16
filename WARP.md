# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

This is a C++ OpenGL project implementing an "Icy Tower" inspired game using legacy OpenGL (immediate mode) with GLUT. The project demonstrates computer graphics concepts including 2D rendering, animation, collision detection, and particle effects.

## Build System and Commands

### Build Commands
```bash
# Create build directory and configure with CMake
mkdir -p build
cd build
cmake ..

# Build the project
make

# Run the executable
./IcyTower
```

### Alternative Build (if using cmake-build-debug)
```bash
# Build using CMake directly
cmake --build cmake-build-debug

# Run from cmake-build-debug directory
./cmake-build-debug/IcyTower
```

### Clean Build
```bash
# From build directory
make clean

# Or remove and recreate build directory
rm -rf build
mkdir build && cd build && cmake .. && make
```

## Architecture and Code Structure

### Core Architecture
- **Single-file architecture**: The entire game logic is contained in `main.cpp` (~2,300 lines)
- **State machine design**: Uses `GameState` enum to manage different game states (START_MENU, CHARACTER_SELECT, PLAYING, GAME_OVER, GAME_WIN)
- **Entity-component pattern**: Game objects (platforms, rocks, collectables, power-ups) are represented as structs with behavior functions
- **Immediate mode OpenGL**: Uses legacy OpenGL with GLUT for rendering primitives directly

### Key Game Systems

#### 1. Character System
- Three playable characters: Witch, Footballer, Businessman
- Each character has unique visual representation using multiple OpenGL primitives
- Character selection affects visual appearance only, not gameplay mechanics

#### 2. Platform Generation
- Procedural platform generation using terrain patterns (MIDDLE_FOCUSED, LEFT_FOCUSED, RIGHT_FOCUSED)
- Edge-to-edge platform placement with ±50 pixel offset rule for challenging but fair gameplay
- Dynamic platform deactivation when touched by rising lava

#### 3. Physics and Movement
- Custom 2D physics system with gravity, friction, and collision detection
- Smooth movement with acceleration/deceleration using key press state tracking
- Jump mechanics with optional double-jump power-up
- Air flip animation during jumps with rotation around character pivot point

#### 4. Rendering Pipeline
- Layered background with parallax scrolling clouds at different speeds
- Animated particle system for atmospheric effects
- Complex primitive-based drawing functions (each game object uses 3+ OpenGL primitives)
- HUD system with brick-textured panels and progress bars

### Critical Code Patterns

#### Collision Detection
The project uses AABB (Axis-Aligned Bounding Box) collision detection:
```cpp
bool checkCollision(float x1, float y1, float w1, float h1,
                   float x2, float y2, float w2, float h2)
```

#### Drawing Functions
All visual elements follow the pattern of using multiple primitives:
- Characters: 4+ primitives each (body parts, accessories, effects)
- UI elements: Brick-textured panels with shadows
- Collectables: Multi-primitive coins with 3D rotation illusion
- Environmental objects: Platforms with decorative elements

#### State Management
Game state is managed through a central switch-based system in the display() function, with separate update logic for each state.

## Development Guidelines

### Graphics Programming
- The project uses immediate mode OpenGL (deprecated but educational)
- All rendering uses basic primitives: GL_QUADS, GL_TRIANGLES, GL_POLYGON, GL_LINES
- Color blending enabled for alpha transparency effects
- No modern shader programming - uses fixed-function pipeline

### Game Logic
- Delta time-based movement and animation for frame-rate independent gameplay
- Entity systems use vector containers with iterator-based cleanup
- Collision detection is brute-force but adequate for the game's scope

### Performance Considerations
- Single-threaded architecture suitable for simple 2D games
- No complex optimization needed due to limited entity count
- Background particles limited to 50 for performance

## Dependencies

### Required Libraries
- **OpenGL**: Graphics rendering (system-provided on macOS)
- **GLUT**: Window management and input handling
  - macOS: Uses framework version (`GLUT/glut.h`)
  - Other platforms: Uses standard version (`GL/glut.h`)

### Platform-Specific Notes
- **macOS**: Uses GL_SILENCE_DEPRECATION to suppress OpenGL deprecation warnings
- **CMake**: Handles platform-specific library linking automatically
- **Build tools**: Requires CMake 3.31+ and C++20 support

## Common Development Tasks

### Adding New Game Elements
1. Define struct for the new element (following existing patterns)
2. Add drawing function using multiple OpenGL primitives
3. Implement update logic in the main update() function
4. Add collision detection if needed
5. Handle cleanup in vector management code

### Modifying Game Balance
- Adjust constants in update() function for movement speed, gravity, lava speed
- Platform generation parameters in initGame() function
- Scoring values in collision detection sections

### Visual Enhancements
- All drawing functions are in main.cpp - search for "draw" prefix
- Modify primitive combinations to change visual appearance
- Add new animation parameters to existing structs

## Troubleshooting

### Build Issues
- Ensure OpenGL and GLUT development packages are installed
- On macOS, Xcode command line tools provide necessary frameworks
- Check CMake minimum version requirement (3.31)

### Runtime Issues
- Window creation failure: Check GLUT initialization
- Performance issues: Reduce background particle count or animation complexity
- Input not working: Verify GLUT callback registration in main()

### Graphics Issues
- Flickering: Check double buffering setup (GLUT_DOUBLE)
- Transparency not working: Verify GL_BLEND is enabled with correct blend function
- Coordinate problems: Remember OpenGL uses bottom-left origin while game logic assumes top-left