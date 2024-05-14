#include "RenderMaterials.h"

SpriteMaterial::SpriteMaterial(SDL_Surface* surface) {
    // Shader program�n� olu�turur
    _prg = new ShaderProgram("vertex.glsl", "sprite.frag");

    // Vertex ve UV �zniteliklerinin konumlar�n� al�r
    _vertexLoc = _prg->GetAttribLocation("aVertexPos");
    _uvLoc = _prg->GetAttribLocation("aUV");

    // Model, view ve projection uniform de�i�kenlerinin konumlar�n� al�r
    _modelLoc = _prg->GetUniformLocation("model");
    _viewLoc = _prg->GetUniformLocation("view");
    _projectionLoc = _prg->GetUniformLocation("projection");

    // Texture nesnesini olu�turur ve ayarlar�n� yapar
    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0,
        GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);
}

SpriteMaterial::~SpriteMaterial() {
    // SpriteMaterial nesnesi yok edildi�inde shader program�n� ve texture nesnesini silmez
    // Bu i�lemler ShaderProgram ve TextureManager s�n�flar�n�n sorumlulu�undad�r
}

void SpriteMaterial::draw(SpriteShaderParams* params) {
    _prg->BeginUse();  // Shader program�n� kullanmaya ba�lar

    // Model, view ve projection matrislerini shader program�na g�nderir
    glUniformMatrix4fv(_modelLoc, 1, GL_FALSE, glm::value_ptr(params->modelMatrix));
    glUniformMatrix4fv(_viewLoc, 1, GL_FALSE, glm::value_ptr(params->viewMatrix));
    glUniformMatrix4fv(_projectionLoc, 1, GL_FALSE, glm::value_ptr(params->projectionMatrix));

    glBindBuffer(GL_ARRAY_BUFFER, params->vbo);  // Vertex buffer nesnesini ba�lar
    glVertexAttribPointer(_vertexLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), NULL);  // Vertex �zniteli�inin konumunu belirtir
    glEnableVertexAttribArray(_vertexLoc);  // Vertex �zniteli�ini etkinle�tirir
    glVertexAttribPointer(_uvLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
        (void*)(3 * sizeof(float)));  // UV �zniteli�inin konumunu belirtir
    glEnableVertexAttribArray(_uvLoc);  // UV �zniteli�ini etkinle�tirir

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, params->ibo);  // �ndex buffer nesnesini ba�lar

    glBindTexture(GL_TEXTURE_2D, _texture);  // Texture nesnesini ba�lar
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);  // Geometriyi �izer

    glDisableVertexAttribArray(_vertexLoc);  // Vertex �zniteli�ini devre d��� b�rak�r
    glDisableVertexAttribArray(_uvLoc);  // UV �zniteli�ini devre d��� b�rak�r

    _prg->EndUse();  // Shader program�n�n kullan�m�n� sonland�r�r
}
