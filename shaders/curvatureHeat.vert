#version 460
layout (location = 0) in vec3 aPos;
layout (location = 7) in vec3 aCurvColor;

out vec3 curvColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view *  model * vec4(aPos, 1.0);
    //gl_Position =  model * vec4(aPos, 1.0);

    curvColor = aCurvColor;
}

