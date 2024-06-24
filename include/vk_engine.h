#ifndef     VK_ENGINE_H
#define     VK_ENGINE_H

#include    <vk_types.h>

#include    <VkBootstrap.h>

#define     SDL_MAIN_HANDLED

#include    <SDL.h>
#include    <SDL_vulkan.h>

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

    VkPhysicalDevice choseGPU;

    VkDevice device;

    VkSurfaceKHR surface;

    static VulkanEngine &Get();

    // Инициализация движка
    void init();

    // Корректная финализация движка
    void cleanup();

    // Цикл отрисовки
    void draw();

    // Главный цикл приложения
    void run();

private:

    void init_vulkan();

    void init_swapchain();

    void init_commands();

    void init_sync_structures();
};

#endif
