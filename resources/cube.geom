#version 330

uniform vec3 DiffuseLight;

layout(triangles) in;
layout(triangle_strip,max_vertices=3) out;

out vec3 normal;
out float Brightness;

vec3 GetNormal(){
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a,b));
}

void main() {   
    vec4 v1 = gl_in[0].gl_Position;
    vec4 v2 = gl_in[1].gl_Position;
    vec4 v3 = gl_in[2].gl_Position;
    normal = GetNormal();

    for(int i=0; i<gl_in.length(); i++){

        vec4 lightVector = normalize( vec4(DiffuseLight, 1.0)- gl_in[i].gl_Position);
        Brightness = dot(vec3((-1.0)* lightVector[0],lightVector[1],lightVector[2]), normal);

        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}
