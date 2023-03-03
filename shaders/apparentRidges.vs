#version 460
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
layout(binding = 20, std430) readonly buffer adjacentFacesBuffer{
    int adjFaces[][20];
};
layout(binding = 21, std430) buffer q1Buffer{
    float q1s[];
};
layout(binding = 22, std430) buffer t1Buffer{
    vec2 t1s[];
};
layout(binding = 23, std430) buffer Dt1q1Buffer{
    float Dt1q1s[];
};
out VertexData{
    vec3 normal;
    vec3 pos;
    vec2 texCoords;
    float normalDotView; //normal dot viewdirection
    vec3 viewDirection;
    vec3 maxPrincpal;
    vec3 minPrincipal;
    float maxCurvature;
    float minCurvature;
    float q1;
    vec2 t1;
    float Dt1q1;
    uint id;
} vertexOut;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 viewPosition;

uniform float threshold;
void main() {
    gl_Position = projection * view *  model * vec4(inPosition, 1.0);

    vec3 position = vec3(model * vec4(inPosition, 1.0)); 
    //position = vec3(projection * view *  model * vec4(inPosition, 1.0));
    vec3 normal = mat3(transpose(inverse(model))) * inNormal;
    
    //I think Assimp normalizes the normals anyway.
    vec3 viewDir = normalize(viewPosition - position);
    
    float ndotv = dot(viewDir,normal);
    
    vertexOut.pos = position;
    vertexOut.normal = normal;
    vertexOut.texCoords = inTexCoords;  
    vertexOut.normalDotView = ndotv;
    vertexOut.viewDirection = viewDir;

    vertexOut.maxPrincpal = normalize(vec3(model *maxPD));
    vertexOut.minPrincipal = normalize(vec3(model *minPD));
    vertexOut.maxCurvature = maxCurv;
    vertexOut.minCurvature = minCurv;

    vertexOut.q1 = q1s[gl_VertexID];
    vertexOut.t1 = t1s[gl_VertexID];
    vertexOut.Dt1q1 = Dt1q1s[gl_VertexID];

    vertexOut.id = gl_VertexID;
}
