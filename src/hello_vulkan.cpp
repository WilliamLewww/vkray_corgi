/******************************************************************************
 * Copyright 1998-2018 NVIDIA Corp. All Rights Reserved.
 *****************************************************************************/

#include "hello_vulkan.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "manipulator.h"
#include "vk_context.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//--------------------------------------------------------------------------------------------------
// Uniform buffer to store all matrices
//
struct UniformBufferObject
{
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 modelIT;
};

//--------------------------------------------------------------------------------------------------
// Convenient function to load the shaders (SPIR-V)
//
std::vector<char> readFile(const std::string& filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if(!file.is_open())
  {
    throw std::runtime_error("failed to open file!");
  }

  auto              fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

//--------------------------------------------------------------------------------------------------
// Vertex binding description used in the graphic pipeline
//
auto Vertex::getBindingDescription()
{
  VkVertexInputBindingDescription bindingDescription = {};
  bindingDescription.binding                         = 0;
  bindingDescription.stride                          = sizeof(Vertex);
  bindingDescription.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

  return bindingDescription;
}

//--------------------------------------------------------------------------------------------------
// Vertex attributes description used in the graphic pipeline
//
auto Vertex::getAttributeDescriptions()
{
  std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

  attributeDescriptions[0].binding  = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset   = offsetof(Vertex, pos);

  attributeDescriptions[1].binding  = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset   = offsetof(Vertex, nrm);

  attributeDescriptions[2].binding  = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[2].offset   = offsetof(Vertex, color);

  attributeDescriptions[3].binding  = 0;
  attributeDescriptions[3].location = 3;
  attributeDescriptions[3].format   = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[3].offset   = offsetof(Vertex, texCoord);

  attributeDescriptions[4].binding  = 0;
  attributeDescriptions[4].location = 4;
  attributeDescriptions[4].format   = VK_FORMAT_R32_SINT;
  attributeDescriptions[4].offset   = offsetof(Vertex, matID);

  return attributeDescriptions;
}

//--------------------------------------------------------------------------------------------------
// Called at each frame to update the camera matrix
//
void HelloVulkan::updateUniformBuffer()
{
  UniformBufferObject ubo = {};
  ubo.model               = glm::mat4(1);
  ubo.modelIT             = glm::inverseTranspose(ubo.model);

  ubo.view = CameraManip.getMatrix();

  const float aspectRatio = m_framebufferSize.width / static_cast<float>(m_framebufferSize.height);
  ubo.proj                = glm::perspective(glm::radians(65.0f), aspectRatio, 0.1f, 1000.0f);
  ubo.proj[1][1] *= -1;  // Inverting Y for Vulkan


  void* data;
  vkMapMemory(VkCtx.getDevice(), m_uniformBufferMemory, 0, sizeof(ubo), 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(VkCtx.getDevice(), m_uniformBufferMemory);
}

//--------------------------------------------------------------------------------------------------
//
//
void HelloVulkan::createDescriptorSetLayout()
{
  // Storing the matrices
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding                      = 0;
  uboLayoutBinding.descriptorCount              = 1;
  uboLayoutBinding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.pImmutableSamplers           = nullptr;
  uboLayoutBinding.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT;

  // Storing all materials
  VkDescriptorSetLayoutBinding uboMatColorLayoutBinding = {};
  uboMatColorLayoutBinding.binding                      = 1;
  uboMatColorLayoutBinding.descriptorCount              = 1;
  uboMatColorLayoutBinding.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  uboMatColorLayoutBinding.pImmutableSamplers           = nullptr;
  uboMatColorLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding                      = 2;
  samplerLayoutBinding.descriptorCount    = static_cast<uint32_t>(m_textureSampler.size());
  samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
      uboLayoutBinding, uboMatColorLayoutBinding, samplerLayoutBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount                    = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings                       = bindings.data();

  if(vkCreateDescriptorSetLayout(VkCtx.getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout)
     != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void HelloVulkan::createGraphicsPipeline(VkExtent2D framebufferSize)
{
  m_framebufferSize   = framebufferSize;
  auto vertShaderCode = readFile("shaders/vert_shader.spv");
  auto fragShaderCode = readFile("shaders/frag_shader.spv");

  VkShaderModule vertShaderModule = VkCtx.createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = VkCtx.createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName  = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  auto bindingDescription    = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions   = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x          = 0.0f;
  viewport.y          = 0.0f;
  viewport.width      = static_cast<float>(m_framebufferSize.width);
  viewport.height     = static_cast<float>(m_framebufferSize.height);
  viewport.minDepth   = 0.0f;
  viewport.maxDepth   = 1.0f;

  VkRect2D scissor = {};
  scissor.offset   = {0, 0};
  scissor.extent   = m_framebufferSize;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports    = &viewport;
  viewportState.scissorCount  = 1;
  viewportState.pScissors     = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;
  rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable  = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable     = VK_FALSE;
  colorBlending.logicOp           = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount   = 1;
  colorBlending.pAttachments      = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable       = VK_TRUE;
  depthStencil.depthWriteEnable      = VK_TRUE;
  depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds        = 0.0f;  // Optional
  depthStencil.maxDepthBounds        = 1.0f;  // Optional
  depthStencil.stencilTestEnable     = VK_FALSE;
  depthStencil.front                 = {};  // Optional
  depthStencil.back                  = {};  // Optional

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount             = 1;
  pipelineLayoutInfo.pSetLayouts                = &m_descriptorSetLayout;

  if(vkCreatePipelineLayout(VkCtx.getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout)
     != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount                   = 2;
  pipelineInfo.pStages                      = shaderStages;
  pipelineInfo.pVertexInputState            = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState          = &inputAssembly;
  pipelineInfo.pViewportState               = &viewportState;
  pipelineInfo.pRasterizationState          = &rasterizer;
  pipelineInfo.pMultisampleState            = &multisampling;
  pipelineInfo.pColorBlendState             = &colorBlending;
  pipelineInfo.pDepthStencilState           = &depthStencil;
  pipelineInfo.layout                       = m_pipelineLayout;
  pipelineInfo.renderPass                   = VkCtx.getRenderPass();
  pipelineInfo.subpass                      = 0;
  pipelineInfo.basePipelineHandle           = VK_NULL_HANDLE;

  if(m_graphicsPipeline != 0)
    vkDestroyPipeline(VkCtx.getDevice(), m_graphicsPipeline, nullptr);

  if(vkCreateGraphicsPipelines(VkCtx.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                               &m_graphicsPipeline)
     != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(VkCtx.getDevice(), fragShaderModule, nullptr);
  vkDestroyShaderModule(VkCtx.getDevice(), vertShaderModule, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Loading the OBJ file and setting up all buffers
//
void HelloVulkan::loadModel(const std::string& filename)
{
  ObjLoader<Vertex> loader;
  loader.loadModel(filename);

  m_nbIndices  = static_cast<uint32_t>(loader.m_indices.size());
  m_nbVertices = static_cast<uint32_t>(loader.m_vertices.size());
  createVertexBuffer(loader.m_vertices);
  createIndexBuffer(loader.m_indices);
  createMaterialBuffer(loader.m_materials);
  createTextureImages(loader.m_textures);
}

//--------------------------------------------------------------------------------------------------
// Create a buffer holding all materials
//
void HelloVulkan::createMaterialBuffer(const std::vector<MatrialObj>& materials)
{
  {
    VkDeviceSize bufferSize = materials.size() * sizeof(MatrialObj);
    VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       m_matColorBuffer, m_matColorBufferMemory);
    void* data;
    vkMapMemory(VkCtx.getDevice(), m_matColorBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, materials.data(), bufferSize);
    vkUnmapMemory(VkCtx.getDevice(), m_matColorBufferMemory);
  }
}

//--------------------------------------------------------------------------------------------------
// Create a buffer with all vertices
//
void HelloVulkan::createVertexBuffer(const std::vector<Vertex>& vertex)
{
  VkDeviceSize bufferSize = sizeof(vertex[0]) * vertex.size();

  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(VkCtx.getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertex.data(), static_cast<size_t>(bufferSize));
  vkUnmapMemory(VkCtx.getDevice(), stagingBufferMemory);

  VkCtx.createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

  VkCtx.copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

  vkDestroyBuffer(VkCtx.getDevice(), stagingBuffer, nullptr);
  vkFreeMemory(VkCtx.getDevice(), stagingBufferMemory, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Create a buffer of the triangle indices
//
void HelloVulkan::createIndexBuffer(const std::vector<uint32_t>& indices)
{
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(VkCtx.getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
  vkUnmapMemory(VkCtx.getDevice(), stagingBufferMemory);

  VkCtx.createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

  VkCtx.copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

  vkDestroyBuffer(VkCtx.getDevice(), stagingBuffer, nullptr);
  vkFreeMemory(VkCtx.getDevice(), stagingBufferMemory, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Setting up the buffers in the descriptor set
//
void HelloVulkan::updateDescriptorSet()
{
  VkDescriptorSetLayout       layouts[] = {m_descriptorSetLayout};
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool              = VkCtx.getDescriptorPool();
  allocInfo.descriptorSetCount          = 1;
  allocInfo.pSetLayouts                 = layouts;

  if(vkAllocateDescriptorSets(VkCtx.getDevice(), &allocInfo, &m_descriptorSet) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor set!");
  }

  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer                 = m_uniformBuffer;
  bufferInfo.offset                 = 0;
  bufferInfo.range                  = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo matColorBufferInfo = {};
  matColorBufferInfo.buffer                 = m_matColorBuffer;
  matColorBufferInfo.offset                 = 0;
  matColorBufferInfo.range                  = VK_WHOLE_SIZE;

  std::vector<VkDescriptorImageInfo> imageInfos;
  for(size_t i = 0; i < m_textureSampler.size(); ++i)
  {
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView             = m_textureImageView[i];
    imageInfo.sampler               = m_textureSampler[i];
    imageInfos.push_back(imageInfo);
  }

  std::vector<VkWriteDescriptorSet> descriptorWrites = {};
  VkWriteDescriptorSet              wds              = {};
  wds.sType                                          = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  wds.dstSet                                         = m_descriptorSet;
  wds.dstArrayElement                                = 0;
  wds.descriptorCount                                = 1;
  // Matrices
  wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  wds.dstBinding     = 0;
  wds.pBufferInfo    = &bufferInfo;
  descriptorWrites.push_back(wds);
  // Materials
  wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  wds.dstBinding     = 1;
  wds.pBufferInfo    = &matColorBufferInfo;
  descriptorWrites.push_back(wds);
  // Textures
  wds.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  wds.dstBinding      = 2;
  wds.descriptorCount = static_cast<uint32_t>(imageInfos.size());
  wds.pImageInfo      = imageInfos.data();
  wds.pBufferInfo     = nullptr;
  descriptorWrites.push_back(wds);

  vkUpdateDescriptorSets(VkCtx.getDevice(), static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Creating the uniform buffer holding the matrices
//
void HelloVulkan::createUniformBuffer()
{
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  VkCtx.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     m_uniformBuffer, m_uniformBufferMemory);
}

//--------------------------------------------------------------------------------------------------
// Creating all textures and samplers
//
void HelloVulkan::createTextureImages(const std::vector<std::string>& textures)
{
  // If no textures are present, create a dummy one to accommodate the pipeline layout
  if(textures.empty())
  {
    int          texWidth = 1, texHeight = 1;
    int          texChannels = 4;
    glm::u8vec4* color       = new glm::u8vec4(255, 0, 255, 255);
    stbi_uc*     pixels      = reinterpret_cast<stbi_uc*>(color);

    VkImage        textureImage;
    VkDeviceMemory textureImageMemory;
    VkCtx.createTextureImage(pixels, texWidth, texHeight, texChannels, textureImage,
                             textureImageMemory);
    m_textureImage.push_back(textureImage);
    m_textureImageMemory.push_back(textureImageMemory);
    m_textureImageView.push_back(
        VkCtx.createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT));
    m_textureSampler.push_back(createTextureSampler());
    return;
  }


  for(const auto& texture : textures)
  {
    std::stringstream o;
    int               texWidth, texHeight, texChannels;
    o << "../media/textures/" << texture;

    stbi_uc* pixels =
        stbi_load(o.str().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if(!pixels)
    {
      texWidth = texHeight = 1;
      texChannels          = 4;
      glm::u8vec4* color   = new glm::u8vec4(255, 0, 255, 255);
      pixels               = reinterpret_cast<stbi_uc*>(color);
    }

    VkImage        textureImage;
    VkDeviceMemory textureImageMemory;
    VkCtx.createTextureImage(pixels, texWidth, texHeight, texChannels, textureImage,
                             textureImageMemory);
    m_textureImage.push_back(textureImage);
    m_textureImageMemory.push_back(textureImageMemory);
    stbi_image_free(pixels);

    m_textureImageView.push_back(
        VkCtx.createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT));
    m_textureSampler.push_back(createTextureSampler());
  }
}

//--------------------------------------------------------------------------------------------------
// Return standard sampler
//
VkSampler HelloVulkan::createTextureSampler()
{
  VkSamplerCreateInfo samplerInfo     = {};
  samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter               = VK_FILTER_LINEAR;
  samplerInfo.minFilter               = VK_FILTER_LINEAR;
  samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable        = VK_TRUE;
  samplerInfo.maxAnisotropy           = 16;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  VkSampler textureSampler;
  if(vkCreateSampler(VkCtx.getDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create texture sampler!");
  }
  return textureSampler;
}


void HelloVulkan::destroyResources()
{

  vkDestroyDescriptorSetLayout(VkCtx.getDevice(), m_descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(VkCtx.getDevice(), m_pipelineLayout, nullptr);
  vkDestroyPipeline(VkCtx.getDevice(), m_graphicsPipeline, nullptr);

  vkDestroyBuffer(VkCtx.getDevice(), m_vertexBuffer, nullptr);
  vkFreeMemory(VkCtx.getDevice(), m_vertexBufferMemory, nullptr);

  vkDestroyBuffer(VkCtx.getDevice(), m_indexBuffer, nullptr);
  vkFreeMemory(VkCtx.getDevice(), m_indexBufferMemory, nullptr);

  vkDestroyBuffer(VkCtx.getDevice(), m_uniformBuffer, nullptr);
  vkFreeMemory(VkCtx.getDevice(), m_uniformBufferMemory, nullptr);

  vkDestroyBuffer(VkCtx.getDevice(), m_matColorBuffer, nullptr);
  vkFreeMemory(VkCtx.getDevice(), m_matColorBufferMemory, nullptr);

  for(size_t i = 0; i < m_textureImage.size(); i++)
  {
    vkDestroySampler(VkCtx.getDevice(), m_textureSampler[i], nullptr);
    vkDestroyImageView(VkCtx.getDevice(), m_textureImageView[i], nullptr);
    vkDestroyImage(VkCtx.getDevice(), m_textureImage[i], nullptr);
    vkFreeMemory(VkCtx.getDevice(), m_textureImageMemory[i], nullptr);
  }
}
