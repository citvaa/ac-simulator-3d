#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uTextColor;

void main()
{
    // Flip Y because FreeType/CPU buffers are top-left origin.
    float alpha = texture(uTexture, vec2(TexCoord.x, 1.0 - TexCoord.y)).r;
    FragColor = vec4(uTextColor.rgb, uTextColor.a * alpha);
}
