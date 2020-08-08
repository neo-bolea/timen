#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 fUV;

void main()
{
	gl_Position = vec4(aPos, 0.f, 1.f);
	fUV = aUV;
}