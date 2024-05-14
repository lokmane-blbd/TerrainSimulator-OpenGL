#include "RenderMaterials.h"

GridMaterial::GridMaterial() {
	init();
}

GridMaterial::~GridMaterial() {
}

void GridMaterial::init() {
	// Yeni bir ShaderProgram �rne�i olu�turuluyor ve _prg'ye atan�yor.
	_prg = new ShaderProgram("vertex.glsl", "sandgrid.frag");

	// ShaderProgram'dan attribute konumlar� al�n�yor ve ilgili de�i�kenlere atan�yor.
	_vertexLoc = _prg->GetAttribLocation("aVertexPos");
	_uvLoc = _prg->GetAttribLocation("aUV");

	// ShaderProgram'dan uniform konumlar� al�n�yor ve ilgili de�i�kenlere atan�yor.
	_modelLoc = _prg->GetUniformLocation("model");
	_viewLoc = _prg->GetUniformLocation("view");
	_projectionLoc = _prg->GetUniformLocation("projection");
	_gridDimLoc = _prg->GetUniformLocation("gridDimension");
	_timeLoc = _prg->GetUniformLocation("time");
	_noShadingLoc = _prg->GetUniformLocation("shadingOff");
	_nMaterialsLoc = _prg->GetUniformLocation("nMaterials");

	// Bir texture olu�turuluyor ve _texture de�i�kenine atan�yor.
	glGenTextures(1, &_texture);
	glBindTexture(GL_TEXTURE_2D, _texture);

	// Texture parametreleri ayarlan�yor.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}


void GridMaterial::initTexture(SDL_Surface* texture, int width, int height) {
	// Texture boyutlar� _width ve _height de�i�kenlerine atan�yor.
	_width = width;
	_height = height;

	// Texture'nin piksellerine eri�im i�in bir i�aret�i olu�turuluyor.
	const void* pixels = texture == NULL ? NULL : texture->pixels;

	// Texture'� GL_TEXTURE_2D hedefine ba�lay�p piksel verilerini y�kl�yoruz.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0,
		GL_BGRA, GL_UNSIGNED_BYTE, pixels);
}

void GridMaterial::putData(void* data) {
	// Texture'� _texture de�i�kenine ba�l�yoruz.
	glBindTexture(GL_TEXTURE_2D, _texture);

	// Verileri texture'a y�kl�yoruz.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0,
		GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
}

// �lgili sabitlerin tan�mlanmas�
const int VERT_SZ = 3;
const int UV_SZ = 2;
const int ATTRIB_SZ = VERT_SZ + UV_SZ;
const void* UV_OFFSET = (const void*)(VERT_SZ * sizeof(float));


void GridMaterial::draw(GridShaderParams* params) {
	// Shader program� kullan�lmaya ba�lan�yor.
	_prg->BeginUse();

	// Model, view ve projection matrisleri uniform de�i�kenlere atan�yor.
	glUniformMatrix4fv(_modelLoc, 1, GL_FALSE, glm::value_ptr(params->modelMatrix));
	glUniformMatrix4fv(_viewLoc, 1, GL_FALSE, glm::value_ptr(params->viewMatrix));
	glUniformMatrix4fv(_projectionLoc, 1, GL_FALSE, glm::value_ptr(params->projectionMatrix));

	// Grid boyutlar� uniform de�i�kenine atan�yor.
	glUniform2f(_gridDimLoc, params->gridWidth, params->gridHeight);

	// Zaman ve shadingOff de�i�kenleri uniform de�i�kenlere atan�yor.
	glUniform1f(_timeLoc, params->time);
	glUniform1ui(_noShadingLoc, params->shadingOff);

	// nMaterials uniform de�i�kenine Material::OutOfBounds de�eri atan�yor.
	glUniform1f(_nMaterialsLoc, (GLfloat)Material::OutOfBounds);

	// Vertex ve UV verileri i�in bellek puan�t�r�c�s� ba�lan�yor.
	glBindBuffer(GL_ARRAY_BUFFER, params->vbo);

	// Vertex pozisyonlar� �zelliklerine eri�im ayarlar� yap�l�yor.
	glVertexAttribPointer(_vertexLoc, VERT_SZ, GL_FLOAT, GL_FALSE, ATTRIB_SZ * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(_vertexLoc);

	// UV koordinatlar� �zelliklerine eri�im ayarlar� yap�l�yor.
	glVertexAttribPointer(_uvLoc, UV_SZ, GL_FLOAT, GL_FALSE, ATTRIB_SZ * sizeof(GLfloat), UV_OFFSET);
	glEnableVertexAttribArray(_uvLoc);

	// �ndeks tamponu ba�lan�yor.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, params->ibo);

	// Texture ba�lan�yor.
	glBindTexture(GL_TEXTURE_2D, _texture);

	// D�rtgen �izimi yap�l�yor.
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);

	// �zellik eri�im ayarlar� devre d��� b�rak�l�yor.
	glDisableVertexAttribArray(_vertexLoc);
	glDisableVertexAttribArray(_uvLoc);

	// Shader program� kullan�m� sonland�r�l�yor.
	_prg->EndUse();
}
