#version 330 core

layout(location = 0) in vec2 inPos;
uniform vec2 uPosOffset; // Za pomeranje objekta ako je potrebno

void main()
{
    gl_Position = vec4(inPos + uPosOffset, 0.0, 1.0);
}