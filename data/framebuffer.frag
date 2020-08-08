#version 330 core
out vec4 FragColor;

in vec2 fUV;

uniform sampler2D uRenderTex;

void main()
{
	FragColor = texture2D(uRenderTex, fUV);
}