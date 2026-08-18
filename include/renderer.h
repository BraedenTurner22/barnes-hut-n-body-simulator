// renderer.h
#pragma once

struct GLFWwindow;

class Renderer {
public:
    Renderer(int width, int height, const char* title);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Returns true once the user has requested the window close
    // (clicked the close button, pressed the bound close key, etc.)
    bool shouldClose() const;

    // Call once per frame: clears the screen, swaps buffers, polls input events.
    // Actual particle drawing gets added here once shaders/VBOs exist.
    void beginFrame();
    void endFrame();

private:
    GLFWwindow* window_ = nullptr;
};