#include    <vk_engine.h>

#include    <vk_images.h>

#include    <chrono>
#include    <thread>

#define     VMA_IMPLEMENTATION
#include    <vk_mem_alloc.h>

constexpr bool bUseValidationLayers = true;

VulkanEngine *loadedEngine = nullptr;

constexpr unsigned int uWaitTimeout = 1000000000;

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VulkanEngine &VulkanEngine::Get()
{
    return *loadedEngine;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::init()
{
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_VULKAN);

    window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        windowExtent.width,
        windowExtent.height,
        window_flags);

    init_vulkan();

    init_swapchain();

    init_commands();

    init_sync_structures();

    isInitialized = true;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::cleanup()
{
    if (isInitialized)
    {
        vkDeviceWaitIdle(device);

        for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            vkDestroyCommandPool(device, frames[i].commandPool, nullptr);

            vkDestroyFence(device, frames[i].renderFence, nullptr);
            vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
            vkDestroySemaphore(device, frames[i].swapchainSemaphore, nullptr);

            frames[i].deletionQueue.flush();
        }

        mainDeletionQueue.flush();

        destroy_swapchain();

        vkDestroySurfaceKHR(instance, surface, nullptr);

        vkDestroyDevice(device, nullptr);

        vkb::destroy_debug_utils_messenger(instance, debug_messeger);

        vkDestroyInstance(instance, nullptr);

        SDL_DestroyWindow(window);
    }

    loadedEngine = nullptr;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::draw()
{
    VK_CHECK(vkWaitForFences(device, 1, &get_current_frame().renderFence, true, uWaitTimeout));

    get_current_frame().deletionQueue.flush();

    VK_CHECK(vkResetFences(device, 1, &get_current_frame().renderFence));

    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(
        device,
        swapchain,
        uWaitTimeout,
        get_current_frame().swapchainSemaphore,
        nullptr,
        &swapchainImageIndex));

    VkCommandBuffer cmd = get_current_frame().mainCommandBuffer;

    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    vkutil::transition_image(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    draw_background(cmd);

    vkutil::transition_image(cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vkutil::copy_image_to_image(cmd, drawImage.image, swapchainImages[swapchainImageIndex], drawExtent, swapchainExtent);

    vkutil::transition_image(cmd, swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));
    //

    VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                                                   get_current_frame().swapchainSemaphore);

    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                                                     get_current_frame().renderSemaphore);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, get_current_frame().renderFence));

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &get_current_frame().renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

    frameNumber++;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::draw_background(VkCommandBuffer cmd)
{
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(frameNumber / 120.0f));
    clearValue = { {0.0f, 0.0f, flash, 1.0f} };

    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdClearColorImage(cmd,
                         drawImage.image,
                         VK_IMAGE_LAYOUT_GENERAL,
                         &clearValue,
                         1,
                         &clearRange);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;

    while (!bQuit)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_WINDOWEVENT)
            {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
                {
                    stop_rendering = true;
                }

                if (e.window.event == SDL_WINDOWEVENT_RESTORED)
                {
                    stop_rendering = false;
                }
            }
        }

        if (stop_rendering)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::init_vulkan()
{
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("Example Vulkan Application")
        .request_validation_layers(bUseValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();

    vkb::Instance vkb_inst = inst_ret.value();

    instance = vkb_inst.instance;
    debug_messeger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(window, instance, &surface);

    VkPhysicalDeviceVulkan13Features features13 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector { vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
                        .set_minimum_version(1, 3)
                        .set_required_features_13(features13)
                        .set_required_features_12(features12)
                        .set_surface(surface)
                        .select()
                        .value();

    vkb::DeviceBuilder deviceBuilder { physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    device = vkbDevice.device;
    chosenGPU = physicalDevice.physical_device;

    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = chosenGPU;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    mainDeletionQueue.push_function([&]() {
       vmaDestroyAllocator(allocator);
    });
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::init_swapchain()
{
    create_swapchain(windowExtent.width, windowExtent.height);

    VkExtent3D drawImageExtent = {
        windowExtent.width,
        windowExtent.height,
        1
    };

    drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::image_create_info(drawImage.imageFormat,
                                                            drawImageUsages,
                                                            drawImageExtent);

    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(allocator,
                   &rimg_info,
                   &rimg_allocinfo,
                   &drawImage.image,
                   &drawImage.allocation,
                   nullptr);

    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(drawImage.imageFormat,
                                                                     drawImage.image,
                                                                     VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(device, &rview_info, nullptr, &drawImage.imageView));

    mainDeletionQueue.push_function([&]() {
        vkDestroyImageView(device, drawImage.imageView, nullptr);
        vmaDestroyImage(allocator, drawImage.image, drawImage.allocation);
    });
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder {chosenGPU, device, surface};

    swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        .set_desired_format(VkSurfaceFormatKHR{ .format = swapchainImageFormat,
                                                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    swapchainExtent = vkbSwapchain.extent;
    swapchain = vkbSwapchain.swapchain;
    swapchainImages = vkbSwapchain.get_images().value();
    swapchainImageViews = vkbSwapchain.get_image_views().value();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::destroy_swapchain()
{
    vkDestroySwapchainKHR(device, swapchain, nullptr);

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::init_commands()
{
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(
        graphicsQueueFamily,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (size_t i = 0; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));

        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frames[i].commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].mainCommandBuffer));
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void VulkanEngine::init_sync_structures()
{
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (size_t i = 0; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));

        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
    }
}
