#version 430
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoords;
layout (location = 3) in vec4 maxPD;
layout (location = 4) in vec4 minPD;
layout (location = 5) in float maxCurv;
layout (location = 6) in float minCurv;
layout(binding = 7, std430) readonly buffer PDBuffer 
{
    vec4 PDs[];
};
layout(binding = 8, std430) readonly buffer curvatureBufffer{
    float curvatures[];
};

out VertexData{
    vec3 normal;
    vec3 pos;
    vec2 texCoords;
    float normalDotView; //normal dot viewdirection
    vec3 viewDirection;
    vec4 maxPrincpal;
    vec4 minPrincipal;
    float maxCurvature;
    float minCurvature;
} vertexOut;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 viewPosition;

void main() {
    gl_Position = projection * view *  model * vec4(inPosition, 1.0f);
    vec3 position = vec3(model * vec4(inPosition, 1.0f)); 
    vertexOut.Pos = position;

    vec3 normal = mat3(transpose(inverse(model))) * inNormal;
    vertexOut.normal = normal;
    vertexOut.texCoords = inTexCoords;  
    
    //I think Assimp normalizes the normals anyway.
    vec3 viewDir = normalize(viewPosition - position);
    vertexOut.normalDotView = dot(viewDir,normal);
    vertexOut.viewDirection = viewDir;

    vertexOut.maxPrincpal = maxPD;
    vertexOut.minPrincipal = minPD;
    vertexOut.maxCurvature = maxCurv;
    vertexOut.minCurvature = minCurv;
}
