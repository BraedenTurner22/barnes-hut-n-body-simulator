#pragma once
#include <memory>
#include <vector>

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
    void endFrame();

    // Uploads this frame's body position to GPU (reallocating only when
    // the body count changes) and re-fits the camera bounds to the
    // current spread every frame, so bodies stay on screen even if the
    // simulation expands, collapses, or ejects outliers.
    void updateParticles(const std::vector<Body>& bodies);
    void drawParticles();

private:
    GLFWwindow* window_ = nullptr;
    int width_ = 0, height_ = 0;
    unsigned int VAO_ = 0, VBO_ = 0;
    std::size_t particleCount_ = 0;
    std::unique_ptr<Shader> shader_;

    // World-space bounds, recomputed each frame from the current bounding
    // box. Kept as plain floats (not glm::mat4) so this header doesn't
    // need to include GLM -- the actual matrix is built in drawParticles().
    float boundsLeft_ = -1.0f, boundsRight_ = 1.0f;
    float boundsBottom_ = -1.0f, boundsTop_ = 1.0f;
};