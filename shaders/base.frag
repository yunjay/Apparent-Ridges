#version 430
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
out vec4 color;

uniform vec3 backgroundColor;
void main() {
    color = vec4(backgroundColor,1.0);
}