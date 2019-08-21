#define GLFW_INCLUDE_VULKAN

#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <sstream>

#include "obj_loader.h"

#define VK_QUEUED_FRAMES 2
#define VK_MAX_POSSIBLE_BACK_BUFFERS 16

struct Vertex {
	glm::vec3 pos;
	glm::vec3 nrm;
	glm::vec3 color;
	glm::vec2 texCoord;
	int matID = 0;

	static auto getBindingDescription();
	static auto getAttributeDescriptions();
};

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 modelIT;
	
	// #VKRay
	glm::mat4 viewInverse;
	glm::mat4 projInverse;
};

class Engine {
private:
	GLFWwindow* window;
	VkSurfaceKHR surface;

	VkInstance instance;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

	uint32_t graphicsQueueIndex;
	VkQueue graphicsQueue;

	VkSurfaceFormatKHR surfaceFormat;
	VkImageSubresourceRange imageRange;
	VkPresentModeKHR presentMode;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

  	VkCommandPool commandPool[VK_QUEUED_FRAMES];
	VkCommandBuffer commandBuffer[VK_QUEUED_FRAMES];
  	VkFence fence[VK_QUEUED_FRAMES];
	VkSemaphore presentCompleteSemaphore[VK_QUEUED_FRAMES];
	VkSemaphore renderCompleteSemaphore[VK_QUEUED_FRAMES];

	VkDescriptorPool descriptorPool;

	VkSwapchainKHR swapchain;
	VkRenderPass renderPass;

	int frameBufferWidth = 0; 
	int frameBufferHeight = 0;
	uint32_t backBufferCount = 0;
	VkImage backBuffer[VK_MAX_POSSIBLE_BACK_BUFFERS];
	VkImageView backBufferView[VK_MAX_POSSIBLE_BACK_BUFFERS];
	VkFramebuffer framebuffer[VK_MAX_POSSIBLE_BACK_BUFFERS];

	VkDescriptorSetLayout descriptorSetLayout;

	uint32_t indexCount;
	uint32_t vertexCount;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

	VkBuffer matColorBuffer;
	VkDeviceMemory matColorBufferMemory;

	std::vector<VkImage> textureImageList;
	std::vector<VkDeviceMemory> textureImageMemoryList;
	std::vector<VkImageView> textureImageViewList;
	std::vector<VkSampler> textureSamplerList;

	void initializeWindow();
	void initializeInstance();
	void initializePhysicalDevice();
	void initializeLogicalDevice();
	void initializeSurface();
	void initializeCommandBuffers();
	void initializeDescriptorPool();

	void initializeSwapchain();
	void initializeRenderPass();
	void initializeImageViews();
	void initializeDepthResources();
	void initializeFrameBuffer();

	void initializeModel(const std::string& filename);
	void initializeVertexBuffer(const std::vector<Vertex>& vertex);
	void initializeIndexBuffer(const std::vector<uint32_t>& indices);
	void initializeMaterialBuffer(const std::vector<MatrialObj>& materials);
	void initializeTextureImages(const std::vector<std::string>& textures);

	void initializeDescriptorSetLayout();
	void initializeUniformBuffer();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void createTextureImage(uint8_t* pixels, int texWidth, int texHeight, int texChannels, VkImage& textureImage, VkDeviceMemory& textureImageMemory);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	VkSampler createTextureSampler();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
public:
	void initialize();
	void start();
	void quit();
};