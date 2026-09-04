// simulation.cpp
#include "simulation.h"

Simulation::Simulation(std::vector<Body> bodies, float theta, float epsilon, float dt)
    : bodies_(std::move(bodies)), tree_(theta, epsilon), dt_(dt) {}

void Simulation::step() {
    // Rebuild the tree from scratch -- positions moved since last step,
    // so the previous tree's structure is stale.
    Quad root_quad = Quad::new_containing(bodies_);
    tree_.clear(root_quad, bodies_.size());

    // Each insert() call takes and releases per-node locks internally
    // (see quad_tree.h), so concurrent insertion is safe here.
    #pragma omp parallel for
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        tree_.insert(bodies_[i].pos_, bodies_[i].mass_);
    }
    tree_.propagate();

    // Compute this step's acceleration for each body (ran in parallel with OpenMP), then let Body's own
    // update() handle integration (semi-implicit Euler: velocity first,
    // then position using the new velocity).
    #pragma omp parallel for
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        bodies_[i].acc_ = tree_.acc(bodies_[i].pos_);
        bodies_[i].update(dt_);
    }
}