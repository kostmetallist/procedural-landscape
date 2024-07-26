#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in VertexData
{
	vec2 TexCoords;
	vec3 FragPosition;
	vec3 Normal;
} data_in_g[3];

out VertexData
{
	vec2 TexCoords;
	vec3 FragPosition;
	vec3 Normal;
} data_out_g;

uniform bool arrows_mode;

void main()
{
		for (int i = 0; i < gl_in.length(); i++)
		{	
			//gl_Position = gl_in[i].gl_Position + vec4(data_in_g[i].Normal, 0.0f);

			gl_Position = gl_in[i].gl_Position;
			EmitVertex();

			//gl_Position = gl_in[i].gl_Position + vec4(-data_in_g[i].Normal.x, -data_in_g[i].Normal.y, -data_in_g[i].Normal.z, 0.0f);
			//gl_Position = gl_in[i].gl_Position;
			//EmitVertex();

			data_out_g.TexCoords = data_in_g[i].TexCoords;
			data_out_g.FragPosition = data_in_g[i].FragPosition;
			data_out_g.Normal = data_in_g[i].Normal;
	
			//EndPrimitive();
		}

		EndPrimitive();
}
