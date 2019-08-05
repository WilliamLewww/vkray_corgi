#include "engine.h"

const int SCREENWIDTH = 1000;
const int SCREENHEIGHT = 600;

const bool enableValidationLayers = true;

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    VK_NV_RAY_TRACING_EXTENSION_NAME
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}},
    {{0.5f, -0.5f}}, 
    {{0.5f, 0.5f}},
    {{-0.5f, 0.5f}}
};

const std::vector<uint32_t> indices = {
    0, 1, 2, 2, 3, 0
};

void Engine::initialize() {
    initializeWindow();
    initializeVulkan();
    initializePhysicalDevice(deviceExtensions);
    initializeQueueFamilly();
    initializeSurfaceFormat();
    initializePresentMode();
    initializeLogicalDevice(deviceExtensions);
    initializeCommandBuffers();
    initializeDescriptorPool();

    initializeSwapchain(swapchain, SCREENHEIGHT, SCREENHEIGHT);
    initializeRenderPass();
    initializeImageViews();
    initializeDepthResources();
    initializeFramebuffer();

    nbIndices = static_cast<uint32_t>(indices.size());
    nbVertices = static_cast<uint32_t>(vertices.size());
    initializeVertexBuffer(vertices);
    initializeIndexBuffer(indices);
    initializeRayTracing();
    initializeGeometryInstances();
    initializeAccelerationStructures();
    initializeRaytracingDescriptorSet();
    initializeRaytracingPipeline();
    initializeShaderBindingTable();
}

void Engine::initializeVertexBuffer(const std::vector<Vertex>& vertex) {
    VkDeviceSize bufferSize = sizeof(vertex[0]) * vertex.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertex.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(logicalDevice, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}

void Engine::initializeIndexBuffer(const std::vector<uint32_t>& indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(logicalDevice, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}

void Engine::initializeRayTracing() {
    raytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
    raytracingProperties.pNext = nullptr;
    raytracingProperties.maxRecursionDepth = 0;
    raytracingProperties.shaderGroupHandleSize = 0;
    VkPhysicalDeviceProperties2 props;
    props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props.pNext = &raytracingProperties;
    props.properties = {};
    vkGetPhysicalDeviceProperties2(physicalDevice, &props);
}

void Engine::initializeGeometryInstances() {
    glm::mat4x4 mat = glm::mat4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
    geometryInstances.push_back({vertexBuffer, nbVertices, 0, indexBuffer, nbIndices, 0, mat});
}

AccelerationStructure Engine::createBottomLevelAS(VkCommandBuffer commandBuffer, std::vector<GeometryInstance> vVertexBuffers) {
    nv_helpers_vk::BottomLevelASGenerator bottomLevelAS;

    for(const auto& buffer : vVertexBuffers) {
        if(buffer.indexBuffer == VK_NULL_HANDLE) {
            bottomLevelAS.AddVertexBuffer(buffer.vertexBuffer, buffer.vertexOffset, buffer.vertexCount,
            sizeof(Vertex), VK_NULL_HANDLE, 0);
        }
        else {
            bottomLevelAS.AddVertexBuffer(buffer.vertexBuffer, buffer.vertexOffset, buffer.vertexCount,
            sizeof(Vertex), buffer.indexBuffer, buffer.indexOffset,
            buffer.indexCount, VK_NULL_HANDLE, 0);
        }
    }

    AccelerationStructure buffers;

    buffers.structure = bottomLevelAS.CreateAccelerationStructure(logicalDevice, VK_FALSE);

    VkDeviceSize scratchSizeInBytes = 0;
    VkDeviceSize resultSizeInBytes = 0;
    bottomLevelAS.ComputeASBufferSizes(logicalDevice, buffers.structure, &scratchSizeInBytes, &resultSizeInBytes);

    nv_helpers_vk::createBuffer(physicalDevice, logicalDevice, scratchSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, &buffers.scratchBuffer, &buffers.scratchMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    nv_helpers_vk::createBuffer(physicalDevice, logicalDevice, resultSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, &buffers.resultBuffer, &buffers.resultMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    bottomLevelAS.Generate(logicalDevice, commandBuffer, buffers.structure, buffers.scratchBuffer, 0, buffers.resultBuffer, buffers.resultMem, false, VK_NULL_HANDLE);

    return buffers;
}

void Engine::createTopLevelAS(VkCommandBuffer commandBuffer, const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4x4>>& instances, VkBool32 updateOnly) {
    if(!updateOnly) {
        for(size_t i = 0; i < instances.size(); i++) {
            topLevelASGenerator.AddInstance(instances[i].first, instances[i].second, static_cast<uint32_t>(i), static_cast<uint32_t>(i));
        }

        topLevelAS.structure = topLevelASGenerator.CreateAccelerationStructure(logicalDevice, VK_TRUE);

        VkDeviceSize scratchSizeInBytes, resultSizeInBytes, instanceDescsSizeInBytes;
        topLevelASGenerator.ComputeASBufferSizes(logicalDevice, topLevelAS.structure, &scratchSizeInBytes, &resultSizeInBytes, &instanceDescsSizeInBytes);

        nv_helpers_vk::createBuffer(physicalDevice, logicalDevice, scratchSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, &topLevelAS.scratchBuffer, &topLevelAS.scratchMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        nv_helpers_vk::createBuffer(physicalDevice, logicalDevice, resultSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, &topLevelAS.resultBuffer, &topLevelAS.resultMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        nv_helpers_vk::createBuffer(physicalDevice, logicalDevice, instanceDescsSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, &topLevelAS.instancesBuffer, &topLevelAS.instancesMem, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); 
    }

    topLevelASGenerator.Generate(logicalDevice, commandBuffer, topLevelAS.structure, topLevelAS.scratchBuffer, 0, topLevelAS.resultBuffer, topLevelAS.resultMem, topLevelAS.instancesBuffer, topLevelAS.instancesMem, updateOnly, updateOnly ? topLevelAS.structure : VK_NULL_HANDLE);
}

void Engine::initializeAccelerationStructures() {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool[frameIndex];
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkResult code =
    vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &commandBuffer);
    if(code != VK_SUCCESS) {
        throw std::logic_error("rt vkAllocateCommandBuffers failed");
    }

    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    bottomLevelAS.resize(geometryInstances.size());

    std::vector<std::pair<VkAccelerationStructureNV, glm::mat4x4>> instances;

    for(size_t i = 0; i < geometryInstances.size(); i++) {
        bottomLevelAS[i] = createBottomLevelAS(commandBuffer, {
            {geometryInstances[i].vertexBuffer, geometryInstances[i].vertexCount,
            geometryInstances[i].vertexOffset, geometryInstances[i].indexBuffer,
            geometryInstances[i].indexCount, geometryInstances[i].indexOffset},
        });
        instances.push_back({bottomLevelAS[i].structure, geometryInstances[i].transform});
    }

    createTopLevelAS(commandBuffer, instances, VK_FALSE);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(logicalDevice, commandPool[frameIndex], 1, &commandBuffer);
}

void Engine::destroyAccelerationStructure(const AccelerationStructure& accelerationStructure) {
    vkDestroyBuffer(logicalDevice, accelerationStructure.scratchBuffer, nullptr);
    vkFreeMemory(logicalDevice, accelerationStructure.scratchMem, nullptr);
    vkDestroyBuffer(logicalDevice, accelerationStructure.resultBuffer, nullptr);
    vkFreeMemory(logicalDevice, accelerationStructure.resultMem, nullptr);
    vkDestroyBuffer(logicalDevice, accelerationStructure.instancesBuffer, nullptr);
    vkFreeMemory(logicalDevice, accelerationStructure.instancesMem, nullptr);
    vkDestroyAccelerationStructureNV(logicalDevice, accelerationStructure.structure, nullptr);
}

void Engine::initializeRaytracingDescriptorSet() {
    VkBufferMemoryBarrier bmb = {};
    bmb.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bmb.pNext = nullptr;
    bmb.srcAccessMask = 0;
    bmb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    bmb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bmb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bmb.offset = 0;
    bmb.size = VK_WHOLE_SIZE;

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    bmb.buffer = vertexBuffer;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &bmb, 0, nullptr);
    bmb.buffer = indexBuffer;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &bmb, 0, nullptr);
    endSingleTimeCommands(commandBuffer);

    // Add the bindings to the resources
    // Top-level acceleration structure, usable by both the ray generation and the
    // closest hit (to shoot shadow rays)
    rtDSG.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, VK_SHADER_STAGE_RAYGEN_BIT_NV);
    // Raytracing output
    rtDSG.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_NV);
    // Scene data
    // Vertex buffer
    rtDSG.AddBinding(3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    // Index buffer
    rtDSG.AddBinding(4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    // Create the descriptor pool and layout
    rtDescriptorPool = rtDSG.GeneratePool(logicalDevice);
    rtDescriptorSetLayout = rtDSG.GenerateLayout(logicalDevice);

    // Generate the descriptor set
    rtDescriptorSet = rtDSG.GenerateSet(logicalDevice, rtDescriptorPool, rtDescriptorSetLayout);

    // Bind the actual resources into the descriptor set
    // Top-level acceleration structure
    VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo;
    descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    descriptorAccelerationStructureInfo.pNext = nullptr;
    descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
    descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.structure;

    rtDSG.Bind(rtDescriptorSet, 0, {descriptorAccelerationStructureInfo});

    // Vertex buffer
    VkDescriptorBufferInfo vertexInfo = {};
    vertexInfo.buffer = vertexBuffer;
    vertexInfo.offset = 0;
    vertexInfo.range = VK_WHOLE_SIZE;

    rtDSG.Bind(rtDescriptorSet, 3, {vertexInfo});

    // Index buffer
    VkDescriptorBufferInfo indexInfo = {};
    indexInfo.buffer = indexBuffer;
    indexInfo.offset = 0;
    indexInfo.range = VK_WHOLE_SIZE;

    rtDSG.Bind(rtDescriptorSet, 4, {indexInfo});
    rtDSG.UpdateSetContents(logicalDevice, rtDescriptorSet);
}

void Engine::updateRaytracingRenderTarget(VkImageView target) {
    VkDescriptorImageInfo descriptorOutputImageInfo;
    descriptorOutputImageInfo.sampler = nullptr;
    descriptorOutputImageInfo.imageView = target;
    descriptorOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    rtDSG.Bind(rtDescriptorSet, 1, {descriptorOutputImageInfo});
    rtDSG.UpdateSetContents(logicalDevice, rtDescriptorSet);
}

void Engine::initializeRaytracingPipeline() {
    nv_helpers_vk::RayTracingPipelineGenerator pipelineGen;
    VkShaderModule rayGenModule = createShaderModule(readFile("shaders/rgen.spv"));
    rayGenIndex = pipelineGen.AddRayGenShaderStage(rayGenModule);

    VkShaderModule missModule = createShaderModule(readFile("shaders/rmiss.spv"));
    missIndex = pipelineGen.AddMissShaderStage(missModule);

    hitGroupIndex = pipelineGen.StartHitGroup();

    VkShaderModule closestHitModule = createShaderModule(readFile("shaders/rchit.spv"));
    pipelineGen.AddHitShaderStage(closestHitModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    pipelineGen.EndHitGroup();

    pipelineGen.SetMaxRecursionDepth(1);

    pipelineGen.Generate(logicalDevice, rtDescriptorSetLayout, &rtPipeline, &rtPipelineLayout);

    vkDestroyShaderModule(logicalDevice, rayGenModule, nullptr);
    vkDestroyShaderModule(logicalDevice, missModule, nullptr);
    vkDestroyShaderModule(logicalDevice, closestHitModule, nullptr);
}

void Engine::initializeShaderBindingTable() {
    sbtGen.AddRayGenerationProgram(rayGenIndex, {});
    sbtGen.AddMissProgram(missIndex, {});

    sbtGen.AddHitGroup(hitGroupIndex, {});

    VkDeviceSize shaderBindingTableSize = sbtGen.ComputeSBTSize(raytracingProperties);

    nv_helpers_vk::createBuffer(physicalDevice, logicalDevice, shaderBindingTableSize,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &shaderBindingTableBuffer,
                              &shaderBindingTableMem, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    sbtGen.Generate(logicalDevice, rtPipeline, shaderBindingTableBuffer, shaderBindingTableMem);
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

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    if(enableValidationLayers) {
        const char* layers[] = {"VK_LAYER_LUNARG_standard_validation"};
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = layers;
    }
    else {
        createInfo.enabledLayerCount = 0;
    }
    instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    if (vkCreateInstance(&createInfo, allocator, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Could not create Vulkan instance");
    }

    if (glfwCreateWindowSurface(instance, window, allocator, &surface) != VK_SUCCESS) {
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

void Engine::initializePresentMode() {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> presentModes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, presentModes.data());

    presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(const auto& presentModeOption : presentModes) {
        if(presentModeOption == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = presentModeOption;
            break;
        }

        if(presentModeOption == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            presentMode = presentModeOption;
        }
    }
}

void Engine::initializeLogicalDevice(const std::vector<const char*>& extensions) {
    auto deviceExtensionCount = static_cast<uint32_t>(extensions.size());
    const uint32_t queueIndex = 0;
    const uint32_t queueCount = 1;
    const float queuePriority[] = {1.0f};
    VkDeviceQueueCreateInfo queueInfo[1] = {};
    queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo[0].queueFamilyIndex = queueFamily;
    queueInfo[0].queueCount = queueCount;
    queueInfo[0].pQueuePriorities = queuePriority;
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = sizeof(queueInfo) / sizeof(queueInfo[0]);
    createInfo.pQueueCreateInfos = queueInfo;
    createInfo.enabledExtensionCount = deviceExtensionCount;
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexFeatures = {};
    descIndexFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    VkPhysicalDeviceFeatures2 supportedFeatures = {};
    supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    supportedFeatures.pNext = &descIndexFeatures;
    vkGetPhysicalDeviceFeatures2(physicalDevice, &supportedFeatures);
    createInfo.pEnabledFeatures = &(supportedFeatures.features);
    createInfo.pNext = &descIndexFeatures;

    // createInfo.pNext = supportedFeatures.pNext;

    if (vkCreateDevice(physicalDevice, &createInfo, allocator, &logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create a logical connection");
    }

    vkGetDeviceQueue(logicalDevice, queueFamily, queueIndex, &queue);
}

void Engine::initializeCommandBuffers() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkCommandPoolCreateInfo commandPoolCreateInfo = {};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = queueFamily;
        if (vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, allocator, &commandPool[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create a command pool");
        }

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool[i];
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &commandBuffer[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate a command buffer");
        }

        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (vkCreateFence(logicalDevice, &fenceCreateInfo, allocator, &fence[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create a fence");
        }

        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, allocator, &presentCompleteSemaphore[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create a semaphore");
        }
        if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, allocator, &renderCompleteSemaphore[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create a semaphore");
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
    if (vkCreateDescriptorPool(logicalDevice, &poolInfo, allocator, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create a descriptor pool");
    }
}

void Engine::initializeSwapchain(VkSwapchainKHR swapchainArg, int width, int height) {
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
    info.presentMode = presentMode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = swapchainArg;
    VkSurfaceCapabilitiesKHR cap;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &cap) != VK_SUCCESS) {
        throw std::runtime_error("failed to find physical device surface capabilities");
    }

    if (cap.maxImageCount > 0) {
        info.minImageCount = (cap.minImageCount + 2 < cap.maxImageCount) ? (cap.minImageCount + 2) : cap.maxImageCount;
    }
    else {
        info.minImageCount = cap.minImageCount + 2;
    }

    info.minImageCount = 2;

    if (cap.currentExtent.width == 0xffffffff) {
        framebufferWidth = width;
        framebufferHeight = height;
        info.imageExtent.width = framebufferWidth;
        info.imageExtent.height = framebufferWidth;
    }
    else {
        framebufferWidth = cap.currentExtent.width;
        framebufferHeight = cap.currentExtent.height;
        info.imageExtent.width = framebufferWidth;
        info.imageExtent.height = framebufferHeight;
    }

    if (vkCreateSwapchainKHR(logicalDevice, &info, allocator, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create a swapchain");
    }
    vkGetSwapchainImagesKHR(logicalDevice, swapchain, &backBufferCount, nullptr);
    if (vkGetSwapchainImagesKHR(logicalDevice, swapchain, &backBufferCount, backBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to get swapchain images");
    }
}

void Engine::initializeRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpasses[2];
    subpasses[0] = {};
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = &color_attachment;
    subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;

    subpasses[1] = {};
    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &color_attachment;
    subpasses[1].pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkSubpassDependency> dependencies(2);

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = 1; // VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    std::vector<VkAttachmentDescription> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 2;
    renderPassInfo.pSubpasses = subpasses;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies.data();
    if (vkCreateRenderPass(logicalDevice, &renderPassInfo, allocator, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create a render pass");
    }
}

void Engine::initializeImageViews() {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = surfaceFormat.format;
    info.components.r = VK_COMPONENT_SWIZZLE_R;
    info.components.g = VK_COMPONENT_SWIZZLE_G;
    info.components.b = VK_COMPONENT_SWIZZLE_B;
    info.components.a = VK_COMPONENT_SWIZZLE_A;
    info.subresourceRange = imageRange;

    for (uint32_t i = 0; i < backBufferCount; i++) {
        info.image = backBuffer[i];
        if (vkCreateImageView(logicalDevice, &info, allocator, &backBufferView[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create an image view");
        }
    }
}

void Engine::initializeDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(framebufferWidth, framebufferHeight, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void Engine::initializeFramebuffer() {
    std::vector<VkImageView> attachments = {backBufferView[0], depthImageView};

    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = renderPass;
    info.attachmentCount = static_cast<uint32_t>(attachments.size());
    info.pAttachments = attachments.data();
    info.width = framebufferWidth;
    info.height = framebufferHeight;
    info.layers = 1;
    for(uint32_t i = 0; i < backBufferCount; i++) {
        attachments[0] = backBufferView[i];
        if (vkCreateFramebuffer(logicalDevice, &info, allocator, &framebuffer[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create a framebuffer");
        }
    }
}

void Engine::renderFrame() {
    frameBegin();
    VkCommandBuffer cmdBuff = commandBuffer[frameIndex];

    clearValue = {0.5f, 0.5f, 0.5f, 1.0f};

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    nv_helpers_vk::imageBarrier(cmdBuff, backBuffer[backBufferIndices[frameIndex]], subresourceRange, 0,
                              VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                              VK_IMAGE_LAYOUT_GENERAL);

    updateRaytracingRenderTarget(backBufferView[backBufferIndices[frameIndex]]);

    beginRenderPass();

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, rtPipeline);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
                          rtPipelineLayout, 0, 1, &rtDescriptorSet,
                          0, nullptr);

    VkDeviceSize rayGenOffset = sbtGen.GetRayGenOffset();
    VkDeviceSize missOffset = sbtGen.GetMissOffset();
    VkDeviceSize missStride = sbtGen.GetMissEntrySize();
    VkDeviceSize hitGroupOffset = sbtGen.GetHitGroupOffset();
    VkDeviceSize hitGroupStride = sbtGen.GetHitGroupEntrySize();

    vkCmdTraceRaysNV(cmdBuff, shaderBindingTableBuffer, rayGenOffset,
                    shaderBindingTableBuffer, missOffset, missStride,
                    shaderBindingTableBuffer, hitGroupOffset, hitGroupStride,
                    VK_NULL_HANDLE, 0, 0, framebufferSize.width,
                    framebufferSize.height, 1);

    vkCmdNextSubpass(cmdBuff, VK_SUBPASS_CONTENTS_INLINE);
    endRenderPass();

    frameEnd();
    framePresent();
}

void Engine::frameBegin() {
    VkResult err;
    for (;;) {
        err = vkWaitForFences(logicalDevice, 1, &fence[frameIndex], VK_TRUE, 100);
        if(err == VK_SUCCESS) {
            break;
        }
        if (err == VK_TIMEOUT) {
            continue;
        }
    }

    err = vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX,
                                presentCompleteSemaphore[frameIndex], VK_NULL_HANDLE,
                                &backBufferIndices[frameIndex]);
    // err = vkResetCommandPool(logicalDevice, m_commandPool[m_frameIndex], 0);
    // check_vk_result(err);
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(commandBuffer[frameIndex], &info);
}

void Engine::beginRenderPass() {
    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = renderPass;
    info.framebuffer = framebuffer[backBufferIndices[frameIndex]];
    info.renderArea.extent.width = SCREENWIDTH;
    info.renderArea.extent.height = SCREENHEIGHT;
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = clearValue.color;
    clearValues[1].depthStencil = {1.0f, 0};

    info.clearValueCount = static_cast<uint32_t>(clearValues.size());
    info.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(commandBuffer[frameIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
}

void Engine::endRenderPass() {
    vkCmdEndRenderPass(commandBuffer[frameIndex]);
}

void Engine::frameEnd() {
    VkResult err;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &presentCompleteSemaphore[frameIndex];
    info.pWaitDstStageMask = &wait_stage;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &commandBuffer[frameIndex];
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &renderCompleteSemaphore[frameIndex];

    err = vkEndCommandBuffer(commandBuffer[frameIndex]);
    err = vkResetFences(logicalDevice, 1, &fence[frameIndex]);
    err = vkQueueSubmit(queue, 1, &info, fence[frameIndex]);
}

void Engine::framePresent() {
    VkResult err;
    VkSwapchainKHR swapchains[1] = {swapchain};
    uint32_t indices[1] = {backBufferIndices[frameIndex]};
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &renderCompleteSemaphore[frameIndex];
    info.swapchainCount = 1;
    info.pSwapchains = swapchains;
    info.pImageIndices = indices;
    err = vkQueuePresentKHR(queue, &info);

    frameIndex = (frameIndex + 1) % VK_QUEUED_FRAMES;
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

        if (indices.isComplete()) {
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

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Engine::querySwapChainSupport(const VkPhysicalDevice& device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkFormat Engine::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat Engine::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }

        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

bool Engine::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Engine::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(logicalDevice, image, imageMemory, 0);
}

uint32_t Engine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

VkImageView Engine::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void Engine::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = 0;  //
    barrier.dstAccessMask = 0;  //

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer Engine::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool[0];
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Engine::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(logicalDevice, commandPool[0], 1, &commandBuffer);
}

std::vector<char> Engine::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if(!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule Engine::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if(vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void Engine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
}

void Engine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}