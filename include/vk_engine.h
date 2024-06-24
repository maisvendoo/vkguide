#ifndef     VK_ENGINE_H
#define     VK_ENGINE_H

#include    <vk_types.h>

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

    static VulkanEngine &Get();

    ///
    void init();

    void cleanup();

    void draw();

    void run();
};

#endif
