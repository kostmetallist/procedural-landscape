#version 330 core

in vec3 vFragPosition;

out vec4 color;

uniform samplerCube sky_box;

void main()
{
	//vec3 lightDir = vec3(0.9f, 1.0f, 0.5f);
	//color = vec4(1.0f, 0.0f, 0.0f, 0.2f);

	color = texture(sky_box, vFragPosition);
}
