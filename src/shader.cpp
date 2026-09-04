// shader.cpp
#include "shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <glm/gtc/type_ptr.hpp>

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << "\n";
        std::exit(EXIT_FAILURE);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int Shader::compileStage(const std::string& source, unsigned int stageType) const {
    unsigned int shader = glCreateShader(stageType);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed ("
                  << (stageType == GL_VERTEX_SHADER ? "vertex" : "fragment")
                  << "):\n" << infoLog << "\n";
        std::exit(EXIT_FAILURE);
    }
    return shader;
}

void Shader::checkLinkErrors() const {
    int success;
    glGetProgramiv(id_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(id_, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed:\n" << infoLog << "\n";
        std::exit(EXIT_FAILURE);
    }
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    unsigned int vertexShader = compileStage(readFile(vertexPath), GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileStage(readFile(fragmentPath), GL_FRAGMENT_SHADER);

    id_ = glCreateProgram();
    glAttachShader(id_, vertexShader);
    glAttachShader(id_, fragmentShader);
    glLinkProgram(id_);
    checkLinkErrors();

    // Once linked into the program, the standalone shader objects are dead weight.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader() {
    glDeleteProgram(id_);
}

void Shader::use() const {
    glUseProgram(id_);
}

void Shader::setMat4(const std::string& name, const glm::mat4& matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(id_, name.c_str()), value);
}