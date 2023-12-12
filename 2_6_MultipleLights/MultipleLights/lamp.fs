#version 330 core
out vec4 FragColor;


void main()
{
	//使用GLSL内建的texture函数来采样纹理的颜色，参数1:纹理采样器，参数2:对应的纹理坐标
	//FragColor = texture(ourTexture, TexCoord);
    //FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
	FragColor = vec4(1.0);
}