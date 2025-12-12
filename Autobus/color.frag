#version 330 core

out vec4 outCol;

uniform vec4 uColor; // Prima boju (RGBA)

void main()
{
    outCol = uColor;
}