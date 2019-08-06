#version 460

layout (location = 0) in vec3 v_color;

layout (location = 0) out vec4 fragmentColor;

void main() {
	if (v_color.r == 0.0 && v_color.g == 0.0 && v_color.b == 0.0) {
		fragmentColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
	else {
    	fragmentColor = vec4(v_color, 1.0);
	}
}