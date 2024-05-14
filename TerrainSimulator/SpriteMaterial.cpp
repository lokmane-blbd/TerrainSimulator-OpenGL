#include "RenderMaterials.h"

SpriteMaterial::SpriteMaterial(SDL_Surface* surface) {
    // Shader programýný oluþturur
    _prg = new ShaderProgram("vertex.glsl", "sprite.frag");

    // Vertex ve UV özniteliklerinin konumlarýný alýr
    _vertexLoc = _prg->GetAttribLocation("aVertexPos");
    _uvLoc = _prg->GetAttribLocation("aUV");

    // Model, view ve projection uniform deðiþkenlerinin konumlarýný alýr
    _modelLoc = _prg->GetUniformLocation("model");
    _viewLoc = _prg->GetUniformLocation("view");
    _projectionLoc = _prg->GetUniformLocation("projection");

    // Texture nesnesini oluþturur ve ayarlarýný yapar
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
    // SpriteMaterial nesnesi yok edildiðinde shader programýný ve texture nesnesini silmez
    // Bu iþlemler ShaderProgram ve TextureManager sýnýflarýnýn sorumluluðundadýr
}

void SpriteMaterial::draw(SpriteShaderParams* params) {
    _prg->BeginUse();  // Shader programýný kullanmaya baþlar

    // Model, view ve projection matrislerini shader programýna gönderir
    glUniformMatrix4fv(_modelLoc, 1, GL_FALSE, glm::value_ptr(params->modelMatrix));
    glUniformMatrix4fv(_viewLoc, 1, GL_FALSE, glm::value_ptr(params->viewMatrix));
    glUniformMatrix4fv(_projectionLoc, 1, GL_FALSE, glm::value_ptr(params->projectionMatrix));

    glBindBuffer(GL_ARRAY_BUFFER, params->vbo);  // Vertex buffer nesnesini baðlar
    glVertexAttribPointer(_vertexLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), NULL);  // Vertex özniteliðinin konumunu belirtir
    glEnableVertexAttribArray(_vertexLoc);  // Vertex özniteliðini etkinleþtirir
    glVertexAttribPointer(_uvLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
        (void*)(3 * sizeof(float)));  // UV özniteliðinin konumunu belirtir
    glEnableVertexAttribArray(_uvLoc);  // UV özniteliðini etkinleþtirir

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, params->ibo);  // Ýndex buffer nesnesini baðlar

    glBindTexture(GL_TEXTURE_2D, _texture);  // Texture nesnesini baðlar
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);  // Geometriyi çizer

    glDisableVertexAttribArray(_vertexLoc);  // Vertex özniteliðini devre dýþý býrakýr
    glDisableVertexAttribArray(_uvLoc);  // UV özniteliðini devre dýþý býrakýr

    _prg->EndUse();  // Shader programýnýn kullanýmýný sonlandýrýr
}
