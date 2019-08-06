#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_textureCoordinate;
layout (location = 2) in vec3 a_normal;

layout (location = 0) out vec2 v_textureCoordinate;
layout (location = 1) out vec3 v_normal;
layout (location = 2) out vec3 fragmentPosition;

layout (binding = 0) uniform CoordinateObject {
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
} coordinateObject;

void main() {
    gl_Position = coordinateObject.projectionMatrix * coordinateObject.viewMatrix * coordinateObject.modelMatrix * vec4(a_position, 1.0);

    v_textureCoordinate = a_textureCoordinate;
    fragmentPosition = vec3(coordinateObject.modelMatrix * vec4(a_position, 1.0));
    v_normal = mat3(transpose(inverse(coordinateObject.modelMatrix))) * a_normal;
}