#version 330

in vec3 color;

layout(location = 0) out vec4 fragColor0;

void main() {
    fragColor0 = vec4(color,1.0) + vec4(0.3, 0.0, 0.3, 0.0);
}
