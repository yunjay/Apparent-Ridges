#version 430
layout(triangles) in;
layout (line_strip, max_vertices=6) out;
layout(binding = 7, std430) buffer smoothedNormalsBuffer  
{
    vec4 smoothedNormals[];
};
//can also use geometryIn[i].gl_Position , which is vec4
in VertexData{
    out vec3 Normal;
    out vec3 FragPos;
    out vec2 TexCoords;
} geometryIn[];

uniform vec3 viewPosition;
uniform float threshold;
uniform mat4 projection;
/*
out VertexData{
    out vec3 Normal;
    out vec3 FragPos;
    out vec2 TexCoords;
} geometryOut[];
*/
//emit two vertices for each line...
void main() {
    //gl_in[0]; 
    //Compute principal view-dependent curvatures and principal view-dependent curv directions at each vertex 
    //For vertex 0 1 2 of the triangle face
    for(int i=0; i<3; i++){
        //I think Assimp normalizes the normals anyway.
        //this serves as cosine.
        float normalDotView = dot(normalize(geometryIn[i].Normal), normalize(viewPosition-vec3(geometryIn[i].gl_Position)));


    }



    //Estimate the derivative by finite difference

    //calculate eigenvalue / eigenvector of 2x2 matrix (tangent space)



}

