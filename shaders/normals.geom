#version 430
layout (triangles) in;
layout (line_strip, max_vertices = 12) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

uniform float magnitude;

uniform mat4 projection;

void normals(int index)
{
    //max principal dir
    /*
    gl_Position = projection * gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = projection * (gl_in[index].gl_Position + vec4(gs_in[index].maxPrincipal, 0.0) * magnitude);
    EmitVertex();
    EndPrimitive();
    */
    //min principal dir
    //gl_Position = projection * gl_in[index].gl_Position;
    gl_Position = projection * (gl_in[index].gl_Position);
    EmitVertex();
    gl_Position = projection * (gl_in[index].gl_Position + vec4(gs_in[index].normal, 0.0) * magnitude);
    EmitVertex();
    EndPrimitive();

}

void main()
{
    normals(0); // first vertex
    normals(1);
    normals(2);
}