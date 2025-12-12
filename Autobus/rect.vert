#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTex;
out vec2 chTex;

uniform float uX; // Pomeraj X
uniform float uY; // Pomeraj Y
uniform float uS; // Skaliranje (Y i X)

void main()
{
    // Primena skaliranja i translacije
    gl_Position = vec4((inPos.x * uS) + uX, (inPos.y * uS) + uY, 0.0, 1.0);
    chTex = inTex;
}