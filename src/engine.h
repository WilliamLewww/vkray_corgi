#define GLFW_INCLUDE_VULKAN

#pragma once
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#define VK_QUEUED_FRAMES 2

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

	void initializeWindow();
	void initializeInstance();
	void initializePhysicalDevice();
	void initializeLogicalDevice();
	void initializeSurface();
	void initializeCommandBuffers();

	bool checkDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& extensions);
public:
	void initialize();
	void start();
	void quit();
};