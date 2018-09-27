#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Device.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "GraphicsPipeline.h"
#include "DescriptorSet.h"
#include "Image.h"
#include "TextureImage.h"
#include "Model.h"
#include "MvpMatrices.h"
#include "Timer.h"
#include "Lighting.h"
#include "Camera.h"
#include "Shape.h"
#include "CubeTextureImage.h"

// graphic API class that create all necessary objects
// and set this as window user pointer
class Vulkan
{
public:
	// create all required objects
	Vulkan(GLFWwindow *window, VkExtent2D frameExtent);

	// destroy objects: surface, callback, instance
	~Vulkan();

	// executes graphics commands and present result image on window surface
	void drawFrame();

	// rebuild swapchain and all dependent objects for new extension
	void resize(VkExtent2D newExtent);

	// moves camera on key press
	void onKeyAction(int key, int action);

	// moves camera on mouse move
	void onMouseMove(float x, float y);

private:
	const std::vector<const char *> VALIDATION_LAYERS =
	{
		"VK_LAYER_LUNARG_standard_validation"  // some debug layers
	};

#ifdef _DEBUG  // validation layers enable only in debug mode
	const bool ENABLE_VALIDATION_LAYERS = true;
#else
	const bool ENABLE_VALIDATION_LAYERS = false;
#endif

	// color that clear each frame
	const VkClearColorValue clearColor = { 0, 0, 0, 1 };

	// files with shaders code
	const std::string MAIN_VERT_SHADER_PATH = File::getExeDir() + "shaders/mainVert.spv";
	const std::string MAIN_FRAG_SHADER_PATH = File::getExeDir() + "shaders/mainFrag.spv";
	const std::string SKYBOX_VERT_SHADER_PATH = File::getExeDir() + "shaders/skyboxVert.spv";
	const std::string SKYBOX_FRAG_SHADER_PATH = File::getExeDir() + "shaders/skyboxFrag.spv";

	VkInstance instance;

	// validation layers callback
	VkDebugReportCallbackEXT callback;

	// surface object for presentation
	VkSurfaceKHR surface;

	// logical and physical device
	Device *pDevice;

	// swapchain object and its images
	SwapChain *pSwapChain;

	// resources for main graphics pipeline
	DescriptorSet *pMainDS;

	// resources for skybox graphics pipeline
	DescriptorSet *pSkyboxDS;

	RenderPass *pRenderPass;

	// graphics pipelines for rendering main objects
	GraphicsPipeline *pMainPipeline;

	// graphics pipeline for rendering skybox
	GraphicsPipeline *pSkyboxPipeline;

	std::vector<VkCommandBuffer> graphicCommands;

	// synchronizing objects
	VkSemaphore imageAvailable = VK_NULL_HANDLE;
	VkSemaphore renderingFinished = VK_NULL_HANDLE;

	// camera attributes
	Camera *pCamera;

	// timer for animations
	Timer frameTimer;

	// resources:

	TextureImage *pEarthTexture;		// texture of earth surface
	TextureImage *pEarthNormalMap;		// map of earth normals
	TextureImage *pEarthSpecularMap;	// map of specular factor
	Model *pEarthModel;			// model of earth

	CubeTextureImage *pSkyboxTexture;	// texture of sky
	Shape *pSkyboxModel;			// model of skybox

	Buffer *pEarthMvpBuffer;  // buffer containing MVP(model, view, projection) matrices for earth model
	MvpMatrices earthMvp;

	Buffer *pLightingBuffer;  // buffer containing lighting attributes
	Lighting lighting;

	Buffer *pSkyboxMvpBuffer;  // buffer containing MVP(model, view, projection) matrices for skybox model
	glm::mat4 skyboxMvp;

	void createInstance();

	static bool checkInstanceLayerSupport(std::vector<const char *> requiredLayers);
	static bool checkInstanceExtensionSupport(std::vector<const char *> requiredExtensions);

	std::vector<const char *> getRequiredExtensions();

	// functions from extensions (EXT) need to get before use
	static VkResult vkCreateDebugReportCallbackEXT(
		VkInstance instance,
		const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
		const VkAllocationCallbacks *pAllocator,
		VkDebugReportCallbackEXT *pCallback
	);
	static void vkDestroyDebugReportCallbackEXT(
		VkInstance instance,
		VkDebugReportCallbackEXT callback,
		const VkAllocationCallbacks *pAllocator
	);

	void createDebugCallback();

	void createSurface(GLFWwindow *window);

	void initCamera();

	void initEarthMvpMatrices();

	void initLighting();

	void initSkyboxMvpMatrices();

	// creates textures, buffers, models, for main pipeline 
	// and adds it to main DS
	void initMainDS();

	// creates textures and models for skybox pipeline 
	// and adds it to skybox DS
	void initSkyboxDS();

	// initialize rendering commands
	void initGraphicCommands();

	static void createSemaphore(VkDevice device, VkSemaphore& semaphore);

	// changes mvp and load it in buffer
	void updateMvpBuffers(float deltaSec);
};

