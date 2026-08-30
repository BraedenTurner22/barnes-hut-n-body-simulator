#include <iostream>

#include "renderer.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main() {
    Renderer renderer(SCR_WIDTH, SCR_HEIGHT, "Barnes-Hut N-Body Simulator");

    while (!renderer.shouldClose()) {
        renderer.checkEscapeKeyInput();
        renderer.beginFrame();
        renderer.draw();
        renderer.endFrame();
    }
    return 0;
}