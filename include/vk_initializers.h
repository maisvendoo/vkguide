#ifndef     VK_INITIALIZERS_H
#define     VK_INITIALIZERS_H

#include    <vk_types.h>

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
namespace vkinit
{

    VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex,
                                                 VkCommandPoolCreateFlags flags = 0);

    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool,
                                                             uint32_t count = 1);

    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
}

#endif
