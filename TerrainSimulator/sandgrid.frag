#version 410

in vec2 uv;
out vec4 fragColor;

uniform sampler2D gridTexture;
uniform vec2 gridDimension;
uniform float time;
uniform uint shadingOff;
uniform vec2 mouseUV;
uniform float nMaterials;

const int m_nothing = 0;
const int m_water	= 1;
const int m_acid	= 2;
const int m_sand	= 3;
const int m_rock	= 4;
const int m_wood	= 5;
const int m_fire	= 6;
const int m_ash		= 7;
const int m_smoke	= 8;
const int m_vapor	= 9;
const int m_dust	= 10;
const int m_poison	= 11;


// https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand(vec2 co){
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

// for on overview of signed distance functions see:
// https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm

float sphere(vec3 p, vec3 c, float r){
	return length(p-c) - r;
}

float wobble(vec3 p, vec3 c, float r){
	vec2 v = vec2(floor(time * 0.01));
	float rr = r * 1.0 + 0.25 * rand(c.xy+v);
	return sphere(p, c, rr);
}

vec3 colorAt(vec2 gridPos, out int mat){
	vec4 col = texture(gridTexture, gridPos/gridDimension);
	mat = int(round((1.0-col.a) * 255.99)); 
	return col.xyz;
}

vec3 fire(vec2 uv){		
	float t = floor(time * 0.05);
	float r = 0.8 + 0.2 * rand(uv + vec2(t, 0));
	float g = 0.8 * rand(uv + vec2(0.1 * rand(t*uv), t));
	return vec3(r, g, 0.0);
}

vec3 acid(vec2 uv){
	float t = floor(time * 0.02);
	vec2 v = floor(uv) + vec2(t);
	return vec3(0.6 + 0.5 * rand(v), 1.0, 0.0);
}

vec3 background(vec2 uv){
	return clamp(pow(0.2+uv.y, 1.2) * vec3(0.1, 0.35, 0.28), vec3(0), vec3(1));
}

float mouseDist(vec2 pos){
	float a = gridDimension.x / gridDimension.y;
	vec2 m = vec2(mouseUV.x, mouseUV.y) - pos;
	m.x *= a;
	return length(m);
}

float trace(vec3 gridPos, float r, out vec3 color, out int material){

	// simplified ray marching (assuming projection is orthographic)
	
	vec3 cellStart = vec3(floor(gridPos).xy, 0.0);
	vec3 center = cellStart + vec3(0.5, 0.5, 0.0);
	
	color = colorAt(center.xy, material);

	float d = 10.0;
	float dMin = 10.0;

	for (int dx = -1; dx <= 1; dx++){

		for (int dy = -1; dy <= 1; dy++){
			
			vec3 o = vec3(dx, dy, 0);
			vec3 c = center + o;

			int mat;
			vec3 clr = colorAt(c.xy, mat);
			
			// nicer borders at boundaries between different materials:
			// place sphere at different z-levels according to material ID
			float z = (1.0 - float(mat) / nMaterials);
			c -= vec3(0, 0, z);
						
			float d2 = 10.0;

			if (mat == m_water || mat == m_acid){
				d2 = wobble(gridPos, c, r);
			} else if (mat >= m_smoke){
				d2 = wobble(gridPos, c, 0.75 * r);

			// everything except "nothing"
			} else if (mat > m_nothing){
				d2 = sphere(gridPos, c, r);
			}

			// union
			d = min(d, d2);			
						
			if (d<dMin){
				dMin = d;
				
				if (mat == m_fire) {
					color = fire(c.xy);
				} else {
					color = clr;
				}

				material = mat;
			}
		}
	}

	return d;
}


vec3 getNormal(vec3 gridPos, float r, float eps){
	
	vec3 col;
	int mat;

	vec3 e = vec3(eps, 0.0, 0.0);
	float d1 = trace(gridPos + e.xyy, r, col, mat);
	float d2 = trace(gridPos - e.xyy, r, col, mat);
	float d3 = trace(gridPos + e.yxy, r, col, mat);
	float d4 = trace(gridPos - e.yxy, r, col, mat);
	float d5 = trace(gridPos + e.yyx, r, col, mat);
	float d6 = trace(gridPos - e.yyx, r, col, mat);

	return normalize(vec3(d1-d2, d3-d4, d5-d6));	
}

void main(){
	const float r = 0.72;
	const float normalSmoothDist = r - 0.25;
	const float startD = 1.0;
	vec3 gridPos = vec3(uv * gridDimension, startD);
	vec2 cellStart = floor(gridPos).xy;
	vec2 center = cellStart + vec2(0.5, 0.5);

	int m;
	fragColor = vec4(colorAt(center, m), 1.0);
	return;

	
}