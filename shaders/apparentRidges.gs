#version 430
layout(triangles) in;
layout (line_strip, max_vertices=6) out;
layout(binding = 7, std430) readonly buffer PDBuffer 
{
    vec4 PDs[];
};
layout(binding = 8, std430) readonly buffer curvatureBufffer{
    float curvatures[];
};
//can also use geometryIn[i].gl_Position , which is vec4
in VertexData{
    vec3 normal;
    vec3 pos;
    vec2 texCoords;
    float normalDotView; //normal dot viewdirection
    vec3 viewDirection;
    vec4 maxPrincpal;
    vec4 minPrincipal;
    float maxCurvature;
    float minCurvature;
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
    //q1 : max view-dep curvature, t1 = max view-dep curvature direction
    float q1[3]={0.0,0.0,0.0};
    vec2 t1[3]={vec2(0.0),vec2(0.0),vec2(0.0)};
    float Dt1q1[3]={0.0,0.0,0.0};
    
    //First we compute the max view dependent curvature and direction at each vertex.------------------
    for(int i=0; i<3; i++){
        //basis with principal directions projected to viewDirection
        float u = dot(gl_in[i].viewDirection,vec3(gl_in[i].maxPrincipal));
        float v = dot(gl_in[i].viewDirection,vec3(gl_in[i].minPrincipal));
        float u2 = u*u;
        float v2 = v*v;
        // Kr * sin^2
        float kr = gl_in[i].maxCurvature * u2 + gl_in[i].minCurvature * v2;
        float csc2 = 1.0/(u2+v2);

        //Q is the view dependent curvature tensor
        //Why it is computed this way is beyond me..
        float sec_min1 = 1.0/abs(gl_in[i].normalDotView) - 1.0;
        float Q11 = gl_in[i].maxCurvature * (1.0 + sec_min1 * u2 *csc2);
        float Q12 = gl_in[i].maxCurvature * (sec_min1 * uv*csc2);
        float Q12 = gl_in[i].maxCurvature * (sec_min1 * uv*csc2);
        float Q22 = gl_in[i].maxCurvature * (1.0 + sec_min1 * v2*csc2);

        //Max singular value = max view-dependent curvature, direction
        //Can't use eigens... why exactly?
        //so we use Q^TQ (Symmetric matrix)
        float QTQ1  = Q11 * Q11 + Q21 * Q21;
        float QTQ12 = Q11 * Q12 + Q21 * Q22;
        float QTQ2  = Q12 * Q12 + Q22 * Q22;

        //Compute largest eigens of QTQ aka singular values
        //q1 : max view-dep curvature, t1 = max view-dep curvature direction
        q1[i] = 0.5 * (QTQ1 + QTQ2);
        if(q1[i] > 0.0)
            q1[i] += sqrt(QTQ12*QTQ12 + 0.25 * (QTQ2-QTQ1)*(QTQ2-QTQ1));
        else
            q1[i] -= sqrt(QTQ12*QTQ12 + 0.25 * (QTQ2-QTQ1)*(QTQ2-QTQ1));
        
        t1[i] = normalize(vec2(QTQ2-q1,-QTQ12));
    }

    //Compute Dt1q1, the derivative of max view-dependent curvature ------------------------------
    for(int i =0 ;i<3;i++){
        vec3 v0 = gl_in[0].pos;
        float currentVertexCurv = q1[i];
        //world coords
        vec3 world_t1 = t1[i][0] * gl_in[i].maxPrincipal + t1[i][1] * gl_in[i].minPrincipal;
        vec3 world_t2 = cross(gl_in[i].normal,world_t1);

        float v0_dot_t2 = dot(v0,world_t2);
        int n =0;
        //Adjacent faces fuck
        // maybe finite differnce the view-dep curv at the vertex (v0) and at the midpoint between v1 and v2
         


    }







}

