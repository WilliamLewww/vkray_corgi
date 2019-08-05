#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS

#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <set>

#include "nv_helpers_vk/RaytracingPipelineGenerator.h"
#include "nv_helpers_vk/DescriptorSetGenerator.h"
#include "nv_helpers_vk/BottomLevelASGenerator.h"
#include "nv_helpers_vk/TopLevelASGenerator.h"
#include "nv_helpers_vk/VKHelpers.h"

struct Vertex {
	glm::vec2 pos;

	static auto getBindingDescription();
	static auto getAttributeDescriptions();
};

const int MAX_FRAMES_IN_FLIGHT = 2;
const int MAX_POSSIBLE_BACK_BUFFERS = 16;

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool isComplete() { return graphicsFamily >= 0 && presentFamily >= 0; }
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

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

class Engine {
private:
	GLFWwindow* window;
	VkAllocationCallbacks* allocator = VK_NULL_HANDLE;
	VkInstance instance = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkPhysicalDeviceRayTracingPropertiesNV raytracingProperties = {};
	std::vector<GeometryInstance> geometryInstances;
	nv_helpers_vk::TopLevelASGenerator topLevelASGenerator;
	AccelerationStructure topLevelAS;
	std::vector<AccelerationStructure> bottomLevelAS;
	nv_helpers_vk::DescriptorSetGenerator rtDSG;
	VkDescriptorPool rtDescriptorPool;
	VkDescriptorSetLayout rtDescriptorSetLayout;
	VkDescriptorSet rtDescriptorSet;
	VkPipelineLayout rtPipelineLayout = VK_NULL_HANDLE;
	VkPipeline rtPipeline = VK_NULL_HANDLE;

	uint32_t rayGenIndex;
	uint32_t hitGroupIndex;
	uint32_t missIndex;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkRenderPass renderPass = VK_NULL_HANDLE;

	uint32_t queueFamily = 0;
	VkQueue queue = VK_NULL_HANDLE;
  	VkSurfaceFormatKHR surfaceFormat = {};
  	VkPresentModeKHR presentMode = {};
  	VkImageSubresourceRange imageRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  	uint32_t frameIndex = 0;
  	VkCommandPool commandPool[MAX_FRAMES_IN_FLIGHT];
  	VkCommandBuffer commandBuffer[MAX_FRAMES_IN_FLIGHT];
	VkFence fence[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore presentCompleteSemaphore[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderCompleteSemaphore[MAX_FRAMES_IN_FLIGHT];

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

	VkExtent2D framebufferSize;
	int framebufferWidth = 0, framebufferHeight = 0;
	uint32_t backBufferCount = 0;
	VkImage backBuffer[MAX_POSSIBLE_BACK_BUFFERS] = {};
	VkImageView backBufferView[MAX_POSSIBLE_BACK_BUFFERS] = {};
	VkFramebuffer framebuffer[MAX_POSSIBLE_BACK_BUFFERS] = {};

	VkImage depthImage = {};
	VkDeviceMemory depthImageMemory = {};
	VkImageView depthImageView = {};

	uint32_t nbIndices;
	uint32_t nbVertices;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void initializeWindow();
	void initializeVulkan();
	void initializePhysicalDevice(const std::vector<const char*>& extensions);
	void initializeQueueFamilly();
	void initializeSurfaceFormat();
	void initializePresentMode();
	void initializeLogicalDevice(const std::vector<const char*>& extensions);
	void initializeCommandBuffers();
	void initializeDescriptorPool();

	void initializeSwapchain(VkSwapchainKHR swapchainArg, int width, int height);
	void initializeRenderPass();
	void initializeImageViews();
	void initializeDepthResources();
	void initializeFramebuffer();

	void initializeVertexBuffer(const std::vector<Vertex>& vertex);
	void initializeIndexBuffer(const std::vector<uint32_t>& indices);

	void initializeRayTracing();
	void initializeGeometryInstances();
	void initializeAccelerationStructures();
	void initializeRaytracingDescriptorSet();
	void initializeRaytracingPipeline();

	AccelerationStructure createBottomLevelAS(VkCommandBuffer commandBuffer, std::vector<GeometryInstance> vVertexBuffers);
	void createTopLevelAS(VkCommandBuffer commandBuffer, const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4x4>>& instances, VkBool32 updateOnly);
	void destroyAccelerationStructure(const AccelerationStructure& as);

	void updateRaytracingRenderTarget(VkImageView target);

	void renderFrame();

	bool isDeviceSuitable(const VkPhysicalDevice& device, const std::vector<const char*>& extensions);
	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device);
	bool checkDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& extensions);
	SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device);

	VkFormat findDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	bool hasStencilComponent(VkFormat format);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
public:
	void initialize();
	void start();
	void quit();
};
