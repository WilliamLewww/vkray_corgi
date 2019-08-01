#include "engine.h"

const int SCREENWIDTH = 1000;
const int SCREENHEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const bool enableValidationLayers = true;
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    VK_NV_RAY_TRACING_EXTENSION_NAME
};

void Engine::initialize() {
    initializeWindow();
    initializeVulkan();
    initializePhysicalDevice();
    initializeLogicalDevice();
}

void Engine::initializeWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(SCREENWIDTH, SCREENHEIGHT, "vkray_corgi", nullptr, nullptr);
}

void Engine::initializeVulkan() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_1;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    extensions.push_back("VK_KHR_get_physical_device_properties2");
    if (enableValidationLayers) {
        extensions.push_back("VK_KHR_surface");
        extensions.push_back("VK_EXT_debug_report");
    }

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

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void Engine::initializePhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    physicalDevice = devices[0];

    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
            for (auto ext : extensions) {
                supportedExtensions.push_back(ext.extensionName);
            }
        }
    }
}

void Engine::initializeLogicalDevice() {
    queueFamilyIndex = -1;
    queuePresentIndex = -1;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (int x = 0; x < queueFamilies.size() && queueFamilyIndex == -1; x++) {
        if (queueFamilies[x].queueCount > 0 && queueFamilies[x].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndex = x;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, x, surface, &presentSupport);

        if (queueFamilies[x].queueCount > 0 && presentSupport) {
            queuePresentIndex = x;
        }
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndex, queuePresentIndex};
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

    std::cout << deviceExtensions[0] << ": " << extensionSupported(deviceExtensions[0]) << std::endl;
    std::cout << deviceExtensions[1] << ": " << extensionSupported(deviceExtensions[1]) << std::endl;
    std::cout << deviceExtensions[2] << ": " << extensionSupported(deviceExtensions[2]) << std::endl;


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
    vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, queuePresentIndex, 0, &presentQueue);
}

void Engine::start() {

}

void Engine::quit() {

}

bool Engine::extensionSupported(std::string extension) {
    return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());
}