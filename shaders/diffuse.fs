#version 430

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

struct Light {
    vec3 position;
    vec3 diffuse;
};

out vec4 color;

uniform Light light;
void main() {
    // diffuse
    float ambient = 0.05;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0); //cos
    //vec3 diffuse = light.diffuse * diff * high;
    
    color = vec4( (diff + ambient) * vec3(1.0), 1.0f);
}