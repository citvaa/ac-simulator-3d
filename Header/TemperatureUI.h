#pragma once

#include "../Header/Renderer2D.h"
#include "../Header/TextRenderer.h"

void drawTemperatureValue(TextRenderer& textRenderer, float value, const RectShape& screen, const Color& color);
void drawStatusIcon(Renderer2D& renderer, const RectShape& screen, float desired, float current);
