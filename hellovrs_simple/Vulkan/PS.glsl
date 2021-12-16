#version 450

layout(location = 0) out vec4 outColor;

void main() 
{
    vec4 position = gl_FragCoord;

    vec4 color = vec4(
        sin(position.x / 10.0f), 
        sin(position.y / 10.0f), 
        cos((position.x + position.y) / 10.0f),
        1);
    outColor = color;
}
