#include <iostream>
#include <array>
#include <vector>
#include <memory>

#include <math.h>

struct Node {
    
    std::array<std::unique_ptr<Node>, 4> children;

    // Default constructs to null for all 4 children
};

struct QuadTree {
    std::vector<Node> nodes;

    void clear() {
        std::fill(nodes.begin(), nodes.end(), nullptr);
    }
};