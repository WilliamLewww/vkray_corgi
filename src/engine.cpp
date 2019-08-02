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
}

void Engine::initializePhysicalDevice() {

}

void Engine::initializeLogicalDevice() {

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