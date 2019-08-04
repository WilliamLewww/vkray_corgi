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

	uint32_t queueFamily = 0;
	VkQueue queue = VK_NULL_HANDLE;
  	VkSurfaceFormatKHR surfaceFormat = {};
  	VkPresentModeKHR presentMode = {};

  	VkCommandPool commandPool[MAX_FRAMES_IN_FLIGHT];
  	VkCommandBuffer commandBuffer[MAX_FRAMES_IN_FLIGHT];
	VkFence fence[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore presentCompleteSemaphore[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderCompleteSemaphore[MAX_FRAMES_IN_FLIGHT];

	void initializeWindow();
	void initializeVulkan();
	void initializePhysicalDevice(const std::vector<const char*>& extensions);
	void initializeQueueFamilly();
	void initializeSurfaceFormat();
	void initializePresentMode();
	void initializeLogicalDevice(const std::vector<const char*>& extensions);
	void initializeCommandBuffers();

	void renderFrame();

	bool isDeviceSuitable(const VkPhysicalDevice& device, const std::vector<const char*>& extensions);
	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device);
	bool checkDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& extensions);
	SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device);

public:
	void initialize();
	void start();
	void quit();
};
