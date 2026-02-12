#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;

out vec2 TexCoord;

uniform vec2 uWindowSize;

void main()
{
    vec2 ndc;
    ndc.x = 2.0 * aPos.x / uWindowSize.x - 1.0;
    ndc.y = 1.0 - 2.0 * aPos.y / uWindowSize.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
    TexCoord = aUV;
}
