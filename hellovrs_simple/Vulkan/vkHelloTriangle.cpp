// vkHelloTriangle.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "framework.h"
#include "vkHelloTriangle.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
VkInstance graphicsInstance;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char*> instanceExtensions = { 
    "VK_KHR_surface", 
    "VK_KHR_win32_surface", 
    "VK_KHR_get_physical_device_properties2" 
};

const std::vector<const char*> deviceExtensions = { 
    "VK_KHR_swapchain",
    "VK_KHR_multiview",
    "VK_KHR_maintenance2",
    "VK_KHR_create_renderpass2",
    "VK_KHR_fragment_shading_rate"
};

typedef void(*Fn_vkCmdSetFragmentShadingRateKHR)(VkCommandBuffer, const VkExtent2D*, const VkFragmentShadingRateCombinerOpKHR combinerOps[2]);
typedef VkResult(*Fn_vkGetPhysicalDeviceFragmentShadingRatesKHR)(VkPhysicalDevice, uint32_t*, VkPhysicalDeviceFragmentShadingRateKHR*);
Fn_vkCmdSetFragmentShadingRateKHR pfnvkCmdSetFragmentShadingRateKHR{};
Fn_vkGetPhysicalDeviceFragmentShadingRatesKHR pfnvkGetPhysicalDeviceFragmentShadingRatesKHR{};

VkDevice graphicsDevice{};
VkSwapchainKHR swapChain{};
std::vector<VkImageView> swapChainImageViews{};
VkRenderPass renderPass{};
VkShaderModule vsShaderModule{};
VkShaderModule psShaderModule{};
VkPipelineLayout pipelineLayout{};
VkPipeline graphicsPipeline{};

struct FrameSynchronization
{
    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
    VkFence inFlight;
};
std::vector<FrameSynchronization> frameSynch;

struct ImageSynchronization
{
    VkFence inFlight;
};
std::vector<FrameSynchronization> imageSynch;

VkQueue graphicsQueue{};
VkQueue presentQueue{};
VkCommandPool commandPool{};
std::vector<VkFramebuffer> swapChainFramebuffers{};
std::vector<VkCommandBuffer> commandBuffers{};
VkExtent2D swapChainExtent{};
VkSurfaceKHR presentationSurface{};
uint32_t currentFrame{};
struct ShadingRateInfo
{
    VkExtent2D CoarsePixelSize;
    std::wstring Name;
};
std::vector<ShadingRateInfo> supportedShadingRates;
int shadingRateIndex{};
bool graphicsInitialized{};

void VerifyVkResult(VkResult r)
{
    assert(r == VK_SUCCESS);
}

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                DrawGraphics();
void                UpdateTitleText(HWND hwnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VKHELLOTRIANGLE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VKHELLOTRIANGLE));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        DrawGraphics();
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VKHELLOTRIANGLE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_VKHELLOTRIANGLE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

bool CheckDebugLayerSupport()
{
    uint32_t layerCount;
    VerifyVkResult(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

    std::vector<VkLayerProperties> availableLayers(layerCount);
    VerifyVkResult(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    for (const char* layerName : validationLayers) 
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) 
        {
            if (strcmp(layerName, layerProperties.layerName) == 0) 
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
{
    std::stringstream strm;
    strm << "VK DBG: " << pCallbackData->pMessage << "\n";

    OutputDebugStringA(strm.str().c_str());

    return VK_FALSE;
}

std::vector<char> LoadFileData(const std::string& filename)
{
    FILE* pFile{};
    fopen_s(&pFile, filename.c_str(), "rb");
    if (!pFile)
    {
        throw std::runtime_error("failed to open file!");
    }

    fseek(pFile, 0, SEEK_END);
    long fileSize = ftell(pFile);
    std::vector<char> buffer(fileSize);
    fseek(pFile, 0, SEEK_SET);
    fread_s(buffer.data(), fileSize, 1, fileSize, pFile);

    return buffer;
}

VkShaderModule CreateShaderModule(const std::string& filename)
{
    std::vector<char> bytecode = LoadFileData(filename);

    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());
    VerifyVkResult(vkCreateShaderModule(graphicsDevice, &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   #ifdef NDEBUG
       const bool enableValidationLayers = false;
   #else
       const bool enableValidationLayers = true;
   #endif

    if (enableValidationLayers)
    {
        CheckDebugLayerSupport();
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo{};
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    if (enableValidationLayers)
    {
        instanceCreateInfo.enabledLayerCount = validationLayers.size();
        instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        debugCreateInfo.pUserData = nullptr; // Optional
        instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        instanceCreateInfo.enabledLayerCount = 0;
    }

    instanceCreateInfo.enabledExtensionCount = instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

    VerifyVkResult(vkCreateInstance(&instanceCreateInfo, nullptr, &graphicsInstance));

    uint32_t extensionCount = 0;
    VerifyVkResult(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

    std::vector<VkExtensionProperties> extensions(extensionCount);

    VerifyVkResult(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()));

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(graphicsInstance, &deviceCount, nullptr);

    if (deviceCount == 0) 
    {
        return FALSE; // No point continuing
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(graphicsInstance, &deviceCount, devices.data());

    // Arbitrarily choose the first device
    VkPhysicalDevice const& primaryDevice = devices[0];

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(primaryDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(primaryDevice, &queueFamilyCount, queueFamilies.data());

    // Create presentation surface
    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo{};
    win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCreateInfo.hwnd = hWnd;
    win32SurfaceCreateInfo.hinstance = hInstance;
    VerifyVkResult(vkCreateWin32SurfaceKHR(graphicsInstance, &win32SurfaceCreateInfo, nullptr, &presentationSurface));

    // Look for graphics queue
    uint32_t graphicsQueueFamilyIndex = -1;
    uint32_t presentQueueFamilyIndex = -1;
    for (size_t i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && graphicsQueueFamilyIndex == -1)
        {
            graphicsQueueFamilyIndex = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(primaryDevice, i, presentationSurface, &presentSupport);
        if (presentSupport && presentQueueFamilyIndex == -1)
        {
            presentQueueFamilyIndex = i;
        }
    }

    if (graphicsQueueFamilyIndex == -1)
    {
        return FALSE;
    }

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{}; // keep all defaults for now

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRateFeatures{};
    shadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
    shadingRateFeatures.pipelineFragmentShadingRate = true;
    shadingRateFeatures.attachmentFragmentShadingRate = false; // screen space image
    shadingRateFeatures.primitiveFragmentShadingRate = false; // per-primitive
    deviceCreateInfo.pNext = &shadingRateFeatures;

    if (vkCreateDevice(primaryDevice, &deviceCreateInfo, nullptr, &graphicsDevice) != VK_SUCCESS)
    {
        assert(false);
    }

    pfnvkCmdSetFragmentShadingRateKHR = (Fn_vkCmdSetFragmentShadingRateKHR)vkGetDeviceProcAddr(graphicsDevice, "vkCmdSetFragmentShadingRateKHR");
    pfnvkGetPhysicalDeviceFragmentShadingRatesKHR = (Fn_vkGetPhysicalDeviceFragmentShadingRatesKHR)vkGetInstanceProcAddr(graphicsInstance, "vkGetPhysicalDeviceFragmentShadingRatesKHR");

    std::vector<VkPhysicalDeviceFragmentShadingRateKHR> fragment_shading_rates{};
    uint32_t                                            fragment_shading_rate_count = 0;
    pfnvkGetPhysicalDeviceFragmentShadingRatesKHR(primaryDevice, &fragment_shading_rate_count, nullptr);
    if (fragment_shading_rate_count > 0)
    {
        fragment_shading_rates.resize(fragment_shading_rate_count);
        for (VkPhysicalDeviceFragmentShadingRateKHR& fragment_shading_rate : fragment_shading_rates)
        {
            fragment_shading_rate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR;
        }
        pfnvkGetPhysicalDeviceFragmentShadingRatesKHR(primaryDevice, &fragment_shading_rate_count, fragment_shading_rates.data());
    }

    // Choose shading rates allowed with sample count 1 (this application is non-MSAA). In theory, all shading rates should support single-sampled,
    // but who knows, maybe there is some situation where shading rates can only be used with sample count > 1.
    for (size_t i = 0; i < fragment_shading_rates.size(); ++i)
    {
        if (!(fragment_shading_rates[i].sampleCounts & 1))
            continue;

        ShadingRateInfo shadingRateInfo{};
        shadingRateInfo.CoarsePixelSize = fragment_shading_rates[i].fragmentSize;

        std::wstringstream strm;
        strm << L"Shading Rate: " << shadingRateInfo.CoarsePixelSize.width << L"x" << shadingRateInfo.CoarsePixelSize.height;
        shadingRateInfo.Name = strm.str();

        supportedShadingRates.push_back(shadingRateInfo);
    }

    vkGetDeviceQueue(graphicsDevice, graphicsQueueFamilyIndex, 0, &graphicsQueue);

    vkGetDeviceQueue(graphicsDevice, presentQueueFamilyIndex, 0, &presentQueue);

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
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

    VerifyVkResult(vkCreateRenderPass(graphicsDevice, &renderPassInfo, nullptr, &renderPass));

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);

    swapChainExtent.width = clientRect.right - clientRect.left;
    swapChainExtent.height = clientRect.bottom - clientRect.top;

    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = presentationSurface;
    swapChainCreateInfo.minImageCount = 2;
    swapChainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapChainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapChainCreateInfo.imageExtent = swapChainExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    std::vector<uint32_t> concurrentPresentQueueFamilyIndices;
    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) 
    {
        concurrentPresentQueueFamilyIndices.push_back(graphicsQueueFamilyIndex);
        concurrentPresentQueueFamilyIndices.push_back(presentQueueFamilyIndex);

        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = concurrentPresentQueueFamilyIndices.size();
        swapChainCreateInfo.pQueueFamilyIndices = concurrentPresentQueueFamilyIndices.data();
    }
    else 
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0; // Optional
        swapChainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VerifyVkResult(vkCreateSwapchainKHR(graphicsDevice, &swapChainCreateInfo, nullptr, &swapChain));

    uint32_t imageCount;
    VerifyVkResult(vkGetSwapchainImagesKHR(graphicsDevice, swapChain, &imageCount, nullptr));
    std::vector<VkImage> swapChainImages;
    swapChainImages.resize(imageCount);
    VerifyVkResult(vkGetSwapchainImagesKHR(graphicsDevice, swapChain, &imageCount, swapChainImages.data()));

    swapChainImageViews.resize(swapChainImages.size());
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) 
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        VerifyVkResult(vkCreateImageView(graphicsDevice, &createInfo, nullptr, &swapChainImageViews[i]));

        // Create framebuffers
        VkImageView attachments[] = { swapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = _countof(attachments);
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;
        VerifyVkResult(vkCreateFramebuffer(graphicsDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
    }

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    frameSynch.resize(2);
    for (int i = 0; i < 2; ++i)
    {
        VerifyVkResult(vkCreateSemaphore(graphicsDevice, &semaphoreInfo, nullptr, &frameSynch[i].imageAvailable));
        VerifyVkResult(vkCreateSemaphore(graphicsDevice, &semaphoreInfo, nullptr, &frameSynch[i].renderFinished));
        VerifyVkResult(vkCreateFence(graphicsDevice, &fenceInfo, nullptr, &frameSynch[i].inFlight));
    }
    imageSynch.resize(swapChainImageViews.size());
    for (int i = 0; i < swapChainImageViews.size(); ++i)
    {
        VerifyVkResult(vkCreateFence(graphicsDevice, &fenceInfo, nullptr, &imageSynch[i].inFlight));
    }

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VerifyVkResult(vkCreateCommandPool(graphicsDevice, &poolInfo, nullptr, &commandPool));

    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    VerifyVkResult(vkAllocateCommandBuffers(graphicsDevice, &allocInfo, commandBuffers.data()));

    vsShaderModule = CreateShaderModule("VS.spv");
    psShaderModule = CreateShaderModule("PS.spv");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vsShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = psShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

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
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    VerifyVkResult(vkCreatePipelineLayout(graphicsDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = _countof(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    VerifyVkResult(vkCreateGraphicsPipelines(graphicsDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));

    graphicsInitialized = true;

    UpdateTitleText(hWnd);

   return TRUE;
}

void DrawGraphics()
{
    if (!graphicsInitialized)
        return;

    // CPU wait for the frame-before-last to finish executing, if it's not done yet.
    vkWaitForFences(graphicsDevice, 1, &frameSynch[currentFrame].inFlight, VK_TRUE, UINT64_MAX);

    // imageAvailableSemaphore is signaled when present is done.
    uint32_t imageIndex{};
    VerifyVkResult(vkAcquireNextImageKHR(graphicsDevice, swapChain, UINT64_MAX, frameSynch[currentFrame].imageAvailable, VK_NULL_HANDLE, &imageIndex));

    // Check if a previous frame is using this same image (i.e. there is its fence to wait on)
    if (imageSynch[imageIndex].inFlight != VK_NULL_HANDLE)
    {
        vkWaitForFences(graphicsDevice, 1, &imageSynch[imageIndex].inFlight, VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imageSynch[imageIndex].inFlight = frameSynch[currentFrame].inFlight;

    // Record all command buffer #imageIndex
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        VerifyVkResult(vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo));

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue clearColor = { {{0.0f, 0.2f, 0.4f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        // Implicitly, this does a transition, because the color attachment specifyes a layoutTransitionAfter of Present.
        vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkExtent2D coarsePixelSize = supportedShadingRates[shadingRateIndex].CoarsePixelSize;
        VkFragmentShadingRateCombinerOpKHR combinerOps[2]{};
        combinerOps[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR; // Use pipeline rate
        combinerOps[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR; // Pass pipeline rate through
        pfnvkCmdSetFragmentShadingRateKHR(commandBuffers[imageIndex], &coarsePixelSize, combinerOps);

        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        
        vkCmdDraw(commandBuffers[imageIndex], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[imageIndex]);
        VerifyVkResult(vkEndCommandBuffer(commandBuffers[imageIndex]));
    }

    // Submit the command buffer
    // Waits for imageAvailableSemaphore.
    // Signals renderFinishedSemaphore, once command buffer has finished executing.
    // Signals inFlight fence, too, once command buffer has finished executing.
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = { frameSynch[currentFrame].imageAvailable };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;

        VkSemaphore signalSemaphores[] = { frameSynch[currentFrame].renderFinished };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        vkResetFences(graphicsDevice, 1, &frameSynch[currentFrame].inFlight);

        VerifyVkResult(vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameSynch[currentFrame].inFlight));
    }

    // Present
    // Waits for renderFinishedSemaphore.
    // Signals nothing explicitly.
    // Signals imageAvailableSemaphore implicitly. (because of the call to vkAcquireNextImageKHR above)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore waitSemaphores[] = { frameSynch[currentFrame].renderFinished };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = waitSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        vkQueuePresentKHR(presentQueue, &presentInfo);
    }

    currentFrame = currentFrame % 2;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);            

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_KEYUP:
        {
        UINT8 key = static_cast<UINT8>(wParam);

            if (supportedShadingRates.size() > 0)
            {
                if (key == 37 && shadingRateIndex > 0) // left
                {
                    shadingRateIndex--;

                    UpdateTitleText(hWnd);
                }
                if (key == 39 && shadingRateIndex < supportedShadingRates.size() - 1) // right
                {
                    shadingRateIndex++;

                    UpdateTitleText(hWnd);
                }
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);

        graphicsInitialized = false;

        vkDestroyPipeline(graphicsDevice, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(graphicsDevice, pipelineLayout, nullptr);

        vkDestroyShaderModule(graphicsDevice, vsShaderModule, nullptr);
        vkDestroyShaderModule(graphicsDevice, psShaderModule, nullptr);

        for (auto framebuffer : swapChainFramebuffers) 
        {
            vkDestroyFramebuffer(graphicsDevice, framebuffer, nullptr);
        }

        vkDestroyCommandPool(graphicsDevice, commandPool, nullptr);

        for (int i = 0; i < frameSynch.size(); ++i)
        {
            vkDestroySemaphore(graphicsDevice, frameSynch[i].renderFinished, nullptr);
            vkDestroySemaphore(graphicsDevice, frameSynch[i].imageAvailable, nullptr);
            vkDestroyFence(graphicsDevice, frameSynch[i].inFlight, nullptr);
        }
        for (int i = 0; i < swapChainImageViews.size(); ++i)
        {
            vkDestroyFence(graphicsDevice, imageSynch[i].inFlight, nullptr);
        }

        vkDestroyPipelineLayout(graphicsDevice, pipelineLayout, nullptr);
        vkDestroyRenderPass(graphicsDevice, renderPass, nullptr);
        for (auto imageView : swapChainImageViews) 
        {
            vkDestroyImageView(graphicsDevice, imageView, nullptr);
        }
        vkDestroySwapchainKHR(graphicsDevice, swapChain, nullptr);
        vkDestroySurfaceKHR(graphicsInstance, presentationSurface, nullptr);
        vkDestroyDevice(graphicsDevice, nullptr);
        vkDestroyInstance(graphicsInstance, nullptr);

        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void UpdateTitleText(HWND hwnd)
{
    wchar_t const* titleText = L"";

    bool platformSupportsVRS = supportedShadingRates.size() > 0;

    if (platformSupportsVRS)
    {
        titleText = supportedShadingRates[shadingRateIndex].Name.c_str();
    }
    else
    {
        titleText = L"{Platform does not support VRS}";
    }


    SetWindowText(hwnd, titleText);
}
