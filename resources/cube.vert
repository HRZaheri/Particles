#version 330

layout(location = 0) in vec4  in_position;

uniform mat4 projMX;
uniform mat4 viewMX;
uniform mat4 modelMX;

out vec2 vTexCoords;

void main() {
    mat4 scaleMX = mat4(10,0,0,0, 0,1,0,0, 0,0,10,0, 0,0,0,1);
    gl_Position = projMX * viewMX * modelMX * scaleMX * in_position;
}
