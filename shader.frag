#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene_texture;

void main()
{
	vec4 TexColor = texture(scene_texture, TexCoords);

	FragColor = TexColor;
}

