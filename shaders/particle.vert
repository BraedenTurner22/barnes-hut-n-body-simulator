#version 410 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in float aRadius;
uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = aRadius * 2.0;
}
