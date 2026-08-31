// simulation.h
#pragma once
#include <vector>
#include "body.h"
#include "quad_tree.h"

class Simulation {
public:
    Simulation(std::vector<Body> bodies, float theta, float epsilon, float dt);

    // Advances the simulation by one timestep.
    void step();

    const std::vector<Body>& bodies() const { return bodies_; }

private:
    std::vector<Body> bodies_;
    QuadTree tree_;
    float dt_;
};