#version 330 core
out vec4 FragColor;


void main()
{
	//ʹ��GLSL�ڽ���texture�����������������ɫ������1:���������������2:��Ӧ����������
	//FragColor = texture(ourTexture, TexCoord);
    //FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
	FragColor = vec4(1.0);
}