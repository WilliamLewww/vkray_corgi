#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS

#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <iostream>
#include <vector>
#include <set>

class Engine {
private:
	GLFWwindow* window;
	VkInstance instance;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice;
	std::vector<std::string> supportedExtensions;

	VkDevice logicalDevice;

	uint32_t queueFamilyIndex;
	uint32_t queuePresentIndex;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	void initializeWindow();
	void initializeVulkan();
	void initializePhysicalDevice();
	void initializeLogicalDevice();

	bool extensionSupported(std::string extension);
public:
	void initialize();
	void start();
	void quit();
};
