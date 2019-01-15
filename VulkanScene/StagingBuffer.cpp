#include <cassert>

#include "StagingBuffer.h"

// public:

StagingBuffer::StagingBuffer(Device *device, VkDeviceSize size)
{
	this->device = device;
	this->size = size;

	createBuffer(
		device,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingMemory);
}

StagingBuffer::~StagingBuffer()
{
	vkFreeMemory(device->get(), stagingMemory, nullptr);
	vkDestroyBuffer(device->get(), stagingBuffer, nullptr);
}

void StagingBuffer::updateData(void *data, VkDeviceSize size, VkDeviceSize offset)
{
	assert(offset + size <= this->size);

	void *bufferData;
	vkMapMemory(device->get(), stagingMemory, offset, size, 0, &bufferData);
	memcpy(bufferData, data, size);
	vkUnmapMemory(device->get(), stagingMemory);
}

void StagingBuffer::copyToImage(VkImage image, std::vector<VkBufferImageCopy> regions) const
{
    VkCommandBuffer commandBuffer = device->beginOneTimeCommands();

	vkCmdCopyBufferToImage(
        commandBuffer,
        stagingBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        regions.size(),
        regions.data());

	device->endOneTimeCommands(commandBuffer);
}

VkBuffer StagingBuffer::get() const
{
	return stagingBuffer;
}

VkDeviceSize StagingBuffer::getSize() const
{
	return size;
}

// protected:

void StagingBuffer::createBuffer(
    Device *device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer *buffer,
    VkDeviceMemory *memory)
{
	VkBufferCreateInfo createInfo{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,							
		0,									
		size,								
		usage,								
		VK_SHARING_MODE_EXCLUSIVE,			
		0,									
		nullptr,							
	};

    const VkResult result = vkCreateBuffer(device->get(), &createInfo, nullptr, buffer);
	assert(result == VK_SUCCESS);

	allocateMemory(device, *buffer, memory, properties);

	vkBindBufferMemory(device->get(), *buffer, *memory, 0);
}

void StagingBuffer::allocateMemory(
    Device *device,
    VkBuffer buffer,
    VkDeviceMemory *memory,
    VkMemoryPropertyFlags properties)
{
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device->get(), buffer, &memRequirements);

    const uint32_t memoryTypeIndex = device->findMemoryTypeIndex(
		memRequirements.memoryTypeBits,
		properties);

	VkMemoryAllocateInfo allocInfo{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,				
		memRequirements.size,	
		memoryTypeIndex,		
	};

    const VkResult result = vkAllocateMemory(device->get(), &allocInfo, nullptr, memory);
	assert(result == VK_SUCCESS);
}
