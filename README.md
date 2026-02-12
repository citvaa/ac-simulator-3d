# AC Simulator (OpenGL, GLFW, GLEW, FreeType)

2D air-conditioner remote simulator: vent animation, desired/current temperature displays, status icons (heating/cooling/target), condensate bowl you must empty, FPS overlay, and a custom remote-shaped cursor.

## Requirements
- OpenGL 3.3+ capable GPU
- C++17 toolchain and development packages: GLFW, GLEW, FreeType, GLM (install via Homebrew on macOS: `brew install pkg-config glfw glew freetype glm`)
- `stb_image` included at `Header/stb_image.h`

## Build (CLion / CMake)
1) Open the project in CLion (it will use the CMakeLists.txt in the repo root) or run CMake manually from the repo root:
   mkdir build && cd build && cmake .. && cmake --build .
2) If CMake can't find libraries on macOS, ensure Homebrew's bin/include/lib are visible to CLion (Toolchains) or install the packages above.
3) On Windows use vcpkg or NuGet to provide GLFW/GLEW/FreeType and point CMake to the include/lib dirs.

## Run
Run the binary from the repository root so shader relative paths resolve, for example from repo root:

```bash
./build/ac-simulator
```

## CLion tips
- Set the Run/Debug configuration Working directory to the project root so shader paths resolve.
- If libraries aren't found, set CMake variables or add Homebrew include/lib paths in CLion settings.

Scene lighting: there is a scene light source with configurable color, intensity and position which illuminates all objects on the scene.

## Run
After building, start from the repo root so shader relative paths resolve, e.g.:

```powershell
.\x64\Debug\ac-simulator.exe
```

## Controls
- Click the lamp (red circle) to power on/off.
- Click the arrow button: top half increases, bottom half decreases desired temperature.
- Arrow Up/Down keys do the same.
- Space empties the bowl and unlocks the unit when full.
- ESC closes the app.

## Simulation Logic
- Vent animates only when ON and the bowl isn’t full.
- Bowl fills over time while running; at max it shuts off and locks until you press Space.
- Displays: left = desired temp, middle = current temp, right = flame (heating), snowflake (cooling), or check (target reached).
- Bottom-right nameplate is rendered as a texture.

## Project Structure
- `Source/` core logic (`Main.cpp`, `State.cpp`, `TemperatureUI.cpp`, `Renderer2D.cpp`, `TextRenderer.cpp`, `Controls.cpp`, `Util.cpp`)
- `Header/` declarations and helpers (`stb_image.h`, `ft2build.h` include)
- `Shaders/` OpenGL shaders for base draw, text, and overlay
- `x64/<Config>/` build outputs once you compile (Debug/Release)

## Notes
- Default font: `C:\Windows\Fonts\arial.ttf` (see `TextRenderer.cpp`); change the path if unavailable.
