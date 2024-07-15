#version 430

uniform sampler2D tex; // texture uniform

in vec2 ftexcoord;

uniform float opacity;

layout(location = 0) out vec4 FragColor;

void main() 
{
   FragColor = texture(tex, ftexcoord);
   FragColor = vec4(FragColor.xyz, opacity);
};