#version 330 core

struct Material {
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec3 VertexColor;

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

void main()
{
    // ambient
    vec3 ambient = light.ambient * material.diffuse * VertexColor;

    // diffuse
    vec3 norm = normalize(Normal);
    //vec3 lightDir = normalize(light.position - FragPos);
    vec3 lightDir = -normalize(light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * material.diffuse * VertexColor;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular;

    // total
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}