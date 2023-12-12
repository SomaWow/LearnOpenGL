#ifndef GAMELEVEL_H
#define GAMELEVEL_H
#include<vector>

#include<glad/glad.h>
#include <glm/glm.hpp>

#include "game_object.h"
#include "sprite_renderer.h"
#include "resource_manager.h"

class GameLevel {
public:
	std::vector<GameObject> Bricks;
	GameLevel() {}
	//从文件中读取关卡信息
	void Load(const GLchar *file, GLuint levelWidth, GLuint levelHeight);
	void Draw(SpriteRenderer &renderer);
	GLboolean IsCompleted();

private:
	void Init(std::vector<std::vector<GLuint>> tileData, GLuint levelWidth, GLuint levelHeight);
};

#endif // !GAMELEVEL_H
