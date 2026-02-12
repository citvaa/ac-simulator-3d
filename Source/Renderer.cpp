#include "Renderer.h"
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer() {}
Renderer::~Renderer() {
  if (phongProgram_ != 0) glDeleteProgram(phongProgram_);
  if (blinnProgram_ != 0) glDeleteProgram(blinnProgram_);
}

bool Renderer::init() {
  // Compile and link shaders
  phongProgram_ = createShaderProgram("Shaders/phong.vert", "Shaders/phong.frag");
  blinnProgram_ = createShaderProgram("Shaders/phong.vert", "Shaders/blinn.frag");
  if (phongProgram_ == 0) {
    std::cerr << "Failed to create Phong shader program" << std::endl;
    return false;
  }
  // blinnProgram_ is optional; warn if missing
  if (blinnProgram_ == 0) {
    std::cerr << "Warning: Blinn-Phong shader failed to compile (blinn optional)" << std::endl;
  }

  // Create a simple white 1x1 texture
  glGenTextures(1, &defaultTex_);
  glBindTexture(GL_TEXTURE_2D, defaultTex_);
  unsigned char white[4] = {255,255,255,255};
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Create cube geometry (positions, normals, texcoords) - 36 vertices
  float verts[] = {
    // positions         normals           tex
    // front
    -0.5f, -0.5f,  0.5f,  0,0,1,  0,0,
     0.5f, -0.5f,  0.5f,  0,0,1,  1,0,
     0.5f,  0.5f,  0.5f,  0,0,1,  1,1,
     0.5f,  0.5f,  0.5f,  0,0,1,  1,1,
    -0.5f,  0.5f,  0.5f,  0,0,1,  0,1,
    -0.5f, -0.5f,  0.5f,  0,0,1,  0,0,
    // back
    -0.5f, -0.5f, -0.5f,  0,0,-1, 0,0,
    -0.5f,  0.5f, -0.5f,  0,0,-1, 0,1,
     0.5f,  0.5f, -0.5f,  0,0,-1, 1,1,
     0.5f,  0.5f, -0.5f,  0,0,-1, 1,1,
     0.5f, -0.5f, -0.5f,  0,0,-1, 1,0,
    -0.5f, -0.5f, -0.5f,  0,0,-1, 0,0,
    // left
    -0.5f,  0.5f,  0.5f, -1,0,0,  1,0,
    -0.5f,  0.5f, -0.5f, -1,0,0,  1,1,
    -0.5f, -0.5f, -0.5f, -1,0,0,  0,1,
    -0.5f, -0.5f, -0.5f, -1,0,0,  0,1,
    -0.5f, -0.5f,  0.5f, -1,0,0,  0,0,
    -0.5f,  0.5f,  0.5f, -1,0,0,  1,0,
    // right
     0.5f,  0.5f,  0.5f, 1,0,0,  1,0,
     0.5f, -0.5f, -0.5f, 1,0,0,  0,1,
     0.5f,  0.5f, -0.5f, 1,0,0,  1,1,
     0.5f, -0.5f, -0.5f, 1,0,0,  0,1,
     0.5f,  0.5f,  0.5f, 1,0,0,  1,0,
     0.5f, -0.5f,  0.5f, 1,0,0,  0,0,
    // top
    -0.5f,  0.5f, -0.5f, 0,1,0,  0,1,
    -0.5f,  0.5f,  0.5f, 0,1,0,  0,0,
     0.5f,  0.5f,  0.5f, 0,1,0,  1,0,
     0.5f,  0.5f,  0.5f, 0,1,0,  1,0,
     0.5f,  0.5f, -0.5f, 0,1,0,  1,1,
    -0.5f,  0.5f, -0.5f, 0,1,0,  0,1,
    // bottom
    -0.5f, -0.5f, -0.5f, 0,-1,0, 0,1,
     0.5f, -0.5f, -0.5f, 0,-1,0, 1,1,
     0.5f, -0.5f,  0.5f, 0,-1,0, 1,0,
     0.5f, -0.5f,  0.5f, 0,-1,0, 1,0,
    -0.5f, -0.5f,  0.5f, 0,-1,0, 0,0,
    -0.5f, -0.5f, -0.5f, 0,-1,0, 0,1
  };
  glGenVertexArrays(1, &cubeVao_);
  glGenBuffers(1, &cubeVbo_);
  glBindVertexArray(cubeVao_);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  // pos
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0));
  // normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
  // tex
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glBindVertexArray(0);
  cubeVboCount_ = 36;

  return true;
}

void Renderer::render() {
  // draw stored scene light marker on top of scene
  if (phongProgram_ == 0) return;
  // construct model transform for marker (half AC size)
  glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), sceneLightPos_);
  lightModel = glm::scale(lightModel, glm::vec3(120.0f, 50.0f, 40.0f));
  // use constant yellow so marker never disappears if intensity changes
  glm::vec3 markerColor = glm::vec3(1.0f, 1.0f, 0.0f);

  // draw on top of scene
  GLboolean prevDepth = glIsEnabled(GL_DEPTH_TEST);
  if (prevDepth) glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  drawCube(lightModel, markerColor);
  glDepthMask(GL_TRUE);
  if (prevDepth) glEnable(GL_DEPTH_TEST);
}

void Renderer::drawCube(const glm::mat4& model, const glm::vec3& color) {
  if (phongProgram_ == 0) return;
  glUseProgram(phongProgram_);

  GLint locModel = glGetUniformLocation(phongProgram_, "model");
  if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
  GLint locMat = glGetUniformLocation(phongProgram_, "materialDiffuse");
  if (locMat >= 0) glUniform3f(locMat, color.r, color.g, color.b);
  GLint locSpec = glGetUniformLocation(phongProgram_, "materialSpecular");
  if (locSpec >= 0) glUniform3f(locSpec, 0.3f, 0.3f, 0.3f);
  GLint locSh = glGetUniformLocation(phongProgram_, "shininess");
  if (locSh >= 0) glUniform1f(locSh, 32.0f);
  GLint alphaLoc = glGetUniformLocation(phongProgram_, "uAlpha");
  if (alphaLoc >= 0) glUniform1f(alphaLoc, 1.0f);

  // bind default texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, defaultTex_);
  GLint texLoc = glGetUniformLocation(phongProgram_, "tex");
  if (texLoc >= 0) glUniform1i(texLoc, 0);
  // ensure flipV is disabled for colored cube draws
  GLint flipLoc = glGetUniformLocation(phongProgram_, "flipV");
  if (flipLoc >= 0) glUniform1i(flipLoc, 0);

  glBindVertexArray(cubeVao_);
  glDrawArrays(GL_TRIANGLES, 0, cubeVboCount_);
  glBindVertexArray(0);
  glUseProgram(0);
}

void Renderer::drawTexturedCube(const glm::mat4& model, GLuint texture, const glm::vec3& color) {
  if (phongProgram_ == 0) return;
  glUseProgram(phongProgram_);

  GLint locModel = glGetUniformLocation(phongProgram_, "model");
  if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
  GLint locMat = glGetUniformLocation(phongProgram_, "materialDiffuse");
  if (locMat >= 0) glUniform3f(locMat, color.r, color.g, color.b);
  GLint locSpec = glGetUniformLocation(phongProgram_, "materialSpecular");
  if (locSpec >= 0) glUniform3f(locSpec, 0.2f, 0.2f, 0.2f);
  GLint locSh = glGetUniformLocation(phongProgram_, "shininess");
  if (locSh >= 0) glUniform1f(locSh, 8.0f);

  // bind provided texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture ? texture : defaultTex_);
  GLint texLoc = glGetUniformLocation(phongProgram_, "tex");
  if (texLoc >= 0) glUniform1i(texLoc, 0);
  // flip vertically so text appears upright on cube faces
  GLint flipLoc = glGetUniformLocation(phongProgram_, "flipV");
  if (flipLoc >= 0) glUniform1i(flipLoc, 1);
  // set alpha to fully opaque for textured faces unless caller changes
  GLint alphaLoc = glGetUniformLocation(phongProgram_, "uAlpha");
  if (alphaLoc >= 0) glUniform1f(alphaLoc, 1.0f);

  glBindVertexArray(cubeVao_);
  glDrawArrays(GL_TRIANGLES, 0, cubeVboCount_);
  glBindVertexArray(0);
  glUseProgram(0);
}

void Renderer::drawParticle(const glm::mat4& model, const glm::vec3& color, float alpha) {
  if (phongProgram_ == 0) return;
  glUseProgram(phongProgram_);
  GLint locModel = glGetUniformLocation(phongProgram_, "model");
  if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
  GLint locMat = glGetUniformLocation(phongProgram_, "materialDiffuse");
  if (locMat >= 0) glUniform3f(locMat, color.r, color.g, color.b);
  GLint locSpec = glGetUniformLocation(phongProgram_, "materialSpecular");
  if (locSpec >= 0) glUniform3f(locSpec, 0.2f, 0.2f, 0.2f);
  GLint locSh = glGetUniformLocation(phongProgram_, "shininess");
  if (locSh >= 0) glUniform1f(locSh, 8.0f);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, defaultTex_);
  GLint texLoc = glGetUniformLocation(phongProgram_, "tex");
  if (texLoc >= 0) glUniform1i(texLoc, 0);
  GLint flipLoc = glGetUniformLocation(phongProgram_, "flipV");
  if (flipLoc >= 0) glUniform1i(flipLoc, 0);
  GLint alphaLoc = glGetUniformLocation(phongProgram_, "uAlpha");
  if (alphaLoc >= 0) glUniform1f(alphaLoc, alpha);

  glBindVertexArray(cubeVao_);
  glDrawArrays(GL_TRIANGLES, 0, cubeVboCount_);
  glBindVertexArray(0);
  glUseProgram(0);
}

void Renderer::drawHollowBoxAt(const glm::vec3& center, float width, float height, float depth, float thickness, const glm::vec3& color) {
  // bottom
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(center.x, center.y - (height * 0.5f) + (thickness * 0.5f), center.z));
  model = glm::scale(model, glm::vec3(width, thickness, depth));
  drawCube(model, color);

  // left wall
  model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(center.x - (width * 0.5f) + (thickness * 0.5f), center.y, center.z));
  model = glm::scale(model, glm::vec3(thickness, height - thickness, depth));
  drawCube(model, color);

  // right wall
  model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(center.x + (width * 0.5f) - (thickness * 0.5f), center.y, center.z));
  model = glm::scale(model, glm::vec3(thickness, height - thickness, depth));
  drawCube(model, color);

  // front wall (positive Z)
  model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(center.x, center.y, center.z + (depth * 0.5f) - (thickness * 0.5f)));
  model = glm::scale(model, glm::vec3(width - thickness * 2.0f, height - thickness, thickness));
  drawCube(model, color);

  // back wall (negative Z)
  model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(center.x, center.y, center.z - (depth * 0.5f) + (thickness * 0.5f)));
  model = glm::scale(model, glm::vec3(width - thickness * 2.0f, height - thickness, thickness));
  drawCube(model, color);
}

void Renderer::drawHollowCylinderAt(const glm::vec3& center, float radius, float height, float thickness, int segments, const glm::vec3& color) {
  // approximate cylinder wall with segments made from thin quads (drawn as cubes)
  if (segments < 6) segments = 6;
  float segmentArc = 2.0f * 3.14159265f / static_cast<float>(segments);
  float segWidth = radius * segmentArc; // approximate arc length
  float innerR = radius - thickness * 0.5f;
  for (int i = 0; i < segments; ++i) {
    float angle = (i + 0.5f) * segmentArc;
    float cx = center.x + innerR * cos(angle);
    float cz = center.z + innerR * sin(angle);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(cx, center.y, cz));
    model = glm::rotate(model, -angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(segWidth, height - thickness, thickness));
    drawCube(model, color);
  }
}

// Simple OBJ loader and model draw implementation appended
int Renderer::loadOBJModel(const std::string& path) {
  std::ifstream in(path);
  if (!in) return -1;
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> texcoords;
  std::vector<float> interleaved;

  std::string line;
  while (std::getline(in, line)) {
    if (line.size() < 2) continue;
    if (line[0] == 'v' && line[1] == ' ') {
      std::istringstream s(line.substr(2));
      glm::vec3 v; s >> v.x >> v.y >> v.z; positions.push_back(v);
    } else if (line.rfind("vn ", 0) == 0) {
      std::istringstream s(line.substr(3)); glm::vec3 n; s >> n.x >> n.y >> n.z; normals.push_back(n);
    } else if (line.rfind("vt ", 0) == 0) {
      std::istringstream s(line.substr(3)); glm::vec2 t; s >> t.x >> t.y; texcoords.push_back(t);
    } else if (line[0] == 'f' && line[1] == ' ') {
      // parse face entries
      std::istringstream s(line.substr(2));
      std::string a,b,c,d;
      s >> a >> b >> c >> d; // d will be empty for triangles
      auto process = [&](const std::string& tok){
        int vi=0,ti=0,ni=0;
        size_t p1 = tok.find('/');
        if (p1==std::string::npos) { vi = std::atoi(tok.c_str()); }
        else {
          vi = std::atoi(tok.substr(0,p1).c_str());
          size_t p2 = tok.find('/', p1+1);
          if (p2==std::string::npos) {
            ti = std::atoi(tok.substr(p1+1).c_str());
          } else {
            if (p2 > p1+1) ti = std::atoi(tok.substr(p1+1, p2-p1-1).c_str());
            ni = std::atoi(tok.substr(p2+1).c_str());
          }
        }
        glm::vec3 pos(0.0f); glm::vec3 nor(0.0f); glm::vec2 tex(0.0f);
        if (vi!=0) {
          int idx = vi > 0 ? vi - 1 : (int)positions.size() + vi;
          if (idx >=0 && idx < (int)positions.size()) pos = positions[idx];
        }
        if (ti!=0) {
          int idx = ti > 0 ? ti - 1 : (int)texcoords.size() + ti;
          if (idx >=0 && idx < (int)texcoords.size()) tex = texcoords[idx];
        }
        if (ni!=0) {
          int idx = ni > 0 ? ni - 1 : (int)normals.size() + ni;
          if (idx >=0 && idx < (int)normals.size()) nor = normals[idx];
        }
        // push interleaved pos(3) normal(3) tex(2)
        interleaved.push_back(pos.x); interleaved.push_back(pos.y); interleaved.push_back(pos.z);
        interleaved.push_back(nor.x); interleaved.push_back(nor.y); interleaved.push_back(nor.z);
        interleaved.push_back(tex.x); interleaved.push_back(tex.y);
      };
      process(a); process(b); process(c);
      if (!d.empty()) { // quad -> emit second tri
        process(a); process(c); process(d);
      }
    }
  }

  if (interleaved.empty()) return -1;

  ModelMesh m{};
  glGenVertexArrays(1, &m.vao);
  glGenBuffers(1, &m.vbo);
  glBindVertexArray(m.vao);
  glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
  glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(float), interleaved.data(), GL_STATIC_DRAW);
  // pos
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  // normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
  // tex
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glBindVertexArray(0);
  m.vertCount = static_cast<int>(interleaved.size() / 8);
  models_.push_back(m);
  return static_cast<int>(models_.size() - 1);
}

void Renderer::drawModel(int modelId, const glm::mat4& model, const glm::vec3& color) {
  if (phongProgram_ == 0) return;
  if (modelId < 0 || modelId >= (int)models_.size()) return;
  const ModelMesh &m = models_[modelId];
  glUseProgram(phongProgram_);
  GLint locModel = glGetUniformLocation(phongProgram_, "model");
  if (locModel >= 0) glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
  GLint locMat = glGetUniformLocation(phongProgram_, "materialDiffuse");
  if (locMat >= 0) glUniform3f(locMat, color.r, color.g, color.b);
  GLint locSpec = glGetUniformLocation(phongProgram_, "materialSpecular");
  if (locSpec >= 0) glUniform3f(locSpec, 0.3f, 0.3f, 0.3f);
  GLint locSh = glGetUniformLocation(phongProgram_, "shininess");
  if (locSh >= 0) glUniform1f(locSh, 32.0f);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, defaultTex_);
  GLint texLoc = glGetUniformLocation(phongProgram_, "tex");
  if (texLoc >= 0) glUniform1i(texLoc, 0);
  GLint flipLoc = glGetUniformLocation(phongProgram_, "flipV");
  if (flipLoc >= 0) glUniform1i(flipLoc, 0);
  glBindVertexArray(m.vao);
  glDrawArrays(GL_TRIANGLES, 0, m.vertCount);
  glBindVertexArray(0);
  glUseProgram(0);
}

void Renderer::setViewProjection(const glm::mat4& view, const glm::mat4& proj) {
  // compute camera position from inverse view
  glm::mat4 invView = glm::inverse(view);
  glm::vec3 camPos(invView[3][0], invView[3][1], invView[3][2]);

  // use stored scene light (set via setSceneLight) so it remains independent from AC state
  glm::vec3 lightPos = sceneLightPos_;
  glm::vec3 lightColor = sceneLightColor_;
  float lightIntensity = sceneLightIntensity_;

  if (phongProgram_ != 0) {
    glUseProgram(phongProgram_);
    GLint loc = glGetUniformLocation(phongProgram_, "view");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view));
    loc = glGetUniformLocation(phongProgram_, "projection");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(proj));

    GLint lp = glGetUniformLocation(phongProgram_, "light.position");
    GLint lc = glGetUniformLocation(phongProgram_, "light.color");
    GLint li = glGetUniformLocation(phongProgram_, "light.intensity");
    if (lp >= 0) glUniform3f(lp, lightPos.x, lightPos.y, lightPos.z);
    if (lc >= 0) glUniform3f(lc, lightColor.r, lightColor.g, lightColor.b);
    if (li >= 0) glUniform1f(li, lightIntensity);

    // lamp (optional)
    GLint lpp = glGetUniformLocation(phongProgram_, "lampLight.position");
    GLint lpc = glGetUniformLocation(phongProgram_, "lampLight.color");
    GLint lpi = glGetUniformLocation(phongProgram_, "lampLight.intensity");
    GLint len = glGetUniformLocation(phongProgram_, "lampEnabled");
    if (lpp >= 0) glUniform3f(lpp, lampPos_.x, lampPos_.y, lampPos_.z);
    if (lpc >= 0) glUniform3f(lpc, lampColor_.x, lampColor_.y, lampColor_.z);
    if (lpi >= 0) glUniform1f(lpi, lampIntensity_);
    if (len >= 0) glUniform1i(len, lampEnabled_ ? 1 : 0);

    GLint viewPosLoc = glGetUniformLocation(phongProgram_, "viewPos");
    if (viewPosLoc >= 0) glUniform3f(viewPosLoc, camPos.x, camPos.y, camPos.z);

    // log once so user can inspect coordinates (helpful for debugging visibility)
    static bool lightLogged = false;
    if (!lightLogged) {
      fprintf(stderr, "SceneLight: pos=(%0.2f,%0.2f,%0.2f) cam=(%0.2f,%0.2f,%0.2f) intensity=%0.2f\n",
              lightPos.x, lightPos.y, lightPos.z, camPos.x, camPos.y, camPos.z, lightIntensity);
      lightLogged = true;
    }
  }

  if (blinnProgram_ != 0) {
    glUseProgram(blinnProgram_);
    GLint loc = glGetUniformLocation(blinnProgram_, "view");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view));
    loc = glGetUniformLocation(blinnProgram_, "projection");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(proj));

    GLint lp = glGetUniformLocation(blinnProgram_, "light.position");
    GLint lc = glGetUniformLocation(blinnProgram_, "light.color");
    GLint li = glGetUniformLocation(blinnProgram_, "light.intensity");
    if (lp >= 0) glUniform3f(lp, lightPos.x, lightPos.y, lightPos.z);
    if (lc >= 0) glUniform3f(lc, lightColor.r, lightColor.g, lightColor.b);
    if (li >= 0) glUniform1f(li, lightIntensity);

    // lamp for blinn shader as well
    GLint lpp = glGetUniformLocation(blinnProgram_, "lampLight.position");
    GLint lpc = glGetUniformLocation(blinnProgram_, "lampLight.color");
    GLint lpi = glGetUniformLocation(blinnProgram_, "lampLight.intensity");
    GLint len = glGetUniformLocation(blinnProgram_, "lampEnabled");
    if (lpp >= 0) glUniform3f(lpp, lampPos_.x, lampPos_.y, lampPos_.z);
    if (lpc >= 0) glUniform3f(lpc, lampColor_.x, lampColor_.y, lampColor_.z);
    if (lpi >= 0) glUniform1f(lpi, lampIntensity_);
    if (len >= 0) glUniform1i(len, lampEnabled_ ? 1 : 0);

    GLint viewPosLoc = glGetUniformLocation(blinnProgram_, "viewPos");
    if (viewPosLoc >= 0) glUniform3f(viewPosLoc, camPos.x, camPos.y, camPos.z);
  }
  glUseProgram(0);
}

void Renderer::setSceneLight(const glm::vec3& pos, const glm::vec3& color, float intensity) {
  sceneLightPos_ = pos;
  sceneLightColor_ = color;
  // keep scene light always on, even if caller passes 0
  sceneLightIntensity_ = (intensity > 0.0f) ? intensity : 2.5f;
}

void Renderer::setLampLight(const glm::vec3& pos, const glm::vec3& color, float intensity, bool enabled) {
  lampPos_ = pos;
  lampColor_ = color;
  lampIntensity_ = intensity;
  lampEnabled_ = enabled;
}

std::string Renderer::loadShaderSource(const char* path) {
  std::vector<std::string> candidates = { std::string(path), std::string("../") + path, std::string("./") + path };
  for (const auto& p : candidates) {
    std::ifstream in(p);
    if (in) {
      std::stringstream ss; ss << in.rdbuf();
      std::cerr << "Loaded shader from: " << p << std::endl;
      return ss.str();
    }
  }

  std::string basename(path);
  auto pos = basename.find_last_of("/\\");
  if (pos != std::string::npos) basename = basename.substr(pos + 1);
  std::vector<std::string> more = { std::string("Shaders/") + basename, std::string("../Shaders/") + basename };
  for (const auto& p : more) {
    std::ifstream in(p);
    if (in) {
      std::stringstream ss; ss << in.rdbuf();
      std::cerr << "Loaded shader from: " << p << std::endl;
      return ss.str();
    }
  }

  return std::string();
}

static unsigned int compileShader(GLenum type, const std::string& src) {
  unsigned int shader = glCreateShader(type);
  const char* cstr = src.c_str();
  glShaderSource(shader, 1, &cstr, nullptr);
  glCompileShader(shader);
  GLint ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint len = 0; glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetShaderInfoLog(shader, len, nullptr, &log[0]);
    std::cerr << "Shader compile error: " << log << std::endl;
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

unsigned int Renderer::createShaderProgram(const char* vertPath, const char* fragPath) {
  std::string vertSrc = loadShaderSource(vertPath);
  std::string fragSrc = loadShaderSource(fragPath);
  if (vertSrc.empty() || fragSrc.empty()) {
    std::cerr << "Failed to load shader sources: " << vertPath << ", " << fragPath << std::endl;
    return 0;
  }

  unsigned int vert = compileShader(GL_VERTEX_SHADER, vertSrc);
  if (vert == 0) return 0;
  unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
  if (frag == 0) { glDeleteShader(vert); return 0; }

  unsigned int prog = glCreateProgram();
  glAttachShader(prog, vert);
  glAttachShader(prog, frag);
  glLinkProgram(prog);
  GLint ok = 0; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetProgramInfoLog(prog, len, nullptr, &log[0]);
    std::cerr << "Program link error: " << log << std::endl;
    glDeleteProgram(prog);
    prog = 0;
  }

  // shaders can be deleted after linking
  glDetachShader(prog, vert);
  glDetachShader(prog, frag);
  glDeleteShader(vert);
  glDeleteShader(frag);

  return prog;
}
