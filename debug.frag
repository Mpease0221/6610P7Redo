#version 330 core
out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D tvTexture;

void main()
{
    FragColor = vec4(texture(tvTexture, texCoords).rgb, 1.0);
}