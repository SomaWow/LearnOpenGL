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

// 初始化球的速度
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
// 球的半径
const GLfloat BALL_RADIUS = 12.5f;

GLfloat ShakeTime = 0.0f;

SpriteRenderer *Renderer;
GameObject *Player;
BallObject *Ball;
ParticleGenerator *Particles;
PostProcessor *Effects;
ISoundEngine *SoundEngine = createIrrKlangDevice();
TextRenderer *Text;

// 构造函数，数据初始化
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
	//加载着色器
	ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.fs", nullptr, "sprite");
	ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.fs", nullptr, "particle");
	ResourceManager::LoadShader("shaders/post_processing.vs", "shaders/post_processing.fs", nullptr, "postprocessing");
	//配置着色器
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

	//加载纹理
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

	//设置专用于渲染的控制
	Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
	Particles = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);
	Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);
	Text = new TextRenderer(this->Width, this->Height);
	Text->Load("fonts/OCRAEXT.TTF", 24);

	//加载关卡
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

// 更新，每帧调用
void Game::Update(GLfloat dt) {
	// 移动，检测边界
	Ball->Move(dt, this->Width);
	// 检测砖块和挡板，消除砖块，反弹
	this->DoCollisions();
	// 更新粒子
	Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2));
	this->UpdatePowerUps(dt);
	// 减少ShakeTime变量直到0
	if (ShakeTime > 0.0f)
	{
		ShakeTime -= dt;
		if (ShakeTime <= 0.0f)
			Effects->Shake = GL_FALSE;
	}
	// 判断失败条件，球是否接触到底部边界
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
// 检测输入，每帧调用
void Game::ProcessInput(GLfloat dt) {
	// 当处于游戏状态，可以控制挡板的左右移动，限制移动的范围
	if (this->State == GAME_ACTIVE)
	{
		GLfloat velocity = PLAYER_VELOCITY * dt;
		if (this->Keys[GLFW_KEY_A]) {
			if (Player->Position.x > 0) {
				// 挡板左移
				Player->Position.x -= velocity;
				// 如果球固定在挡板上，一起左移
				if (Ball->Stuck)
					Ball->Position.x -= velocity;
			}
		}
		if (this->Keys[GLFW_KEY_D]) {
			if (Player->Position.x < this->Width - Player->Size.x) {
				// 挡板右移
				Player->Position.x += velocity;
				// 如果球固定在挡板上，一起右移
				if (Ball->Stuck)
					Ball->Position.x += velocity;
			}
		}
		if (this->Keys[GLFW_KEY_SPACE])
			Ball->Stuck = false;
	}
	// 当处于菜单界面可以选择菜单
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
	// 处于胜利界面，点击回车进入菜单
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
// 渲染，每帧调用
void Game::Render() {
	// 三种状态都需要绘制的
	if (this->State == GAME_ACTIVE || this->State == GAME_MENU || this->State == GAME_WIN)
	{
		Effects->BeginRender();
		// 绘制背景
		Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0, 0), glm::vec2(this->Width, this->Height), 0.0f);
		// 绘制关卡
		this->Levels[this->Level].Draw(*Renderer);
		// 绘制挡板
		Player->Draw(*Renderer);
		// 绘制道具
		for (PowerUp &powerUp : this->PowerUps) {
			if (!powerUp.Destroyed)
				powerUp.Draw(*Renderer);
		}
		// 绘制粒子
		Particles->Draw();
		// 绘制小球
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
// 重置此关
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
// 重置挡板和小球
void Game::ResetPlayer() {
	Player->Size = PLAYER_SIZE;
	Player->Position = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
	Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -(BALL_RADIUS * 2)), INITIAL_BALL_VELOCITY);
	// 去掉所有效果
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

	//检查球与砖块的碰撞
	for (GameObject &box : this->Levels[this->Level].Bricks) {
		if (!box.Destroyed)
		{
			Collision collision = CheckCollision(*Ball, box);
			if (std::get<0>(collision)) //如果collision是true
			{
				//如果砖块不是实心就销毁掉，并且随机产生道具
				if (!box.IsSolid) {
					box.Destroyed = GL_TRUE;
					this->SpawnPowerUps(box);
					SoundEngine->play2D("audio/bleep.mp3", GL_FALSE);
				}
				else // 如果是实心的激活shake特效，抖一抖
				{
					ShakeTime = 0.05f;
					Effects->Shake = true;
					SoundEngine->play2D("audio/solid.wav", GL_FALSE);
				}
				//碰撞处理
				Direction dir = std::get<1>(collision); //获得碰撞方向
				//由圆心指向碰撞点的向量
				glm::vec2 diff_vector = std::get<2>(collision);
				if (!(Ball->PassThrough && !box.IsSolid)) {
					if (dir == LEFT || dir == RIGHT)
					{
						Ball->Velocity.x = -Ball->Velocity.x; //水平速度反转
						//重定位，进去砖块了，出来一点，展示边缘触碰的效果
						GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
						if (dir == LEFT)
							Ball->Position.x += penetration;
						else
							Ball->Position.x -= penetration;
					}
					else
					{
						Ball->Velocity.y = -Ball->Velocity.y; //垂直速度反转
						//重定位
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
	//检查小球与挡板的碰撞
	Collision result = CheckCollision(*Ball, *Player);
	if (!Ball->Stuck && std::get<0>(result))
	{
		//检查碰到了挡板的哪个位置，并根据碰到的位置来改变速度
		GLfloat centerBoard = Player->Position.x + Player->Size.x / 2;
		GLfloat distance = (Ball->Position.x + Ball->Radius) - centerBoard;
		//计算 球心到板心的距离/板的半边长
		GLfloat percentage = distance / (Player->Size.x / 2);
		//根据结果移动，距离板心越远，x方向上的速度越大，y方向上的速度越小，速度的值不变
		GLfloat strength = 2.0f;
		glm::vec2 oldVelocity = Ball->Velocity;
		Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
		//防止粘板，反弹后速度一定是向上
		Ball->Velocity.y = -1 * abs(Ball->Velocity.y);
		Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);

		Ball->Stuck = Ball->Sticky;

		SoundEngine->play2D("audio/bleep.wav", GL_FALSE);
	}

	// 检查道具与挡板的碰撞
	for (PowerUp &powerUp : this->PowerUps)
	{
		if (!powerUp.Destroyed)
		{
			if (powerUp.Position.y >= this->Height)
				powerUp.Destroyed = GL_TRUE;
			if (CheckCollision(*Player, powerUp))
			{
				ActivatePowerUp(powerUp);
				// 被摧毁
				powerUp.Destroyed = GL_TRUE;
				// 效果激活

				powerUp.Activated = GL_TRUE;
				SoundEngine->play2D("audio/powerup.wav", GL_FALSE);
			}
		}
	}
}

//AABB-AABB碰撞
GLboolean CheckCollision(GameObject &one, GameObject &two) {
	bool collisionX = one.Position.x + one.Size.x >= two.Position.x
		&& one.Position.x <= two.Position.x + two.Size.x;
	bool collisionY = one.Position.y + one.Size.y >= two.Position.y
		&& one.Position.y <= two.Position.y + two.Size.y;
	return collisionX && collisionY;
}

Direction VectorDirection(glm::vec2 target) {
	//定义四个方向
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

//ABB-圆碰撞，利用一个元组类型作为输出
Collision CheckCollision(BallObject &one, GameObject &two) {
	//获取圆心的位置
	glm::vec2 center(one.Position + one.Radius);
	//计算半边长
	glm::vec2 aabb_half_extents(two.Size.x / 2, two.Size.y / 2);
	//计算长方形的中心
	glm::vec2 aabb_center(
		two.Position.x + aabb_half_extents.x,
		two.Position.y + aabb_half_extents.y
	);
	//获取两个中心的差矢量
	glm::vec2 difference = center - aabb_center;
	glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
	//获取碰撞箱上距离圆最近的点
	glm::vec2 closest = aabb_center + clamped;
	difference = closest - center;
	//不仅返回true或false，还包含方向和差矢量
	if (glm::length(difference) < one.Radius)
		return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
	else
		return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}

GLboolean ShouldSpawn(GLuint chance) {
	GLuint random = rand() % chance;
	return random == 0;
}
// 砖块被摧毁时以一定几率生成一个道具
void Game::SpawnPowerUps(GameObject &block) {
	if (ShouldSpawn(75)) // 1/75的几率
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
	// 根据道具类型发动道具
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
				// 将这个道具移除
				powerUp.Activated = GL_FALSE;
				// 当没有其他同类buff时，停用效果
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
	// 循环PowerUp容器，若一个道具被摧毁停用，则移除它
	this->PowerUps.erase(
		std::remove_if(
			this->PowerUps.begin(), 
			this->PowerUps.end(),
			[](const PowerUp &powerUp) {return powerUp.Destroyed && !powerUp.Activated;}), // 传入一个[]重载方法
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