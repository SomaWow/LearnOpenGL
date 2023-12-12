#ifndef GAME_H
#define GAME_H
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "game_level.h"
#include "power_up.h"

//使用枚举定义状态
enum GameState {
	GAME_ACTIVE,
	GAME_MENU,
	GAME_WIN
};

enum Direction {
	UP,
	RIGHT,
	DOWN,
	LEFT
};

typedef std::tuple<GLboolean, Direction, glm::vec2> Collision;

const glm::vec2 PLAYER_SIZE(100, 20);
const GLfloat PLAYER_VELOCITY(500.0f);

class Game {
public:
	GameState State;
	GLboolean Keys[1024];
	GLboolean KeysProcessed[1024];
	GLuint Width, Height;
	std::vector<GameLevel> Levels; // vector动态数组，默认初始化，元素个数为0
	GLuint Level;
	std::vector<PowerUp> PowerUps; // 默认初始化，元素个数为0
	GLuint Lives;
	//构造函数和析构函数
	Game(GLuint width, GLuint height);
	~Game();
	//初始化游戏
	void Init();
	//游戏循环
	void ProcessInput(GLfloat dt); //处理输入
	void Update(GLfloat dt); //更新游戏设置状态
	void Render(); //渲染
	void DoCollisions(); //碰撞检测
	// Reset
	void ResetLevel();
	void ResetPlayer();
	// 管理道具
	void SpawnPowerUps(GameObject &block); // 在给定的砖块的位置生成一个道具
	void UpdatePowerUps(GLfloat dt); //管理所有当前被激活的道具
};

#endif