#include "Headers.h"
#include "Shader.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class Shader {
private:
    int _shader = -1;  // Shader nesnesinin tan�mland��� de�i�ken

public:
    Shader(const char* filePath, GLenum shaderType) {
        std::ifstream file(filePath);  // Dosya okuma i�lemi i�in dosya a��l�r

        std::string content, line;
        while (std::getline(file, line)) {
            content += line + '\n';  // Dosyan�n i�eri�i sat�r sat�r okunarak content de�i�kenine eklenir
        }
        file.close();  // Dosya kapat�l�r

        _shader = glCreateShader(shaderType);  // Shader olu�turulur

        const GLchar* data = content.c_str();  // Shader i�eri�i const GLchar* veri tipine d�n��t�r�l�r
        glShaderSource(_shader, 1, &data, NULL);  // Shader'a kaynak kodu atan�r
        glCompileShader(_shader);  // Shader derlenir
        GLint success = 0;
        glGetShaderiv(_shader, GL_COMPILE_STATUS, &success);  // Derleme durumu kontrol edilir

        if (success == GL_FALSE) {  // Derleme ba�ar�s�zsa
            GLint infoLen = 0;
            glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &infoLen);  // Derleme hatas� bilgisi i�in log uzunlu�u al�n�r
            std::vector<GLchar> infoLog(infoLen);
            glGetShaderInfoLog(_shader, infoLen, &infoLen, &infoLog[0]);  // Hata logu al�n�r
            std::string infoString(infoLog.begin(), infoLog.end());

            std::cout << "Error compiling shader: " << infoString << std::endl;  // Derleme hatas� konsola yazd�r�l�r

            glDeleteShader(_shader);  // Ba�ar�s�z olan shader silinir
            _shader = -1;  // Shader de�i�keni s�f�rlan�r
        }
    }

    ~Shader() {
        glDeleteShader(_shader);  // Shader nesnesi yok edildi�inde shader silinir
        _shader = -1;  // Shader de�i�keni s�f�rlan�r
    }

    GLuint getShader() {
        return _shader;  // Shader nesnesini d�nd�r�r
    }
};

ShaderProgram::ShaderProgram(const char* vertexPath, const char* fragmentPath) {
    _program = glCreateProgram();  // Program nesnesi olu�turulur
    Shader* vertexShader = new Shader(vertexPath, GL_VERTEX_SHADER);  // Vertex shader nesnesi olu�turulur
    GLuint vs = vertexShader->getShader();  // Vertex shader'�n id'si al�n�r

    Shader* fragmentShader = new Shader(fragmentPath, GL_FRAGMENT_SHADER);  // Fragment shader nesnesi olu�turulur
    GLuint fs = fragmentShader->getShader();  // Fragment shader'�n id'si al�n�r

    if (vs >= 0 && fs >= 0) {
        glAttachShader(_program, vs);  // Programa vertex shader eklenir
        glAttachShader(_program, fs);  // Programa fragment shader eklenir
        glLinkProgram(_program);  // Program ba�lan�r
    }
    else {
        std::cout << "Error linking shader program!" << std::endl;  // Shader program�n�n ba�lanmas� ba�ar�s�z oldu�unda hata mesaj� verilir
    }

    delete vertexShader;  // Vertex shader nesnesi silinir
    delete fragmentShader;  // Fragment shader nesnesi silinir
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(_program);  // Program nesnesi yok edildi�inde program silinir
    _program = -1;  // Program de�i�keni s�f�rlan�r
}

GLint ShaderProgram::GetAttribLocation(const char* name) {
    GLint loc = glGetAttribLocation(_program, name);  // Belirtilen �zniteli�in konumu al�n�r
    if (loc < 0) {
        std::cout << "No such attribute: " << name << std::endl;  // �znitelik bulunamazsa hata mesaj� verilir
    }
    return loc;  // �znitelik konumu d�nd�r�l�r
}

GLint ShaderProgram::GetUniformLocation(const char* name) {
    GLint loc = glGetUniformLocation(_program, name);  // Belirtilen uniform de�i�keninin konumu al�n�r
    if (loc < 0) {
        std::cout << "No such uniform: " << name << std::endl;  // Uniform de�i�keni bulunamazsa hata mesaj� verilir
    }
    return loc;  // Uniform de�i�keninin konumu d�nd�r�l�r
}

void ShaderProgram::BeginUse() {
    glUseProgram(_program);  // Program kullan�lmaya ba�land���nda ilgili OpenGL fonksiyonu �a�r�l�r
}

void ShaderProgram::EndUse() {
    glUseProgram((GLuint)NULL);  // Program kullan�m� sonland���nda ilgili OpenGL fonksiyonu �a�r�l�r
}
