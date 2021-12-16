#version 450

vec2 positions[3] = vec2[](
    vec2(-1, -1), // top left
    vec2(1, -1), // top right
    vec2(-1, 1) // bottom left
);

void main() 
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}