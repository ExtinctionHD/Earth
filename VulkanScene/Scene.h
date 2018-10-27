#pragma once

#include "Camera.h"
#include "Timer.h"
#include "Lighting.h"
#include "AssimpModel.h"
#include "SkyboxModel.h"
#include "File.h"
#include "Controller.h"
#include "TerrainModel.h"

// provides scene for rendering
// contains: camera, lighting, models
class Scene
{
public:
	Scene(Device *pDevice, VkExtent2D cameraExtent);
	~Scene();

	Controller* getController() const;

	uint32_t getBufferCount() const;

	uint32_t getTextureCount() const;

	uint32_t getDescriptorSetCount() const;

	void prepareSceneRendering(DescriptorPool *pDescriptorPool, const RenderPassesMap &renderPasses);

	void updateScene();

	void drawDepth(VkCommandBuffer commandBuffer);

	void draw(VkCommandBuffer commandBuffer);

	void resizeExtent(VkExtent2D newExtent);

private:
	Device *pDevice;

	Controller *pController;

	// camera attributes
	Camera *pCamera{};

	// timer for animations
	Timer frameTimer;

	// scene lighting attributes
	Lighting *pLighting{};

	// scene descriptors:

    // depth renderPass resources
	VkDescriptorSet depthDescriptorSet{};
	VkDescriptorSetLayout depthDsLayout{};

    // final renderPass resources
	VkDescriptorSet sceneDescriptorSet{};
	VkDescriptorSetLayout sceneDsLayout{};

	// models
	AssimpModel *pCar{};
	SkyboxModel *pSkybox{};
	TerrainModel *pTerrain{};
	std::vector<Model*> models;

	GraphicsPipeline *pDepthPipeline{};
	std::vector<GraphicsPipeline*> pipelines;

	void initCamera(VkExtent2D cameraExtent);

	void initLighting();

	void initModels();

	void initDescriptorSets(DescriptorPool *pDescriptorPool, RenderPassesMap renderPasses);

	void initPipelines(RenderPassesMap renderPasses);
};

