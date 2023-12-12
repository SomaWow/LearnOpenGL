#ifndef GAME_H
#define GAME_H
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "game_level.h"
#include "power_up.h"

//ʹ��ö�ٶ���״̬
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
	std::vector<GameLevel> Levels; // vector��̬���飬Ĭ�ϳ�ʼ����Ԫ�ظ���Ϊ0
	GLuint Level;
	std::vector<PowerUp> PowerUps; // Ĭ�ϳ�ʼ����Ԫ�ظ���Ϊ0
	GLuint Lives;
	//���캯������������
	Game(GLuint width, GLuint height);
	~Game();
	//��ʼ����Ϸ
	void Init();
	//��Ϸѭ��
	void ProcessInput(GLfloat dt); //��������
	void Update(GLfloat dt); //������Ϸ����״̬
	void Render(); //��Ⱦ
	void DoCollisions(); //��ײ���
	// Reset
	void ResetLevel();
	void ResetPlayer();
	// �������
	void SpawnPowerUps(GameObject &block); // �ڸ�����ש���λ������һ������
	void UpdatePowerUps(GLfloat dt); //�������е�ǰ������ĵ���
};

#endif