#include "../Header/Renderer2D.h"

#include "../Header/Util.h"
#include "Renderer.h"

#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
    void fillRectVertices(float x, float y, float w, float h, float windowWidth, float windowHeight, float* outVertices)
    {
        // Convert from pixel coords (origin top-left) to NDC (-1..1)
        float x0 = 2.0f * x / windowWidth - 1.0f;
        float x1 = 2.0f * (x + w) / windowWidth - 1.0f;
        float y0 = 1.0f - 2.0f * y / windowHeight;
        float y1 = 1.0f - 2.0f * (y + h) / windowHeight;

        // Two triangles (6 vertices)
        outVertices[0] = x0; outVertices[1] = y0;
        outVertices[2] = x1; outVertices[3] = y0;
        outVertices[4] = x1; outVertices[5] = y1;

        outVertices[6] = x0; outVertices[7] = y0;
        outVertices[8] = x1; outVertices[9] = y1;
        outVertices[10] = x0; outVertices[11] = y1;
    }
}

Renderer2D::Renderer2D(int windowWidth, int windowHeight, const char* vertexShaderPath, const char* fragmentShaderPath)
    : m_windowWidth(static_cast<float>(windowWidth))
    , m_windowHeight(static_cast<float>(windowHeight))
{
    m_program = createShader(vertexShaderPath, fragmentShaderPath);
    m_uColorLocation = glGetUniformLocation(m_program, "uColor");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    float initialVertices[12] = { 0.0f };
    glBufferData(GL_ARRAY_BUFFER, sizeof(initialVertices), initialVertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

Renderer2D::~Renderer2D()
{
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_program != 0) glDeleteProgram(m_program);
}

void Renderer2D::setWindowSize(float width, float height)
{
    m_windowWidth = width;
    m_windowHeight = height;
}

// Helper: convert pixel center to world position using simple mapping (pixels -> world * scale)
static glm::vec3 pixelToWorld(float px, float py, float windowW, float windowH)
{
    // center origin
    float sx = (px - windowW * 0.5f);
    float sy = (windowH * 0.5f - py);
    const float scale = 0.5f; // world units per pixel (empirical)
    return glm::vec3(sx * scale, sy * scale, 80.0f); // place slightly in front of AC base
}

void Renderer2D::drawRect(float x, float y, float w, float h, const Color& color) const
{
    if (renderer3D_) {
        // draw thin box in 3D at mapped position
        float cx = x + w * 0.5f;
        float cy = y + h * 0.5f;
        glm::vec3 pos = pixelToWorld(cx, cy, m_windowWidth, m_windowHeight);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, glm::vec3(w * 0.5f, h * 0.5f, 2.0f));
        renderer3D_->drawCube(model, glm::vec3(color.r, color.g, color.b));
        return;
    }

    float vertices[12];
    fillRectVertices(x, y, w, h, m_windowWidth, m_windowHeight, vertices);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glUseProgram(m_program);
    glUniform4f(m_uColorLocation, color.r, color.g, color.b, color.a);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer2D::drawCircle(float cx, float cy, float radius, const Color& color, int segments) const
{
    if (renderer3D_) {
        glm::vec3 pos = pixelToWorld(cx, cy, m_windowWidth, m_windowHeight);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, glm::vec3(radius * 0.5f, radius * 0.5f, 2.0f));
        renderer3D_->drawCube(model, glm::vec3(color.r, color.g, color.b));
        return;
    }

    // Fan triangulation for filled circle.
    std::vector<float> vertices;
    vertices.reserve((segments + 2) * 2);

    float centerX = 2.0f * cx / m_windowWidth - 1.0f;
    float centerY = 1.0f - 2.0f * cy / m_windowHeight;
    vertices.push_back(centerX);
    vertices.push_back(centerY);

    const float twoPi = 6.28318530718f;
    for (int i = 0; i <= segments; ++i)
    {
        float angle = twoPi * static_cast<float>(i) / static_cast<float>(segments);
        float px = cx + std::cos(angle) * radius;
        float py = cy + std::sin(angle) * radius;

        float ndcX = 2.0f * px / m_windowWidth - 1.0f;
        float ndcY = 1.0f - 2.0f * py / m_windowHeight;

        vertices.push_back(ndcX);
        vertices.push_back(ndcY);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glUseProgram(m_program);
    glUniform4f(m_uColorLocation, color.r, color.g, color.b, color.a);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size() / 2));
    glBindVertexArray(0);
}

void Renderer2D::drawFrame(const RectShape& rect, float thickness) const
{
    drawRect(rect.x, rect.y, rect.w, thickness, rect.color); // top
    drawRect(rect.x, rect.y + rect.h - thickness, rect.w, thickness, rect.color); // bottom
    drawRect(rect.x, rect.y, thickness, rect.h, rect.color); // left
    drawRect(rect.x + rect.w - thickness, rect.y, thickness, rect.h, rect.color); // right
}

void Renderer2D::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color& color) const
{
    if (renderer3D_) {
        float cx = (x1 + x2 + x3) / 3.0f;
        float cy = (y1 + y2 + y3) / 3.0f;
        glm::vec3 pos = pixelToWorld(cx, cy, m_windowWidth, m_windowHeight);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = glm::scale(model, glm::vec3(10.0f, 10.0f, 2.0f));
        renderer3D_->drawCube(model, glm::vec3(color.r, color.g, color.b));
        return;
    }

    float vertices[6];
    auto toNdc = [&](float x, float y)
    {
        float ndcX = 2.0f * x / m_windowWidth - 1.0f;
        float ndcY = 1.0f - 2.0f * y / m_windowHeight;
        return std::pair<float, float>(ndcX, ndcY);
    };

    auto p1 = toNdc(x1, y1);
    auto p2 = toNdc(x2, y2);
    auto p3 = toNdc(x3, y3);

    vertices[0] = p1.first; vertices[1] = p1.second;
    vertices[2] = p2.first; vertices[3] = p2.second;
    vertices[4] = p3.first; vertices[5] = p3.second;

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glUseProgram(m_program);
    glUniform4f(m_uColorLocation, color.r, color.g, color.b, color.a);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Renderer2D::set3DRenderer(Renderer* r) {
    renderer3D_ = r;
}
