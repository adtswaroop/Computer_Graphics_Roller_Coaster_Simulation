#version 150

in vec3 viewPosition;
in vec3 viewNormal;
out vec4 c; 

vec4 La = vec4(1.0f, 1.0f, 1.0f, 1.0f); // light ambient
vec4 Ld = vec4(1.5f, 1.5f, 1.5f, 1.5f); // light diffuse
vec4 Ls = vec4(1.0f, 1.0f, 1.0f, 1.0f); // light specular
uniform vec3 viewLightDirection;

vec4 ka = vec4(0.1f, 0.1f, 0.1f, 1.0f); // mesh ambient
vec4 kd = vec4(0.5f, 0.5f, 0.5f, 1.0f); // mesh diffuse
vec4 ks = vec4(0.9f, 0.9f, 0.9f, 1.0f); // mesh specular
float alpha = 15.0f; // shininess

void main()
{
	// camera is at (0,0,0) after the modelview transformation
	vec3 eyedir = normalize(vec3(0, 0, 0) - viewPosition);

	// reflected light direction
	vec3 reflectDir = -reflect(viewLightDirection, viewNormal);

	// Phong lighting
	float d = max(dot(viewLightDirection, viewNormal), 0.0f);
	float s = max(dot(reflectDir, eyedir), 0.0f);

	// compute the final color
	c = ka * La + d * kd * Ld + pow(s, alpha) * ks * Ls;
}
