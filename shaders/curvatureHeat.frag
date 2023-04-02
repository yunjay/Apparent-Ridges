#version 460 
//vec3 smoothedNormals[20][]; -> not legal
out vec4 color;
in vec3 curvColor;

void main()
{
    color = vec4(curvColor, 1.0);
}