#include "../Header/TextRenderer.h"

#include "../Header/Util.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <iostream>
#include <fstream>

namespace
{
    constexpr const char* kTextVertexShader = "Shaders/text.vert";
    constexpr const char* kTextFragmentShader = "Shaders/text.frag";

    static std::string detectDefaultFontPath()
    {
        const char* candidates[] = {
            "C:\\Windows\\Fonts\\arial.ttf",
            "/Library/Fonts/Arial.ttf",
            "/System/Library/Fonts/Supplemental/Arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
        };
        for (const char* p : candidates)
        {
            std::ifstream f(p);
            if (f.good()) return std::string(p);
        }
        return std::string();
    }
}

TextRenderer::TextRenderer(int windowWidth, int windowHeight)
    : m_windowWidth(static_cast<float>(windowWidth))
    , m_windowHeight(static_cast<float>(windowHeight))
{
    m_program = createShader(kTextVertexShader, kTextFragmentShader);
    m_uTextColor = glGetUniformLocation(m_program, "uTextColor");
    m_uWindowSize = glGetUniformLocation(m_program, "uWindowSize");
    m_uTexture = glGetUniformLocation(m_program, "uTexture");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Create a tiny 1x1 white fallback texture bound to texture unit 0 so shaders always have a valid texture.
    unsigned char whitePixel[4] = { 255, 255, 255, 255 };
    glGenTextures(1, &m_blankTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_blankTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Attempt to load a default font from common system locations so the UI is usable out of the box.
    std::string detectedFont = detectDefaultFontPath();
    if (!detectedFont.empty())
    {
        m_fontPath = detectedFont;
        loadFont(m_fontPath, 48);
    }
    else
    {
        m_fontPath = std::string();
        std::cout << "No default font found on system; text rendering will be disabled until you set a font path.\n";
    }
}

TextRenderer::~TextRenderer()
{
    cleanup();
}

void TextRenderer::cleanup()
{
    destroyGlyphTextures();

    if (m_blankTexture != 0) glDeleteTextures(1, &m_blankTexture);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_program != 0) glDeleteProgram(m_program);

    m_blankTexture = 0;
    m_vbo = 0;
    m_vao = 0;
    m_program = 0;
}

void TextRenderer::destroyGlyphTextures()
{
    for (auto& kv : m_glyphs)
    {
        if (kv.second.texture != 0)
        {
            glDeleteTextures(1, &kv.second.texture);
        }
    }
    m_glyphs.clear();
}

bool TextRenderer::loadFont(const std::string& fontPath, unsigned int pixelHeight)
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "FreeType init failed.\n";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face))
    {
        std::cout << "Failed to load font: " << fontPath << "\n";
        FT_Done_FreeType(ft);
        return false;
    }

    m_fontPath = fontPath;
    FT_Set_Pixel_Sizes(face, 0, pixelHeight);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    destroyGlyphTextures();
    m_fontPixelHeight = pixelHeight;

    // preload a broad set of common printable ASCII characters so UI strings render reliably
    const std::string charset = " !\"#$%&'()*+,-./0123456789:;<>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"; // glyphs we preload up front
    for (char c : charset)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "Failed to load glyph: " << c << "\n";
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Glyph glyph;
        glyph.texture = texture;
        glyph.width = face->glyph->bitmap.width;
        glyph.height = face->glyph->bitmap.rows;
        glyph.bearingX = face->glyph->bitmap_left;
        glyph.bearingY = face->glyph->bitmap_top;
        glyph.advance = static_cast<unsigned int>(face->glyph->advance.x);

        m_glyphs[c] = glyph;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return !m_glyphs.empty();
}

void TextRenderer::setWindowSize(float width, float height)
{
    m_windowWidth = width;
    m_windowHeight = height;
}

TextMetrics TextRenderer::measure(const std::string& text, float scale) const
{
    float width = 0.0f;
    float maxAscent = 0.0f;
    float maxDescent = 0.0f;

    for (char c : text)
    {
        auto it = m_glyphs.find(c);
        if (it == m_glyphs.end()) continue;
        const Glyph& g = it->second;

        width += (g.advance >> 6) * scale;
        maxAscent = std::max(maxAscent, static_cast<float>(g.bearingY) * scale);
        float descent = static_cast<float>(g.height - g.bearingY) * scale;
        maxDescent = std::max(maxDescent, descent);
    }

    // Fallback so height is non-zero even if glyph set is incomplete.
    if (maxAscent + maxDescent <= 0.0f && m_fontPixelHeight > 0)
    {
        maxAscent = static_cast<float>(m_fontPixelHeight) * scale;
    }

    TextMetrics metrics;
    metrics.width = width;
    metrics.ascent = maxAscent;
    metrics.height = maxAscent + maxDescent;
    return metrics;
}

void TextRenderer::drawText(const std::string& text, float x, float y, float scale, const Color& color)
{
    if (m_glyphs.empty()) return;

    TextMetrics m = measure(text, scale);
    float baselineY = y + m.ascent;

    glUseProgram(m_program);
    glUniform4f(m_uTextColor, color.r, color.g, color.b, color.a);
    glUniform2f(m_uWindowSize, m_windowWidth, m_windowHeight);
    glUniform1i(m_uTexture, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_vao);

    float cursorX = x;
    for (char c : text)
    {
        auto it = m_glyphs.find(c);
        if (it == m_glyphs.end()) continue;
        const Glyph& g = it->second;

        float xpos = cursorX + static_cast<float>(g.bearingX) * scale;
        float ypos = baselineY - static_cast<float>(g.bearingY) * scale;

        float w = static_cast<float>(g.width) * scale;
        float h = static_cast<float>(g.height) * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos,     ypos,     0.0f, 1.0f },
            { xpos + w, ypos,     1.0f, 1.0f },

            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos + w, ypos,     1.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, g.texture);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        cursorX += (g.advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool TextRenderer::createTextTexture(const std::string& text, const Color& textColor, const Color& bgColor, unsigned int padding, unsigned int pixelHeight, GLuint& outTexture, int& outWidth, int& outHeight)
{
    auto decodeUtf8 = [](const std::string& s)
    {
        std::vector<uint32_t> codepoints;
        size_t i = 0;
        while (i < s.size())
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (c < 0x80)
            {
                codepoints.push_back(c);
                ++i;
            }
            else if ((c >> 5) == 0x6 && i + 1 < s.size())
            {
                uint32_t cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(s[i + 1]) & 0x3F);
                codepoints.push_back(cp);
                i += 2;
            }
            else if ((c >> 4) == 0xE && i + 2 < s.size())
            {
                uint32_t cp = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 6) | (static_cast<unsigned char>(s[i + 2]) & 0x3F);
                codepoints.push_back(cp);
                i += 3;
            }
            else if ((c >> 3) == 0x1E && i + 3 < s.size())
            {
                uint32_t cp = ((c & 0x07) << 18) | ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 12) | ((static_cast<unsigned char>(s[i + 2]) & 0x3F) << 6) | (static_cast<unsigned char>(s[i + 3]) & 0x3F);
                codepoints.push_back(cp);
                i += 4;
            }
            else
            {
                ++i; // skip invalid byte
            }
        }
        return codepoints;
    };

    std::vector<uint32_t> codepoints = decodeUtf8(text);
    if (codepoints.empty())
    {
        std::cout << "No characters to render for text texture.\n";
        return false;
    }

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "FreeType init failed for text texture.\n";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, m_fontPath.c_str(), 0, &face))
    {
        std::cout << "Failed to load font for text texture: " << m_fontPath << "\n";
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, pixelHeight);

    // Measure text
    int width = 0;
    int maxAscent = 0;
    int maxDescent = 0;
    for (uint32_t cp : codepoints)
    {
        if (FT_Load_Char(face, cp, FT_LOAD_RENDER))
        {
            std::cout << "Failed to load glyph codepoint: " << cp << "\n";
            continue;
        }
        width += face->glyph->advance.x >> 6;
        maxAscent = std::max(maxAscent, face->glyph->bitmap_top);
        int descent = static_cast<int>(face->glyph->bitmap.rows) - face->glyph->bitmap_top;
        maxDescent = std::max(maxDescent, descent);
    }

    if (width == 0)
    {
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        return false;
    }

    width += static_cast<int>(padding) * 2;
    int height = maxAscent + maxDescent + static_cast<int>(padding) * 2;
    outWidth = width;
    outHeight = height;

    std::vector<unsigned char> pixels(static_cast<size_t>(width * height * 4), 0);

    auto toByte = [](float v)
    {
        float clamped = std::max(0.0f, std::min(1.0f, v));
        return static_cast<unsigned char>(clamped * 255.0f + 0.5f);
    };

    unsigned char bgR = toByte(bgColor.r);
    unsigned char bgG = toByte(bgColor.g);
    unsigned char bgB = toByte(bgColor.b);
    unsigned char bgA = toByte(bgColor.a);

    unsigned char textR = toByte(textColor.r);
    unsigned char textG = toByte(textColor.g);
    unsigned char textB = toByte(textColor.b);
    float textAlpha = std::max(0.0f, std::min(1.0f, textColor.a));

    for (int i = 0; i < width * height; ++i)
    {
        pixels[i * 4 + 0] = bgR;
        pixels[i * 4 + 1] = bgG;
        pixels[i * 4 + 2] = bgB;
        pixels[i * 4 + 3] = bgA;
    }

    int cursorX = static_cast<int>(padding);
    int baseline = static_cast<int>(padding) + maxAscent;

    for (uint32_t cp : codepoints)
    {
        if (FT_Load_Char(face, cp, FT_LOAD_RENDER))
        {
            continue;
        }

        int glyphW = face->glyph->bitmap.width;
        int glyphH = face->glyph->bitmap.rows;
        int bearingX = face->glyph->bitmap_left;
        int bearingY = face->glyph->bitmap_top;

        int xPos = cursorX + bearingX;
        int yPos = baseline - bearingY;

        for (int row = 0; row < glyphH; ++row)
        {
            for (int col = 0; col < glyphW; ++col)
            {
                unsigned char alpha = face->glyph->bitmap.buffer[row * glyphW + col];
                if (alpha == 0) continue;

                int px = xPos + col;
                int py = yPos + row;
                if (px < 0 || py < 0 || px >= width || py >= height) continue;

                size_t idx = (static_cast<size_t>(py) * static_cast<size_t>(width) + static_cast<size_t>(px)) * 4;
                pixels[idx + 0] = textR;
                pixels[idx + 1] = textG;
                pixels[idx + 2] = textB;
                pixels[idx + 3] = static_cast<unsigned char>(std::min(255.0f, alpha * textAlpha));
            }
        }

        cursorX += face->glyph->advance.x >> 6;
    }

    glGenTextures(1, &outTexture);
    glBindTexture(GL_TEXTURE_2D, outTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return true;
}
