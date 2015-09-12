#version 330

uniform sampler2D tex;
uniform vec3 pickIdCol;
uniform vec3 ambientLight;
uniform int drawID; //1 = color ID 2= normal 3 = texture

in vec2 texCoords;
in vec3 normal;
in float Brightness;

layout(location = 0) out vec4 fragColor0;

void main() {
    fragColor0 = texture2D(tex, texCoords);

	fragColor0 = vec4(1.0, 1.0, 1.0, 1.0);
}
