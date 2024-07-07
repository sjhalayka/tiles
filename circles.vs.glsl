#version 430
layout (location = 0) in vec4 position;

//uniform mat4  u_modelviewprojection_matrix;

void main()
{
    gl_Position = position;// u_modelviewprojection_matrix* vec4(position, 1.0);
}