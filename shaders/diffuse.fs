#version 430

struct Light {
    vec3 position;
    vec3 diffuse;
};

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;
uniform Light light;
void main() {
    // diffuse
    float ambient = 0.05;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0); //cos
    //vec3 diffuse = light.diffuse * diff * high;
     // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);
    vec3 specular;
    if(dot(lightDir,norm)>=0){
        specular = vec3(1.0,1.0,1.0)* spec ;
	} else specular =vec3(0,0,0); 

    color = vec4( (diff + ambient+specular) * vec3(1.0), 1.0f);
}