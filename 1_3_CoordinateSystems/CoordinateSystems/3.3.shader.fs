#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;


void main()
{
	//ʹ��GLSL�ڽ���texture�����������������ɫ������1:���������������2:��Ӧ����������
	//FragColor = texture(ourTexture, TexCoord);
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
}