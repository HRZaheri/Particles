#version 330

layout(location = 0) in vec4  in_position;

uniform mat4 projMX;
uniform mat4 viewMX;
uniform mat4 modelMX;

out vec3 color;

void main() {
    color = vec3(in_position.x * in_position.y, in_position.x * in_position.z,
                 in_position.y * in_position.z);
    gl_Position = projMX * viewMX * modelMX * in_position;
}
