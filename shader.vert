#version 330 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_texCoords;

out vec2 TexCoords;

void main()
{  
	TexCoords = a_texCoords;
	gl_Position = vec4(a_Pos.x, -a_Pos.y, 1.0, 1.0);
}
