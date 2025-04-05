#include <iostream>
#include <vector>
#include <cstring>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <fstream>

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &);
VkShaderModule createShaderModule(const std::vector<char> &, VkDevice);
std::vector<char> readFile(const std::string &);

int main()
{
  if (!glfwInit())
  {
    std::cerr << "Falha ao iniciar o GLFW\n";
    return -1;
  }

  if (!glfwVulkanSupported())
  {
    std::cerr << "Seu sistema é fraco. Sem suporte ao Vulkan\n";
    glfwTerminate();
    return -1;
  }

  // Cria a janela
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Sem OpenGL
  GLFWwindow *window = glfwCreateWindow(800, 600, "GLFW + Vulkan, baby!", nullptr, nullptr);
  if (!window)
  {
    std::cerr << "Falha ao criar janela\n";
    glfwTerminate();
    return -1;
  }

  // Coleta extensões requeridas pelo GLFW
  uint32_t glfwExtensionCount{0};
  const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  // Cria o VkInstance
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Vulkan";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;

  VkInstance instance;
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
  {
    std::cerr << "Falha ao criar o VkInstance\n";
    return -1;
  }

  std::cout << "Vulkan inicializado com sucesso!\n";

  // Loop da janela
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
  }

  // Conexão entre a janela (GLFW) e a GPU.
  VkSurfaceKHR surface;

  // Criar o VkSurfaceKHR (ponte entre o GLFW e o Vulkan)
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
  {
    std::cerr << "Falha ao criar a Surface\n";
    return -1;
  }

  // Verifica se tem GPU disponivel
  uint32_t deviceCount{0};
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0)
  {
    std::cerr << "Nenhuma GPU encontrada\n";
    return -1;
  }

  //
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  // Escolhe o primeiro dispositivo que suporte o surface
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  for (const auto &device : devices)
  {
    // Verifica se a device suporta apresentar na surface
    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilies.size(); i++)
    {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport)
      {
        physicalDevice = device;
        break;
      }
    }

    if (physicalDevice != VK_NULL_HANDLE)
      break;
  }

  if (physicalDevice == VK_NULL_HANDLE)
  {
    std::cerr << "Nenhuma GPU com suporte gráfico + apresentação encontrada!\n";
    return -1;
  }

  std::cout << "GPU selecionada com sucesso!\n";

  // Criar o VkDevice: a GPU lógica.
  // Conseguir uma VkQueue : a fila onde vamos mandar os comandos de renderização.

  // Encontra o índice da fila que suporta gráfico + apresentação
  int graphicsFamilyIndex{-1};
  uint32_t queueFamilyCount{0};
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

  for (uint32_t i = 0; i < queueFamilies.size(); i++)
  {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport)
    {
      graphicsFamilyIndex = i;
      break;
    }
  }

  if (graphicsFamilyIndex == -1)
  {
    std::cerr << "Não achou uma fila de gráficos com suporte à janela\n";
    return -1;
  }

  // Criar a fila

  // Cria a fila no dispositivo logico
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  // Cria o dispositivo logico
  VkDevice device;
  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

  if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
  {
    std::cerr << "Falha ao criar o dispositivo lógico\n";
    return -1;
  }

  std::cout << "Dispositivo lógico criado com sucesso!\n";

  VkQueue graphicsQueue;
  vkGetDeviceQueue(device, graphicsFamilyIndex, 0, &graphicsQueue);

  // Consulta capacidades da surface
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

  // Pega formatos suportados
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

  // Pega modos de apresentação
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
  std::vector<VkPresentModeKHR> presentsModes(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentsModes.data());

  // Escolhe formato e modo
  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  VkExtent2D extent = capabilities.currentExtent;

  // Cria swapchain
  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
  {
    imageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapchainInfo{};
  swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainInfo.surface = surface;
  swapchainInfo.minImageCount = imageCount;
  swapchainInfo.imageFormat = surfaceFormat.format;
  swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapchainInfo.imageExtent = extent;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainInfo.preTransform = capabilities.currentTransform;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.presentMode = presentMode;
  swapchainInfo.clipped = VK_TRUE;
  swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

  VkSwapchainKHR swapchain;

  if (vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS)
  {
    std::cerr << "Falha ao criar swapchain\n";
    return -1;
  }

  std::cout << "Swapchain criado com sucesso!\n";

  // Criar os image views
  // Pegando as imagens do swapchain
  uint32_t swapImageCount;
  vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, nullptr);
  std::vector<VkImage> swapchainImages(swapImageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, swapchainImages.data());

  // Salva o formato pra usar depois
  VkFormat swapchainImageFormat = surfaceFormat.format;
  VkExtent2D swapchainExtent = extent;

  // Cria um image view pra cada imagem
  std::vector<VkImageView> swapchainImageViews;
  swapchainImageViews.resize(swapImageCount);

  for (size_t i = 0; i < swapImageCount; i++)
  {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = swapchainImages[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapchainImageFormat;

    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
    {
      std::cerr << "Falha ao criar image view\n";
      return -1;
    }
  }

  std::cout << "ImageViews criadas com sucesso!\n";

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapchainImageFormat;          // Mesmo formato da imagem da janela
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;        // Sem multisample
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Limpa a imagem no começo do render
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Salva ela no fim (pra exibir)
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Pra poder mostrar na janela

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0; // Referência pro primeiro attachment
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VkRenderPass renderPass;
  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
  {
    std::cerr << "Falha ao criar o render pass!\n";
    return -1;
  }

  std::cout << "Render pass criado com sucesso!\n";

  std::vector<VkFramebuffer> swapchainFramebuffers;
  swapchainFramebuffers.resize(swapchainImageViews.size());

  for (size_t i = 0; i < swapchainImageViews.size(); i++)
  {
    VkImageView attachments[] = {
        swapchainImageViews[i],
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass; // Aquele que criamos agora
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapchainExtent.width;
    framebufferInfo.height = swapchainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
    {
      std::cerr << "Falha ao criar framebuffer " << i << "\n";
      return -1;
    }
  }

  std::cout << "Framebuffers criados com sucesso!\n";

  auto vertShaderCode = readFile("shader.vert.spv");
  auto fragShaderCode = readFile("shader.frag.spv");

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, device);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, device);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapchainExtent.width;
  viewport.height = (float)swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchainExtent;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkPipelineLayout pipelineLayout;
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Falha ao criar pipeline layout");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;

  VkPipeline graphicsPipeline;
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("Falha ao criar graphics pipeline");
  }

  // Cleanup
  vkDestroyInstance(instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

// Função auxiliar pra pegar o formato de surface
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats)
{
  for (const auto &format : formats)
  {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return format;
    }
  }
  return formats[0];
}

VkShaderModule createShaderModule(const std::vector<char> &code, VkDevice device)
{
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
  {
    throw std::runtime_error("Falha ao criar shader module");
  }

  return shaderModule;
}

std::vector<char> readFile(const std::string &filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("Não foi possível abrir o arquivo: " + filename);

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}
