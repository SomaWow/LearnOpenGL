#include "particle_generator.h"

ParticleGenerator::ParticleGenerator(Shader shader, Texture2D texture, GLuint amount)
	: shader(shader), texture(texture), amount(amount)
{
	this->init();
}


void ParticleGenerator::init() {
	GLuint VBO;
	GLfloat particle_quad[] = {
		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,

		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 0.0f
	};
	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(this->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(particle_quad), particle_quad, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glBindVertexArray(0);

	// 填充粒子数组
	for (GLuint i = 0; i < this->amount; i++)
		this->particles.push_back(Particle());
}

void ParticleGenerator::Update(GLfloat dt, GameObject &object, GLuint newParticles, glm::vec2 offset) 
{
	// 一次重生newParticles个粒子
	for (GLuint i = 0; i < newParticles; i++)
	{
		int unusedParticle = this->firstUnusedParticle();
		this->respawnParticle(this->particles[unusedParticle], object, offset);
	}
	// 更新所有粒子
	for (GLuint i = 0; i < this->amount; i++)
	{
		Particle &p = this->particles[i];
		p.Life -= dt; // 生命消逝
		// 如果还活着
		if (p.Life > 0.0f)
		{
			// 移动
			p.Position -= p.Velocity * dt;
			// 透明度减少
			p.Color.a -= dt * 2.5;
		}
	}
}

// 渲染所有粒子
void ParticleGenerator::Draw() 
{
	// 利用混合获得发光效果
	// 两个参数：源因子，目标因子，GL_ONE使粒子叠加在一起的时候产生一种平滑的发热的效果
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	this->shader.use();
	for (Particle particle : this->particles)
	{
		if (particle.Life > 0.0f) {
			this->shader.setVec2("offset", particle.Position);
			this->shader.setVec4("color", particle.Color);
			this->texture.Bind();
			glBindVertexArray(this->VAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);
		}
	}
	// 恢复默认
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


// 最后使用的粒子的索引
GLuint lastUsedParticle = 0;
GLuint ParticleGenerator::firstUnusedParticle() {
	// 从上一个启用的粒子开始遍历
	for (GLuint i = lastUsedParticle; i < this->amount; i++)
	{
		if (this->particles[i].Life <= 0.0f) {
			lastUsedParticle = i;
			return i;
		}
	}
	// 否则从头遍历
	for (GLuint i = 0; i < lastUsedParticle; i++)
	{
		if (this->particles[i].Life <= 0.0f)
		{
			lastUsedParticle = i;
			return i;
		}
	}
	lastUsedParticle = 0;
	return 0;
}

void ParticleGenerator::respawnParticle(Particle &particle, GameObject &object, glm::vec2 offset) {
	// random在-5~5之间
	GLfloat random = ((rand() % 100) - 50) / 10.0f;
	// rColor在0.5~1.5之间
	GLfloat rColor = 0.5 + ((rand() % 100) / 100.0f);
	particle.Position = object.Position + random + offset;
	particle.Color = glm::vec4(rColor, rColor, rColor, 1.0f);
	particle.Life = 1.0f;
	particle.Velocity = object.Velocity * 0.1f;
}