#include "RenderMaterials.h"

GridMaterial::GridMaterial() {
	init();
}

GridMaterial::~GridMaterial() {
}

void GridMaterial::init() {
	// Yeni bir ShaderProgram örneði oluþturuluyor ve _prg'ye atanýyor.
	_prg = new ShaderProgram("vertex.glsl", "sandgrid.frag");

	// ShaderProgram'dan attribute konumlarý alýnýyor ve ilgili deðiþkenlere atanýyor.
	_vertexLoc = _prg->GetAttribLocation("aVertexPos");
	_uvLoc = _prg->GetAttribLocation("aUV");

	// ShaderProgram'dan uniform konumlarý alýnýyor ve ilgili deðiþkenlere atanýyor.
	_modelLoc = _prg->GetUniformLocation("model");
	_viewLoc = _prg->GetUniformLocation("view");
	_projectionLoc = _prg->GetUniformLocation("projection");
	_gridDimLoc = _prg->GetUniformLocation("gridDimension");
	_timeLoc = _prg->GetUniformLocation("time");
	_noShadingLoc = _prg->GetUniformLocation("shadingOff");
	_nMaterialsLoc = _prg->GetUniformLocation("nMaterials");

	// Bir texture oluþturuluyor ve _texture deðiþkenine atanýyor.
	glGenTextures(1, &_texture);
	glBindTexture(GL_TEXTURE_2D, _texture);

	// Texture parametreleri ayarlanýyor.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}


void GridMaterial::initTexture(SDL_Surface* texture, int width, int height) {
	// Texture boyutlarý _width ve _height deðiþkenlerine atanýyor.
	_width = width;
	_height = height;

	// Texture'nin piksellerine eriþim için bir iþaretçi oluþturuluyor.
	const void* pixels = texture == NULL ? NULL : texture->pixels;

	// Texture'ý GL_TEXTURE_2D hedefine baðlayýp piksel verilerini yüklüyoruz.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0,
		GL_BGRA, GL_UNSIGNED_BYTE, pixels);
}

void GridMaterial::putData(void* data) {
	// Texture'ý _texture deðiþkenine baðlýyoruz.
	glBindTexture(GL_TEXTURE_2D, _texture);

	// Verileri texture'a yüklüyoruz.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0,
		GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, data);
}

// Ýlgili sabitlerin tanýmlanmasý
const int VERT_SZ = 3;
const int UV_SZ = 2;
const int ATTRIB_SZ = VERT_SZ + UV_SZ;
const void* UV_OFFSET = (const void*)(VERT_SZ * sizeof(float));


void GridMaterial::draw(GridShaderParams* params) {
	// Shader programý kullanýlmaya baþlanýyor.
	_prg->BeginUse();

	// Model, view ve projection matrisleri uniform deðiþkenlere atanýyor.
	glUniformMatrix4fv(_modelLoc, 1, GL_FALSE, glm::value_ptr(params->modelMatrix));
	glUniformMatrix4fv(_viewLoc, 1, GL_FALSE, glm::value_ptr(params->viewMatrix));
	glUniformMatrix4fv(_projectionLoc, 1, GL_FALSE, glm::value_ptr(params->projectionMatrix));

	// Grid boyutlarý uniform deðiþkenine atanýyor.
	glUniform2f(_gridDimLoc, params->gridWidth, params->gridHeight);

	// Zaman ve shadingOff deðiþkenleri uniform deðiþkenlere atanýyor.
	glUniform1f(_timeLoc, params->time);
	glUniform1ui(_noShadingLoc, params->shadingOff);

	// nMaterials uniform deðiþkenine Material::OutOfBounds deðeri atanýyor.
	glUniform1f(_nMaterialsLoc, (GLfloat)Material::OutOfBounds);

	// Vertex ve UV verileri için bellek puançtýrýcýsý baðlanýyor.
	glBindBuffer(GL_ARRAY_BUFFER, params->vbo);

	// Vertex pozisyonlarý özelliklerine eriþim ayarlarý yapýlýyor.
	glVertexAttribPointer(_vertexLoc, VERT_SZ, GL_FLOAT, GL_FALSE, ATTRIB_SZ * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(_vertexLoc);

	// UV koordinatlarý özelliklerine eriþim ayarlarý yapýlýyor.
	glVertexAttribPointer(_uvLoc, UV_SZ, GL_FLOAT, GL_FALSE, ATTRIB_SZ * sizeof(GLfloat), UV_OFFSET);
	glEnableVertexAttribArray(_uvLoc);

	// Ýndeks tamponu baðlanýyor.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, params->ibo);

	// Texture baðlanýyor.
	glBindTexture(GL_TEXTURE_2D, _texture);

	// Dörtgen çizimi yapýlýyor.
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);

	// Özellik eriþim ayarlarý devre dýþý býrakýlýyor.
	glDisableVertexAttribArray(_vertexLoc);
	glDisableVertexAttribArray(_uvLoc);

	// Shader programý kullanýmý sonlandýrýlýyor.
	_prg->EndUse();
}
