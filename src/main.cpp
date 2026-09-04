#include "renderer.h"
#include "simulation.h"
#include <random>
#include <cmath>
#include <chrono>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const float THETA = 0.5f;
const float EPSILON = 0.15f;
const unsigned int BODY_COUNT = 100000;

std::vector<Body> makeCluster(int count, float radius, float theta, float epsilon, unsigned seed = 42) {
    std::vector<Body> bodies;
    bodies.reserve(count);

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
    std::uniform_real_distribution<float> radiusDist(0.0f, radius);
    std::uniform_real_distribution<float> massDist(0.5f, 2.0f);

    for (int i = 0; i < count; ++i) {
        float angle = angleDist(rng);
        float r = radiusDist(rng);
        vec2 pos(r * std::cos(angle), r * std::sin(angle));
        float mass = massDist(rng);
        bodies.emplace_back(pos, vec2(0.0f, 0.0f), mass, 0.5f);
    }

    // Solid-body rotation (v = omega * r) has no relation to how much
    // mass is actually pulling on each body -- it was either wildly
    // under- or over-rotating relative to the cluster's real gravity,
    // which is why the disc collapsed/ejected bodies instead of
    // orbiting. Give each body the tangential speed that balances the
    // ACTUAL gravity at its position instead (v = sqrt(r * |g|), the
    // standard "cold start" circular-velocity trick for a quasi-stable
    // disc).
    //
    // The field itself comes from one Barnes-Hut tree build + query pass
    // (same theta/epsilon Simulation::step() uses every frame), not a
    // direct O(N^2) pairwise sum -- an O(N log N) setup pass matches the
    // complexity the rest of this project is built around, and it means
    // the initial velocities are balanced against the exact (approximated)
    // field the sim will actually apply from step 1 onward.
    Quad root = Quad::new_containing(bodies);
    QuadTree tree(theta, epsilon);
    tree.clear(root, bodies.size());
    #pragma omp parallel for
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        tree.insert(bodies[i].pos_, bodies[i].mass_);
    }
    tree.propagate();

    #pragma omp parallel for
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        vec2 g = tree.acc(bodies[i].pos_);
        float r = bodies[i].pos_.length();
        if (r > 1e-4f) {
            float speed = std::sqrt(r * g.length());
            vec2 tangent = vec2(-bodies[i].pos_.y, bodies[i].pos_.x) * (1.0f / r);
            bodies[i].vel_ = tangent * speed;
        }
    }

    return bodies;
}

int main() {
    Renderer renderer(SCR_WIDTH, SCR_HEIGHT, "Barnes-Hut N-Body Simulator");

    std::vector<Body> bodies = makeCluster(/*count=*/BODY_COUNT, /*radius=*/50.0f, THETA, EPSILON,
        static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    Simulation sim(bodies, THETA, EPSILON, /*dt=*/0.005f);

    // How many physics steps run per rendered frame. dt stays small (for
    // integration accuracy), and this multiplies how much simulated time
    // elapses per second of wall-clock time instead.
    const int stepsPerFrame = 1;

    while (!renderer.shouldClose()) {
        renderer.checkEscapeKeyInput();
            
        for (int i = 0; i < stepsPerFrame; ++i) {
            sim.step();
        }

        renderer.updateParticles(sim.bodies());
        renderer.beginFrame();
        renderer.drawParticles();
        renderer.endFrame();
    }
    return 0;
}