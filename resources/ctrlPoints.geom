#version 330

layout(points) in;
layout(points, max_vertices = 100) out;

uniform int offset_x;
uniform int offset_y;
uniform sampler2D tex;
uniform vec3 translate;
uniform mat4 viewMX;
uniform mat4 projMX;
uniform float time;

out vec4 color;
out float mixingvalue;

void main() {

    int counter = 0 ;
    gl_Position = gl_in[0].gl_Position;
    gl_PointSize = gl_in[0].gl_PointSize;
    mixingvalue = (gl_Position.x + gl_Position.y + gl_Position.z)/3.0;
    color = vec4(0.0, 0.0, 1.0, 1.0);
    EmitVertex();
    EndPrimitive();
    ++counter;

    color = vec4(0.6, 0.6, 0.6, 0.5);
    for(int i=offset_x; i<10+offset_x; i++){
        for(int j=offset_y; j<10+offset_y; j++){
            vec4 val = texture2D(tex, vec2(float(i)/10., float(j)/10.));
            gl_Position = gl_in[0].gl_Position + 0.5 * vec4(val.rgb , 0.0) ;
            gl_PointSize = gl_in[0].gl_PointSize;
            mixingvalue = (gl_Position.x + gl_Position.y + gl_Position.z)/3.0;
            EmitVertex();
            EndPrimitive();
            if(++counter == 100)
                break;
        }
    }
}
