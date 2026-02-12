cmake_minimum_required(VERSION 3.16)
project(ac-simulator LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
file(GLOB_RECURSE AC_SIM_SRCS "${CMAKE_SOURCE_DIR}/Source/*.cpp")
add_executable(ac-simulator ${AC_SIM_SRCS})

target_include_directories(ac-simulator PRIVATE "${CMAKE_SOURCE_DIR}/Header" /opt/homebrew/include /usr/local/include)

# OpenGL
find_package(OpenGL REQUIRED)
if(TARGET OpenGL::GL)
  target_link_libraries(ac-simulator PRIVATE OpenGL::GL)
endif()

# GLM (header-only)
find_path(GLM_INCLUDE_DIR glm/glm.hpp HINTS /opt/homebrew/include /usr/include /usr/local/include)
if(GLM_INCLUDE_DIR)
  target_include_directories(ac-simulator PRIVATE ${GLM_INCLUDE_DIR})
else()
  message(STATUS "GLM not found; install glm (e.g., brew install glm) or set GLM_INCLUDE_DIR")
endif()

# Use pkg-config to find common dependencies (glfw3, glew, freetype2)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(GLFW3 QUIET glfw3)
  pkg_check_modules(GLEW QUIET glew)
  pkg_check_modules(FREETYPE2 QUIET freetype2)

  if(GLFW3_FOUND)
    target_include_directories(ac-simulator PRIVATE ${GLFW3_INCLUDE_DIRS})
    target_link_libraries(ac-simulator PRIVATE ${GLFW3_LIBRARIES})
  endif()

  if(GLEW_FOUND)
    target_include_directories(ac-simulator PRIVATE ${GLEW_INCLUDE_DIRS})
    target_link_libraries(ac-simulator PRIVATE ${GLEW_LIBRARIES})
  endif()

  if(FREETYPE2_FOUND)
    target_include_directories(ac-simulator PRIVATE ${FREETYPE2_INCLUDE_DIRS})
    target_link_libraries(ac-simulator PRIVATE ${FREETYPE2_LIBRARIES})
  endif()
endif()

# Fallback: try finding libraries directly (Homebrew paths)
find_library(GLFW_LIB NAMES glfw3 glfw PATHS /opt/homebrew/lib /usr/local/lib)
find_library(GLEW_LIB NAMES GLEW glew PATHS /opt/homebrew/lib /usr/local/lib)
find_library(FREETYPE_LIB NAMES freetype PATHS /opt/homebrew/lib /usr/local/lib)

if(GLFW_LIB)
  target_link_libraries(ac-simulator PRIVATE ${GLFW_LIB})
endif()
if(GLEW_LIB)
  target_link_libraries(ac-simulator PRIVATE ${GLEW_LIB})
endif()
if(FREETYPE_LIB)
  target_link_libraries(ac-simulator PRIVATE ${FREETYPE_LIB})
endif()

# Add Homebrew and /usr/local library directories so the linker finds brewed libraries on macOS
target_link_directories(ac-simulator PRIVATE /opt/homebrew/lib /usr/local/lib)

# Optional model loaders
find_package(ASSIMP QUIET)
if(ASSIMP_FOUND)
  target_include_directories(ac-simulator PRIVATE ${ASSIMP_INCLUDE_DIRS})
  target_link_libraries(ac-simulator PRIVATE ${ASSIMP_LIBRARIES})
elseif(TARGET assimp::assimp)
  target_link_libraries(ac-simulator PRIVATE assimp::assimp)
endif()

# tinyobjloader (header-only) - try common include paths
find_path(TINYOBJLOADER_INCLUDE tiny_obj_loader.h HINTS /usr/include /usr/local/include /opt/homebrew/include)
if(TINYOBJLOADER_INCLUDE)
  target_include_directories(ac-simulator PRIVATE ${TINYOBJLOADER_INCLUDE})
endif()

# Note about running
message(STATUS "Note: Run the binary from the repository root so shader relative paths resolve (see README.md).")
