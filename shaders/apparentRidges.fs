#version 460
out vec4 color;
in float fade;
uniform vec3 backgroundColor;
uniform vec3 lineColor;
void main(){
    clamp(fade,0.0,1.0);
    //color = vec4(lineColor,1.0);
    color = vec4(lineColor - (1.0-fade)*(lineColor-backgroundColor),1.0);
}