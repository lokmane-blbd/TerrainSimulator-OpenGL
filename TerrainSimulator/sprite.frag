#version 410

in vec2 uv;
out vec4 fragColor;

uniform sampler2D tex;

void main(){
	
	vec4 col = texture(tex, uv);
	fragColor = col;
}