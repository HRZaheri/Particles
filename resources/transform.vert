#version 330

layout(location = 0) in vec4  in_position;
layout(location = 1) in vec3  in_speed;
layout(location = 2) in float  _ID;

uniform vec3 target;
uniform float scale;
uniform vec2 rand;
uniform int mode;
uniform sampler2D randomtex;

uniform int offset_velocity;
uniform sampler2D tex;
uniform int tex_width;
uniform int tex_height;
uniform int counter;

out vec4 Position;

vec4 randomMotion(vec3 speed){
    vec3 velocity = ((target - in_position.xyz)) * scale ;
    vec4 vert = in_position + vec4(velocity*0.04 + (in_speed*0.5+speed*0.01)*0.1, 0.0);
    return vert;
}

vec4 drawCube(int _ID, vec3 speed){
    float offset = 0.0;//float(_ID)/10000000.0f;
    _ID = (_ID) % (tex_width * tex_height);
    float col = offset + float(_ID)/(tex_width-100);
    vec4 val = texture2D(tex, vec2(col, 0.0));
    vec3 velocity = (((target*0.8 + val.xyz*0.2)  - in_position.xyz)) * scale ;
    vec4 vert = in_position + vec4(velocity*0.06 + (in_speed*0.01+speed*0.07)*0.002, 0.0);

    return vert;
}


vec4 drawEF(int _ID, vec3 speed){
    float offset = 0.0;//float(_ID)/100000.0f;
    _ID = (_ID) % (tex_width * tex_height);
    float col = offset + float(_ID)/(tex_width);
    vec4 val = texture2D(tex, vec2(col, 0.0));
    vec3 velocity = (((target*0.8 + val.xyz*0.2)  - in_position.xyz)) * scale ;
    vec4 vert = in_position + vec4(velocity*0.06 + (in_speed*0.01+speed*0.07)*0.002, 0.0);

    return vert;
}


vec4 drawSphere(int _ID, vec3 speed){
    _ID = (100 *_ID) % (tex_width * tex_height);
    float col = float(_ID%tex_width)/(tex_width);
    float row = float(_ID/tex_width)/(tex_height);
    vec4 val = texture2D(tex, vec2(col, row));

    vec3 velocity = (((target*0.8 + val.xyz*0.2)  - in_position.xyz)) * scale ;
    vec4 vert = in_position + vec4(velocity*0.05 + (in_speed*0.5+speed*0.01)*0.0, 0.0);

    return vert;
}

vec4 drawTorus(int _ID, vec3 speed){
    _ID = (500 *_ID) % (tex_width * tex_height);
    float col = float(_ID%tex_width)/(tex_width);
    float row = float(_ID/tex_width)/(tex_height);
    vec4 val = texture2D(tex, vec2(col, row));

    vec3 velocity = (((target*0.8 + val.xyz*0.2)  - in_position.xyz)) * scale ;
    vec4 vert = in_position + vec4(velocity*0.05 + (in_speed*0.5+speed*0.01)*0.0, 0.0);

    return vert;
}

void main() {
    //    int _ID = gl_InstanceID;
    vec4 vert;
    vec4 speed = texture2D(randomtex, vec2(float(in_position.x * rand.x),
                                          float(in_position.y * rand.y)));


    if(mode == 0) vert = randomMotion(speed.xyz);
    else if(mode == 1) vert = drawCube(int(_ID), speed.xyz);
    else if(mode == 5) vert = drawEF(int(_ID), speed.xyz);
    else if(mode == 2) vert = drawSphere(int(_ID), speed.xyz);
    else if(mode == 3) vert = drawTorus(int(_ID), speed.xyz);
    Position = vert ;
}
