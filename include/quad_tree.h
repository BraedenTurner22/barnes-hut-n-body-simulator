// quadtree.h
#pragma once
#include <iostream>
#include <array>
#include <vector>
#include <limits>
#include <cmath>
#include <atomic>

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
    std::size_t depth;

    // Default-constructible so the node pool can be preallocated in one
    // shot (QuadTree::clear) instead of grown via push_back -- required
    // once insert() runs concurrently, since a growing std::vector can
    // reallocate and invalidate every other thread's in-flight node
    // references.
    Node() : children(0), next_(0), pos(vec2(0, 0)), mass(0.0f), quad_(Quad{vec2(0, 0), 0.0f}), depth(0) {}
    Node(std::size_t next, Quad quad, std::size_t depth)
        : children(0), next_(next), pos(vec2(0, 0)), mass(0.0f), quad_(quad), depth(depth) {}

    bool is_leaf() const { return children == 0; }
    bool is_branch() const { return children != 0; }
    bool is_empty() const { return mass == 0.0f; }
};

class QuadTree {
private:
    float t_sq; // theta squared, opening angle criterion
    float e_sq; // epsilon squared, gravitational softening
    const size_t ROOT = 0;

    // Preallocated, fixed-capacity node pool. Concurrent insert() calls
    // claim node slots via nodeCount_ (an atomic bump allocator) instead
    // of nodes_.push_back(), which would race on the vector's internal
    // reallocation. Capacity is sized generously in clear() from the
    // body count and never grows mid-build -- if a run ever needs more
    // (pathologically clustered positions subdividing far deeper than
    // usual), allocateNode() aborts loudly rather than silently
    // corrupting memory past the end of the vector.
    std::vector<Node> nodes_;
    std::atomic<std::size_t> nodeCount_{0};

    // One spinlock per node slot, guarding that node's is_leaf/is_branch
    // state and its pos/mass/children fields during insertion. Only ever
    // one lock held at a time (descend releases the parent's lock before
    // acquiring the child's) so there's no lock-ordering cycle possible,
    // hence no deadlock risk.
    std::vector<std::atomic<int>> locks_;

    void lockNode(std::size_t idx) {
        int expected = 0;
        while (!locks_[idx].compare_exchange_weak(
            expected, 1, std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = 0;
        }
    }
    void unlockNode(std::size_t idx) {
        locks_[idx].store(0, std::memory_order_release);
    }

public:
    // No default constructor -- t_sq/e_sq must always be provided,
    // so there's no way to end up with a QuadTree whose acc() reads
    // uninitialized memory.
    QuadTree(float theta, float epsilon)
        : t_sq(theta * theta), e_sq(epsilon * epsilon) {}

    // Allocates 4 fresh node slots and initializes them as children of
    // `node` (whose lock the caller must already hold). Only ever called
    // on a node the caller has exclusive access to, so the writes below
    // need no locking of their own -- publishing nodes_[node].children
    // last is what makes the new subtree visible to other threads.
    std::size_t subdivide(std::size_t node) {
        std::size_t childDepth = nodes_[node].depth + 1;
        std::size_t children = nodeCount_.fetch_add(4, std::memory_order_relaxed);
        if (children + 3 >= nodes_.size()) {
            std::cerr << "QuadTree node pool exhausted (capacity=" << nodes_.size()
                      << "). Bump the capacity multiplier in clear().\n";
            std::exit(EXIT_FAILURE);
        }

        auto quads = nodes_[node].quad_.subdivide();
        std::size_t nexts[4] = {children + 1, children + 2, children + 3, nodes_[node].next_};
        for (int i = 0; i < 4; ++i) {
            nodes_[children + i] = Node(nexts[i], quads[i], childDepth);
        }
        nodes_[node].children = children;
        return children;
    }

    void insert(vec2 pos, float mass) {
        std::size_t node = ROOT;

        while (true) {
            lockNode(node);

            if (nodes_[node].is_branch()) {
                // children is write-once (a node is only ever subdivided
                // while its own lock is held below, and never again once
                // is_branch() is true), so it's safe to read it here and
                // release before locking the child -- never holding two
                // node locks at once.
                auto q = nodes_[node].quad_.find_quadrant(pos);
                std::size_t next = nodes_[node].children + q;
                unlockNode(node);
                node = next;
                continue;
            }

            if (nodes_[node].is_empty()) {
                nodes_[node].pos = pos;
                nodes_[node].mass = mass;
                unlockNode(node);
                return;
            }

            vec2 p = nodes_[node].pos;
            float m = nodes_[node].mass;

            if (pos.x == p.x && pos.y == p.y) {
                nodes_[node].mass += mass;
                unlockNode(node);
                return;
            }

            // Two distinct bodies want this leaf: subdivide while still
            // holding node's lock, so no other thread can observe it
            // mid-split or double-subdivide it.
            std::size_t children = subdivide(node);
            unlockNode(node);

            auto q1 = nodes_[node].quad_.find_quadrant(p);
            auto q2 = nodes_[node].quad_.find_quadrant(pos);

            if (q1 != q2) {
                std::size_t n1 = children + q1, n2 = children + q2;
                lockNode(n1); nodes_[n1].pos = p; nodes_[n1].mass = m; unlockNode(n1);
                lockNode(n2); nodes_[n2].pos = pos; nodes_[n2].mass = mass; unlockNode(n2);
                return;
            }

            std::size_t child = children + q1;
            if (nodes_[child].quad_.size < 1e-6f) {
                // Handle infinite subdivision loop near float precision
                // limit: merge here at the weighted-average position
                // instead of splitting further.
                lockNode(child);
                nodes_[child].mass = m + mass;
                nodes_[child].pos = (p * m + pos * mass) * (1.0f / nodes_[child].mass);
                unlockNode(child);
                return;
            }

            // Still colliding one level down and not yet degenerate --
            // place the pre-existing body here, then let the next pass
            // through this same loop (with node = child) resolve `pos`
            // against it, recursing into further subdivisions exactly
            // like the single-threaded version's inner loop did.
            lockNode(child);
            nodes_[child].pos = p;
            nodes_[child].mass = m;
            unlockNode(child);
            node = child;
        }
    }

    // Iterate depth-first from the deepest level up to compute center of
    // masses. All parent nodes' center of masses require the center of
    // mass of their children first, hence the reverse (leaves-up) order.
    //
    // Parallelized level-by-level: nodes at the same depth are siblings
    // or cousins with no dependency on each other, so each level is
    // safe to run as a plain parallel for. OpenMP's parallel-for has an
    // implicit barrier at the end, which is exactly the synchronization
    // needed before the next (shallower) level starts -- every node one
    // level down is guaranteed finished before its parent reads it.
    void propagate() {
        std::size_t n = nodeCount_.load(std::memory_order_relaxed);
        if (n == 0) return;

        std::size_t maxDepth = 0;
        for (std::size_t i = 0; i < n; ++i) {
            if (nodes_[i].is_branch()) maxDepth = std::max(maxDepth, nodes_[i].depth);
        }

        std::vector<std::vector<std::size_t>> levels(maxDepth + 1);
        for (std::size_t i = 0; i < n; ++i) {
            if (nodes_[i].is_branch()) levels[nodes_[i].depth].push_back(i);
        }

        for (std::size_t d = levels.size(); d-- > 0;) {
            auto& level = levels[d];
            #pragma omp parallel for
            for (std::size_t k = 0; k < level.size(); ++k) {
                std::size_t idx = level[k];
                auto i = nodes_[idx].children;

                nodes_[idx].pos = nodes_[i].pos * nodes_[i].mass +
                                   nodes_[i + 1].pos * nodes_[i + 1].mass +
                                   nodes_[i + 2].pos * nodes_[i + 2].mass +
                                   nodes_[i + 3].pos * nodes_[i + 3].mass;

                nodes_[idx].mass = nodes_[i].mass + nodes_[i + 1].mass +
                                    nodes_[i + 2].mass + nodes_[i + 3].mass;

                auto total_mass = nodes_[idx].mass;
                if (total_mass > 0.0f) {
                    nodes_[idx].pos /= total_mass;
                } else {
                    nodes_[idx].pos = vec2(0, 0);
                }
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
            auto denominator = (d_sq + e_sq) * std::sqrt(d_sq + e_sq);

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

    // bodyCount sizes the node pool up front: 8 nodes/body is a generous
    // margin for clustered distributions (worst case -- many bodies
    // subdividing repeatedly down near the 1e-6 size floor -- could
    // still exceed it; subdivide() aborts loudly rather than silently
    // overrunning the pool if that happens).
    void clear(Quad root_quad, std::size_t bodyCount) {
        std::size_t capacity = 1 + bodyCount * 8;
        nodes_.assign(capacity, Node{});
        locks_ = std::vector<std::atomic<int>>(capacity);
        nodeCount_.store(0, std::memory_order_relaxed);
        nodes_[0] = Node(0, root_quad, 0);
        nodeCount_.store(1, std::memory_order_relaxed);
    }
};
