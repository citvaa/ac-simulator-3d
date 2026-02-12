#pragma once

#include "../Header/Renderer2D.h"

#include <GL/glew.h>
#include <map>
#include <string>
#include <vector>

struct Glyph
{
    GLuint texture = 0;
    int width = 0;
    int height = 0;
    int bearingX = 0;
    int bearingY = 0;
    unsigned int advance = 0;
};

struct TextMetrics
{
    float width = 0.0f;
    float height = 0.0f;
    float ascent = 0.0f;
};

class TextRenderer
{
public:
    TextRenderer(int windowWidth, int windowHeight);
    ~TextRenderer();

    bool loadFont(const std::string& fontPath, unsigned int pixelHeight = 48);
    void setWindowSize(float width, float height);

    // Draw text with origin at top-left corner of the first glyph box.
    void drawText(const std::string& text, float x, float y, float scale, const Color& color);
    TextMetrics measure(const std::string& text, float scale = 1.0f) const;
    bool createTextTexture(const std::string& text, const Color& textColor, const Color& bgColor, unsigned int padding, unsigned int pixelHeight, GLuint& outTexture, int& outWidth, int& outHeight);

private:
    void cleanup();
    void destroyGlyphTextures();

    float m_windowWidth = 0.0f;
    float m_windowHeight = 0.0f;
    unsigned int m_fontPixelHeight = 0;
    std::string m_fontPath;

    GLuint m_program = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_blankTexture = 0;
    GLint m_uTextColor = -1;
    GLint m_uWindowSize = -1;
    GLint m_uTexture = -1;

    std::map<char, Glyph> m_glyphs;
};
