#include    <vk_images.h>

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void vkutil::transition_image(VkCommandBuffer cmd,
                              VkImage image,
                              VkImageLayout currentLayout,
                              VkImageLayout newLayout)
{
    VkImageMemoryBarrier2 imageBarrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    imageBarrier.pNext = nullptr;


}
