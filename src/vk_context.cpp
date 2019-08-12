/******************************************************************************
 * Copyright 1998-2018 NVIDIA Corp. All Rights Reserved.
 *****************************************************************************/
// Mix of ImGui and vulkan-tutorial

#include <array>
#include <cstdio>   // printf, fprintf
#include <cstdlib>  // abort
#include <set>
#include <vector>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw_vulkan.h"
#include "vk_context.h"

namespace nv_helpers_vk {

#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT      flags,
                                                   VkDebugReportObjectTypeEXT objectType,
                                                   uint64_t                   object,
                                                   size_t                     location,
                                                   int32_t                    messageCode,
                                                   const char*                pLayerPrefix,
                                                   const char*                pMessage,
                                                   void*                      pUserData)
{
  (void)flags;
  (void)object;
  (void)pUserData;
  (void)pLayerPrefix;
  (void)messageCode;
  (void)location;
  printf("[vulkan] ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
  return VK_FALSE;
}
#endif  // IMGUI_VULKAN_DEBUG_REPORT

//--------------------------------------------------------------------------------------------------
//
//
static void check_vk_result(VkResult err)
{
  if(err == 0)
  {
    return;
  }
  printf("VkResult %d\n", err);
  if(err < 0)
  {
    abort();
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::resizeVulkan(int w, int h)
{
  VkResult       err;
  VkSwapchainKHR old_swapchain = m_swapchain;
  err                          = vkDeviceWaitIdle(m_device);
  check_vk_result(err);

  // Destroy old Framebuffer:
  for(uint32_t i = 0; i < m_backBufferCount; i++)
  {
    if(m_backBufferView[i])
    {
      vkDestroyImageView(m_device, m_backBufferView[i], m_allocator);
    }
  }

  for(uint32_t i = 0; i < m_backBufferCount; i++)
  {
    if(m_framebuffer[i])
    {
      vkDestroyFramebuffer(m_device, m_framebuffer[i], m_allocator);
    }
  }

  if(m_renderPass)
  {
    vkDestroyRenderPass(m_device, m_renderPass, m_allocator);
  }

  // Create Swapchain:
  createSwapchain(old_swapchain, w, h);

  if(old_swapchain)
  {
    vkDestroySwapchainKHR(m_device, old_swapchain, m_allocator);
  }

  // Create the Render Pass:
  createRenderPass();

  // Create The Image Views
  createImageViews();

  createDepthResources();

  // Create Framebuffer:
  createFrameBuffer();
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createFrameBuffer()
{
  VkResult err;

  std::array<VkImageView, 2> attachments = {m_backBufferView[0], m_depthImageView};
  ///  VkImageView attachment[1];
  VkFramebufferCreateInfo info = {};
  info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  info.renderPass              = m_renderPass;
  info.attachmentCount         = static_cast<uint32_t>(attachments.size());
  info.pAttachments            = attachments.data();
  info.width                   = fb_width;
  info.height                  = fb_height;
  info.layers                  = 1;
  for(uint32_t i = 0; i < m_backBufferCount; i++)
  {
    attachments[0] = m_backBufferView[i];
    err            = vkCreateFramebuffer(m_device, &info, m_allocator, &m_framebuffer[i]);
    check_vk_result(err);
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createImageViews()
{
  VkResult err;

  VkImageViewCreateInfo info = {};
  info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
  info.format                = m_surfaceFormat.format;
  info.components.r          = VK_COMPONENT_SWIZZLE_R;
  info.components.g          = VK_COMPONENT_SWIZZLE_G;
  info.components.b          = VK_COMPONENT_SWIZZLE_B;
  info.components.a          = VK_COMPONENT_SWIZZLE_A;
  info.subresourceRange      = m_imageRange;

  for(uint32_t i = 0; i < m_backBufferCount; i++)
  {
    info.image = m_backBuffer[i];
    err        = vkCreateImageView(m_device, &info, m_allocator, &m_backBufferView[i]);
    check_vk_result(err);
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createRenderPass()
{
  VkResult err;

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format                  = m_surfaceFormat.format;
  colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment = {};
  color_attachment.attachment            = 0;
  color_attachment.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format                  = findDepthFormat();
  depthAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment            = 1;
  depthAttachmentRef.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpasses[2];
  subpasses[0]                         = {};
  subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpasses[0].colorAttachmentCount    = 1;
  subpasses[0].pColorAttachments       = &color_attachment;
  subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;

  subpasses[1]                         = {};
  subpasses[1].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpasses[1].colorAttachmentCount    = 1;
  subpasses[1].pColorAttachments       = &color_attachment;
  subpasses[1].pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass          = 0;
  dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask       = 0;
  dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  std::array<VkSubpassDependency, 2> dependencies;

  dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass    = 0;
  dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass   = 0;
  dependencies[1].dstSubpass   = 1;  // VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].srcAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  std::array<VkAttachmentDescription, 2> attachments    = {colorAttachment, depthAttachment};
  VkRenderPassCreateInfo                 renderPassInfo = {};
  renderPassInfo.sType                                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount                        = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments                           = attachments.data();
  renderPassInfo.subpassCount                           = 2;
  renderPassInfo.pSubpasses                             = subpasses;
  renderPassInfo.dependencyCount                        = 2;
  renderPassInfo.pDependencies                          = dependencies.data();
  err = vkCreateRenderPass(m_device, &renderPassInfo, m_allocator, &m_renderPass);
  check_vk_result(err);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createSwapchain(VkSwapchainKHR old_swapchain, int w, int h)
{
  VkResult err;

  VkSwapchainCreateInfoKHR info = {};
  info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info.surface                  = m_surface;
  info.imageFormat              = m_surfaceFormat.format;
  info.imageColorSpace          = m_surfaceFormat.colorSpace;
  info.imageArrayLayers         = 1;
  info.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
  info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  info.presentMode      = m_presentMode;
  info.clipped          = VK_TRUE;
  info.oldSwapchain     = old_swapchain;
  VkSurfaceCapabilitiesKHR cap;
  err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu, m_surface, &cap);
  check_vk_result(err);

  if(cap.maxImageCount > 0)
  {
    info.minImageCount =
        (cap.minImageCount + 2 < cap.maxImageCount) ? (cap.minImageCount + 2) : cap.maxImageCount;
  }
  else
  {
    info.minImageCount = cap.minImageCount + 2;
  }

  info.minImageCount = 2;

  if(cap.currentExtent.width == 0xffffffff)
  {
    fb_width                = w;
    fb_height               = h;
    info.imageExtent.width  = fb_width;
    info.imageExtent.height = fb_height;
  }
  else
  {
    fb_width                = cap.currentExtent.width;
    fb_height               = cap.currentExtent.height;
    info.imageExtent.width  = fb_width;
    info.imageExtent.height = fb_height;
  }

  err = vkCreateSwapchainKHR(m_device, &info, m_allocator, &m_swapchain);
  check_vk_result(err);
  err = vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_backBufferCount, nullptr);
  check_vk_result(err);
  err = vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_backBufferCount, m_backBuffer);
  check_vk_result(err);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::getQueueFamilly()
{
  uint32_t count;
  vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &count, nullptr);
  std::vector<VkQueueFamilyProperties> queues(count);
  vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &count, queues.data());
  for(uint32_t i = 0; i < count; i++)
  {
    if(queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      m_queueFamily = i;
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::setupVulkan(
    GLFWwindow*                     window,
    bool                            useDebugLayerInDebugMode /* = true */,
    const std::vector<const char*>& extensions /*  = {"VK_KHR_swapchain"} */)
{
  VkResult err;

  // Create Vulkan Instance
  createInstance(useDebugLayerInDebugMode);

  // Create Window Surface
  {
    err = glfwCreateWindowSurface(m_instance, window, m_allocator, &m_surface);
    check_vk_result(err);
  }

  // Get GPU
  getGpu(extensions);

  // Get queue
  getQueueFamilly();

  // Check for WSI support
  {
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, m_queueFamily, m_surface, &res);
    if(res != VK_TRUE)
    {
      fprintf(stderr, "Error no WSI support on physical device 0\n");
      exit(-1);
    }
  }

  // Get Surface Format
  findSurfaceFormat();

  // Get Present Mode
  getPresentMode();

  // Create Logical Device
  createLogicalDevice(extensions);

  // Create Command Buffers
  createCommandBuffers();

  // Create Descriptor Pool
  createDescriptorPool();

  // Create Framebuffers
  {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    resizeVulkan(w, h);
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createDescriptorPool()
{
  VkResult err;

  VkDescriptorPoolSize pool_size[] = {
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
  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets                    = 1000;
  pool_info.poolSizeCount              = _countof(pool_size);
  pool_info.pPoolSizes                 = pool_size;
  err = vkCreateDescriptorPool(m_device, &pool_info, m_allocator, &m_descriptorPool);
  check_vk_result(err);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createCommandBuffers()
{
  VkResult err;
  for(int i = 0; i < IMGUI_VK_QUEUED_FRAMES; i++)
  {
    {
      VkCommandPoolCreateInfo info = {};
      info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      info.queueFamilyIndex        = m_queueFamily;
      err = vkCreateCommandPool(m_device, &info, m_allocator, &m_commandPool[i]);
      check_vk_result(err);
    }
    {
      VkCommandBufferAllocateInfo info = {};
      info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      info.commandPool                 = m_commandPool[i];
      info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      info.commandBufferCount          = 1;
      err = vkAllocateCommandBuffers(m_device, &info, &m_commandBuffer[i]);
      check_vk_result(err);
    }
    {
      VkFenceCreateInfo info = {};
      info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;
      err                    = vkCreateFence(m_device, &info, m_allocator, &m_fence[i]);
      check_vk_result(err);
    }
    {
      VkSemaphoreCreateInfo info = {};
      info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      err = vkCreateSemaphore(m_device, &info, m_allocator, &m_presentCompleteSemaphore[i]);
      check_vk_result(err);
      err = vkCreateSemaphore(m_device, &info, m_allocator, &m_renderCompleteSemaphore[i]);
      check_vk_result(err);
    }
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createLogicalDevice(
    const std::vector<const char*>& device_extensions /*  = {"VK_KHR_swapchain"} */)
{
  VkResult err;

  auto                    device_extension_count = static_cast<uint32_t>(device_extensions.size());
  const uint32_t          queue_index            = 0;
  const uint32_t          queue_count            = 1;
  const float             queue_priority[]       = {1.0f};
  VkDeviceQueueCreateInfo queue_info[1]          = {};
  queue_info[0].sType                            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info[0].queueFamilyIndex                 = m_queueFamily;
  queue_info[0].queueCount                       = queue_count;
  queue_info[0].pQueuePriorities                 = queue_priority;
  VkDeviceCreateInfo create_info                 = {};
  create_info.sType                              = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount               = sizeof(queue_info) / sizeof(queue_info[0]);
  create_info.pQueueCreateInfos                  = queue_info;
  create_info.enabledExtensionCount              = device_extension_count;
  create_info.ppEnabledExtensionNames            = device_extensions.data();

  VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexFeatures = {};
  descIndexFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
  VkPhysicalDeviceFeatures2 supportedFeatures = {};
  supportedFeatures.sType                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  supportedFeatures.pNext                     = &descIndexFeatures;
  vkGetPhysicalDeviceFeatures2(m_gpu, &supportedFeatures);
  create_info.pEnabledFeatures = &(supportedFeatures.features);
  create_info.pNext            = &descIndexFeatures;

  // create_info.pNext = supportedFeatures.pNext;

  err = vkCreateDevice(m_gpu, &create_info, m_allocator, &m_device);
  check_vk_result(err);
  vkGetDeviceQueue(m_device, m_queueFamily, queue_index, &m_queue);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::getPresentMode()
{
  uint32_t count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu, m_surface, &count, nullptr);
  std::vector<VkPresentModeKHR> presentModes(count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu, m_surface, &count, presentModes.data());

  m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
  for(const auto& presentMode : presentModes)
  {
    if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      m_presentMode = presentMode;
      break;
    }

    if(presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
    {
      m_presentMode = presentMode;
    }
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::findSurfaceFormat()
{
  // Per Spec Format and View Format are expected to be the same unless
  // VK_IMAGE_CREATE_MUTABLE_BIT was set at image creation Assuming that the
  // default behavior is without setting this bit, there is no need for separate
  // Spawchain image and image view format additionally several new color spaces
  // were introduced with Vulkan Spec v1.0.40 hence we must make sure that a
  // format with the mostly available color space,
  // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, is found and used
  uint32_t count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &count, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu, m_surface, &count, formats.data());

  // first check if only one format, VK_FORMAT_UNDEFINED, is available, which
  // would imply that any format is available
  if(count == 1)
  {
    if(formats[0].format == VK_FORMAT_UNDEFINED)
    {
      m_surfaceFormat.format     = VK_FORMAT_B8G8R8A8_UNORM;
      m_surfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    }
    else
    {  // no point in searching another format
      m_surfaceFormat = formats[0];
    }
  }
  else
  {
    // request several formats, the first found will be used
    VkFormat requestSurfaceImageFormat[]     = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
    VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    bool            requestedFound           = false;
    for(size_t i = 0; i < sizeof(requestSurfaceImageFormat) / sizeof(requestSurfaceImageFormat[0]);
        i++)
    {
      if(requestedFound)
      {
        break;
      }
      for(uint32_t j = 0; j < count; j++)
      {
        if(formats[j].format == requestSurfaceImageFormat[i]
           && formats[j].colorSpace == requestSurfaceColorSpace)
        {
          m_surfaceFormat = formats[j];
          requestedFound  = true;
        }
      }
    }

    // if none of the requested image formats could be found, use the first
    // available
    if(!requestedFound)
    {
      m_surfaceFormat = formats[0];
    }
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::getGpu(const std::vector<const char*>& extensions)
{
  VkResult err;

  uint32_t gpu_count;
  err = vkEnumeratePhysicalDevices(m_instance, &gpu_count, nullptr);
  check_vk_result(err);

  std::vector<VkPhysicalDevice> gpus(gpu_count);
  err = vkEnumeratePhysicalDevices(m_instance, &gpu_count, gpus.data());
  check_vk_result(err);

  for(const auto& gpu : gpus)
  {
    if(isDeviceSuitable(gpu, extensions))
    {
      m_gpu = gpu;
      break;
    }
  }

  if(m_gpu == VK_NULL_HANDLE)
  {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  // If a number >1 of GPUs got reported, you should find the best fit GPU for
  // your purpose e.g. VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU if available, or
  // with the greatest memory available, etc. for sake of simplicity we'll just
  // take the first one, assuming it has a graphics queue family. m_gpu =
  // gpus[0];
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createInstance(bool useDebugLayerInDebugMode /* = true */)
{
  VkResult err;

  uint32_t                 extensions_count;
  const char**             glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
  std::vector<const char*> instanceExtensions;
  instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  for(uint32_t i = 0; i < extensions_count; i++)
  {
    instanceExtensions.push_back(glfw_extensions[i]);
  }

  VkInstanceCreateInfo create_info = {};
  create_info.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

#ifdef IMGUI_VULKAN_DEBUG_REPORT
  if(useDebugLayerInDebugMode)
  {
    // enabling multiple validation layers grouped as lunarg standard validation
    const char* layers[]            = {"VK_LAYER_LUNARG_standard_validation"};
    create_info.enabledLayerCount   = 1;
    create_info.ppEnabledLayerNames = layers;
  }
  else
  {
    create_info.enabledLayerCount = 0;
  }
  instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

#endif  // IMGUI_VULKAN_DEBUG_REPORT

  create_info.enabledExtensionCount   = static_cast<uint32_t>(instanceExtensions.size());
  create_info.ppEnabledExtensionNames = instanceExtensions.data();

  err = vkCreateInstance(&create_info, m_allocator, &m_instance);
  check_vk_result(err);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
  // create the debug report callback
  VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
  debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT
                          | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
  debug_report_ci.pfnCallback = debug_report;
  debug_report_ci.pUserData   = nullptr;

  // get the proc address of the function pointer, required for used extensions
  auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
      vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT"));

  err = vkCreateDebugReportCallbackEXT(m_instance, &debug_report_ci, m_allocator, &m_debugReport);
  check_vk_result(err);
#endif  // IMGUI_VULKAN_DEBUG_REPORT
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::cleanupVulkan()
{
  vkDestroyDescriptorPool(m_device, m_descriptorPool, m_allocator);
  for(int i = 0; i < IMGUI_VK_QUEUED_FRAMES; i++)
  {
    vkDestroyFence(m_device, m_fence[i], m_allocator);
    vkFreeCommandBuffers(m_device, m_commandPool[i], 1, &m_commandBuffer[i]);
    vkDestroyCommandPool(m_device, m_commandPool[i], m_allocator);
    vkDestroySemaphore(m_device, m_presentCompleteSemaphore[i], m_allocator);
    vkDestroySemaphore(m_device, m_renderCompleteSemaphore[i], m_allocator);
  }

  for(uint32_t i = 0; i < m_backBufferCount; i++)
  {
    vkDestroyImageView(m_device, m_backBufferView[i], m_allocator);
    vkDestroyFramebuffer(m_device, m_framebuffer[i], m_allocator);
  }

  vkDestroyImageView(m_device, m_depthImageView, nullptr);
  vkDestroyImage(m_device, m_depthImage, nullptr);
  vkFreeMemory(m_device, m_depthImageMemory, nullptr);

  vkDestroyRenderPass(m_device, m_renderPass, m_allocator);
  vkDestroySwapchainKHR(m_device, m_swapchain, m_allocator);
  vkDestroySurfaceKHR(m_instance, m_surface, m_allocator);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
  // get the proc address of the function pointer, required for used extensions
  auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
      vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT"));
  if(func != nullptr)
  {
    func(m_instance, m_debugReport, m_allocator);
  }
#endif  // IMGUI_VULKAN_DEBUG_REPORT

  vkDestroyDevice(m_device, m_allocator);
  vkDestroyInstance(m_instance, m_allocator);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::frameBegin()
{
  VkResult err;
  for(;;)
  {
    err = vkWaitForFences(m_device, 1, &m_fence[m_frameIndex], VK_TRUE, 100);
    if(err == VK_SUCCESS)
    {
      break;
    }
    if(err == VK_TIMEOUT)
    {
      continue;
    }
    check_vk_result(err);
  }
  {
    err = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
                                m_presentCompleteSemaphore[m_frameIndex], VK_NULL_HANDLE,
                                &m_backBufferIndices[m_frameIndex]);
    check_vk_result(err);
  }
  {
    // err = vkResetCommandPool(m_device, m_commandPool[m_frameIndex], 0);
    // check_vk_result(err);
    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(m_commandBuffer[m_frameIndex], &info);
    check_vk_result(err);
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::beginRenderPass()
{
  VkRenderPassBeginInfo info              = {};
  info.sType                              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.renderPass                         = m_renderPass;
  info.framebuffer                        = m_framebuffer[m_backBufferIndices[m_frameIndex]];
  info.renderArea.extent.width            = fb_width;
  info.renderArea.extent.height           = fb_height;
  std::array<VkClearValue, 2> clearValues = {};
  clearValues[0].color                    = m_clearValue.color;
  clearValues[1].depthStencil             = {1.0f, 0};

  info.clearValueCount = static_cast<uint32_t>(clearValues.size());
  info.pClearValues    = clearValues.data();
  vkCmdBeginRenderPass(m_commandBuffer[m_frameIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::endRenderPass()
{
  vkCmdEndRenderPass(m_commandBuffer[m_frameIndex]);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::frameEnd()
{
  VkResult err;

  {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo         info       = {};
    info.sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount         = 1;
    info.pWaitSemaphores            = &m_presentCompleteSemaphore[m_frameIndex];
    info.pWaitDstStageMask          = &wait_stage;
    info.commandBufferCount         = 1;
    info.pCommandBuffers            = &m_commandBuffer[m_frameIndex];
    info.signalSemaphoreCount       = 1;
    info.pSignalSemaphores          = &m_renderCompleteSemaphore[m_frameIndex];

    err = vkEndCommandBuffer(m_commandBuffer[m_frameIndex]);
    check_vk_result(err);
    err = vkResetFences(m_device, 1, &m_fence[m_frameIndex]);
    check_vk_result(err);
    err = vkQueueSubmit(m_queue, 1, &info, m_fence[m_frameIndex]);
    check_vk_result(err);
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::framePresent()
{
  VkResult         err;
  VkSwapchainKHR   swapchains[1] = {m_swapchain};
  uint32_t         indices[1]    = {m_backBufferIndices[m_frameIndex]};
  VkPresentInfoKHR info          = {};
  info.sType                     = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount        = 1;
  info.pWaitSemaphores           = &m_renderCompleteSemaphore[m_frameIndex];
  info.swapchainCount            = 1;
  info.pSwapchains               = swapchains;
  info.pImageIndices             = indices;
  err                            = vkQueuePresentKHR(m_queue, &info);
  check_vk_result(err);

  m_frameIndex = (m_frameIndex + 1) % IMGUI_VK_QUEUED_FRAMES;
}

//--------------------------------------------------------------------------------------------------
//
//
VkFormat VkContext::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                        VkImageTiling                tiling,
                                        VkFormatFeatureFlags         features)
{
  for(VkFormat format : candidates)
  {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(m_gpu, format, &props);

    if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
    {
      return format;
    }

    if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
    {
      return format;
    }
  }

  throw std::runtime_error("failed to find supported format!");
}

//--------------------------------------------------------------------------------------------------
//
//
VkFormat VkContext::findDepthFormat()
{
  return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

//--------------------------------------------------------------------------------------------------
//
//
bool VkContext::hasStencilComponent(VkFormat format)
{
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createDepthResources()
{
  VkFormat depthFormat = findDepthFormat();

  createImage(fb_width, fb_height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              m_depthImage, m_depthImageMemory);
  m_depthImageView = createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

  transitionImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

//--------------------------------------------------------------------------------------------------
//
//
VkImageView VkContext::createImageView(VkImage            image,
                                       VkFormat           format,
                                       VkImageAspectFlags aspectFlags)
{
  VkImageViewCreateInfo viewInfo           = {};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = format;
  viewInfo.subresourceRange.aspectMask     = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  VkImageView imageView;
  if(vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create texture image view!");
  }

  return imageView;
}

//--------------------------------------------------------------------------------------------------
//
//
bool VkContext::isDeviceSuitable(const VkPhysicalDevice&         gpu,
                                 const std::vector<const char*>& extensions)
{
  QueueFamilyIndices indices = findQueueFamilies(gpu);

  bool extensionsSupported = checkDeviceExtensionSupport(gpu, extensions);

  bool swapChainAdequate = false;
  if(extensionsSupported)
  {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(gpu);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(gpu, &supportedFeatures);

  return indices.isComplete() && extensionsSupported && swapChainAdequate
         && supportedFeatures.samplerAnisotropy;
}

//--------------------------------------------------------------------------------------------------
//
//
QueueFamilyIndices VkContext::findQueueFamilies(const VkPhysicalDevice& gpu)
{
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for(const auto& queueFamily : queueFamilies)
  {
    if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, m_surface, &presentSupport);

    if(queueFamily.queueCount > 0 && presentSupport)
    {
      indices.presentFamily = i;
    }

    if(indices.isComplete())
    {
      break;
    }

    i++;
  }

  return indices;
}

//--------------------------------------------------------------------------------------------------
//
//
SwapChainSupportDetails VkContext::querySwapChainSupport(const VkPhysicalDevice& gpu)
{
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_surface, &formatCount, nullptr);

  if(formatCount != 0)
  {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &presentModeCount, nullptr);

  if(presentModeCount != 0)
  {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &presentModeCount,
                                              details.presentModes.data());
  }

  return details;
}

//--------------------------------------------------------------------------------------------------
//
//
bool VkContext::checkDeviceExtensionSupport(const VkPhysicalDevice&         gpu,
                                            const std::vector<const char*>& extensions)
{
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

  for(const auto& extension : availableExtensions)
  {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::transitionImageLayout(VkImage       image,
                                      VkFormat      format,
                                      VkImageLayout oldLayout,
                                      VkImageLayout newLayout)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier = {};
  barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout            = oldLayout;
  barrier.newLayout            = newLayout;
  barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;

  barrier.image                           = image;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;

  barrier.srcAccessMask = 0;  //
  barrier.dstAccessMask = 0;  //

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if(hasStencilComponent(format))
    {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  else
  {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
     && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
         || newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL))
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
          && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
          && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  }
  else
  {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);
  endSingleTimeCommands(commandBuffer);
}

//--------------------------------------------------------------------------------------------------
//
//
VkCommandBuffer VkContext::beginSingleTimeCommands()
{
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool                 = m_commandPool[0];
  allocInfo.commandBufferCount          = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(m_queue);

  vkFreeCommandBuffers(m_device, m_commandPool[0], 1, &commandBuffer);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createImage(uint32_t              width,
                            uint32_t              height,
                            VkFormat              format,
                            VkImageTiling         tiling,
                            VkImageUsageFlags     usage,
                            VkMemoryPropertyFlags properties,
                            VkImage&              image,
                            VkDeviceMemory&       imageMemory)
{
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType         = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width      = width;
  imageInfo.extent.height     = height;
  imageInfo.extent.depth      = 1;
  imageInfo.mipLevels         = 1;
  imageInfo.arrayLayers       = 1;
  imageInfo.format            = format;
  imageInfo.tiling            = tiling;
  imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage             = usage;
  imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

  if(vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(m_device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize       = memRequirements.size;
  allocInfo.memoryTypeIndex      = findMemoryType(memRequirements.memoryTypeBits, properties);

  if(vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(m_device, image, imageMemory, 0);
}

//--------------------------------------------------------------------------------------------------
//
//
uint32_t VkContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(m_gpu, &memProperties);

  for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
  {
    if((typeFilter & (1 << i))
       && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

//--------------------------------------------------------------------------------------------------
//
//
VkShaderModule VkContext::createShaderModule(const std::vector<char>& code)
{
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize                 = code.size();
  createInfo.pCode                    = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createBuffer(VkDeviceSize          size,
                             VkBufferUsageFlags    usage,
                             VkMemoryPropertyFlags properties,
                             VkBuffer&             buffer,
                             VkDeviceMemory&       bufferMemory)
{
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size               = size;
  bufferInfo.usage              = usage;
  bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

  if(vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize       = memRequirements.size;
  allocInfo.memoryTypeIndex      = findMemoryType(memRequirements.memoryTypeBits, properties);

  if(vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion = {};
  copyRegion.size         = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::createTextureImage(uint8_t*        pixels,
                                   int             texWidth,
                                   int             texHeight,
                                   int             texChannels,
                                   VkImage&        textureImage,
                                   VkDeviceMemory& textureImageMemory)
{
  VkDeviceSize   imageSize = texWidth * texHeight * 4;
  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(m_device, stagingBufferMemory);

  createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(m_device, stagingBuffer, nullptr);
  vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

//--------------------------------------------------------------------------------------------------
//
//
void VkContext::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region               = {};
  region.bufferOffset                    = 0;
  region.bufferRowLength                 = 0;
  region.bufferImageHeight               = 0;
  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;
  region.imageOffset                     = {0, 0, 0};
  region.imageExtent                     = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &region);

  endSingleTimeCommands(commandBuffer);
}

}  // namespace nv_helpers_vk
