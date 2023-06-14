#version 330 core
out vec4 FragColor;
in vec3 Position;

uniform sampler2D ourTexture;
uniform float fadeOut;
uniform float fadeMult;
uniform bool drawAxis;

void main()
{
    float originDist = length(Position);
    vec3 baseColor = vec3(0.4,0.4,0.4);
    vec3 xColor = vec3(0.5,0.2,0.2);
    vec3 zColor = vec3(0.2,0.2,0.5);
    vec3 color = baseColor;
    if(drawAxis){
        if(Position.z == 0.0){
            color = xColor;
        }
        if(Position.x == 0.0){
            color = zColor;
        }
    }
    FragColor = vec4(color, (1-originDist/fadeOut)*fadeMult);
}