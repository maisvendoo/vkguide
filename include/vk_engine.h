#ifndef     VK_ENGINE_H
#define     VK_ENGINE_H

#include    <vk_types.h>

#include    <VkBootstrap.h>

#define     SDL_MAIN_HANDLED

#include    <SDL.h>
#include    <SDL_vulkan.h>

#include    <vk_initializers.h>

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
class VulkanEngine
{
public:

    bool isInitialized { false };

    int frameNumber {0};

    bool stop_rendering {false};

    VkExtent2D windowExtent{1360, 768};

    struct SDL_Window *window { nullptr };

    VkInstance instance;

    VkDebugUtilsMessengerEXT debug_messeger;

    VkPhysicalDevice chosenGPU;

    VkDevice device;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;

    VkFormat swapchainImageFormat;

    std::vector<VkImage> swapchainImages;

    std::vector<VkImageView> swapchainImageViews;

    VkExtent2D swapchainExtent;

    FrameData frames[FRAME_OVERLAP];

    FrameData &get_current_frame()
    {
        return frames[frameNumber % FRAME_OVERLAP];
    }

    VkQueue graphicsQueue;

    uint32_t graphicsQueueFamily;

    DeletionQueue mainDeletionQueue;

    VmaAllocator allocator;

    AllocatedImage drawImage;

    VkExtent2D drawExtent;

    static VulkanEngine &Get();

    // Инициализация движка
    void init();

    // Корректная финализация движка
    void cleanup();

    // Цикл отрисовки
    void draw();

    // Рисование фона
    void draw_background(VkCommandBuffer cmd);

    // Главный цикл приложения
    void run();

private:

    void init_vulkan();

    void init_swapchain();

    void create_swapchain(uint32_t width, uint32_t height);

    void destroy_swapchain();

    void init_commands();

    void init_sync_structures();
};

#endif
