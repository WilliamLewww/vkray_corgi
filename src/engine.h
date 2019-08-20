#define GLFW_INCLUDE_VULKAN

#pragma once
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#define VK_QUEUED_FRAMES 2
#define VK_MAX_POSSIBLE_BACK_BUFFERS 16

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

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
public:
	void initialize();
	void start();
	void quit();
};