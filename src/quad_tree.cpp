#include <iostream>
#include <array>
#include <vector>
#include <memory>

// include project math utilities (vec2)
#include "../include/math.h"

struct Quad {
    vec2 center;
    float size;
};

struct Node {
    
    int children[4] = {0, 0, 0, 0};
    Quad quad;

    // Default constructs to 0 for all 4 children, 0 represents absence of child node
    Node() = default;

};

struct QuadTree {
    std::vector<Node> nodes;

    void clear() {
        nodes.clear();
        nodes.push_back(Node{});
    }

};