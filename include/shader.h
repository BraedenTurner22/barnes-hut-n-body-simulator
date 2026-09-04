#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    void setMat4(const std::string& name, const glm::mat4& matrix) const;
    void setFloat(const std::string& name, float value) const;

private:
    unsigned int id_ = 0;
    static std::string readFile(const std::string& path);
    unsigned int compileStage(const std::string& source, unsigned int stageType) const;
    void checkLinkErrors() const;
};