#pragma once

class ShaderProgram {

private:
	int _program = -1;

public:
	ShaderProgram(const char* vertexPath, const char* fragmentPath);
	~ShaderProgram();

	void BeginUse();
	void EndUse();

	GLint GetAttribLocation(const char* name);
	GLint GetUniformLocation(const char* name);
};
