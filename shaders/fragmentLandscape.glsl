#version 330 core
//in vec3 vFragPosition;
//in vec2 vTexCoords;
//in vec3 vNormal;

in VertexData
{
	vec2 TexCoords;
	vec3 FragPosition;
	vec3 Normal;
} data_in_f;

out vec4 color;

uniform vec3 light_pos;
uniform bool normals_mode;
uniform bool texture_mode;
uniform bool fog_mode;

uniform sampler2D texture_snow;
uniform sampler2D texture_rock;
uniform sampler2D texture_grass;
uniform sampler2D texture_sand;

void main()
{
	vec2 vTexCoords = data_in_f.TexCoords;
	vec3 vFragPosition = data_in_f.FragPosition;
	vec3 vNormal = data_in_f.Normal;

	vec3 lightDir = light_pos;
	vec3 col;
	float h = vFragPosition.y;
	float kd = max(dot(vNormal, lightDir), 0.0);

	float z_depth = gl_FragCoord.z / gl_FragCoord.w;

	if (normals_mode)
		//color = vec4(vNormal, 1.0f);
		color = vec4(0.5f * vNormal + vec3(0.5f, 0.5f, 0.5f), 1.0f);

	else if (texture_mode)
	{
		if (h > 4.7f)
		{
			col = vec3(0.898f, 0.894f, 0.882f);
			color = mix(texture(texture_snow,  vTexCoords), 
						vec4(kd * col, 1.0f), 0.7f);
		}

		else if (h > 4.5f)
		{
			col = vec3(0.533f + (h-4.5f)*0.365f/0.2f, 
					   0.482f + (h-4.5f)*0.412f/0.2f, 
					   0.349f + (h-4.5f)*0.533f/0.2f);

			color = mix(texture(texture_rock, vTexCoords), 
						texture(texture_snow, vTexCoords), 
						(h-4.5f)/0.2f);
			color = mix(color, vec4(kd * col, 1.0f), 0.7f);
		}

		else if (h > 3.5f)
		{
			col = vec3(0.533f, 0.482f, 0.349f);
			color = mix(texture(texture_rock,  vTexCoords), 
						vec4(kd * col, 1.0f), 0.7f);
		}

		else if (h > 2.7f)
		{
			col = vec3(0.686f + (h-2.7f)*(-0.153f)/0.8f, 
					   0.866f + (h-2.7f)*(-0.384f)/0.8f, 
					   0.274f + (h-2.7f)*0.074f/0.8f);

			color = mix(texture(texture_grass, vTexCoords), 
						texture(texture_rock, vTexCoords), 
						(h-2.7f)/0.8f);
			color = mix(color, vec4(kd * col, 1.0f), 0.7f);
		}

		else if (h > 1.5f)
		{
			col = vec3(0.686f, 0.866f, 0.274f);
			color = mix(texture(texture_grass,  vTexCoords), 
						vec4(kd * col, 1.0f), 0.7f);
		}

		else if (h > 1.3f)
		{
			col = vec3(0.839f + (h-1.3f)*(-0.153f)/0.2f, 
					   0.752f + (h-1.3f)*0.113f/0.2f, 
					   0.341f + (h-1.3f)*(-0.067f)/0.2f);

			color = mix(texture(texture_sand, vTexCoords), 
						texture(texture_grass, vTexCoords), 
						(h-1.3f)/0.2f);
			color = mix(color, vec4(kd * col, 1.0f), 0.7f);
		}

		else 
		{
			col = vec3(0.839f, 0.752f, 0.341f);
			color = mix(texture(texture_sand,  vTexCoords), 
						vec4(kd * col, 1.0f), 0.7f);
		}

		if (fog_mode)

			color = mix(color, vec4(0.9f, 0.9f, 0.9f, 0.7f), 
						z_depth * 0.015f);
	}

	else // ordinary mode
	{
		if (h > 4.7f)
			col = vec3(0.898f, 0.894f, 0.882f);

		else if (h > 4.5f)

			col = vec3(0.533f + (h-4.5f)*0.365f/0.2f, 
					   0.482f + (h-4.5f)*0.412f/0.2f, 
					   0.349f + (h-4.5f)*0.533f/0.2f);

		else if (h > 3.5f)
			col = vec3(0.533f, 0.482f, 0.349f);

		else if (h > 2.7f)

			col = vec3(0.686f + (h-2.7f)*(-0.153f)/0.8f, 
					   0.866f + (h-2.7f)*(-0.384f)/0.8f, 
					   0.274f + (h-2.7f)*0.074f/0.8f);

		else if (h > 1.5f)
			col = vec3(0.686f, 0.866f, 0.274f);

		else if (h > 1.3f)

			col = vec3(0.839f + (h-1.3f)*(-0.153f)/0.2f, 
					   0.752f + (h-1.3f)*0.113f/0.2f, 
					   0.341f + (h-1.3f)*(-0.067f)/0.2f);

		else 
			col = vec3(0.839f, 0.752f, 0.341f);

		color = vec4(kd * col, 1.0f);

		if (fog_mode)

			color = mix(color, 
						vec4(0.9f, 0.9f, 0.9f, 0.7f), 
						z_depth * 0.015f);
	}
}
