#ifndef MATH_H
#define MATH_H

#include <iostream>
#include <cmath>


struct vec2 {
    float x = 0.0f;
    float y = 0.0f;

    // Constructors
    vec2() = default;
    vec2(float _x, float _y) : x(_x), y(_y) {}
    vec2(float scalar) : x(scalar), y(scalar) {}

    // Operator Overloading for Vector Math
    vec2 operator+(const vec2& other) const { return vec2(x + other.x, y + other.y); }
    vec2 operator-(const vec2& other) const { return vec2(x - other.x, y - other.y); }
    vec2 operator*(float scalar) const { return vec2(x * scalar, y * scalar); }

    // Utility functions
    float length() const { return std::sqrt(x * x + y * y); }
};

#endif