// quadtree.h
#pragma once
#include <iostream>
#include <array>
#include <vector>
#include <limits>
#include <cmath>

#include "vec2.h"
#include "body.h"

struct Quad {
    vec2 center;
    float size;

    // static: doesn't use `this` at all, so it can be called as
    // Quad::new_containing(bodies) without needing an existing Quad instance
    static Quad new_containing(const std::vector<Body>& bodies) {
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
        float size = std::max(max_x - min_x, max_y - min_y);
        return Quad{center, size};
    }

    // Find quadrant number: {1, 0}
    //                       {3, 2}
    std::size_t find_quadrant(vec2 pos) const {
        return (pos.y < center.y) << 1 | (pos.x < center.x);
    }

    Quad into_quadrant(int i) const {
        Quad q = *this; // Doesn't mutate the original
        q.size *= 0.5f;
        q.center.x += (0.5f - static_cast<float>(i & 1)) * q.size;
        q.center.y += (0.5f - static_cast<float>((i >> 1) & 1)) * q.size;
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
    // children is just the index for the first child.
    // From this index, we can deduce the indices of the other 3 child nodes
    std::size_t children;
    std::size_t next_;
    vec2 pos;
    float mass;
    Quad quad_;

    Node(std::size_t next, Quad quad)
        : children(0), next_(next), pos(vec2(0, 0)), mass(0.0f), quad_(quad) {}

    bool is_leaf() const { return children == 0; }
    bool is_branch() const { return children != 0; }
    bool is_empty() const { return mass == 0.0f; }
};

class QuadTree {
private:
    float t_sq; // theta squared, opening angle criterion
    float e_sq; // epsilon squared, gravitational softening
    const size_t ROOT = 0;
    std::vector<Node> nodes_;
    std::vector<size_t> parents_;

public:
    // No default constructor -- t_sq/e_sq must always be provided,
    // so there's no way to end up with a QuadTree whose acc() reads
    // uninitialized memory.
    QuadTree(float theta, float epsilon)
        : t_sq(theta * theta), e_sq(epsilon * epsilon), nodes_(), parents_() {}

    std::size_t subdivide(std::size_t node) {
        parents_.push_back(node);
        std::size_t children = nodes_.size();
        nodes_[node].children = children;

        std::vector<size_t> nexts = {children + 1, children + 2, children + 3, nodes_[node].next_};

        auto quads = nodes_[node].quad_.subdivide();
        for (int i{0}; i < 4; ++i) {
            nodes_.push_back(Node{nexts[i], quads[i]});
        }
        return children;
    }

    void insert(vec2 pos, float mass) {
        auto node = ROOT;

        while (nodes_[node].is_branch()) {
            auto q = nodes_[node].quad_.find_quadrant(pos);
            node = nodes_[node].children + q;
        }

        if (nodes_[node].is_empty()) {
            nodes_[node].pos = pos;
            nodes_[node].mass = mass;
            return;
        }

        auto p = nodes_[node].pos;
        auto m = nodes_[node].mass;

        if (pos.x == p.x && pos.y == p.y) {
            nodes_[node].mass += mass;
            return;
        }

        while (true) {
            auto children = subdivide(node);

            auto q1 = nodes_[node].quad_.find_quadrant(p);
            auto q2 = nodes_[node].quad_.find_quadrant(pos);

            if (q1 == q2) {
                node = children + q1;

                if (nodes_[node].quad_.size < 1e-6f) {
                    nodes_[node].mass = m + mass;
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

    void propagate() {
        if (nodes_.empty()) return;

        for (std::size_t node_idx = parents_.size(); node_idx-- > 0;) {
            auto i = nodes_[node_idx].children;

            nodes_[node_idx].pos = nodes_[i].pos * nodes_[i].mass +
                               nodes_[i + 1].pos * nodes_[i + 1].mass +
                               nodes_[i + 2].pos * nodes_[i + 2].mass +
                               nodes_[i + 3].pos * nodes_[i + 3].mass;

            nodes_[node_idx].mass = nodes_[i].mass + nodes_[i + 1].mass +
                                nodes_[i + 2].mass + nodes_[i + 3].mass;

            auto total_mass = nodes_[node_idx].mass;
            if (total_mass > 0.0f) {
                nodes_[node_idx].pos /= total_mass;
            } else {
                nodes_[node_idx].pos = vec2(0, 0);
            }
        }
    }

    vec2 acc(vec2 pos) {
        vec2 acc = vec2(0, 0);
        auto node = ROOT;

        while (true) {
            auto& n = nodes_[node];

            vec2 d = n.pos - pos;
            float d_sq = d.mag_sq();
            auto denominator = (d_sq + e_sq) * std::sqrt(d_sq);

            if (n.is_leaf() || n.quad_.size * n.quad_.size < d_sq * t_sq) {
                acc += d * std::min(n.mass / denominator, std::numeric_limits<float>::max());
                if (n.next_ == 0) break;
                node = n.next_;
            } else {
                node = n.children;
            }
        }
        return acc;
    }

    void clear(Quad root_quad) {
        nodes_.clear();
        parents_.clear();
        nodes_.emplace_back(0, root_quad);
    }
};