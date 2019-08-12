#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) flat in int matIndex;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

// layout(binding = 1) uniform sampler2D texSampler;

struct WaveFrontMaterial
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 transmittance;
  vec3 emission;
  float shininess;
  float ior;      // index of refraction
  float dissolve; // 1 == opaque; 0 == fully transparent
  int illum;      // illumination model (see http://www.fileformat.info/format/material/)
  int textureId;
};
const int sizeofMat = 5;

layout(binding = 1) buffer MatColorBufferObject { vec4[] m; }
materials;

layout(binding = 2) uniform sampler2D[] textureSamplers;

WaveFrontMaterial unpackMaterial()
{
  WaveFrontMaterial m;
  vec4 d0 = materials.m[sizeofMat * matIndex + 0];
  vec4 d1 = materials.m[sizeofMat * matIndex + 1];
  vec4 d2 = materials.m[sizeofMat * matIndex + 2];
  vec4 d3 = materials.m[sizeofMat * matIndex + 3];
  vec4 d4 = materials.m[sizeofMat * matIndex + 4];

  m.ambient = vec3(d0.x, d0.y, d0.z);
  m.diffuse = vec3(d0.w, d1.x, d1.y);
  m.specular = vec3(d1.z, d1.w, d2.x);
  m.transmittance = vec3(d2.y, d2.z, d2.w);
  m.emission = vec3(d3.x, d3.y, d3.z);
  m.shininess = d3.w;
  m.ior = d4.x;
  m.dissolve = d4.y;
  m.illum = floatBitsToInt(d4.z);
  m.textureId = floatBitsToInt(d4.w);

  return m;
}

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 lightVector = normalize(vec3(5, 4, 3));

  float dot_product = max(dot(lightVector, normalize(fragNormal)), 0.2);

  WaveFrontMaterial m = unpackMaterial();
  vec3 c = m.diffuse;
  if (m.textureId >= 0)
    c *= texture(textureSamplers[m.textureId], fragTexCoord).xyz;
  c *= dot_product;

  outColor = vec4(c, 1); // texture(texSampler, fragTexCoord);
}
