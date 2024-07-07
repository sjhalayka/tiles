#version 430

out vec4 fragColor;

uniform vec4 colour;

void main()
{
    fragColor = colour;
}