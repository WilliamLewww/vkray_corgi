#include "engine.h"

const int SCREENWIDTH = 1000;
const int SCREENHEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const bool enableValidationLayers = true;

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    VK_NV_RAY_TRACING_EXTENSION_NAME
};

void Engine::initialize() {
    initializeWindow();
    initializeVulkan();
    initializePhysicalDevice(deviceExtensions);
    initializeQueueFamilly();
    initializeSurfaceFormat();
    initializeLogicalDevice(deviceExtensions);
}

void Engine::initializeWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(SCREENWIDTH, SCREENHEIGHT, "vkray_corgi", nullptr, nullptr);
}

void Engine::initializeVulkan() {
    uint32_t extensions_count;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    std::vector<const char*> instanceExtensions;
    instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    for(uint32_t i = 0; i < extensions_count; i++) {
        instanceExtensions.push_back(glfw_extensions[i]);
    }

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    if(enableValidationLayers) {
        const char* layers[] = {"VK_LAYER_LUNARG_standard_validation"};
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
    }
    else {
        create_info.enabledLayerCount = 0;
    }
    instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    create_info.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    create_info.ppEnabledExtensionNames = instanceExtensions.data();

    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Could not create Vulkan instance");
    }

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Could not create window surface");
    }
}

void Engine::initializePhysicalDevice(const std::vector<const char*>& extensions) {
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device, extensions)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void Engine::initializeQueueFamilly() {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queues(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queues.data());
    for (uint32_t i = 0; i < count; i++) {
        if(queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamily = i;
            break;
        }
    }
}

void Engine::initializeSurfaceFormat() {
    uint32_t count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, formats.data());

    if (count == 1) {
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
            for (uint32_t j = 0; j < count; j++) {
                if (formats[j].format == requestSurfaceImageFormat[i] && formats[j].colorSpace == requestSurfaceColorSpace) {
                    surfaceFormat = formats[j];
                    requestedFound  = true;
                }
            }
        }

        if (!requestedFound) {
            surfaceFormat = formats[0];
        }
    }
}

void Engine::initializeLogicalDevice(const std::vector<const char*>& extensions) {

}

void Engine::renderFrame() {

}

void Engine::start() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderFrame();
    }
}

void Engine::quit() {

}

bool Engine::isDeviceSuitable(const VkPhysicalDevice& device, const std::vector<const char*>& extensions) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device, extensions);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices Engine::findQueueFamilies(const VkPhysicalDevice& device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
        }

        if(indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool Engine::checkDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& extensions) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

    for(const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Engine::querySwapChainSupport(const VkPhysicalDevice& device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if(formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if(presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}