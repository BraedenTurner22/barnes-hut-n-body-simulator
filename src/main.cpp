#include <iostream>

#include "renderer.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

#include "renderer.h"

int main() {
    Renderer renderer(SCR_WIDTH, SCR_HEIGHT, "Barnes-Hut N-Body Simulator");

    while (!renderer.shouldClose()) {
        renderer.beginFrame();
        // (particle rendering goes here, once shaders/VBOs exist)
        renderer.endFrame();
    }

    return 0;
}