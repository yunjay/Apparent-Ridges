#version 430
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 maxPD;
layout (location = 4) in vec3 minPD;
layout (location = 5) in float maxCurv;
layout (location = 6) in float minCurv;


out VertexData{
    out vec3 ormal;
    out vec3 fragPos;
    out vec2 texCoords;
} vertexOut

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view *  model * vec4(aPos, 1.0f);
    fragPos = vec3(model * vec4(aPos, 1.0f)); 
    normal = mat3(transpose(inverse(model))) * aNormal;
    texCoords = aTexCoords;
}

