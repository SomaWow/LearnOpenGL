#include "resource_manager.h"
#include <stb_image.h>

#include <iostream>
#include <sstream>
#include <fstream>

std::map<std::string, Shader> ResourceManager::Shaders;
std::map<std::string, Texture2D> ResourceManager::Textures;

//Shader
Shader ResourceManager::LoadShader(const GLchar *vShaderFile, const GLchar *fShaderFile, const GLchar *gShaderFile, std::string name)
{
	Shaders[name] = loadShaderFromFile(vShaderFile, fShaderFile, gShaderFile);
	return Shaders[name];
}

Shader ResourceManager::GetShader(std::string name) {
	return Shaders[name];
}
//Texture2D
Texture2D ResourceManager::LoadTexture(const GLchar *file, std::string name) {
	Textures[name] = loadTextureFromFile(file);
	return Textures[name];
}

Texture2D ResourceManager::GetTexture(std::string name) {
	return Textures[name];
}

void ResourceManager::Clear() {
	for (auto iter : Shaders)
		glDeleteProgram(iter.second.ID);
	for (auto iter : Textures)
		glDeleteTextures(1, &iter.second.ID);
}

Shader ResourceManager::loadShaderFromFile(const GLchar *vShaderFile, const GLchar *fShaderFile, const GLchar *gShaderFile)
{
	Shader shader(vShaderFile, fShaderFile, gShaderFile != nullptr ? gShaderFile : nullptr);
	return shader;
}

Texture2D ResourceManager::loadTextureFromFile(char const * path)
{
	Texture2D texture;

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		texture.Generate(width, height, data, nrComponents);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return texture;
}