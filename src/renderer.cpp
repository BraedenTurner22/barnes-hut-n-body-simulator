#include "renderer.h"
#include "shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>

namespace {
void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW error " << error << ": " << description << "\n";
}
} // namespace

Renderer::Renderer(int width, int height, const char* title) {
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

    // --- Everything below here needs a valid context + loaded GL functions,
    // which is why it happens down here in the constructor BODY, not as a
    // default-initialized member -- see the ordering warning from before. ---

    float vertices[] = {   // local now, not a global -- no more ODR risk
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    // Representation for rectangle object if desired, must be iomplemented with Element Buffer OBject (EBO)
    // float vertices[] = {
    //     0.5f,  0.5f, 0.0f,  // top right
    //     0.5f, -0.5f, 0.0f,  // bottom right
    //     -0.5f, -0.5f, 0.0f,  // bottom left
    //     -0.5f,  0.5f, 0.0f   // top left 
    // };
    // unsigned int indices[] = {  // note that we start from 0!
    //     0, 1, 3,   // first triangle
    //     1, 2, 3    // second triangle
    // };

    // unsigned int EBO;
    // glGenBuffers(1, &EBO);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);

    // Bind vertex array object
    glBindVertexArray(VAO_);
    // Copy vertices array in buffer for OpenGL to use
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    /*
    1. The first parameter specifies which vertex attribute we want to configure.
       Remember that we specified the location of the position vertex attribute in the vertex shader with layout (location = 0).
       This sets the location of the vertex attribute to 0 and since we want to pass data to this vertex attribute, we pass in 0.

    2. The next argument specifies the size of the vertex attribute. The vertex attribute is a vec3 so it is composed of 3 values.

    3. The third argument specifies the type of the data which is GL_FLOAT (a vec* in GLSL consists of floating point values).

    4. The next argument specifies if we want the data to be normalized.
       If we're inputting integer data types (int, byte) and we've set this to GL_TRUE, the integer data is normalized to
       0 (or -1 for signed data) and 1 when converted to float. This is not relevant for us so we'll leave this at GL_FALSE.

    5. The fifth argument is known as the stride and tells us the space between consecutive vertex attributes.
       Since the next set of position data is located exactly 3 times the size of a float away we specify that value as the stride.
       Note that since we know that the array is tightly packed (there is no space between the next vertex attribute value)
       we could've also specified the stride as 0 to let OpenGL determine the stride (this only works when values are tightly packed).
       Whenever we have more vertex attributes we have to carefully define the spacing between each vertex attribute but we'll
       get to see more examples of that later on.

    6. The last parameter is of type void* and thus requires that weird cast. This is the offset of where the position data begins in the buffer.
       Since the position data is at the start of the data array this value is just 0. We will explore this parameter in more detail later on.
    */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); // Vertex attribute location as the argument

    shader_ = std::make_unique<Shader>("shaders/triangle.vert", "shaders/triangle.frag");
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

void Renderer::draw() {
    shader_->use();
    glBindVertexArray(VAO_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::endFrame() {
    glfwSwapBuffers(window_);
    glfwPollEvents();
}