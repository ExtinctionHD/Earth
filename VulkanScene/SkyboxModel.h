#pragma once

#include "Mesh.h"
#include "Model.h"
#include "ImageSetInfo.h"

class SkyboxModel : public Model
{
public:
	static const int CUBE_SIDE_COUNT = 6;

	static const std::vector<std::string> FILENAMES;

	SkyboxModel(Device *device, ImageSetInfo imageSetInfo);

	~SkyboxModel();

protected:
	VkVertexInputBindingDescription getVertexBindingDescription(uint32_t binding) override;

	std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescriptions(
        uint32_t binding,
        uint32_t locationOffset) override;

private:
	TextureImage *texture;
};

