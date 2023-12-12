#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <map>
#include <string>

//函数库
#include <glad/glad.h>

#include "texture.h"
#include "shader.h"

//资源管理器
class ResourceManager {
public:
	static std::map<std::string, Shader> Shaders;
	static std::map<std::string, Texture2D> Textures;

	static Shader LoadShader(const GLchar *vShaderFile, const GLchar *fShaderFile, const GLchar *gShaderFile, std::string name);
	static Shader GetShader(std::string name);
	static Texture2D LoadTexture(const GLchar *file, std::string name);
	static Texture2D GetTexture(std::string name);
	static void Clear();
private:
	ResourceManager(){}
	static Shader loadShaderFromFile(const GLchar *vShaderFile, const GLchar *fShaderFile, const GLchar *gShaderFile = nullptr);
	static Texture2D loadTextureFromFile(const GLchar *file);

};

#endif