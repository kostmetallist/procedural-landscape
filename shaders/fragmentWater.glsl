#version 330 core

in vec3 vFragPosition;
in vec2 vTexCoords;
in vec3 vNormal;

in vec3 pos_eye;
in vec3 n_eye;

out vec4 color;

uniform vec3 light_pos;
uniform bool texture_mode;
uniform bool fog_mode;

uniform samplerCube texture_skybox;
uniform sampler2D texture_water;

uniform mat4 view;

void main()
{
	vec3 lightDir = light_pos;
	vec3 col = vec3(0.431f, 0.584f, 0.831f);
	float kd = max(dot(vNormal, lightDir), 0.0);
	float z_depth = gl_FragCoord.z / gl_FragCoord.w;

	vec3 incident_eye = normalize(pos_eye);
	vec3 normal_eye  = normalize(n_eye);
	vec3 reflected = reflect(incident_eye, normal_eye);

	reflected = vec3(inverse(view) * vec4(reflected, 0.0f));

	if (texture_mode)
	{
		color = mix(texture(texture_water, vTexCoords), 
					vec4(kd * col, 0.7f), 
					0.7f);

		color = mix(texture(texture_skybox, reflected), 
					color, 
					0.7f);

		if (fog_mode)

			color = mix(color, 
						vec4(0.9f, 0.9f, 0.9f, 0.7f), 
						z_depth * 0.015f);
	}

	else
	{
		color = mix(texture(texture_skybox, reflected), 
					vec4(kd * col, 0.7f), 
					0.7f);

		if (fog_mode)

			color = mix(color, 
						vec4(0.9f, 0.9f, 0.9f, 0.7f), 
						z_depth * 0.015f);
	}
}
