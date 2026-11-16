# lgnite
**A Lightweight, Single-File 2D Game Library for C++ (OpenGL 3.3 Core)**

[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)]()
[![License](https://img.shields.io/badge/license-MIT-green.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()
[![Download](https://img.shields.io/badge/Download-Latest%20Release-blue.svg)](https://github.com/harold372002/lgnite.h/archive/refs/heads/main.zip)

## 📖 Overview
`lgnite` is a modern, single-file 2D game development library written in C++17. Built directly on native OpenGL 3.3 Core and GLFW, it provides a comprehensive, high-performance toolset for creating 2D games—ranging from simple prototypes to large-scale projects. By combining a hardware-accelerated batch renderer, a robust entity-component system (ECS), and a progressive, flat API, `lgnite` eliminates the boilerplate and complex build systems typically associated with C++ game development, allowing developers to focus entirely on gameplay.

## ✨ Key Features
*   **Single-File Architecture:** The entire library logic is contained within `lgnite.h`. Simply drop it into your project and include it. No CMake, package managers, or complex setups required.
*   **Hardware Accelerated:** Utilizes a highly optimized batch renderer and native OpenGL 3.3 Core, ensuring smooth performance even with thousands of sprites and tiles.
*   **Zero Hidden Dependencies:** Only relies on GLFW and three well-known, public-domain single-header libraries (`stb_image.h`, `stb_truetype.h`, `miniaudio.h`).
*   **Progressive API Design:** Start by drawing a rectangle in 10 lines of code, and seamlessly scale up to complex systems with Tilemaps, Animations, Physics, and ECS as your game grows.
*   **No Exceptions:** Failures return `bool` and log descriptive errors via the built-in `Log` system, ensuring predictable and stable runtime behavior.
*   **Cross-Platform:** Compiles and runs seamlessly on Windows, Linux, and macOS.

---

## 🚀 Environment Configuration & Build Guide
To use `lgnite`, you need a C++17 compiler, GLFW, and the three required single-header files.

### 1. Project Directory Setup
For Windows users, place the following files in your project's root directory. *(Linux/macOS users typically install GLFW system-wide via package managers and only need the `.h` files alongside `main.cpp`).*

```text
YourProject/
├── lgnite.h
├── stb_image.h
├── stb_truetype.h
├── miniaudio.h
├── GLFW/
│   └── glfw3.h          (Must be inside a "GLFW" folder)
├── glfw3.lib            (Windows only)
├── glfw3.dll            (Windows only, must be next to the compiled .exe)
└── main.cpp             (Your game code)
```

### 2. Writing the Code
In exactly one `.cpp` file (usually `main.cpp`), define the implementation macro before including the header:

```cpp
#define LGNITE_IMPLEMENTATION
#include "lgnite.h"
using namespace lgnite;

int main() {
    App app;
    app.title = "My lgnite Game";
    
    app.draw = [] {
        Renderer::drawRect(100, 100, 200, 120, Color::Orange);
    };

    return app.run();
}
```

### 3. Compiler & Environment Configuration

#### Option A: Visual Studio (Windows / MSVC)
1.  **Create Project:** Create a new "Empty C++ Project" and set the build configuration to `x64`.
2.  **Include Directories:** Right-click Project -> Properties -> C/C++ -> General -> Additional Include Directories -> Add `$(ProjectDir)`.
3.  **Library Directories:** Linker -> General -> Additional Library Directories -> Add `$(ProjectDir)`.
4.  **Dependencies:** Linker -> Input -> Additional Dependencies -> Add `opengl32.lib` and `glfw3.lib`.
5.  **Working Directory:** Debugging -> Working Directory -> Set to `$(ProjectDir)` (ensures images and `.dll`s are found correctly during runtime).

#### Option B: Command Line (Windows / MSVC)
Open the "Developer Command Prompt for VS" and run:
```bat
cl /std:c++17 /EHsc main.cpp /I"path_to_glfw_include" /link opengl32.lib glfw3.lib /LIBPATH:"path_to_glfw_lib"
```

#### Option C: GCC / Clang (Linux)
Ensure GLFW is installed via your package manager (e.g., `sudo apt install libglfw3-dev`). Compile with:
```bash
g++ -std=c++17 main.cpp -lglfw -lGL -ldl -lpthread -o mygame
```

#### Option D: Clang (macOS)
Ensure GLFW is installed via Homebrew (`brew install glfw`). Compile with:
```bash
clang++ -std=c++17 main.cpp -lglfw -framework OpenGL -o mygame
```

---

## 🧩 Core Modules Overview

| Module | Key APIs | Description |
| :--- | :--- | :--- |
| **App** | `setup`, `update`, `draw`, `run()` | Encapsulates the window, main loop, and initialization. |
| **Renderer** | `drawRect`, `drawCircle`, `setCamera` | Hardware-accelerated rendering for shapes and textures. |
| **Sprite** | `.tex`, `.pos`, `.size`, `.draw()` | Renderable game objects with rotation, scaling, and tinting. |
| **Animation** | `setRange()`, `update()`, `current()` | SpriteSheet-based frame animation player. |
| **Camera2D** | `pos`, `zoom`, `follow()` | Smooth viewport tracking, zooming, and boundary clamping. |
| **Tilemap** | `create()`, `setTileset()`, `isSolidAt()` | Grid-based level design with built-in AABB collision. |
| **Input** | `isKeyDown()`, `isKeyPressed()` | Polling for keyboard, mouse, and gamepad. |
| **Audio** | `AudioClip`, `SoundPool` | Audio playback and BGM streaming. |
| **ECS** | `World`, `Entity`, `System` | Entity-Component-System for decoupling and large-scale logic. |
| **Utils** | `Vec2`, `Rect`, `Color`, `Json` | Math, geometry, color manipulation, and data parsing. |

---

## 💻 Quick Example
Here is a complete, runnable example demonstrating Sprite animation, physics (gravity and jumping), and Camera2D tracking:

```cpp
#define LGNITE_IMPLEMENTATION
#include "lgnite.h"
using namespace lgnite;

int main() {
    App app;
    app.width = 800;
    app.height = 600;
    app.title = "lgnite Example";
    app.clearColor = Color{0.15f, 0.15f, 0.2f, 1.0f};

    Sprite hero;
    Camera2D cam;
    float vy = 0;
    const float gravity = 1800.0f;
    const float groundY = 500.0f;

    app.setup = [&] {
        hero.autoSize = false;
        hero.size = {64, 64};
        hero.pos = {300, groundY - 64};
        
        cam.clampEnabled = true;
        cam.clampRect = {0, 0, 3000, 600};
    };

    app.update = [&](float dt) {
        float vx = 0;
        if (Input::isKeyDown(GLFW_KEY_D)) vx = 300.0f;
        if (Input::isKeyDown(GLFW_KEY_A)) vx = -300.0f;

        bool onGround = (hero.pos.y + hero.size.y >= groundY);
        if (Input::isKeyPressed(GLFW_KEY_SPACE) && onGround) vy = -650.0f;

        vy += gravity * dt;
        hero.pos.x += vx * dt;
        hero.pos.y += vy * dt;

        if (hero.pos.y + hero.size.y >= groundY) {
            hero.pos.y = groundY - hero.size.y;
            vy = 0;
        }

        Vec2 center = {hero.pos.x + 32, hero.pos.y + 32};
        cam.follow(center, dt, 800, 600);
    };

    app.draw = [&] {
        Renderer::setCamera(&cam);
        Renderer::drawRect(0, groundY, 3000, 100, Color::Green);
        Renderer::drawRect(hero.pos.x, hero.pos.y, hero.size.x, hero.size.y, Color::Orange);
        Renderer::setCamera(nullptr);
    };

    return app.run();
}
```

---

## 📄 License
This project is licensed under the MIT License. Free for personal and commercial use.
