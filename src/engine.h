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
	GLFWwindow* window;
	VkSurfaceKHR surface;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

	uint32_t queueFamilyIndex;
	uint32_t queuePresentIndex;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

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
