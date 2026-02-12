#include "../Header/Controls.h"

bool pointInRect(double px, double py, const RectShape& rect)
{
    return px >= rect.x && px <= rect.x + rect.w && py >= rect.y && py <= rect.y + rect.h;
}

void drawHalfArrow(Renderer2D& renderer, const RectShape& button, bool isUp, const Color& arrowColor, const Color& bgColor)
{
    float cx = button.x + button.w * 0.5f;
    float cy = button.y + button.h * 0.5f;
    float margin = button.w * 0.22f;
    float topY = button.y + margin;
    float bottomY = button.y + button.h - margin;

    renderer.drawRect(button.x, button.y, button.w, button.h, bgColor);

    float ax = cx;
    float bx = button.x + margin;
    float cxv = button.x + button.w - margin;

    // Render arrow as stacked rectangles so it appears correctly in 3D mode
    int steps = 8;
    float stepH = (bottomY - topY) / static_cast<float>(steps);
    float maxW = button.w - 2.0f * margin;
    for (int i = 0; i < steps; ++i)
    {
        float t = (static_cast<float>(i) + 1.0f) / static_cast<float>(steps);
        float w = maxW * t;
        float rx = cx - w * 0.5f;
        float ry;
        if (isUp) {
            ry = topY + static_cast<float>(i) * stepH;
        } else {
            // stack from bottom upwards for down arrow
            ry = bottomY - (static_cast<float>(i) + 1.0f) * stepH;
        }
        // slightly shrink height so stacks look triangular
        renderer.drawRect(rx, ry, w, stepH * 0.9f, arrowColor);
    }
}
