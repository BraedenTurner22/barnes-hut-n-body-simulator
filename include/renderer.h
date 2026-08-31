#pragma once
#include <memory>

struct GLFWwindow;
struct Body;
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

    // Uploads this frame's body position to GPU. Reallocates the
    // buffer only when the body count changes, otherwise reuses it
    void updateParticles(const std::vector<Body>& bodies);
    void drawParticles();

private:
    GLFWwindow* window_ = nullptr;
    unsigned int VAO_ = 0, VBO_ = 0;
    std::size_t particle_count = 0;
    std::unique_ptr<Shader> shader_;
};