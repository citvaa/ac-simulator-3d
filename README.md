# AC Simulator 3D

Overview

This repository contains AC Simulator 3D, a C++ project that implements a 3D simulation and renderer. The project is built with CMake and organizes code and resources into the following directories: `Source/`, `Header/`, `Shaders/`, and `Assets/`.

Key Features

- Real-time 3D simulation and rendering (configured in CMake; typically uses OpenGL/GLFW or another graphics backend)
- Clear project layout for sources, headers, shaders, and assets

Requirements

- A C++ compiler with C++17 support or newer
- CMake (3.x+)
- System libraries listed in `CMakeLists.txt` (e.g. OpenGL, GLFW, GLEW/GLAD, GLM). Check `CMakeLists.txt` for exact dependencies and how to obtain them.

Quick Start (example)

1. Create a build directory and run CMake:

   mkdir build && cd build
   cmake ..
   cmake --build .

2. Run the generated executable (name depends on CMake configuration):

   ./ac-simulator-3d

Notes

- For detailed configuration and external dependencies, consult `CMakeLists.txt`.
- Edit shaders and assets in the `Shaders/` and `Assets/` folders respectively.

Project Structure

- Header/ — header files (.h/.hpp)
- Source/ — source files (.cpp)
- Shaders/ — GLSL or other shader files
- Assets/ — models, textures and other resources
- CMakeLists.txt — build configuration
