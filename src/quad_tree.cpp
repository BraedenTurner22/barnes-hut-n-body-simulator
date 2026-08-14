#include <iostream>
#include <array>
#include <vector>
#include <memory>
#include <utility>
#include <ranges>

// include project math utilities (vec2)
#include "../include/math.h"
#include "../include/body.h"

struct Quad {
    vec2 center;
    float size;

    // New qudrant containing the amount of bodies below, center/size is calculated
    auto new_containing(const std::vector<Body>& bodies) {
        float min_x = std::numeric_limits<float>::max();
        float min_y = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float max_y = std::numeric_limits<float>::lowest();

        for (const auto& body : bodies) {
            min_x = std::min(min_x, body.pos_.x);
            min_y = std::min(min_y, body.pos_.y);
            max_x = std::max(max_x, body.pos_.x);
            max_y = std::max(max_y, body.pos_.y);
        }

        vec2 center = vec2(min_x + max_x, min_y + max_y) * 0.5f;
        float size = std::max(max_x -  min_x, max_y - min_y);

        return Quad{center, size};
    }

    // Find quadrant number: {1, 0}
    //                       {3, 2}
    std::size_t find_quadrant(vec2 pos) const {
        return (pos.y > center.y) << 1 | (pos.x > center.x);
    }

    auto into_quadrant(int i) const {
        Quad q = *this; // Doesn't mutate the original
        q.size *= 0.5; // Halve the size of the parent (both height and width, generates NW, NE, SW, SE)
        // If i & 1 == 0: 0.5 - 0 = 0.5 → shifts center right by 0.5 * size
        // If i & 1 == 1: 0.5 - 1 = -0.5 → shifts center left by 0.5 * size
        q.center.x += (0.5f - static_cast<float>(i & 1))        * q.size;
        // Same concept above for y, top and bottom
        q.center.y += (0.5f - static_cast<float>((i >> 1) & 1)) * q.size; // Picks top or bottom (0, 1)
        return q;
    }

    std::array<Quad, 4> subdivide() const {
        std::array<Quad, 4> result;
        for (int i{0}; i < 4; ++i) {
            result[i] = into_quadrant(i);
        }
        return result;
    }
};

struct Node {
    
    // children is just the index for the first child, defaults to 0 in constructor
    // From this index, we can deduce the indices of the other 3 child nodes
    std::size_t children;
    Quad quad;
    vec2 pos;
    float mass;

    Node(Quad _quad) : quad(_quad), children(0), pos(vec2(0, 0)), mass(0.0) {}

    bool is_leaf() {
        return children == 0;
    }

    bool is_branch() {
        return children != 0;
    }

    bool is_empty() {
        return mass == 0.0f;
    }



};

class QuadTree {
    const size_t ROOT = 0;
    std::vector<Node> nodes_;

public:
    std::size_t subdivide(std::size_t node) {
        std::size_t children = nodes_.size();
        nodes_[node].children = children;

        auto quads = nodes_[node].quad.subdivide();
        for (int i{0}; i < 4; ++i) {
            nodes_.push_back(Node{quads[i]});
        }
        return children;
    }

    void insert(vec2 pos, float mass) {
        auto node = ROOT;

        // Find leaf node of given position
        while (nodes_[node].is_branch()) {
            auto q = nodes_[node].quad.find_quadrant(pos);
            node = nodes_[node].children + q;
        }

        // If it is empty, set position/mass
        if (nodes_[node].is_empty()) {
            nodes_[node].pos = pos;
            nodes_[node].mass = mass;
            return;
        }

        // Check edge case where positions are equal. If so, combine masses
        auto p = nodes_[node].pos;
        auto m = nodes_[node].mass;

        if (pos.x == p.x && pos.y == p.y) {
            nodes_[node].mass += mass;
            return;
        }
        
        // Subdivide until the positions are no longer in the same leaf node
        // Then set their respective positions/masses
        while (true) {
            auto children = subdivide(node);

            auto q1 = nodes_[node].quad.find_quadrant(p);
            auto q2 = nodes_[node].quad.find_quadrant(pos);

            if (q1 == q2) {
                node = children + q1;

                // Handle infinite subdivision loop near float precision limit
                if (nodes_[node].quad.size < 1e-6f) {
                    nodes_[node].mass = m + mass;

                    // Place the combined center of mass at the weighted avg. position
                    nodes_[node].pos = (p * m + pos * mass) * (1.0f / nodes_[node].mass);
                    return;
                }
            } else {
                auto n1 = children + q1;
                auto n2 = children + q2;

                nodes_[n1].pos = p;
                nodes_[n1].mass = m;
                nodes_[n2].pos = pos;
                nodes_[n2].mass = mass;
                return;
            }
        }
    }

    // Iterate over nodes in reverse to calculate center of masses
    // All parent nodes' center of masses require the center of mass of their children first, hence reverse calculation
    void propagate() {

        if (nodes_.empty()) return;

        for (std::size_t node_idx = nodes_.size(); node_idx-- > 0;) {
            if (nodes_[node_idx].is_leaf()) {
                continue;
            }

            auto i = nodes_[node_idx].children;

            nodes_[node_idx].pos = nodes_[i].pos * nodes_[i].mass +
                               nodes_[i + 1].pos * nodes_[i + 1].mass +
                               nodes_[i + 2].pos * nodes_[i + 2].mass +
                               nodes_[i + 3].pos * nodes_[i + 3].mass;
            
            nodes_[node_idx].mass = nodes_[i].mass +
                                nodes_[i + 1].mass +
                                nodes_[i + 2].mass +
                                nodes_[i + 3].mass;
            
            // Ensures no division by 0
            auto total_mass = nodes_[node_idx].mass;
            if (total_mass > 0.0f) {
                nodes_[node_idx].pos /= total_mass;
            } else {
                nodes_[node_idx].pos = vec2(0, 0); // Default fallback for empty branches
            }
        }
    }



    void clear(Quad root_quad) {
        nodes_.clear();
        nodes_.push_back(Node{root_quad});
    }

};