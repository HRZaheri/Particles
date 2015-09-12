#version 330

uniform mat4 projMX;
uniform mat4 viewMX;
uniform mat4 modelMX;
//uniform vec3 translate;

layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;

out vec3 normal;

vec3 GetNormal(){
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a,b));
}

void main() {
//    vec4 _translate = vec4(translate, 0.0);
    vec4 v1 = gl_in[0].gl_Position;
    vec4 v2 = gl_in[1].gl_Position;
    vec4 v3 = gl_in[2].gl_Position;
    normal = GetNormal();

    gl_Position = projMX * viewMX * modelMX * v1;
    EmitVertex();

    gl_Position = projMX * viewMX * modelMX * v2;
    EmitVertex();

    gl_Position = projMX * viewMX * modelMX * v3;
    EmitVertex();

    EndPrimitive();
}
