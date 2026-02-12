#pragma once

#include <GL/glew.h>

struct Color
{
    float r, g, b, a;
};

struct RectShape
{
    float x, y, w, h;
    Color color;
};

struct CircleShape
{
    float x, y, radius;
    Color color;
};

class Renderer;

class Renderer2D
{
public:
    Renderer2D(int windowWidth, int windowHeight, const char* vertexShaderPath, const char* fragmentShaderPath);
    ~Renderer2D();

    void drawRect(float x, float y, float w, float h, const Color& color) const;
    void drawCircle(float cx, float cy, float radius, const Color& color, int segments = 48) const;
    void drawFrame(const RectShape& rect, float thickness) const;
    void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color& color) const;
    void setWindowSize(float width, float height);

    void set3DRenderer(Renderer* r);

private:
    float m_windowWidth;
    float m_windowHeight;
    GLuint m_program = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLint m_uColorLocation = -1;

    // optional 3D renderer to draw placeholders instead of 2D
    Renderer* renderer3D_ = nullptr;
};
