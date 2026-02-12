#pragma once

#include "../Header/Renderer2D.h"

bool pointInRect(double px, double py, const RectShape& rect);
void drawHalfArrow(Renderer2D& renderer, const RectShape& button, bool isUp, const Color& arrowColor, const Color& bgColor);
