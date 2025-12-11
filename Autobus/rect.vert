#version 330 core

layout(location = 0) in vec2 inPos; // Pozicija
layout(location = 1) in vec2 inTex; // Teksturne koordinate
out vec2 chTex;

void main()
{
    gl_Position = vec4(inPos, 0.0, 1.0); // Crtamo direktno na inPos
    chTex = inTex;
}