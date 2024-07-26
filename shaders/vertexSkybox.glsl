#version 330 core

layout(location = 0) in vec3 vertex;

out vec3 vFragPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 camera_position;

void main()
{
	vec3 temp;

	temp.x = vertex.x + camera_position.x;
	temp.y = vertex.y + camera_position.y;
	temp.z = vertex.z + camera_position.z;

	gl_Position = projection * view * model * vec4(temp, 1.0f);

	vFragPosition = vertex;
}
