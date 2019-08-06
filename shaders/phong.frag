#version 460

layout (location = 0) in vec2 v_textureCoordinate;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec3 fragmentPosition;

layout (location = 0) out vec4 fragmentColor;

layout (binding = 1) uniform sampler2D textureSampler;

layout (binding = 2) uniform LightObject {
	vec3 lightPosition;
	vec3 lightColor;

	vec3 viewPosition;
} lightObject;

void main() {
	float ambientIntensity = 0.1;
	vec3 ambientColor = vec3(1.0, 1.0, 1.0);
	vec3 ambient = ambientIntensity * ambientColor;

	float diffuseIntensity = 0.85;
	vec3 normal = normalize(v_normal);
	vec3 lightDirection = normalize(lightObject.lightPosition - fragmentPosition);
	float diffuseLight = max(dot(normal, lightDirection), 0.0);
	vec3 diffuse = diffuseIntensity * diffuseLight * lightObject.lightColor;

	float specularIntensity = 0.5;
	vec3 viewDirection = normalize(lightObject.viewPosition - fragmentPosition);
	vec3 reflectDirection = reflect(-lightDirection, normal);
	float specularLight = pow(max(dot(viewDirection, reflectDirection), 0.0), 32);
	vec3 specular = specularIntensity * specularLight * lightObject.lightColor;

	vec3 lightingColor = (ambient + diffuse + specular);

	fragmentColor = vec4(lightingColor, 1.0) * texture(textureSampler, v_textureCoordinate);
}