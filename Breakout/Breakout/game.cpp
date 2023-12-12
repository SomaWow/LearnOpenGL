#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "ball_object.h"
#include "particle_generator.h"
#include "post_processor.h"
#include "text_renderer.h"

#include <algorithm>
#include<irrklang/irrKlang.h>
using namespace irrklang;

// ��ʼ������ٶ�
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
// ��İ뾶
const GLfloat BALL_RADIUS = 12.5f;

GLfloat ShakeTime = 0.0f;

SpriteRenderer *Renderer;
GameObject *Player;
BallObject *Ball;
ParticleGenerator *Particles;
PostProcessor *Effects;
ISoundEngine *SoundEngine = createIrrKlangDevice();
TextRenderer *Text;

// ���캯�������ݳ�ʼ��
Game::Game(GLuint width, GLuint height)
	: State(GAME_MENU), Keys(), Width(width), Height(height), Level(0), Lives(3)
{

}

Game::~Game() {
	delete Renderer;
	delete Player;
	delete Ball;
	delete Particles;
	delete Effects;
	SoundEngine->drop();
	delete Text;
}

void Game::Init() {
	//������ɫ��
	ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.fs", nullptr, "sprite");
	ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.fs", nullptr, "particle");
	ResourceManager::LoadShader("shaders/post_processing.vs", "shaders/post_processing.fs", nullptr, "postprocessing");
	//������ɫ��
	glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(this->Width),
		static_cast<GLfloat>(this->Height), 0.0f, -1.0f, 1.0f);
	Shader s = ResourceManager::GetShader("sprite");
	s.use();
	s.setInt("image", 0);
	s.setMat4("projection", projection);
	s = ResourceManager::GetShader("particle");
	s.use();
	s.setInt("sprite", 0);
	s.setMat4("projection", projection);

	//��������
	ResourceManager::LoadTexture("textures/background.jpg", "background");
	ResourceManager::LoadTexture("textures/awesomeface.png", "face");
	ResourceManager::LoadTexture("textures/block.png", "block");
	ResourceManager::LoadTexture("textures/block_solid.png", "block_solid");
	ResourceManager::LoadTexture("textures/paddle.png", "paddle");
	ResourceManager::LoadTexture("textures/particle.png", "particle");
	ResourceManager::LoadTexture("textures/powerup_speed.png", "powerup_speed");
	ResourceManager::LoadTexture("textures/powerup_sticky.png", "powerup_sticky");
	ResourceManager::LoadTexture("textures/powerup_passthrough.png", "powerup_passthrough");
	ResourceManager::LoadTexture("textures/powerup_increase.png", "powerup_increase");
	ResourceManager::LoadTexture("textures/powerup_confuse.png", "powerup_confuse");
	ResourceManager::LoadTexture("textures/powerup_chaos.png", "powerup_chaos");

	//����ר������Ⱦ�Ŀ���
	Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
	Particles = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);
	Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);
	Text = new TextRenderer(this->Width, this->Height);
	Text->Load("fonts/OCRAEXT.TTF", 24);

	//���عؿ�
	GameLevel one;
	one.Load("levels/one.lvl", this->Width, this->Height * 0.5);
	GameLevel two;
	two.Load("levels/two.lvl", this->Width, this->Height * 0.5);
	GameLevel three;
	three.Load("levels/three.lvl", this->Width, this->Height * 0.5);
	GameLevel four;
	four.Load("levels/four.lvl", this->Width, this->Height * 0.5);
	this->Levels.push_back(one);
	this->Levels.push_back(two);
	this->Levels.push_back(three);
	this->Levels.push_back(four);
	this->Level = 0;

	glm::vec2 playerPos = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
	Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));

	glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -BALL_RADIUS * 2);
	Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));

	SoundEngine->play2D("audio/breakout.mp3", GL_TRUE);
}

// ���£�ÿ֡����
void Game::Update(GLfloat dt) {
	// �ƶ������߽�
	Ball->Move(dt, this->Width);
	// ���ש��͵��壬����ש�飬����
	this->DoCollisions();
	// ��������
	Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2));
	this->UpdatePowerUps(dt);
	// ����ShakeTime����ֱ��0
	if (ShakeTime > 0.0f)
	{
		ShakeTime -= dt;
		if (ShakeTime <= 0.0f)
			Effects->Shake = GL_FALSE;
	}
	// �ж�ʧ�����������Ƿ�Ӵ����ײ��߽�
	if (Ball->Position.y >= this->Height)
	{
		this->Lives--;
		if (this->Lives == 0)
		{
			this->ResetLevel();
			this->State = GAME_MENU;
		}
		this->ResetPlayer();
	}
	if (this->State == GAME_ACTIVE && this->Levels[this->Level].IsCompleted())
	{
		this->ResetLevel();
		this->ResetPlayer();
		Effects->Chaos = GL_TRUE;
		this->State = GAME_WIN;
	}
}
// ������룬ÿ֡����
void Game::ProcessInput(GLfloat dt) {
	// ��������Ϸ״̬�����Կ��Ƶ���������ƶ��������ƶ��ķ�Χ
	if (this->State == GAME_ACTIVE)
	{
		GLfloat velocity = PLAYER_VELOCITY * dt;
		if (this->Keys[GLFW_KEY_A]) {
			if (Player->Position.x > 0) {
				// ��������
				Player->Position.x -= velocity;
				// �����̶��ڵ����ϣ�һ������
				if (Ball->Stuck)
					Ball->Position.x -= velocity;
			}
		}
		if (this->Keys[GLFW_KEY_D]) {
			if (Player->Position.x < this->Width - Player->Size.x) {
				// ��������
				Player->Position.x += velocity;
				// �����̶��ڵ����ϣ�һ������
				if (Ball->Stuck)
					Ball->Position.x += velocity;
			}
		}
		if (this->Keys[GLFW_KEY_SPACE])
			Ball->Stuck = false;
	}
	// �����ڲ˵��������ѡ��˵�
	if (this->State == GAME_MENU)
	{
		if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER]) 
		{
			this->State = GAME_ACTIVE;
			this->KeysProcessed[GLFW_KEY_ENTER] = GL_TRUE;
		}
		if (this->Keys[GLFW_KEY_W] && !this->KeysProcessed[GLFW_KEY_W])
		{
			this->Level = (this->Level + 1) % 4;
			this->KeysProcessed[GLFW_KEY_W] = GL_TRUE;
		}
		if (this->Keys[GLFW_KEY_S] && !this->KeysProcessed[GLFW_KEY_S])
		{
			if (this->Level > 0)
				--this->Level;
			else
				this->Level = 3;
			this->KeysProcessed[GLFW_KEY_S] = GL_TRUE;

		}
	}
	// ����ʤ�����棬����س�����˵�
	if (this->State == GAME_WIN)
	{
		if (this->Keys[GLFW_KEY_ENTER])
		{
			this->KeysProcessed[GLFW_KEY_ENTER] = GL_TRUE;
			Effects->Chaos = GL_FALSE;
			this->State = GAME_MENU;
		}

	}
}
// ��Ⱦ��ÿ֡����
void Game::Render() {
	// ����״̬����Ҫ���Ƶ�
	if (this->State == GAME_ACTIVE || this->State == GAME_MENU || this->State == GAME_WIN)
	{
		Effects->BeginRender();
		// ���Ʊ���
		Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0, 0), glm::vec2(this->Width, this->Height), 0.0f);
		// ���ƹؿ�
		this->Levels[this->Level].Draw(*Renderer);
		// ���Ƶ���
		Player->Draw(*Renderer);
		// ���Ƶ���
		for (PowerUp &powerUp : this->PowerUps) {
			if (!powerUp.Destroyed)
				powerUp.Draw(*Renderer);
		}
		// ��������
		Particles->Draw();
		// ����С��
		Ball->Draw(*Renderer);
		Effects->EndRender();
		Effects->Render(glfwGetTime());

		std::stringstream ss;
		ss << this->Lives;
		Text->RenderText("Lives: " + ss.str(), 5.0f, 5.0f, 1.0f);
	}
	if (this->State == GAME_MENU)
	{
		Text->RenderText("Press ENTER to start", 250.0f, this->Height / 2, 1.0f);
		Text->RenderText("Press W or S to select level", 245.0f, this->Height / 2 + 20.0f, 0.75f);
	}
	if (this->State == GAME_WIN)
	{
		Text->RenderText(
			"You WON!!!", 320.0f, this->Height / 2 - 20.0f, 1.0, glm::vec3(0.0f, 1.0f, 0.0f)
		);
		Text->RenderText(
			"Press ENTER to retry or ESC to quit", 130.0f, this->Height / 2, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f)
		);
	}
}
// ���ô˹�
void Game::ResetLevel() {
	if (this->Level == 0) this->Levels[0].Load("levels/one.lvl", this->Width, this->Height * 0.5);
	else if (this->Level == 1)
		this->Levels[1].Load("levels/two.lvl", this->Width, this->Height * 0.5);
	else if (this->Level == 2)
		this->Levels[2].Load("levels/three.lvl", this->Width, this->Height * 0.5);
	else if (this->Level == 3)
		this->Levels[3].Load("levels/four.lvl", this->Width, this->Height * 0.5);

	this->Lives = 3;
}
// ���õ����С��
void Game::ResetPlayer() {
	Player->Size = PLAYER_SIZE;
	Player->Position = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
	Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -(BALL_RADIUS * 2)), INITIAL_BALL_VELOCITY);
	// ȥ������Ч��
	Effects->Chaos = Effects->Confuse = GL_FALSE;
	Ball->PassThrough = Ball->Sticky = GL_FALSE;
	Player->Color = glm::vec3(1.0f);
	Ball->Color = glm::vec3(1.0f);
}

Direction VectorDirection(glm::vec2 target);
Collision CheckCollision(BallObject &one, GameObject &two);
GLboolean CheckCollision(GameObject &one, GameObject &two);
void ActivatePowerUp(PowerUp &powerUp);

void Game::DoCollisions() {

	//�������ש�����ײ
	for (GameObject &box : this->Levels[this->Level].Bricks) {
		if (!box.Destroyed)
		{
			Collision collision = CheckCollision(*Ball, box);
			if (std::get<0>(collision)) //���collision��true
			{
				//���ש�鲻��ʵ�ľ����ٵ������������������
				if (!box.IsSolid) {
					box.Destroyed = GL_TRUE;
					this->SpawnPowerUps(box);
					SoundEngine->play2D("audio/bleep.mp3", GL_FALSE);
				}
				else // �����ʵ�ĵļ���shake��Ч����һ��
				{
					ShakeTime = 0.05f;
					Effects->Shake = true;
					SoundEngine->play2D("audio/solid.wav", GL_FALSE);
				}
				//��ײ����
				Direction dir = std::get<1>(collision); //�����ײ����
				//��Բ��ָ����ײ�������
				glm::vec2 diff_vector = std::get<2>(collision);
				if (!(Ball->PassThrough && !box.IsSolid)) {
					if (dir == LEFT || dir == RIGHT)
					{
						Ball->Velocity.x = -Ball->Velocity.x; //ˮƽ�ٶȷ�ת
						//�ض�λ����ȥש���ˣ�����һ�㣬չʾ��Ե������Ч��
						GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
						if (dir == LEFT)
							Ball->Position.x += penetration;
						else
							Ball->Position.x -= penetration;
					}
					else
					{
						Ball->Velocity.y = -Ball->Velocity.y; //��ֱ�ٶȷ�ת
						//�ض�λ
						GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
						if (dir == UP)
							Ball->Position.y -= penetration;
						else
							Ball->Position.y += penetration;
					}
				}
			}
		}
	}
	//���С���뵲�����ײ
	Collision result = CheckCollision(*Ball, *Player);
	if (!Ball->Stuck && std::get<0>(result))
	{
		//��������˵�����ĸ�λ�ã�������������λ�����ı��ٶ�
		GLfloat centerBoard = Player->Position.x + Player->Size.x / 2;
		GLfloat distance = (Ball->Position.x + Ball->Radius) - centerBoard;
		//���� ���ĵ����ĵľ���/��İ�߳�
		GLfloat percentage = distance / (Player->Size.x / 2);
		//���ݽ���ƶ����������ԽԶ��x�����ϵ��ٶ�Խ��y�����ϵ��ٶ�ԽС���ٶȵ�ֵ����
		GLfloat strength = 2.0f;
		glm::vec2 oldVelocity = Ball->Velocity;
		Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
		//��ֹճ�壬�������ٶ�һ��������
		Ball->Velocity.y = -1 * abs(Ball->Velocity.y);
		Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);

		Ball->Stuck = Ball->Sticky;

		SoundEngine->play2D("audio/bleep.wav", GL_FALSE);
	}

	// �������뵲�����ײ
	for (PowerUp &powerUp : this->PowerUps)
	{
		if (!powerUp.Destroyed)
		{
			if (powerUp.Position.y >= this->Height)
				powerUp.Destroyed = GL_TRUE;
			if (CheckCollision(*Player, powerUp))
			{
				ActivatePowerUp(powerUp);
				// ���ݻ�
				powerUp.Destroyed = GL_TRUE;
				// Ч������

				powerUp.Activated = GL_TRUE;
				SoundEngine->play2D("audio/powerup.wav", GL_FALSE);
			}
		}
	}
}

//AABB-AABB��ײ
GLboolean CheckCollision(GameObject &one, GameObject &two) {
	bool collisionX = one.Position.x + one.Size.x >= two.Position.x
		&& one.Position.x <= two.Position.x + two.Size.x;
	bool collisionY = one.Position.y + one.Size.y >= two.Position.y
		&& one.Position.y <= two.Position.y + two.Size.y;
	return collisionX && collisionY;
}

Direction VectorDirection(glm::vec2 target) {
	//�����ĸ�����
	glm::vec2 compass[] = {
		glm::vec2(0.0f, 1.0f),
		glm::vec2(1.0f, 0.0f),
		glm::vec2(0.0f, -1.0f),
		glm::vec2(-1.0f, 0.0f)
	};
	GLfloat max = 0.0f;
	GLuint best_match = -1;
	for (GLuint i = 0; i < 4; i++)
	{
		GLfloat dot_product = glm::dot(glm::normalize(target), compass[i]);
		if (max < dot_product)
		{
			max = dot_product;
			best_match = i;
		}
	}
	return (Direction)best_match;
}

//ABB-Բ��ײ������һ��Ԫ��������Ϊ���
Collision CheckCollision(BallObject &one, GameObject &two) {
	//��ȡԲ�ĵ�λ��
	glm::vec2 center(one.Position + one.Radius);
	//�����߳�
	glm::vec2 aabb_half_extents(two.Size.x / 2, two.Size.y / 2);
	//���㳤���ε�����
	glm::vec2 aabb_center(
		two.Position.x + aabb_half_extents.x,
		two.Position.y + aabb_half_extents.y
	);
	//��ȡ�������ĵĲ�ʸ��
	glm::vec2 difference = center - aabb_center;
	glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
	//��ȡ��ײ���Ͼ���Բ����ĵ�
	glm::vec2 closest = aabb_center + clamped;
	difference = closest - center;
	//��������true��false������������Ͳ�ʸ��
	if (glm::length(difference) < one.Radius)
		return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
	else
		return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}

GLboolean ShouldSpawn(GLuint chance) {
	GLuint random = rand() % chance;
	return random == 0;
}
// ש�鱻�ݻ�ʱ��һ����������һ������
void Game::SpawnPowerUps(GameObject &block) {
	if (ShouldSpawn(75)) // 1/75�ļ���
		this->PowerUps.push_back(PowerUp("speed", glm::vec3(0.5f, 0.5f, 1.0f), 0.0f, block.Position, ResourceManager::GetTexture("powerup_speed")));
	if (ShouldSpawn(75))
		this->PowerUps.push_back(PowerUp("sticky", glm::vec3(1.0f, 0.5f, 1.0f), 20.0f, block.Position, ResourceManager::GetTexture("powerup_sticky")));
	if (ShouldSpawn(75))
		this->PowerUps.push_back(PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, ResourceManager::GetTexture("powerup_passthrough")));
	if (ShouldSpawn(75))
		this->PowerUps.push_back(PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4), 0.0f, block.Position, ResourceManager::GetTexture("powerup_increase")));
	if (ShouldSpawn(15)) 
		this->PowerUps.push_back(PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 5.0f, block.Position, ResourceManager::GetTexture("powerup_confuse")));
	if (ShouldSpawn(15))
		this->PowerUps.push_back(PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 5.0f, block.Position, ResourceManager::GetTexture("powerup_chaos")));
}

void ActivatePowerUp(PowerUp &powerUp) {
	// ���ݵ������ͷ�������
	if (powerUp.Type == "speed")
	{
		Ball->Velocity *= 1.2;
	}
	else if (powerUp.Type == "sticky")
	{
		Ball->Sticky = GL_TRUE;
		Player->Color = glm::vec3(1.0f, 0.5f, 1.0f);
	}
	else if (powerUp.Type == "pass-through")
	{
		Ball->PassThrough = GL_TRUE;
		Ball->Color = glm::vec3(1.0f, 0.5f, 0.5f);
	}
	else if (powerUp.Type == "pad-size-increase")
	{
		Player->Size.x += 50;
	}
	else if (powerUp.Type == "confuse")
	{
		if (!Effects->Chaos)
			Effects->Confuse = GL_TRUE;
	}
	else if (powerUp.Type == "chaos")
	{
		if (!Effects->Confuse)
			Effects->Chaos = GL_TRUE;
	}
}

GLboolean IsOtherPowerUpActive(std::vector<PowerUp> &powerUp, std::string type);

void Game::UpdatePowerUps(GLfloat dt) {
	for (PowerUp & powerUp : this->PowerUps)
	{
		powerUp.Position += powerUp.Velocity * dt;
		if (powerUp.Activated)
		{
			powerUp.Duration -= dt;
			if (powerUp.Duration <= 0.0f)
			{
				// ����������Ƴ�
				powerUp.Activated = GL_FALSE;
				// ��û������ͬ��buffʱ��ͣ��Ч��
				if (powerUp.Type == "sticky")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "sticky"))
					{
						Ball->Sticky = GL_FALSE;
						Player->Color = glm::vec3(1.0f);
					}
				}
				else if (powerUp.Type == "pass-through")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "pass-through"))
					{
						Ball->PassThrough = GL_FALSE;
						Ball->Color = glm::vec3(1.0f);
					}
				}
				else if (powerUp.Type == "confuse")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "confuse"))
					{
						Effects->Confuse = GL_FALSE;
					}
				}
				else if (powerUp.Type == "chaos")
				{
					if (!IsOtherPowerUpActive(this->PowerUps, "chaos"))
					{
						Effects->Chaos = GL_FALSE;
					}
				}
			}
		}
	}
	// ѭ��PowerUp��������һ�����߱��ݻ�ͣ�ã����Ƴ���
	this->PowerUps.erase(
		std::remove_if(
			this->PowerUps.begin(), 
			this->PowerUps.end(),
			[](const PowerUp &powerUp) {return powerUp.Destroyed && !powerUp.Activated;}), // ����һ��[]���ط���
			this->PowerUps.end()
		);
}

GLboolean IsOtherPowerUpActive(std::vector<PowerUp> &powerUp, std::string type)
{
	for (const PowerUp &powerUp : powerUp)
	{
		if (powerUp.Activated && powerUp.Type == type)
			return GL_TRUE;
	}
	return GL_FALSE;
}