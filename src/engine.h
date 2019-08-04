#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS

#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <iostream>
#include <vector>
#include <set>

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

class Engine {
private:
	GLFWwindow* window;
	VkInstance instance = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkRenderPass renderPass = VK_NULL_HANDLE;

	uint32_t queueFamily = 0;
	VkQueue queue = VK_NULL_HANDLE;
  	VkSurfaceFormatKHR surfaceFormat = {};
  	VkPresentModeKHR presentMode = {};
  	VkImageSubresourceRange imageRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  	VkCommandPool commandPool[MAX_FRAMES_IN_FLIGHT];
  	VkCommandBuffer commandBuffer[MAX_FRAMES_IN_FLIGHT];
	VkFence fence[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore presentCompleteSemaphore[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderCompleteSemaphore[MAX_FRAMES_IN_FLIGHT];

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

	int framebufferWidth = 0, framebufferHeight = 0;
	uint32_t backBufferCount = 0;
	VkImage backBuffer[MAX_POSSIBLE_BACK_BUFFERS] = {};
	VkImageView backBufferView[MAX_POSSIBLE_BACK_BUFFERS] = {};
	VkFramebuffer framebuffer[MAX_POSSIBLE_BACK_BUFFERS] = {};

	VkImage depthImage = {};
	VkDeviceMemory depthImageMemory = {};
	VkImageView depthImageView = {};

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
public:
	void initialize();
	void start();
	void quit();
};
