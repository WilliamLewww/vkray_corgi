// ImGui - standalone example application for Glfw + Vulkan, using programmable
// pipeline If you are new to ImGui, see examples/README.txt and documentation
// at the top of imgui.cpp.

#include <array>

#include "imgui.h"
#include "imgui_impl_glfw_vulkan.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "src/hello_vulkan.h"
#include "src/manipulator.h"
#include "src/vk_context.h"

static bool g_ResizeWanted = false;
static int  g_winWidth = 1280, g_winHeight = 720;

//////////////////////////////////////////////////////////////////////////
#define UNUSED(x) (void)(x)
//////////////////////////////////////////////////////////////////////////

static void check_vk_result(VkResult err)
{
  if(err == 0)
    return;
  printf("VkResult %d\n", err);
  if(err < 0)
    abort();
}

static void on_errorCallback(int error, const char* description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void on_resizeCallback(GLFWwindow* window, int w, int h)
{
  UNUSED(window);
  CameraManip.setWindowSize(w, h);
  g_ResizeWanted = true;
  g_winWidth     = w;
  g_winHeight    = h;
}

//-----------------------------------------------------------------------------
// Trapping the keyboard
//
static void on_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
  if(ImGui::GetIO().WantCaptureMouse)
  {
    return;
  }

  if(action == GLFW_RELEASE)  // Don't deal with key up
  {
    return;
  }

  if(key == GLFW_KEY_ESCAPE || key == 'Q')
  {
    glfwSetWindowShouldClose(window, 1);
  }
}

static void on_mouseMoveCallback(GLFWwindow* window, double mouseX, double mouseY)
{
  ImGuiIO& io = ImGui::GetIO();
  if(io.WantCaptureKeyboard || io.WantCaptureMouse)
  {
    return;
  }

  using nv_helpers_vk::Manipulator;
  Manipulator::Inputs inputs;
  inputs.lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  inputs.mmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
  inputs.rmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
  if(!inputs.lmb && !inputs.rmb && !inputs.mmb)
  {
    return;  // no mouse button pressed
  }

  inputs.ctrl  = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
  inputs.shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
  inputs.alt   = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;

  CameraManip.mouseMove(static_cast<int>(mouseX), static_cast<int>(mouseY), inputs);
}

static void on_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
  if(ImGui::GetIO().WantCaptureMouse)
  {
    return;
  }

  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  CameraManip.setMousePosition(static_cast<int>(xpos), static_cast<int>(ypos));
}

static void on_scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
  if(ImGui::GetIO().WantCaptureMouse)
  {
    return;
  }
  CameraManip.wheel(static_cast<int>(yoffset));
}

int main(int argc, char** argv)
{
  UNUSED(argc);
  UNUSED(argv);

  // Setup window
  glfwSetErrorCallback(on_errorCallback);
  if(!glfwInit())
  {
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(g_winWidth, g_winHeight,
                                        "NVIDIA Vulkan Raytracing Tutorial", nullptr, nullptr);

  glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
  glfwSetScrollCallback(window, on_scrollCallback);
  glfwSetKeyCallback(window, on_keyCallback);
  glfwSetCursorPosCallback(window, on_mouseMoveCallback);
  glfwSetMouseButtonCallback(window, on_mouseButtonCallback);
  // glfwSetWindowSizeCallback(window, on_resizeCallback);
  glfwSetFramebufferSizeCallback(window, on_resizeCallback);

  // Setup camera
  CameraManip.setWindowSize(g_winWidth, g_winHeight);
  CameraManip.setLookat(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  // Setup Vulkan
  if(!glfwVulkanSupported())
  {
    printf("GLFW: Vulkan Not Supported\n");
    return 1;
  }
  VkCtx.setupVulkan(window, true,
                    {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                     VK_NV_RAY_TRACING_EXTENSION_NAME,
                     VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME});

  // Setup Dear ImGui binding
  if(1 == 1)
  {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui_ImplGlfwVulkan_Init_Data init_data = {};
    init_data.allocator                      = VkCtx.getAllocator();
    init_data.gpu                            = VkCtx.getPhysicalDevice();
    init_data.device                         = VkCtx.getDevice();
    init_data.render_pass                    = VkCtx.getRenderPass();
    init_data.pipeline_cache                 = VkCtx.getPipelineCache();
    init_data.descriptor_pool                = VkCtx.getDescriptorPool();
    init_data.check_vk_result                = check_vk_result;

    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard
    // Controls
    ImGui_ImplGlfwVulkan_Init(window, false, &init_data);

    // Setup style
    ImGui::StyleColorsDark();
  }

  // Upload Fonts
  if(1 == 1)
  {
    uint32_t g_FrameIndex = VkCtx.getFrameIndex();
    VkResult err;
    err = vkResetCommandPool(VkCtx.getDevice(), VkCtx.getCommandPool()[g_FrameIndex], 0);
    check_vk_result(err);
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(VkCtx.getCommandBuffer()[g_FrameIndex], &begin_info);
    check_vk_result(err);

    ImGui_ImplGlfwVulkan_CreateFontsTexture(VkCtx.getCommandBuffer()[g_FrameIndex]);

    VkSubmitInfo end_info       = {};
    end_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers    = &VkCtx.getCommandBuffer()[g_FrameIndex];
    err                         = vkEndCommandBuffer(VkCtx.getCommandBuffer()[g_FrameIndex]);
    check_vk_result(err);
    err = vkQueueSubmit(VkCtx.getQueue(), 1, &end_info, VK_NULL_HANDLE);
    check_vk_result(err);

    err = vkDeviceWaitIdle(VkCtx.getDevice());
    check_vk_result(err);
    ImGui_ImplGlfwVulkan_InvalidateFontUploadObjects();
  }

  HelloVulkan helloVulkan;
  helloVulkan.loadModel("res/cube_multi.obj");
  helloVulkan.createDescriptorSetLayout();
  helloVulkan.createGraphicsPipeline(
      {static_cast<uint32_t>(g_winWidth), static_cast<uint32_t>(g_winHeight)});
  helloVulkan.createUniformBuffer();
  helloVulkan.updateDescriptorSet();

  helloVulkan.initRayTracing();
  helloVulkan.createGeometryInstances();
  helloVulkan.createAccelerationStructures();
  helloVulkan.createRaytracingDescriptorSet();
  helloVulkan.createRaytracingPipeline();
  helloVulkan.createShaderBindingTable();

  bool use_raster_render = true;

  ImVec4 clear_color = ImVec4(1, 1, 1, 1.00f);

  // Main loop
  while(!glfwWindowShouldClose(window))
  {
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application. Generally you may always pass all inputs
    // to dear imgui, and hide them from your application based on those two
    // flags.
    glfwPollEvents();

    if(g_ResizeWanted)
    {
      VkCtx.resizeVulkan(g_winWidth, g_winHeight);
      helloVulkan.createGraphicsPipeline(
          {static_cast<uint32_t>(g_winWidth), static_cast<uint32_t>(g_winHeight)});
    }
    g_ResizeWanted = false;

    helloVulkan.updateUniformBuffer();

    // 1. Show a simple window.
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets
    // automatically appears in a window called "Debug".
    if(1 == 1)
    {
      ImGui_ImplGlfwVulkan_NewFrame();
      ImGui::ColorEdit3("clear color",
                        reinterpret_cast<float*>(
                            &clear_color));  // Edit 3 floats representing a color

      ImGui::Checkbox("Raster mode",
  		&use_raster_render); // Switch between raster and ray tracing


      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

      {
        static int item = 1;
        if(ImGui::Combo("Up Vector", &item, "X\0Y\0Z\0\0"))
        {
          glm::vec3 pos, eye, up;
          CameraManip.getLookat(pos, eye, up);
          up = glm::vec3(item == 0, item == 1, item == 2);
          CameraManip.setLookat(pos, eye, up);
        }
      }
    }

    VkClearValue cv;
    memcpy(&cv.color.float32[0], &clear_color.x, 4 * sizeof(float));
    VkCtx.setClearValue(cv);
    VkCtx.frameBegin();
    VkCommandBuffer cmdBuff = VkCtx.getCommandBuffer()[VkCtx.getFrameIndex()];

    if(use_raster_render)
    {
      VkCtx.beginRenderPass();
      vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, helloVulkan.m_graphicsPipeline);
      VkBuffer     vertexBuffers[] = {helloVulkan.m_vertexBuffer};
      VkDeviceSize offsets[]       = {0};
      vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              helloVulkan.m_pipelineLayout, 0, 1, &helloVulkan.m_descriptorSet, 0,
                              nullptr);

      vkCmdBindVertexBuffers(cmdBuff, 0, 1, vertexBuffers, offsets);
      vkCmdBindIndexBuffer(cmdBuff, helloVulkan.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(cmdBuff, static_cast<uint32_t>(helloVulkan.m_nbIndices), 1, 0, 0, 0);
    }
    else
    {
    	VkClearValue clearColor = {0.0f, 0.5f, 0.0f, 1.0f};

		VkImageSubresourceRange subresourceRange;
		subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel   = 0;
		subresourceRange.levelCount     = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount     = 1;

		nv_helpers_vk::imageBarrier(cmdBuff, VkCtx.getCurrentBackBuffer(), subresourceRange, 0,
		                          VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		                          VK_IMAGE_LAYOUT_GENERAL);

		helloVulkan.updateRaytracingRenderTarget(VkCtx.getCurrentBackBufferView());

		VkCtx.beginRenderPass();

		vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, helloVulkan.m_rtPipeline);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
		                      helloVulkan.m_rtPipelineLayout, 0, 1, &helloVulkan.m_rtDescriptorSet,
		                      0, nullptr);

		VkDeviceSize rayGenOffset   = helloVulkan.m_sbtGen.GetRayGenOffset();
		VkDeviceSize missOffset     = helloVulkan.m_sbtGen.GetMissOffset();
		VkDeviceSize missStride     = helloVulkan.m_sbtGen.GetMissEntrySize();
		VkDeviceSize hitGroupOffset = helloVulkan.m_sbtGen.GetHitGroupOffset();
		VkDeviceSize hitGroupStride = helloVulkan.m_sbtGen.GetHitGroupEntrySize();

		vkCmdTraceRaysNV(cmdBuff, helloVulkan.m_shaderBindingTableBuffer, rayGenOffset,
		                helloVulkan.m_shaderBindingTableBuffer, missOffset, missStride,
		                helloVulkan.m_shaderBindingTableBuffer, hitGroupOffset, hitGroupStride,
		                VK_NULL_HANDLE, 0, 0, helloVulkan.m_framebufferSize.width,
		                helloVulkan.m_framebufferSize.height, 1);
    }

    {
      vkCmdNextSubpass(cmdBuff, VK_SUBPASS_CONTENTS_INLINE);
      ImGui_ImplGlfwVulkan_Render(cmdBuff);
      VkCtx.endRenderPass();
    }

    VkCtx.frameEnd();

    VkCtx.framePresent();
  }

  // Cleanup
  VkResult err = vkDeviceWaitIdle(VkCtx.getDevice());
  check_vk_result(err);
  ImGui_ImplGlfwVulkan_Shutdown();
  ImGui::DestroyContext();
  helloVulkan.destroyResources();
  VkCtx.cleanupVulkan();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
