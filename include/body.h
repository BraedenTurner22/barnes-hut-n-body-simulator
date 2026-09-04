#ifndef BODY_H
#define BODY_H

#include <iostream>
#include "vec2.h"

struct Body {
    vec2 pos_;
    vec2 vel_;
    vec2 acc_;
    float mass_;
    float radius_;

    Body(vec2 pos, vec2 vel, float mass, float radius) : pos_(pos), vel_(vel), acc_(0.0), mass_(mass), radius_(radius) {}

    void update(float dt) {
        vel_ += acc_ * dt;
        pos_ += vel_ * dt;
    }
};

#endif