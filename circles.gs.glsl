#version 430
layout (lines) in;                              
layout (triangle_strip, max_vertices = 34) out; // 16 steps

uniform int img_width;
uniform int img_height;

uniform float line_thickness;

const vec2  u_viewportSize = vec2(img_width, img_height);
const float u_thickness = line_thickness;
const float steps = 16;
const float pi = 4.0*atan(1.0);

void main()
{
    const vec4 position = gl_in[0].gl_Position;
    const float radius = 10.0;// vec2(u_thickness / u_viewportSize);

    float slice = 2.0f * pi / 20;

	for (int i = 0; i < steps; i++)
	{
		float angle = slice * i;
		float x = position.x + radius * cos(angle);
		float y = position.y + radius * sin(angle);

		gl_Position = vec4(10, 10, 0, 1);// vec4(x, y, 0, 0);
		EmitVertex();
	}

    EndPrimitive();
}