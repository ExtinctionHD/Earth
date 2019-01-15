#pragma once

#include <vector>
#include "TextureImage.h"
#include <glm/glm.hpp>
#include <string>
#include <assimp/scene.h>
#include <unordered_map>
#include "Buffer.h"
#include "DescriptorPool.h"
#include "RgbaUNorm.h"

class Material
{
public:
	Material(Device *device);

	~Material();

	// order of textures for each material
	static const std::vector<aiTextureType> TEXTURES_ORDER;

	uint32_t index{};

	struct MaterialColors
	{
		glm::vec4 diffuseColor;

		glm::vec4 specularColor;

		float opacity;
	} colors{};

	std::vector<TextureImage*> getTextures() const;

	bool isSolid() const;

	Buffer *pColorsBuffer;

	// loads colors data in buffer
	void updateColorsBuffer();

	// adds texture of this type
	void addTexture(aiTextureType type, TextureImage *pTexture);

	void initDescriptorSet(DescriptorPool *pDescriptorPool);

	VkDescriptorSet getDescriptorSet() const;

	static VkDescriptorSetLayout getDsLayout();

	static void initDefaultTextures(Device *device);

private:
	Device *device;

	static uint32_t objectCount;

	std::unordered_map<aiTextureType, TextureImage*> textures;

	VkDescriptorSet descriptorSet{};

	static VkDescriptorSetLayout dsLayout;

	static std::vector<RgbaUNorm> DEFAULT_TEXTURES_COLORS;

	static std::unordered_map<aiTextureType, TextureImage*> defaultTextures;

};

