#pragma once
//////////////////////////////////////////////////////////////////////////

#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "obj_loader.h"


struct Vertex
{
  glm::vec3 pos;
  glm::vec3 nrm;
  glm::vec3 color;
  glm::vec2 texCoord;
  int       matID = 0;

  static auto getBindingDescription();
  static auto getAttributeDescriptions();
};

//--------------------------------------------------------------------------------------------------
//
//
class HelloVulkan
{
public:
  void      createDescriptorSetLayout();
  void      createGraphicsPipeline(VkExtent2D framebufferSize);
  void      loadModel(const std::string& filename);
  void      createMaterialBuffer(const std::vector<MatrialObj>& materials);
  void      createVertexBuffer(const std::vector<Vertex>& vertex);
  void      createIndexBuffer(const std::vector<uint32_t>& indices);
  void      updateDescriptorSet();
  void      createUniformBuffer();
  void      createTextureImages(const std::vector<std::string>& textures);
  VkSampler createTextureSampler();
  void      updateUniformBuffer();

  void destroyResources();

  VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
  VkPipelineLayout      m_pipelineLayout      = VK_NULL_HANDLE;
  VkPipeline            m_graphicsPipeline    = VK_NULL_HANDLE;
  VkDescriptorSet       m_descriptorSet;

  uint32_t m_nbIndices;
  uint32_t m_nbVertices;

  VkBuffer       m_vertexBuffer;
  VkDeviceMemory m_vertexBufferMemory;

  VkBuffer       m_indexBuffer;
  VkDeviceMemory m_indexBufferMemory;

  VkBuffer       m_uniformBuffer;
  VkDeviceMemory m_uniformBufferMemory;

  VkBuffer       m_matColorBuffer;
  VkDeviceMemory m_matColorBufferMemory;

  VkExtent2D m_framebufferSize;

  std::vector<VkImage>        m_textureImage;
  std::vector<VkDeviceMemory> m_textureImageMemory;
  std::vector<VkImageView>    m_textureImageView;
  std::vector<VkSampler>      m_textureSampler;
};
