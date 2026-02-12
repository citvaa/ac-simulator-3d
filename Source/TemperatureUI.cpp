#include "../Header/TemperatureUI.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>

namespace
{
    int clampInt(int v, int lo, int hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    // Simple flame shape built from circles and bands.
    void drawHeatIcon(Renderer2D& renderer, float cx, float cy, float radius, const Color& outer, const Color& inner)
    {
        float baseY = cy + radius * 0.04f; // slight drop to center visually

        // Outer flame: layered taper made of circles and shrinking bands
        renderer.drawCircle(cx, baseY, radius, outer);

        float bandHeight = radius * 0.25f;
        float bandWidth = radius * 1.3f;
        for (int i = 0; i < 5; ++i)
        {
            float t = static_cast<float>(i);
            float shrink = 1.0f - t * 0.18f;
            float w = bandWidth * shrink;
            float h = bandHeight;
            float y = baseY + radius * 0.35f - t * (h * 0.75f);
            renderer.drawRect(cx - w * 0.5f, y - h * 0.5f, w, h, outer);
        }

        // Inner flame: smaller droplet for contrast
        float innerR = radius * 0.6f;
        renderer.drawCircle(cx, baseY + innerR * 0.05f, innerR, inner);

        float innerBandW = innerR * 1.1f;
        float innerBandH = innerR * 0.35f;
        for (int i = 0; i < 3; ++i)
        {
            float t = static_cast<float>(i);
            float shrink = 1.0f - t * 0.22f;
            float w = innerBandW * shrink;
            float h = innerBandH;
            float y = baseY + innerR * 0.4f - t * (h * 0.8f);
            renderer.drawRect(cx - w * 0.5f, y - h * 0.5f, w, h, inner);
        }
    }

    void drawSnowIcon(Renderer2D& renderer, float cx, float cy, float size, const Color& color)
    {
        float arm = size * 0.48f;
        float thickness = size * 0.12f;

        // Cross arms
        renderer.drawRect(cx - thickness * 0.5f, cy - arm, thickness, arm * 2.0f, color);
        renderer.drawRect(cx - arm, cy - thickness * 0.5f, arm * 2.0f, thickness, color);

        // End caps
        float cap = thickness * 1.2f;
        renderer.drawRect(cx - cap * 0.5f, cy - arm - cap * 0.5f, cap, cap, color);
        renderer.drawRect(cx - cap * 0.5f, cy + arm - cap * 0.5f, cap, cap, color);
        renderer.drawRect(cx - arm - cap * 0.5f, cy - cap * 0.5f, cap, cap, color);
        renderer.drawRect(cx + arm - cap * 0.5f, cy - cap * 0.5f, cap, cap, color);

        // Diagonal arms made of stepped squares (since we can't rotate rects)
        float step = thickness * 0.9f;
        int steps = 4;
        auto drawDiag = [&](float dxSign, float dySign)
        {
            for (int i = 1; i <= steps; ++i)
            {
                float off = step * static_cast<float>(i);
                renderer.drawRect(cx + dxSign * off - step * 0.5f, cy + dySign * off - step * 0.5f, step, step, color);
                renderer.drawRect(cx + dxSign * (off - step * 0.5f) - step * 0.5f, cy + dySign * (off + step * 0.5f) - step * 0.5f, step, step, color);
            }
        };

        drawDiag(1.0f, 1.0f);
        drawDiag(1.0f, -1.0f);
        drawDiag(-1.0f, 1.0f);
        drawDiag(-1.0f, -1.0f);
    }

    // Checkmark assembled from tiny squares.
    void drawCheckIcon(Renderer2D& renderer, float cx, float cy, float size, const Color& color)
    {
        float dot = size * 0.1f;
        float startX = cx - size * 0.35f;
        float startY = cy + size * 0.05f;

        for (int i = 0; i < 4; ++i)
        {
            float step = static_cast<float>(i) * dot * 1.1f;
            renderer.drawRect(startX + step, startY + step, dot, dot, color);
        }

        float midX = startX + 3.0f * dot * 1.1f;
        float midY = startY + 3.0f * dot * 1.1f;
        for (int i = 0; i < 6; ++i)
        {
            float step = static_cast<float>(i) * dot * 1.1f;
            renderer.drawRect(midX + step, midY - step, dot, dot, color);
        }
    }
}

void drawTemperatureValue(TextRenderer& textRenderer, float value, const RectShape& screen, const Color& color)
{
    // Auto-scale numeric text to fit screen bounds.
    int rounded = static_cast<int>(std::round(value));
    rounded = clampInt(rounded, -99, 99);

    std::ostringstream ss;
    ss << rounded;
    std::string text = ss.str();

    TextMetrics metrics = textRenderer.measure(text, 1.0f);
    float baseWidth = std::max(metrics.width, 1.0f);
    float baseHeight = std::max(metrics.height, 1.0f);

    // Scale text to fit the display area with a small margin.
    float scaleX = screen.w / baseWidth;
    float scaleY = screen.h / baseHeight;
    float scale = std::min(scaleX, scaleY) * 0.85f;

    float finalWidth = metrics.width * scale;
    float finalHeight = metrics.height * scale;

    float startX = screen.x + (screen.w - finalWidth) * 0.5f;
    float startY = screen.y + (screen.h - finalHeight) * 0.5f;

    textRenderer.drawText(text, startX, startY, scale, color);
}

void drawStatusIcon(Renderer2D& renderer, const RectShape& screen, float desired, float current)
{
    float cx = screen.x + screen.w * 0.5f;
    float cy = screen.y + screen.h * 0.5f;
    float size = std::min(screen.w, screen.h) * 0.35f;

    const float tolerance = 0.25f;
    float diff = desired - current;

    Color heatOuter{ 0.96f, 0.46f, 0.28f, 1.0f };
    Color heatInner{ 0.99f, 0.66f, 0.32f, 1.0f };
    Color snowColor{ 0.66f, 0.85f, 0.98f, 1.0f };
    Color checkColor{ 0.38f, 0.92f, 0.58f, 1.0f };

    if (diff > tolerance)
    {
        drawHeatIcon(renderer, cx, cy, size, heatOuter, heatInner);
    }
    else if (diff < -tolerance)
    {
        drawSnowIcon(renderer, cx, cy, size, snowColor);
    }
    else
    {
        drawCheckIcon(renderer, cx, cy, size, checkColor);
    }
}
