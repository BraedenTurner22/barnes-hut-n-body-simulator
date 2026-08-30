// shader.h
#pragma once
#include <string>

class Shader {
public:
    // Takes file paths, not source strings -- keeps GLSL editable as
    // real .vert/.frag files with syntax highlighting, no C++ escaping.
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Makes this program active for subsequent draw calls.
    void use() const;

    // You'll want at least these once you get to the camera step --
    // fill in signatures for the uniform types you actually need
    // (a 4x4 matrix for projection/view, vec2/float for point size, etc).
    // void setMat4(const std::string& name, /* ... */) const;
    // void setFloat(const std::string& name, float value) const;

private:
    unsigned int id_ = 0;  // the linked GL program handle

    // Helper: reads a file's full contents into a string.
    static std::string readFile(const std::string& path);

    // Helper: compiles ONE shader stage (vertex or fragment) from source,
    // returns its GL handle. Needs to know which stage (GL_VERTEX_SHADER
    // vs GL_FRAGMENT_SHADER) via a parameter.
    // This is the one place that should call glGetShaderiv(..., GL_COMPILE_STATUS, ...)
    // and glGetShaderInfoLog on failure -- write it once, use it twice
    // (once per stage) from the constructor.
    unsigned int compileStage(const std::string& source, unsigned int stageType) const;

    // Helper: same idea, but for glGetProgramiv(..., GL_LINK_STATUS, ...)
    // after glLinkProgram -- called once from the constructor.
    void checkLinkErrors() const;
};