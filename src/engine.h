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
	VkPresentModeKHR presentMode;

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
	VkImage backBuffer[VK_MAX_POSSIBLE_BACK_BUFFERS] = {};

	void initializeWindow();
	void initializeInstance();
	void initializePhysicalDevice();
	void initializeLogicalDevice();
	void initializeSurface();
	void initializeCommandBuffers();
	void initializeDescriptorPool();

	void initializeSwapchain();
	void initializeRenderPass();
public:
	void initialize();
	void start();
	void quit();
};