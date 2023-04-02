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
layout(binding = 21, std430) coherent buffer q1Buffer{
    float q1s[];
};
out VS_OUT {
    vec3 maxPrincipal;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform uint size;
//uniform mat4 projection;

void main()
{
    gl_Position =  view * model * vec4(inPosition, 1.0); 
    /*
    vec3 maxPD = vec3(PDs[gl_VertexID]);
    vec3 minPD = vec3(PDs[gl_VertexID+size]);
    */
 
    vs_out.maxPrincipal = normalize(mat3(view*model) * vec3(q1s[gl_VertexID]));

}