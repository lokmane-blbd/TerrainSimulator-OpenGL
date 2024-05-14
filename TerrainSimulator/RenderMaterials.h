#pragma once

#include "Headers.h"
#include "Shader.h"
#include "Sandgrid.h"

struct SpriteShaderParams {
	GLuint vbo;
	GLuint ibo;
	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
};

class SpriteMaterial {

private:
	ShaderProgram* _prg;
	GLuint _texture;
	GLint _vertexLoc;
	GLint _uvLoc;
	GLint _modelLoc;
	GLint _viewLoc;
	GLint _projectionLoc;

public:

	SpriteMaterial(SDL_Surface* surface);
	~SpriteMaterial();
	void draw(SpriteShaderParams* params);
};


struct GridShaderParams {
	GLuint vbo;
	GLuint ibo;

	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	GLfloat gridWidth;
	GLfloat gridHeight;
	GLfloat time;
	GLuint shadingOff;
	GLfloat nMaterials;
	glm::vec2 mouseUV;
};

class GridMaterial {

private:

	ShaderProgram* _prg;

	int _width;
	int _height;

	GLint _gridDimLoc;
	GLint _vertexLoc;
	GLint _uvLoc;
	GLint _modelLoc;
	GLint _viewLoc;
	GLint _projectionLoc;
	GLint _timeLoc;
	GLint _noShadingLoc;
	GLint _mouseUVLoc;
	GLuint _texture;
	GLint _nMaterialsLoc;

	void init();

public:
	GridMaterial();
	~GridMaterial();
	void initTexture(SDL_Surface* texture, int width, int height);
	void putData(void* data);
	void draw(GridShaderParams* params);
};

