#version 330

layout(location = 0) in vec4  in_position;
layout(location = 1) in vec3  in_speed;

uniform vec3 translate;
uniform mat4 viewMX;
uniform mat4 projMX;
uniform float  pointSize;
uniform vec3 rand;
uniform sampler2D randomtex;
uniform float time;

out vec4 color;

void main() {
    float _ID = float(gl_InstanceID);
    color = vec4( in_speed.x*0.5 + in_position.x*0.6 + 0.5 ,
                  in_speed.y*0.3 + in_position.y*0.7 + 0.2 ,
                  in_speed.z*0.6 + in_position.z*0.4 + 0.5, 1.0);
	gl_PointSize = pointSize;
    vec4 vert =  (in_position //+ vec4(_ID/50.0-_ID*_ID/25.0, _ID*_ID/50.0-_ID/25.0,sqrt(_ID*2)/10.0, 0.0)
                  - vec4(translate,0.0));
    vert.xz = clamp(vert.xz, -0.5, 0.5);
    vert.y = clamp(vert.y, 0.0, 0.5);
        gl_Position = projMX * viewMX * vert;
}
