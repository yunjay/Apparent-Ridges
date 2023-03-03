#version 460
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
    vec3 maxPrincpal;
    vec3 minPrincipal;
    float maxCurvature;
    float minCurvature;
    float q1;
    vec2 t1;
    float Dt1q1;
    uint id;
} geometryIn[]; //instance name can be different from vertex shader stage
//gl_in[] for gl_PerVertex which carries gl_Position
//geometryIn[] for the output we made
uniform vec3 viewPosition;
uniform float threshold;
uniform bool drawFaded;
uniform bool cull;
uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

const float epsilon = 1e-6;

out float fade;

//emit two vertices for each line...
void drawApparentRidgeSegment(const int v0,const int v1,const int v2,
             float emax0, float emax1, float emax2,
             float kmax0, float kmax1, float kmax2,
             const vec3 tmax0, const vec3 tmax1, const vec3 tmax2,
             bool to_center, bool do_test){
    float w10 = abs(emax0) / (abs(emax0) + abs(emax1));
    float w01 = 1.0 - w10;
    
    vec3 p01 = w01 * geometryIn[v0].pos + w10 * geometryIn[v1].pos;
    float k01 = abs(w01 * kmax0 + w10 * kmax1);

    vec3 p12;
    float k12;
    if (to_center) {
      // Connect first point to center of triangle
      p12 = (geometryIn[v0].pos +
             geometryIn[v1].pos +
             geometryIn[v2].pos) / 3.0;
      k12 = abs(kmax0 + kmax1 + kmax2) / 3.0;
    } else {
        // Connect first point to second one (on next edge)
        float w21 = abs(emax1) / (abs(emax1) + abs(emax2));
        float w12 = 1.0 - w21;
        p12 = w12 * geometryIn[v1].pos + w21 * geometryIn[v2].pos;
        k12 = abs(w12 * kmax1 + w21 * kmax2);
    }

   // Don't draw below threshold
   k01 -= threshold;
   if (k01 < 0.0)
      k01 = 0.0;
   k12 -= threshold;
   if (k12 < 0.0)
      k12 = 0.0;

   // Skip lines that you can't see...
   if (k01 == 0.0 && k12 == 0.0)
      return;

   // Perform test: do the tmax-es point *towards* the segment? (Fig 6)
   if (do_test) {
      // Find the vector perpendicular to the segment (p01 <-> p12)
      vec3 perp =cross( 0.5*cross((geometryIn[v1].pos-geometryIn[v0].pos),(geometryIn[v2].pos-geometryIn[v0].pos)), p01 - p12);
      // We want tmax1 to point opposite to perp, and
      // tmax0 and tmax2 to point along it.  Otherwise, exit out.
      if ((dot(tmax0,perp)) <= 0.0 ||
          (dot(tmax1,perp)) >= 0.0 ||
          (dot(tmax2,perp)) <= 0.0)
         return;
   }

   // Faded lines
   if (drawFaded) {
      k01 /= (k01 + threshold);
      k12 /= (k12 + threshold);
   } else {
      k01 = k12 = 1.0;
   }

    //Draw line segment
    //gl_Position = vec4(p01,1.0);
    bool firstEmitted=false;
    gl_Position = projection * view *vec4(p01,1.0);
    fade = k01;
    EmitVertex();
    //gl_Position = vec4(p12,1.0);
    gl_Position = projection * view *vec4(p12,1.0);
    fade = k12;
    EmitVertex();
    EndPrimitive();

}
void main() {
    if(cull){
        for(int i =0;i<3;i++){
            if(geometryIn[i].normalDotView<=-0.05) return;
        }
    }

    //geometryIn[0],1,2; 
    //Compute principal view-dependent curvatures and principal view-dependent curv directions at each vertex 
    //For vertex 0 1 2 of the triangle face
    //q1 : max view-dep curvature, t1 = max view-dep curvature direction
    const float kmax0 = geometryIn[0].q1;
    const float kmax1 = geometryIn[1].q1;
    const float kmax2 = geometryIn[2].q1;
    if (kmax0 <= threshold && kmax1 <= threshold && kmax2 <= threshold) //initial filter
        return;
   // tmax are principal directions of view-dependent curvature,
   // flipped to point in the direction of increasing curvature.
   // emax are derivatives of max curvatures in max direction
    const float emax0 = geometryIn[0].Dt1q1;
    const float emax1 = geometryIn[1].Dt1q1;
    const float emax2 = geometryIn[2].Dt1q1;

    vec3 world_t1_0 = geometryIn[0].t1[0] * geometryIn[0].maxPrincpal +
                      geometryIn[0].t1[1] * geometryIn[0].minPrincipal;
    vec3 world_t1_1 = geometryIn[1].t1[0] * geometryIn[1].maxPrincpal +
                      geometryIn[1].t1[1] * geometryIn[1].minPrincipal;
    vec3 world_t1_2 = geometryIn[2].t1[0] * geometryIn[2].maxPrincpal +
                      geometryIn[2].t1[1] * geometryIn[2].minPrincipal;
    vec3 tmax0 = geometryIn[0].Dt1q1 * world_t1_0;
    vec3 tmax1 = geometryIn[1].Dt1q1 * world_t1_1;
    vec3 tmax2 = geometryIn[2].Dt1q1 * world_t1_2;

    //"zero crossing" if the tmaxes along an edge point in opposite directions
    bool zeroCross01 = (dot(tmax0,tmax1) <= 0.0);
    bool zeroCross12 = (dot(tmax1,tmax2) <= 0.0);
    bool zeroCross20 = (dot(tmax2,tmax0) <= 0.0);
    if (int(zeroCross01) + int(zeroCross12) + int(zeroCross20) < 2)
        return;
    //Draw lines
    if (!zeroCross01) {
      drawApparentRidgeSegment(1, 2, 0,
                    emax1, emax2, emax0,
                    kmax1, kmax2, kmax0,
                    tmax1, tmax2, tmax0,
                    false, true);
   } else if (!zeroCross12) {
      drawApparentRidgeSegment(2, 0, 1,
                   emax2, emax0, emax1,
                   kmax2, kmax0, kmax1,
                   tmax2, tmax0, tmax1,
                   false, true);
   } else if (!zeroCross20) {
      drawApparentRidgeSegment(0, 1, 2,
                   emax0, emax1, emax2,
                   kmax0, kmax1, kmax2,
                   tmax0, tmax1, tmax2,
                   false, true);
   } else {
      // All three edges have crossings -- connect all to center
      drawApparentRidgeSegment(1, 2, 0,
                   emax1, emax2, emax0,
                   kmax1, kmax2, kmax0,
                   tmax1, tmax2, tmax0,
                     true, true);
      drawApparentRidgeSegment(2, 0, 1,
                   emax2, emax0, emax1,
                   kmax2, kmax0, kmax1,
                   tmax2, tmax0, tmax1,
                   true, true);
      drawApparentRidgeSegment(0, 1, 2,
                   emax0, emax1, emax2,
                   kmax0, kmax1, kmax2,
                   tmax0, tmax1, tmax2,
                   true, true);
   }
}

