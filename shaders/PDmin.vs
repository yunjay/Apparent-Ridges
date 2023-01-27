#version 430 
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout(binding = 7, std430) readonly buffer PDBuffer 
{
    vec4 PDs[];
};
out VS_OUT {
    vec3 maxPrincipal;
    vec3 minPrincipal;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform uint size;
//uniform mat4 projection;

void main()
{
    vec3 maxPD = vec3(PDs[gl_VertexID]);
    vec3 minPD = vec3(PDs[gl_VertexID+size]);
    gl_Position =  view * model * vec4(aPos, 1.0); 
    vs_out.maxPrincipal = mat3(transpose(inverse(view*model))) * maxPD;
    vs_out.minPrincipal = mat3(transpose(inverse(view*model))) * minPD;

}