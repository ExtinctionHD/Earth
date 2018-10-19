#include "Model.h"

// public:

Model::~Model()
{
	objectCount--;

	if (objectCount == 0 && transformDSLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(pDevice->device, transformDSLayout, nullptr);
		transformDSLayout = VK_NULL_HANDLE;
	}

	// cleanup materials
	for (auto it = materials.begin(); it != materials.end(); ++it)
	{
		delete((*it).second);
	}

	// cleanup meshes
	for (auto it = meshes.begin(); it != meshes.end(); ++it)
	{
		delete(*it);
	}

	delete(pTransformBuffer);
}

glm::mat4 Model::getTransform()
{
	return transform;
}

void Model::setTransform(glm::mat4 matrix)
{
	transform = matrix;
	pTransformBuffer->updateData(&transform, sizeof(transform), 0);
}

uint32_t Model::getBufferCount() const 
{
	return 1 + meshes.size();
}

uint32_t Model::getTextureCount() const
{
	uint32_t textureCount = 0;

	for (MeshBase *pMesh : meshes)
	{
		textureCount += pMesh->pMaterial->getTextures().size();
	}

	return textureCount;
}

uint32_t Model::getMeshCount() const
{
	return meshes.size();
}

void Model::initDescriptorSets(DescriptorPool * pDescriptorPool)
{
	transformDescriptorSet = pDescriptorPool->getDescriptorSet({ pTransformBuffer }, { }, transformDSLayout == VK_NULL_HANDLE, transformDSLayout);

	for (std::pair<uint32_t, Material*> pair : materials)
	{
		pair.second->initDescritorSet(pDescriptorPool);
	}
}

void Model::draw(VkCommandBuffer commandBuffer, std::vector<VkDescriptorSet> descriptorSets)
{
	descriptorSets.push_back(transformDescriptorSet);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getPipeline()->pipeline);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getPipeline()->layout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

	for (int i = 0; i < meshes.size(); i++)
	{
		VkDescriptorSet materialDescriptorSet = meshes[i]->pMaterial->getDesriptorSet();
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getPipeline()->layout, descriptorSets.size(), 1, &materialDescriptorSet, 0, nullptr);

		VkBuffer vertexBuffer = meshes[i]->getVertexBuffer();
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);

		VkBuffer indexBuffer = meshes[i]->getIndexBuffer();
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		uint32_t indexCount = meshes[i]->getIndexCount();
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
	}
}

// protected:

Model::Model(Device *pDevice)
{
	this->pDevice = pDevice;

	pTransformBuffer = new Buffer(pDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHADER_STAGE_VERTEX_BIT, sizeof(transform));
	setTransform(glm::mat4(1.0f));

	objectCount++;
}

VkDescriptorSetLayout Model::transformDSLayout = VK_NULL_HANDLE;

// private:

uint32_t Model::objectCount = 0;

