#version 430
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout(binding = 7, std430) readonly buffer PDBuffer 
{
    vec4 PDs[];
};
layout(binding = 8, std430) readonly buffer curvatureBufffer{
    float curvatures[];
};

out VertexData{
    out vec3 normal;
    out vec3 fragPos;
    out vec2 texCoords;
} vertexOut

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view *  model * vec4(aPos, 1.0f);
    vertexOut.fragPos = vec3(model * vec4(aPos, 1.0f)); 
    vertexOut.normal = mat3(transpose(inverse(model))) * aNormal;
    vertexOut.texCoords = aTexCoords;
}

