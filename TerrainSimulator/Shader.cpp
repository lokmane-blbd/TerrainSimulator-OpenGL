#include "Headers.h"
#include "Shader.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class Shader {
private:
    int _shader = -1;  // Shader nesnesinin tanýmlandýðý deðiþken

public:
    Shader(const char* filePath, GLenum shaderType) {
        std::ifstream file(filePath);  // Dosya okuma iþlemi için dosya açýlýr

        std::string content, line;
        while (std::getline(file, line)) {
            content += line + '\n';  // Dosyanýn içeriði satýr satýr okunarak content deðiþkenine eklenir
        }
        file.close();  // Dosya kapatýlýr

        _shader = glCreateShader(shaderType);  // Shader oluþturulur

        const GLchar* data = content.c_str();  // Shader içeriði const GLchar* veri tipine dönüþtürülür
        glShaderSource(_shader, 1, &data, NULL);  // Shader'a kaynak kodu atanýr
        glCompileShader(_shader);  // Shader derlenir
        GLint success = 0;
        glGetShaderiv(_shader, GL_COMPILE_STATUS, &success);  // Derleme durumu kontrol edilir

        if (success == GL_FALSE) {  // Derleme baþarýsýzsa
            GLint infoLen = 0;
            glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &infoLen);  // Derleme hatasý bilgisi için log uzunluðu alýnýr
            std::vector<GLchar> infoLog(infoLen);
            glGetShaderInfoLog(_shader, infoLen, &infoLen, &infoLog[0]);  // Hata logu alýnýr
            std::string infoString(infoLog.begin(), infoLog.end());

            std::cout << "Error compiling shader: " << infoString << std::endl;  // Derleme hatasý konsola yazdýrýlýr

            glDeleteShader(_shader);  // Baþarýsýz olan shader silinir
            _shader = -1;  // Shader deðiþkeni sýfýrlanýr
        }
    }

    ~Shader() {
        glDeleteShader(_shader);  // Shader nesnesi yok edildiðinde shader silinir
        _shader = -1;  // Shader deðiþkeni sýfýrlanýr
    }

    GLuint getShader() {
        return _shader;  // Shader nesnesini döndürür
    }
};

ShaderProgram::ShaderProgram(const char* vertexPath, const char* fragmentPath) {
    _program = glCreateProgram();  // Program nesnesi oluþturulur
    Shader* vertexShader = new Shader(vertexPath, GL_VERTEX_SHADER);  // Vertex shader nesnesi oluþturulur
    GLuint vs = vertexShader->getShader();  // Vertex shader'ýn id'si alýnýr

    Shader* fragmentShader = new Shader(fragmentPath, GL_FRAGMENT_SHADER);  // Fragment shader nesnesi oluþturulur
    GLuint fs = fragmentShader->getShader();  // Fragment shader'ýn id'si alýnýr

    if (vs >= 0 && fs >= 0) {
        glAttachShader(_program, vs);  // Programa vertex shader eklenir
        glAttachShader(_program, fs);  // Programa fragment shader eklenir
        glLinkProgram(_program);  // Program baðlanýr
    }
    else {
        std::cout << "Error linking shader program!" << std::endl;  // Shader programýnýn baðlanmasý baþarýsýz olduðunda hata mesajý verilir
    }

    delete vertexShader;  // Vertex shader nesnesi silinir
    delete fragmentShader;  // Fragment shader nesnesi silinir
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(_program);  // Program nesnesi yok edildiðinde program silinir
    _program = -1;  // Program deðiþkeni sýfýrlanýr
}

GLint ShaderProgram::GetAttribLocation(const char* name) {
    GLint loc = glGetAttribLocation(_program, name);  // Belirtilen özniteliðin konumu alýnýr
    if (loc < 0) {
        std::cout << "No such attribute: " << name << std::endl;  // Öznitelik bulunamazsa hata mesajý verilir
    }
    return loc;  // Öznitelik konumu döndürülür
}

GLint ShaderProgram::GetUniformLocation(const char* name) {
    GLint loc = glGetUniformLocation(_program, name);  // Belirtilen uniform deðiþkeninin konumu alýnýr
    if (loc < 0) {
        std::cout << "No such uniform: " << name << std::endl;  // Uniform deðiþkeni bulunamazsa hata mesajý verilir
    }
    return loc;  // Uniform deðiþkeninin konumu döndürülür
}

void ShaderProgram::BeginUse() {
    glUseProgram(_program);  // Program kullanýlmaya baþlandýðýnda ilgili OpenGL fonksiyonu çaðrýlýr
}

void ShaderProgram::EndUse() {
    glUseProgram((GLuint)NULL);  // Program kullanýmý sonlandýðýnda ilgili OpenGL fonksiyonu çaðrýlýr
}
