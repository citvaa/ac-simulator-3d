#pragma once

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class Renderer {
public:
  Renderer();
  ~Renderer();

  bool init();
  void render();

  void setViewProjection(const glm::mat4& view, const glm::mat4& proj);

  // Simple mesh draw helpers
  void drawCube(const glm::mat4& model, const glm::vec3& color);

  // draw cube but sample the provided texture (bound to GL_TEXTURE0)
  // color tints the sampled texture (use alpha for mask)
  void drawTexturedCube(const glm::mat4& model, GLuint texture, const glm::vec3& color = glm::vec3(1.0f));
  // draw semi-transparent particle (approximated sphere as cube)
  void drawParticle(const glm::mat4& model, const glm::vec3& color, float alpha);

  // approximate hollow cylinder by a ring of thin quads
  void drawHollowCylinderAt(const glm::vec3& center, float radius, float height, float thickness, int segments, const glm::vec3& color);

  // draw a hollow box (open at the top) centered at `center` with full width/height/depth
  // thickness is wall thickness in world units
  void drawHollowBoxAt(const glm::vec3& center, float width, float height, float depth, float thickness, const glm::vec3& color);

private:
  // scene light visualization stored here so marker can be drawn after scene
  glm::vec3 sceneLightPos_ = glm::vec3(-350.0f, 260.0f, 40.0f);
  glm::vec3 sceneLightColor_ = glm::vec3(1.0f, 0.95f, 0.2f);
  float sceneLightIntensity_ = 2.5f;

  std::string loadShaderSource(const char* path);
  unsigned int createShaderProgram(const char* vertPath, const char* fragPath);
  unsigned int phongProgram_ = 0;
  unsigned int blinnProgram_ = 0;

public:
  // set scene light explicitly (independent from AC/lamp)
  void setSceneLight(const glm::vec3& pos, const glm::vec3& color, float intensity);

  // cube mesh
  unsigned int cubeVao_ = 0;
  unsigned int cubeVbo_ = 0;
  unsigned int cubeVboCount_ = 0;

  unsigned int defaultTex_ = 0;

  // optional lamp parameters set by application
  glm::vec3 lampPos_ = glm::vec3(0.0f);
  glm::vec3 lampColor_ = glm::vec3(1.0f, 0.0f, 0.0f);
  float lampIntensity_ = 0.0f;
  bool lampEnabled_ = false;

  // loaded models
  struct ModelMesh { unsigned int vao; unsigned int vbo; int vertCount; };
  std::vector<ModelMesh> models_;

public:
  void setLampLight(const glm::vec3& pos, const glm::vec3& color, float intensity, bool enabled);
  int loadOBJModel(const std::string& path);
  void drawModel(int modelId, const glm::mat4& model, const glm::vec3& color);
};
