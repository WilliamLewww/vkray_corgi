#include "engine.h"

const int SCREENWIDTH = 1000;
const int SCREENHEIGHT = 600;

const std::vector<const char*> instanceExtensions = {
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_NV_RAY_TRACING_EXTENSION_NAME,
	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
};

const bool enableValidationLayers = true;
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

void Engine::initialize() {
	initializeWindow();
	initializeInstance();
	initializePhysicalDevice();
	initializeSurface();
	initializeLogicalDevice();
	initializeCommandBuffers();
	initializeDescriptorPool();
	initializeSwapchain();
}

void Engine::initializeWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(SCREENWIDTH, SCREENHEIGHT, "vkray_corgi", nullptr, nullptr);
}

void Engine::initializeInstance() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	for (int x = 0; x < instanceExtensions.size(); x++) {
		extensions.push_back(instanceExtensions[x]);
	}

	if (enableValidationLayers) {
		extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void Engine::initializePhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	physicalDevice = devices[0];
}

void Engine::initializeLogicalDevice() {
	graphicsQueueIndex = -1;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (int x = 0; x < queueFamilies.size(); x++) {
		if (queueFamilies[x].queueCount > 0 && graphicsQueueIndex == -1 && queueFamilies[x].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsQueueIndex = x;
		}

		VkBool32 presentSupport = false;
    	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, x, surface, &presentSupport);
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::vector<uint32_t> uniqueQueueFamilies = {graphicsQueueIndex};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledLayerCount = 0;

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}
	vkGetDeviceQueue(logicalDevice, graphicsQueueIndex, 0, &graphicsQueue);
}

void Engine::initializeSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

	if (formatCount == 1) {
		if (formats[0].format == VK_FORMAT_UNDEFINED) {
			surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
			surfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		}
		else {
			surfaceFormat = formats[0];
		}
	}
	else {
		VkFormat requestSurfaceImageFormat[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
		VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		bool requestedFound = false;
		for (size_t i = 0; i < sizeof(requestSurfaceImageFormat) / sizeof(requestSurfaceImageFormat[0]); i++) {
			if (requestedFound) {
				break;
			}
			for (uint32_t j = 0; j < formatCount; j++) {
				if(formats[j].format == requestSurfaceImageFormat[i] && formats[j].colorSpace == requestSurfaceColorSpace) {
					surfaceFormat = formats[j];
					requestedFound  = true;
				}
			}
		}

		if (!requestedFound) {
			surfaceFormat = formats[0];
		}
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

	presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& presentModeArg : presentModes) {
		if (presentModeArg == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = presentModeArg;
			break;
		}

		if (presentModeArg == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			presentMode = presentModeArg;
		}
	}
}

void Engine::initializeCommandBuffers() {
	for(int i = 0; i < VK_QUEUED_FRAMES; i++) {
		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = graphicsQueueIndex;
		if (vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, nullptr, &commandPool[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = commandPool[i];
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = 1;
		if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &commandBuffer[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command buffers!");
		}

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		if (vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fence[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create fence!");
		}

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphore!");
		}
		if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphore!");
		}
	}
}

void Engine::initializeDescriptorPool() {
	VkDescriptorPoolSize poolSize[] = {
		//{VK_DESCRIPTOR_TYPE_SAMPLER, 0},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		//{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0},
		//{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0},
		//{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0},
		//{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 0},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		//{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0},
		//{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 0},
		//{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0}
	};
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000;
	poolInfo.poolSizeCount = _countof(poolSize);
	poolInfo.pPoolSizes = poolSize;
	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void Engine::initializeSwapchain() {
	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.imageFormat = surfaceFormat.format;
	info.imageColorSpace = surfaceFormat.colorSpace;
	info.imageArrayLayers = 1;
	info.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	info.clipped = VK_TRUE;
	info.oldSwapchain = nullptr;
	VkSurfaceCapabilitiesKHR cap;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &cap);

	if(cap.maxImageCount > 0) {
		info.minImageCount = (cap.minImageCount + 2 < cap.maxImageCount) ? (cap.minImageCount + 2) : cap.maxImageCount;
	}
	else {
		info.minImageCount = cap.minImageCount + 2;
	}

	info.minImageCount = 2;

	if(cap.currentExtent.width == 0xffffffff) {
		frameBufferWidth = SCREENWIDTH;
		frameBufferHeight = SCREENHEIGHT;
		info.imageExtent.width = frameBufferWidth;
		info.imageExtent.height = frameBufferHeight;
	}
	else {
		frameBufferWidth = cap.currentExtent.width;
		frameBufferHeight = cap.currentExtent.height;
		info.imageExtent.width = frameBufferWidth;
		info.imageExtent.height = frameBufferHeight;
	}

	if (vkCreateSwapchainKHR(logicalDevice, &info, nullptr, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swapchain!");
	}
	if (vkGetSwapchainImagesKHR(logicalDevice, swapchain, &backBufferCount, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("failed to get swapchain images!");
	}
	if (vkGetSwapchainImagesKHR(logicalDevice, swapchain, &backBufferCount, backBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to get swapchain images!");
	}
}

void Engine::start() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void Engine::quit() {

}