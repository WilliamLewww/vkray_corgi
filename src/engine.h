#define GLFW_INCLUDE_VULKAN

#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <fstream>
#include <vector>
#include <set>
#include <chrono>
#include <array>
#include <unordered_map>
#include <iostream>

#include "nv_helpers_vk/RaytracingPipelineGenerator.h"
#include "nv_helpers_vk/DescriptorSetGenerator.h"
#include "nv_helpers_vk/BottomLevelASGenerator.h"
#include "nv_helpers_vk/TopLevelASGenerator.h"
#include "nv_helpers_vk/VKHelpers.h"

struct GeometryInstance {
	VkBuffer vertexBuffer;
	uint32_t vertexCount;
	VkDeviceSize vertexOffset;
	VkBuffer indexBuffer;
	uint32_t indexCount;
	VkDeviceSize indexOffset;
	glm::mat4x4 transform;
};

struct AccelerationStructure {
	VkBuffer scratchBuffer = VK_NULL_HANDLE;
	VkDeviceMemory scratchMem = VK_NULL_HANDLE;
	VkBuffer resultBuffer = VK_NULL_HANDLE;
	VkDeviceMemory resultMem = VK_NULL_HANDLE;
	VkBuffer instancesBuffer = VK_NULL_HANDLE;
	VkDeviceMemory instancesMem = VK_NULL_HANDLE;
	VkAccelerationStructureNV structure = VK_NULL_HANDLE;
};

struct Camera {
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	float pitch;
	float yaw;
};

struct CoordinateObject {
    alignas(16) glm::mat4 modelMatrix;
    alignas(16) glm::mat4 viewMatrix;
    alignas(16) glm::mat4 projectionMatrix;
};

struct LightObject {
    alignas(16) glm::vec3 lightPosition;
    alignas(16) glm::vec3 lightColor;

    alignas(16) glm::vec3 viewPosition;
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 textureCoordinate;

	static std::vector<VkVertexInputBindingDescription> getBindingDescription();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

	bool operator==(const Vertex& other) const;
};

class Engine {
private:
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

	uint32_t m_rayGenIndex;
	uint32_t m_hitGroupIndex;
	uint32_t m_missIndex;

	void initializeRayTracing();
	void initializeGeometryInstances();
	void initializeAccelerationStructures();
	void initializeRaytracingDescriptorSet();
	void initializeRaytracingPipeline();

	void updateRaytracingRenderTarget(VkImageView target);
	void destroyAccelerationStructure(const AccelerationStructure& as);

	AccelerationStructure createBottomLevelAS(VkCommandBuffer commandBuffer, std::vector<GeometryInstance> vVertexBuffers);
	void createTopLevelAS(VkCommandBuffer commandBuffer, const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4x4>>& instances, VkBool32 updateOnly);

	Camera camera;
	CoordinateObject coordinateObject;
	LightObject lightObject;

	float currentModelRotation = 0.01f;

	GLFWwindow* window;
	VkSurfaceKHR surface;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

    uint32_t graphicsQueueFamilyIndex;
    uint32_t computeQueueFamilyIndex;
    uint32_t transferQueueFamilyIndex;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue transferQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<Vertex> vertexList;
	std::vector<uint32_t> indexList;
	
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> coordinateObjectBuffer;
	std::vector<VkDeviceMemory> coordinateObjectBufferMemory;

	std::vector<VkBuffer> lightObjectBuffer;
	std::vector<VkDeviceMemory> lightObjectBufferMemory;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	size_t currentFrame = 0;

	void initializeWindow();
	void initializeVulkan();
	void initializePhysicalDevice();
	void initializeLogicalDevice();
	void initializeSwapChain();
	void initializeImageViews();
	void initializeRenderPass();
	void initializeDescriptorSetLayout();
	void initializeGraphicsPipeline();
	void initializeCommandPool();
	void initializeDepthResources();
	void initializeFramebuffers();

	void initializeTextureImage();
	void initializeTextureImageView();
	void initializeTextureSampler();

	void initializeModel();
	void initializeVertexBuffer();
	void initializeIndexBuffer();
	void initializeUniformBuffers();

	void initializeDescriptorPool();
	void initializeDescriptorSets();

	void initializeCommandBuffer();
	void initializeSyncObjects();

	void render();
	void updateUniformBuffer(uint32_t currentImage);

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	std::vector<char> readFile(const std::string& filepath);
	VkShaderModule createShaderModule(const std::vector<char>& code);
	
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
public:
	void initialize();
	void start();
	void quit();
};
