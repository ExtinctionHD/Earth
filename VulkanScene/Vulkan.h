#pragma once

#include <vector>
#include "Instance.h"
#include "Surface.h"
#include "Device.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "Scene.h"
#include "DescriptorPool.h"
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

// graphic API class that create all necessary objects
// and set this as window user pointer
class Vulkan
{
public:
	// create all required objects
	Vulkan(HINSTANCE hInstance, HWND hWnd, VkExtent2D frameExtent);

	// destroy objects: pSurface, callback, instance
	~Vulkan();

	bool minimized = false;

	// executes graphics commands and present result image on window pSurface
	void drawFrame();

	// rebuild swapchain and all dependent objects for new extension
	void resize(VkExtent2D newExtent);

	void keyDownCallback(int key);

	void keyUpCallback(int key);

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

	const std::vector<const char *> EXTENTIONS = 
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		"VK_KHR_win32_surface"
	};

	// color that clear each frame
	const VkClearColorValue CLEAR_COLOR = {{0.0f, 0.0f, 0.0f, 1.0f}};

	Instance *pInstance;

	// pSurface object for presentation
	Surface *pSurface;

	// logical and physical device
	Device *pDevice;

	// swapchain object and its images
	SwapChain *pSwapChain;

	RenderPassesMap renderPasses;

	// drawing scene
	Scene *pScene;

	DescriptorPool *pDescriptorPool;

	std::vector<VkCommandBuffer> graphicCommands;

	// synchronizing objects
	VkSemaphore imageAvailable = VK_NULL_HANDLE;
	VkSemaphore renderingFinished = VK_NULL_HANDLE;

	void createRenderPasses();

	// initialize rendering commands
	void initGraphicsCommands();

	void beginDepthRenderPass(VkCommandBuffer commandBuffer);

	void beginGeometryRenderPass(VkCommandBuffer commandBuffer);

	void beginLightingRenderPass(VkCommandBuffer commandBuffer, uint32_t index);

	void beginFinalRenderPass(VkCommandBuffer commandBuffer, uint32_t index);

	static void createSemaphore(VkDevice device, VkSemaphore& semaphore);
};

