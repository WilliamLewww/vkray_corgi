#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <vulkan/vulkan.h>

namespace nv_helpers_vk {

#ifndef IMGUI_MAX_POSSIBLE_BACK_BUFFERS
#define IMGUI_MAX_POSSIBLE_BACK_BUFFERS 16
#endif  // !IMGUI_MAX_POSSIBLE_BACK_BUFFERS

#ifndef IMGUI_VK_QUEUED_FRAMES
#define IMGUI_VK_QUEUED_FRAMES 2
#endif  // !IMGUI_VK_QUEUED_FRAMES

struct QueueFamilyIndices
{
  int graphicsFamily = -1;
  int presentFamily  = -1;

  bool isComplete() { return graphicsFamily >= 0 && presentFamily >= 0; }
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR        capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR>   presentModes;
};

class VkContext
{
public:
  // Factory.
  static VkContext& Singleton()
  {
    static VkContext vkmini;
    return vkmini;
  }

  void           setupVulkan(GLFWwindow*                     window,
                             bool                            useDebugLayerInDebugMode = true,
                             const std::vector<const char*>& extensions = {"VK_KHR_swapchain"});
  void           createDescriptorPool();
  void           createCommandBuffers();
  void           createLogicalDevice(const std::vector<const char*>& device_extensions = {"VK_KHR_"
                                                                                "swapchain"});
  void           getPresentMode();
  void           findSurfaceFormat();
  void           getQueueFamilly();
  void           getGpu(const std::vector<const char*>& extensions);
  void           createInstance(bool useDebugLayerInDebugMode = true);
  void           resizeVulkan(int w, int h);
  void           createFrameBuffer();
  void           createImageViews();
  void           createRenderPass();
  void           cleanupVulkan();
  void           frameBegin();
  void           beginRenderPass();
  void           endRenderPass();
  void           frameEnd();
  void           framePresent();
  void           createSwapchain(VkSwapchainKHR old_swapchain, int w, int h);
  VkShaderModule createShaderModule(const std::vector<char>& code);

  void createBuffer(VkDeviceSize          size,
                    VkBufferUsageFlags    usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer&             buffer,
                    VkDeviceMemory&       bufferMemory);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  void createTextureImage(uint8_t*        pixels,
                          int             texWidth,
                          int             texHeight,
                          int             texChannels,
                          VkImage&        textureImage,
                          VkDeviceMemory& textureImageMemory);
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
  const VkPhysicalDevice& getPhysicalDevice() { return m_gpu; }
  const VkDevice&         getDevice() { return m_device; }
  const VkRenderPass&     getRenderPass() { return m_renderPass; }
  const VkPipelineCache&  getPipelineCache() { return m_pipelineCache; }
  const VkDescriptorPool& getDescriptorPool() { return m_descriptorPool; }
  const VkQueue&          getQueue() { return m_queue; }
  const VkSwapchainKHR&   getSwapchain() { return m_swapchain; }
  VkAllocationCallbacks*  getAllocator() { return m_allocator; }
  VkCommandPool*          getCommandPool() { return m_commandPool; }
  VkCommandBuffer*        getCommandBuffer() { return m_commandBuffer; }
  uint32_t                getFrameIndex() { return m_frameIndex; }
  void setClearValue(const VkClearValue& clear_value) { m_clearValue = clear_value; }

  VkFormat getSurfaceFormat() const { return m_surfaceFormat.format; }

  VkImage getCurrentBackBuffer() const { return m_backBuffer[m_backBufferIndices[m_frameIndex]]; }
  VkImageView getCurrentBackBufferView() const
  {
    return m_backBufferView[m_backBufferIndices[m_frameIndex]];
  }

  VkCommandBuffer beginSingleTimeCommands();
  void            endSingleTimeCommands(VkCommandBuffer commandBuffer);
  VkImageView     createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
  void            transitionImageLayout(VkImage       image,
                                        VkFormat      format,
                                        VkImageLayout oldLayout,
                                        VkImageLayout newLayout);

private:
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                               VkImageTiling                tiling,
                               VkFormatFeatureFlags         features);
  VkFormat findDepthFormat();
  bool     hasStencilComponent(VkFormat format);
  void     createDepthResources();
  void     createImage(uint32_t              width,
                       uint32_t              height,
                       VkFormat              format,
                       VkImageTiling         tiling,
                       VkImageUsageFlags     usage,
                       VkMemoryPropertyFlags properties,
                       VkImage&              image,
                       VkDeviceMemory&       imageMemory);

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

  VkAllocationCallbacks*   m_allocator   = VK_NULL_HANDLE;
  VkInstance               m_instance    = VK_NULL_HANDLE;
  VkSurfaceKHR             m_surface     = VK_NULL_HANDLE;
  VkPhysicalDevice         m_gpu         = VK_NULL_HANDLE;
  VkDevice                 m_device      = VK_NULL_HANDLE;
  VkSwapchainKHR           m_swapchain   = VK_NULL_HANDLE;
  VkRenderPass             m_renderPass  = VK_NULL_HANDLE;
  uint32_t                 m_queueFamily = 0;
  VkQueue                  m_queue       = VK_NULL_HANDLE;
  VkDebugReportCallbackEXT m_debugReport = VK_NULL_HANDLE;

  VkSurfaceFormatKHR      m_surfaceFormat = {};
  VkImageSubresourceRange m_imageRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  VkPresentModeKHR        m_presentMode   = {};

  VkPipelineCache  m_pipelineCache  = VK_NULL_HANDLE;
  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

  int      fb_width = 0, fb_height = 0;
  uint32_t m_backBufferIndices[IMGUI_VK_QUEUED_FRAMES];  // keep track of recently
                                                         // rendered swapchain frame
                                                         // indices
  uint32_t      m_backBufferCount                                 = 0;
  VkImage       m_backBuffer[IMGUI_MAX_POSSIBLE_BACK_BUFFERS]     = {};
  VkImageView   m_backBufferView[IMGUI_MAX_POSSIBLE_BACK_BUFFERS] = {};
  VkFramebuffer m_framebuffer[IMGUI_MAX_POSSIBLE_BACK_BUFFERS]    = {};

  uint32_t        m_frameIndex = 0;
  VkCommandPool   m_commandPool[IMGUI_VK_QUEUED_FRAMES];
  VkCommandBuffer m_commandBuffer[IMGUI_VK_QUEUED_FRAMES];
  VkFence         m_fence[IMGUI_VK_QUEUED_FRAMES];
  VkSemaphore     m_presentCompleteSemaphore[IMGUI_VK_QUEUED_FRAMES];
  VkSemaphore     m_renderCompleteSemaphore[IMGUI_VK_QUEUED_FRAMES];

  VkImage        m_depthImage       = {};
  VkDeviceMemory m_depthImageMemory = {};
  VkImageView    m_depthImageView   = {};

  VkClearValue m_clearValue = {};
  bool isDeviceSuitable(const VkPhysicalDevice& gpu, const std::vector<const char*>& extensions);

  QueueFamilyIndices      findQueueFamilies(const VkPhysicalDevice& gpu);
  SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& gpu);
  bool                    checkDeviceExtensionSupport(const VkPhysicalDevice&         gpu,
                                                      const std::vector<const char*>& extensions);
};

}  // namespace nv_helpers_vk

// Access to the Vulkan singleton context
#define VkCtx nv_helpers_vk::VkContext::Singleton()
