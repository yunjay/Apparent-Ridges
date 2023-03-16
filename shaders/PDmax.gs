#version 430
layout (triangles) in;
layout (line_strip, max_vertices = 12) out;

in VS_OUT {
    vec3 maxPrincipal;
    vec3 minPrincipal;
} gs_in[];

uniform float magnitude;
uniform mat4 projection;

void principalDirections(int index)
{
    //max principal dir
    gl_Position = projection * (gl_in[index].gl_Position - vec4(gs_in[index].maxPrincipal, 0.0) * magnitude);
    EmitVertex();
    gl_Position = projection * (gl_in[index].gl_Position + vec4(gs_in[index].maxPrincipal, 0.0) * magnitude);
    EmitVertex();
    EndPrimitive();

    //min principal dir
    /*
    gl_Position = projection * gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = projection * (gl_in[index].gl_Position + vec4(gs_in[index].minPrincipal, 0.0) * magnitude);
    EmitVertex();
    EndPrimitive();
    */
}

void main()
{
    principalDirections(0); // first vertex
    principalDirections(1);
    principalDirections(2);
}