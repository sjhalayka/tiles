#version 430
layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;

out vec2 ftexcoord;

uniform int viewport_width;
uniform int viewport_height;

uniform mat4 projection;// = ortho(0.0f, 800.0f, 600.0f, 0.0f, -1000.0f, 1000.0f);
uniform mat4 view;// = lookAt(vec3(0, 0, 1), vec3(0, 0, 0), vec3(0, 1, 0));
uniform mat4 model;// = mat4();


void main()
{
	gl_Position = projection*view*model*position;

    ftexcoord = texcoord;
}