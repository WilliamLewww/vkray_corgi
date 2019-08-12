#pragma once
//////////////////////////////////////////////////////////////////////////

#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "obj_loader.h"

// #VKRay
#include "nv_helpers_vk/ShaderBindingTableGenerator.h"
#include "nv_helpers_vk/RaytracingPipelineGenerator.h"
#include "nv_helpers_vk/DescriptorSetGenerator.h"
#include "nv_helpers_vk/BottomLevelASGenerator.h"
#include "nv_helpers_vk/TopLevelASGenerator.h"
#include "nv_helpers_vk/VKHelpers.h"

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

struct GeometryInstance
{
	VkBuffer vertexBuffer;
	uint32_t vertexCount;
	VkDeviceSize vertexOffset;
	VkBuffer indexBuffer;
	uint32_t indexCount;
	VkDeviceSize indexOffset;
	glm::mat4x4 transform;
};

struct AccelerationStructure
{
	VkBuffer scratchBuffer = VK_NULL_HANDLE;
	VkDeviceMemory scratchMem = VK_NULL_HANDLE;
	VkBuffer resultBuffer = VK_NULL_HANDLE;
	VkDeviceMemory resultMem = VK_NULL_HANDLE;
	VkBuffer instancesBuffer = VK_NULL_HANDLE;
	VkDeviceMemory instancesMem = VK_NULL_HANDLE;
	VkAccelerationStructureNV structure = VK_NULL_HANDLE;
};

//--------------------------------------------------------------------------------------------------
//
//
class HelloVulkan
{
public:
	VkPhysicalDeviceRayTracingPropertiesNV m_raytracingProperties = {};
	std::vector<GeometryInstance> m_geometryInstances;
	nv_helpers_vk::TopLevelASGenerator m_topLevelASGenerator;
	AccelerationStructure m_topLevelAS;
	std::vector<AccelerationStructure> m_bottomLevelAS;
	nv_helpers_vk::DescriptorSetGenerator m_rtDSG;
	VkDescriptorPool m_rtDescriptorPool;
	VkDescriptorSetLayout m_rtDescriptorSetLayout;
	VkDescriptorSet m_rtDescriptorSet;
	VkPipelineLayout m_rtPipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_rtPipeline = VK_NULL_HANDLE;
	nv_helpers_vk::ShaderBindingTableGenerator m_sbtGen;
	VkBuffer m_shaderBindingTableBuffer;
	VkDeviceMemory m_shaderBindingTableMem;

	uint32_t m_rayGenIndex;
	uint32_t m_hitGroupIndex;
	uint32_t m_missIndex;

	void initRayTracing();
	void createGeometryInstances();

	AccelerationStructure createBottomLevelAS(VkCommandBuffer commandBuffer, std::vector<GeometryInstance> vVertexBuffers);
	void createTopLevelAS(VkCommandBuffer commandBuffer, const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4x4>>& instances, VkBool32 updateOnly);

	void createAccelerationStructures();
	void destroyAccelerationStructure(const AccelerationStructure& as);

	void createRaytracingDescriptorSet();
	void updateRaytracingRenderTarget(VkImageView target);

	void createRaytracingPipeline();

	void createShaderBindingTable();

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
