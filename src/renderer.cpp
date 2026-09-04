#include "renderer.h"
#include "shader.h"
#include "body.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <cmath>
#include <algorithm>

namespace {
void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW error " << error << ": " << description << "\n";
}
} // namespace
Renderer::Renderer(int width, int height, const char* title)
    : width_(width), height_(height) {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) { std::cerr << "Failed to initialize GLFW\n"; std::exit(EXIT_FAILURE); }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) { std::cerr << "Failed to create GLFW window\n"; glfwTerminate(); std::exit(EXIT_FAILURE); }
    glfwMakeContextCurrent(window_);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize glad\n"; std::exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << "\n";
    // Without this, gl_PointSize set in the vertex shader is silently
    // ignored and points render at fixed, default size
    glEnable(GL_PROGRAM_POINT_SIZE);
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    // Bind vertex array object
    glBindVertexArray(VAO_);
    shader_ = std::make_unique<Shader>("shaders/particle.vert", "shaders/particle.frag");
}
Renderer::~Renderer() {
    glDeleteVertexArrays(1, &VAO_);
    glDeleteBuffers(1, &VBO_);
    glfwDestroyWindow(window_);
    glfwTerminate();
}
bool Renderer::shouldClose() const { return glfwWindowShouldClose(window_); }
void Renderer::checkEscapeKeyInput() {
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window_, true);
        std::cout << "Escape key pressed" << std::endl;
    }
}
void Renderer::beginFrame() {
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
void Renderer::updateParticles(const std::vector<Body>& bodies) {
    // Robust camera framing: center on the mean position and size the
    // view to contain a fixed PERCENTILE of bodies by distance from that
    // center, not the RMS spread. RMS is still a squared-distance
    // statistic -- among thousands of bodies, a handful of far-flung
    // stragglers/ejections can inflate it more than the 1/N averaging
    // suppresses, which was zooming the view out and shrinking the core
    // cluster's motion to nothing. A percentile ignores whatever lies
    // past the cutoff entirely, so the frame tracks wherever the
    // MAJORITY of bodies actually are instead of chasing outliers.
    //
    // NOTE: this only affects what the CAMERA frames -- the physics
    // itself (QuadTree's own bounding box, via Quad::new_containing
    // inside Simulation) is untouched and still correctly includes every
    // body, as Barnes-Hut correctness requires.
    vec2 meanPos(0.0f, 0.0f);
    for (const auto& b : bodies) meanPos += b.pos_;
    meanPos = meanPos * (1.0f / static_cast<float>(bodies.size()));

    std::vector<float> dists;
    dists.reserve(bodies.size());
    for (const auto& b : bodies) dists.push_back((b.pos_ - meanPos).length());

    // 85th percentile: comfortably covers the core cluster while letting
    // stragglers past that radius drift outside the frame instead of
    // dictating its scale. 1.2x padding keeps the edge from feeling
    // cropped.
    std::size_t idx = std::min(
        static_cast<std::size_t>(dists.size() * 0.85f), dists.size() - 1);
    std::nth_element(dists.begin(), dists.begin() + idx, dists.end());
    float halfSize = std::max(dists[idx] * 1.2f, 1e-3f);

    float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    if (aspect >= 1.0f) {
        boundsLeft_   = meanPos.x - halfSize * aspect;
        boundsRight_  = meanPos.x + halfSize * aspect;
        boundsBottom_ = meanPos.y - halfSize;
        boundsTop_    = meanPos.y + halfSize;
    } else {
        boundsLeft_   = meanPos.x - halfSize;
        boundsRight_  = meanPos.x + halfSize;
        boundsBottom_ = meanPos.y - halfSize / aspect;
        boundsTop_    = meanPos.y + halfSize / aspect;
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    if (bodies.size() != particleCount_) {
        // Body count changed, reallocate
        glBufferData(GL_ARRAY_BUFFER, bodies.size() * sizeof(Body), bodies.data(), GL_DYNAMIC_DRAW);
        particleCount_ = bodies.size();
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Body), (void*)offsetof(Body, pos_));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Body), (void*)offsetof(Body, radius_));
        glEnableVertexAttribArray(1);
    } else {
        // Same count as last frame, refresh contents in place
        glBufferSubData(GL_ARRAY_BUFFER, 0, bodies.size() * sizeof(Body), bodies.data());
    }
}
void Renderer::drawParticles() {
    glm::mat4 projection = glm::ortho(boundsLeft_, boundsRight_, boundsBottom_, boundsTop_, -1.0f, 1.0f);
    shader_->use();
    shader_->setMat4("uProjection", projection);
    glBindVertexArray(VAO_);
    glDrawArrays(GL_POINTS, 0, static_cast<int>(particleCount_));
}
void Renderer::endFrame() {
    glfwSwapBuffers(window_);
    glfwPollEvents();
}