#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uTint;

void main()
{
    // Flip Y because textures we upload are top-left origin.
    FragColor = texture(uTexture, vec2(vUV.x, 1.0 - vUV.y)) * uTint;
}
