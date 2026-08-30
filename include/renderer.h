#pragma once
#include <memory>

struct GLFWwindow;
class Shader;

class Renderer {
public:
    Renderer(int width, int height, const char* title);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool shouldClose() const;
    void checkEscapeKeyInput();
    void beginFrame();
    void draw();
    void endFrame();

private:
    GLFWwindow* window_ = nullptr;
    unsigned int VAO_ = 0, VBO_ = 0;
    std::unique_ptr<Shader> shader_;
};