#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

layout (std140) uniform Matrices
{
	mat4 view;
	mat4 projection;
};

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;
out vec3 VertexColor;

uniform mat4 model;
uniform vec3 color;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	Normal = vec3(mat4(transpose(inverse(model))) * vec4(aNormal, 1.0f));
    FragPos = vec3(model * vec4(aPos, 1.0));
	TexCoords = aTexCoords;
	VertexColor = color;
}