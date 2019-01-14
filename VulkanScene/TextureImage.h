#pragma once
#include "Image.h"

#include <stb_image.h>

// create texture image from file
class TextureImage : 
	public Image
{
public:
	TextureImage() = default;

    // loads texture from files
	// array layer count must be equal to paths count
	// images must have same extent
	TextureImage(
		Device *pDevice, 
		std::vector<std::string> paths, 
		uint32_t arrayLayers, 
		bool isCube,
		VkFilter filter,
		VkSamplerAddressMode samplerAddressMode);

    // creates device local texture
	TextureImage(
	    Device *pDevice,
	    VkExtent3D extent,
		VkImageCreateFlags flags,
		VkSampleCountFlagBits sampleCount,
	    VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
	    VkImageAspectFlags aspectFlags,
        VkImageViewType viewType,
	    uint32_t arrayLayers,
		VkSamplerAddressMode samplerAddressMode);

	~TextureImage();

	// image in shader
	VkSampler sampler{};

protected:
	// returns pixel bytes and save image extent
	stbi_uc* loadPixels(const std::string &path);

	// generate mipmap levels and transit image layout to SHADER_READ_ONLY
	void generateMipmaps(Device *pDevice, uint32_t arrayLayers, VkImageAspectFlags aspectFlags, VkFilter filter);

	void createSampler(VkFilter filter, VkSamplerAddressMode addressMode);
};

