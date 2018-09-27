#include <set>
#include "Logger.h"
#include <array>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ShaderModule.h"

#include "Vulkan.h"

// public:

Vulkan::Vulkan(GLFWwindow *window, VkExtent2D frameExtent)
{
	glfwSetWindowUserPointer(window, this);

	createInstance();
	createDebugCallback();
	createSurface(window);
	
	std::vector<const char*> requiredLayers = ENABLE_VALIDATION_LAYERS ? 
		VALIDATION_LAYERS : std::vector<const char*>();

	pDevice = new Device(instance, surface, requiredLayers);
	pSwapChain = new SwapChain(pDevice, surface, frameExtent);

	pRenderPass = new RenderPass(pDevice, pSwapChain);

	const uint32_t binding = 0;

	// create DS and pipeline for rendering main objects:
	pMainDS = new DescriptorSet(pDevice);
	initMainDS();

	std::vector<ShaderModule*> mainShaderModules{
		new ShaderModule(pDevice->device, MAIN_VERT_SHADER_PATH, VK_SHADER_STAGE_VERTEX_BIT),
		new ShaderModule(pDevice->device, MAIN_FRAG_SHADER_PATH, VK_SHADER_STAGE_FRAGMENT_BIT),
	};
	pMainPipeline = new GraphicsPipeline(
		pDevice->device, 
		{ pMainDS->layout }, 
		pRenderPass, 
		mainShaderModules,
		Vertex::getBindingDescription(binding),
		Vertex::getAttributeDescriptions(binding)
	);

	// create DS and pipeline for rendering skybox
	pSkyboxDS = new DescriptorSet(pDevice);
	initSkyboxDS();

	std::vector<ShaderModule*> skyboxShaderModules{
		new ShaderModule(pDevice->device, SKYBOX_VERT_SHADER_PATH, VK_SHADER_STAGE_VERTEX_BIT),
		new ShaderModule(pDevice->device, SKYBOX_FRAG_SHADER_PATH, VK_SHADER_STAGE_FRAGMENT_BIT),
	};
	pSkyboxPipeline = new GraphicsPipeline(
		pDevice->device,
		{ pSkyboxDS->layout },
		pRenderPass,
		skyboxShaderModules,
		Position::getBindingDescription(binding),
		Position::getAttributeDescriptions(binding)
	);

	initGraphicCommands();

	createSemaphore(pDevice->device, imageAvailable);
	createSemaphore(pDevice->device, renderingFinished);
}

Vulkan::~Vulkan()
{
	vkDeviceWaitIdle(pDevice->device);

	vkDestroySemaphore(pDevice->device, imageAvailable, nullptr);
	vkDestroySemaphore(pDevice->device, renderingFinished, nullptr);

	delete(pCamera);
	delete(pMainPipeline);
	delete(pMainDS);
	delete(pSkyboxDS);
	delete(pSwapChain);
	delete(pDevice);

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDebugReportCallbackEXT(instance, callback, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void Vulkan::drawFrame()
{
	// update resources
	float deltaSec = frameTimer.getDeltaSec();
	pCamera->moveCamera(deltaSec);
	updateMvpBuffers(deltaSec);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		pDevice->device,
		pSwapChain->swapChain,
		(std::numeric_limits<uint64_t>::max)(),
		imageAvailable,
		VK_NULL_HANDLE,
		&imageIndex
	);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		resize(pSwapChain->extent);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		LOGGER_FATAL(Logger::FAILED_TO_ACQUIRE_NEXT_FRAME);
	}

	std::vector<VkSemaphore> waitSemaphores{ imageAvailable };
	std::vector<VkPipelineStageFlags> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> signalSemaphores{ renderingFinished };
	VkSubmitInfo submitInfo{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,	// sType;
		nullptr,						// pNext;
		waitSemaphores.size(),			// waitSemaphoreCount;
		waitSemaphores.data(),			// pWaitSemaphores;
		waitStages.data(),				// pWaitDstStageMask;
		1,								// commandBufferCount;
		&graphicCommands[imageIndex],	// pCommandBuffers;
		signalSemaphores.size(),		// signalSemaphoreCount;
		signalSemaphores.data(),		// pSignalSemaphores;
	};

	result = vkQueueSubmit(pDevice->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS)
	{
		LOGGER_FATAL(Logger::FAILED_TO_SUBMIT_COMMANDS);
	}

	std::vector<VkSwapchainKHR> swapChains{ pSwapChain->swapChain };
	VkPresentInfoKHR presentInfo{
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,	// sType;
		nullptr,							// pNext;
		signalSemaphores.size(),			// waitSemaphoreCount;
		signalSemaphores.data(),			// pWaitSemaphores;
		swapChains.size(),					// swapchainCount;
		swapChains.data(),					// pSwapchains;
		&imageIndex,						// pImageIndices;
		nullptr,							// pResults;
	};

	result = vkQueuePresentKHR(pDevice->presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		resize(pSwapChain->extent);
		return;
	}
	else if (result != VK_SUCCESS)
	{
		LOGGER_FATAL(Logger::FAILED_TO_PRESENT_FRAME);
	}
}

void Vulkan::resize(VkExtent2D newExtent)
{
	vkDeviceWaitIdle(pDevice->device);

	delete(pRenderPass);
	delete(pSwapChain);

	pSwapChain = new SwapChain(pDevice, surface, newExtent);
	pRenderPass = new RenderPass(pDevice, pSwapChain);
	pMainPipeline->recreate(pRenderPass);
	pSkyboxPipeline->recreate(pRenderPass);

	initGraphicCommands();

	// TODO: add change camera extent func
	initCamera();
}

void Vulkan::onKeyAction(int key, int action)
{
	if (action == GLFW_PRESS)
	{
		pCamera->onKeyDown(key);
	}

	if (action == GLFW_RELEASE)
	{
		pCamera->onKeyUp(key);
	}
}

void Vulkan::onMouseMove(float x, float y)
{
	pCamera->onMouseMove(x, y);
}

// private:

void Vulkan::createInstance()
{
	// validation layers
	Logger::infoValidationLayers(ENABLE_VALIDATION_LAYERS);
	if (ENABLE_VALIDATION_LAYERS)
	{
		if (!checkInstanceLayerSupport(VALIDATION_LAYERS))
		{
			LOGGER_FATAL(Logger::VALIDATION_LAYERS_NOT_AVAILABLE);
		}
	}

	// required extenstions
	std::vector<const char *> extensions = getRequiredExtensions();
	if (!checkInstanceExtensionSupport(extensions))
	{
		LOGGER_FATAL(Logger::INSTANCE_EXTENSIONS_NOT_AVAILABLE);
	}

	// infoabout application for vulkan
	VkApplicationInfo appInfo =
	{
		VK_STRUCTURE_TYPE_APPLICATION_INFO,	// sType
		nullptr,							// pNext
		"Earth",							// pApplicationName
		VK_MAKE_VERSION(1, 0, 0),			// applicationVersion
		"No Engine",						// pEngineName
		VK_MAKE_VERSION(1, 0, 0),			// engineVersion
		VK_API_VERSION_1_0					// apiVersion
	};

	// info for vulkan instance
	VkInstanceCreateInfo createInfo =
	{
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,	// sType
		nullptr,								// pNext
		0,										// flags
		&appInfo,								// pApplicationInfo
		0,										// enabledLayerCount
		nullptr,								// ppEnabledLayerNames
		extensions.size(),						// enabledExtensionCount
		extensions.data()						// ppEnabledExtensionNames
	};

	// add validation layers if they are enabled
	if (ENABLE_VALIDATION_LAYERS)
	{
		createInfo.enabledLayerCount = VALIDATION_LAYERS.size();
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (result != VK_SUCCESS)
	{
		LOGGER_FATAL(Logger::FAILED_TO_CREATE_INSTANCE);
	}
}

bool Vulkan::checkInstanceLayerSupport(std::vector<const char*> requiredLayers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);  // get count

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());  // get layers

	std::set<std::string> requiredLayerSet(requiredLayers.begin(), requiredLayers.end());

	for (const auto& layer : availableLayers)
	{
		requiredLayerSet.erase(layer.layerName);
	}

	// empty if all required layers are supported by instance
	return requiredLayerSet.empty();
}

bool Vulkan::checkInstanceExtensionSupport(std::vector<const char*> requiredExtensions)
{
	uint32_t extensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);  // get count

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());  // get extensions

	std::set<std::string> requiredExtensionSet(requiredExtensions.begin(), requiredExtensions.end());

	for (const auto& layer : availableExtensions)
	{
		requiredExtensionSet.erase(layer.extensionName);
	}

	// empty if all required extensions are supported by instance
	return requiredExtensionSet.empty();
}

std::vector<const char*> Vulkan::getRequiredExtensions()
{
	std::vector<const char*> extensions;

	// glfw extensions, at least VK_KHR_surface
	unsigned int glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (int i = 0; i < glfwExtensionCount; i++)
	{
		extensions.push_back(glfwExtensions[i]);
	}

	// extension for validation layers callback
	if (ENABLE_VALIDATION_LAYERS)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

VkResult Vulkan::vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugReportCallbackEXT * pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
		instance, "vkCreateDebugReportCallbackEXT"
	);

	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Vulkan::vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks * pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

void Vulkan::createDebugCallback()
{
	// don't need callback if validation layers are not enabled
	if (!ENABLE_VALIDATION_LAYERS)
	{
		return;
	}

	VkDebugReportCallbackCreateInfoEXT createInfo = 
	{
		VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,			// sType
		nullptr,															// pNext
		VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,	// flags
		Logger::validationLayerCallback,									// pfnCallback
		nullptr																// pUserData
	};

	VkResult result = vkCreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback);
	if (result != VK_SUCCESS)
	{
		LOGGER_FATAL(Logger::FAILED_TO_CREATE_CALLBACK);
	}
}

void Vulkan::createSurface(GLFWwindow *window)
{
	// glfw library create surface by it self
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (result != VK_SUCCESS)
	{
		LOGGER_FATAL(Logger::FAILED_TO_CREATE_SURFACE);
	}
}

void Vulkan::initCamera()
{
	if (pCamera != nullptr)
	{
		delete(pCamera);
	}

	glm::vec3 pos{ 0.0f, 0.0f, -3.0f };
	glm::vec3 forward{ 0.0f, 0.0f, 1.0f };
	glm::vec3 up{ 0.0f, -1.0f, 0.0f };

	pCamera = new Camera(pos, forward, up, pSwapChain->extent);
}

void Vulkan::initLighting()
{
	lighting = Lighting{
		glm::vec3(1.0f, 1.0f, 1.0f),		// color
		0.025f,								// ambientStrength
		glm::vec3(-0.89f, 0.4f, -0.21f),	// direction
		1.0f,								// diffuseStrength
		pCamera->getPos(),					// cameraPos
		2.0f								// specularPower
	};
}

void Vulkan::initEarthMvpMatrices()
{
	// attributes for view matrix
	const glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
	const glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);

	// attributes for projection matrix
	const float viewAngle = 45.0f;
	const float zNear = 0.1f;
	const float zFar = 50.0f;

	earthMvp = MvpMatrices{
		glm::mat4(1),
		glm::lookAt(pCamera->getPos(), pCamera->getTarget(), pCamera->getUp()),
		glm::perspective(glm::radians(viewAngle), pSwapChain->getAspect(), zNear, zFar)
	};

	earthMvp.model = glm::rotate(earthMvp.model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));

}

void Vulkan::initMainDS()
{
	// create models:

	const std::string EARTH_MODEL_PATH = File::getExeDir() + "models/sphere.obj";

	pEarthModel = new Model(pDevice, EARTH_MODEL_PATH);
	pEarthModel->normilize();

	//create buffers:

	initCamera();

	pLightingBuffer = new Buffer(pDevice, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(lighting));
	initLighting();
	pLightingBuffer->updateData(&lighting, sizeof(lighting), 0);

	pEarthMvpBuffer = new Buffer(pDevice, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHADER_STAGE_VERTEX_BIT, sizeof(earthMvp));
	initEarthMvpMatrices();
	pEarthMvpBuffer->updateData(&earthMvp, sizeof(earthMvp), 0);

	// create textures:

	const std::string EARTH_TEXTURE_PATH = File::getExeDir() + "textures/earth_texture.jpg";
	const std::string EARTH_NORMAL_MAP_PATH = File::getExeDir() + "textures/earth_normal_map.jpg";
	const std::string EARTH_SPECULAR_MAP_PATH = File::getExeDir() + "textures/earth_specular_map.jpg";

	pEarthTexture = new TextureImage(pDevice, { EARTH_TEXTURE_PATH }, 1);
	pEarthNormalMap = new TextureImage(pDevice, { EARTH_NORMAL_MAP_PATH }, 1);
	pEarthSpecularMap = new TextureImage(pDevice, { EARTH_SPECULAR_MAP_PATH }, 1);

	// load resources in descriptor set:

	pMainDS->addMesh(pEarthModel);

	pMainDS->addBuffer(pEarthMvpBuffer);
	pMainDS->addBuffer(pLightingBuffer);

	pMainDS->addTexture(pEarthTexture);
	pMainDS->addTexture(pEarthNormalMap);
	pMainDS->addTexture(pEarthSpecularMap);

	// save changes in descriptor set
	pMainDS->update();
}

void Vulkan::initSkyboxMvpMatrices()
{
	skyboxMvp = earthMvp.proj * earthMvp.view * glm::translate(glm::mat4(1), pCamera->getPos());
}

void Vulkan::initSkyboxDS()
{
	// create models:

	std::vector<Position> cubeVertices{
		glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3(-1.0f, 1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),
		glm::vec3(1.0f, 1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(-1.0f, -1.0f, 1.0f),
		glm::vec3(-1.0f, 1.0f, 1.0f)
	};
	std::vector<uint32_t> cubeIndices{
		0, 1, 2,
		2, 1, 3,
		2, 3, 4,
		4, 3, 5,
		4, 5, 6,
		6, 5, 7,
		6, 7, 0,
		0, 7, 1,
		6, 0, 2,
		2, 4, 6,
		7, 5, 3,
		7, 3, 1
	};

	pSkyboxModel = new Shape(pDevice, cubeVertices, cubeIndices);

	// create buffers:

	pSkyboxMvpBuffer = new Buffer(pDevice, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_SHADER_STAGE_VERTEX_BIT, sizeof(skyboxMvp));
	initSkyboxMvpMatrices();
	pSkyboxMvpBuffer->updateData(&skyboxMvp, sizeof(skyboxMvp), 0);

	// create textures:

	const std::array<std::string, CubeTextureImage::CUBE_SIDE_COUNT> SKYBOX_TEXTURE_PATHES{
		File::getExeDir() + "textures/skybox1/right.jpg",
		File::getExeDir() + "textures/skybox1/left.jpg",
		File::getExeDir() + "textures/skybox1/top.jpg",
		File::getExeDir() + "textures/skybox1/bottom.jpg",
		File::getExeDir() + "textures/skybox1/front.jpg",
		File::getExeDir() + "textures/skybox1/back.jpg"
	};

	pSkyboxTexture = new CubeTextureImage(pDevice, SKYBOX_TEXTURE_PATHES);

	// load in descriptor set

	pSkyboxDS->addMesh(pSkyboxModel);
	pSkyboxDS->addBuffer(pSkyboxMvpBuffer);
	pSkyboxDS->addTexture(pSkyboxTexture);

	pSkyboxDS->update();
}

void Vulkan::initGraphicCommands()
{
	// return old command buffers to pool
	if (!graphicCommands.empty())
	{
		vkFreeCommandBuffers(pDevice->device, pDevice->commandPool, graphicCommands.size(), graphicCommands.data());
	}

	graphicCommands.resize(pSwapChain->imageCount);

	VkCommandBufferAllocateInfo allocInfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	// sType;
		nullptr,										// pNext;
		pDevice->commandPool,							// commandPool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,				// level;
		graphicCommands.size(),							// commandBufferCount;
	};

	VkResult result = vkAllocateCommandBuffers(pDevice->device, &allocInfo, graphicCommands.data());
	if (result != VK_SUCCESS)
	{
		LOGGER_FATAL(Logger::FAILED_TO_ALLOC_COMMAND_BUFFERS);
	}

	// clear values for each frame
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = clearColor;
	clearValues[1].depthStencil = { 1, 0 };

	// render area for each frame
	VkRect2D renderArea{
		{ 0, 0 },			// offset
		pSwapChain->extent	// extent
	};

	for (int i = 0; i < graphicCommands.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// sType;
			nullptr,										// pNext;
			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,	// flags;
			nullptr,										// pInheritanceInfo;
		};

		result = vkBeginCommandBuffer(graphicCommands[i], &beginInfo);
		if (result != VK_SUCCESS)
		{
			LOGGER_FATAL(Logger::FAILED_TO_BEGIN_COMMAND_BUFFER);
		}

		VkRenderPassBeginInfo renderPassBeginInfo{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// sType;
			nullptr,									// pNext;
			pRenderPass->renderPass,					// renderPass;
			pRenderPass->framebuffers[i],				// framebuffer;
			renderArea,									// renderArea;
			clearValues.size(),							// clearValueCount;
			clearValues.data()							// pClearValues;
		};

		vkCmdBeginRenderPass(graphicCommands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// draw earth model
		vkCmdBindPipeline(graphicCommands[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pMainPipeline->pipeline);
		pMainDS->bind(graphicCommands[i], pMainPipeline->layout);
		pMainDS->drawMeshes(graphicCommands[i]);

		// draw skybox
		vkCmdBindPipeline(graphicCommands[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pSkyboxPipeline->pipeline);
		pSkyboxDS->bind(graphicCommands[i], pSkyboxPipeline->layout);
		pSkyboxDS->drawMeshes(graphicCommands[i]);

		vkCmdEndRenderPass(graphicCommands[i]);

		result = vkEndCommandBuffer(graphicCommands[i]);
		if (result != VK_SUCCESS)
		{
			LOGGER_FATAL(Logger::FAILED_TO_END_COMMAND_BUFFER);
		}
	}
}

void Vulkan::createSemaphore(VkDevice device, VkSemaphore& semaphore)
{
	if (semaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(device, semaphore, nullptr);
	}

	VkSemaphoreCreateInfo createInfo{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,	// sType;
		nullptr,									// pNext;
		0,											// flags;
	};

	VkResult result = vkCreateSemaphore(device, &createInfo, nullptr, &semaphore);
	if (result != VK_SUCCESS)
	{
		LOGGER_FATAL(Logger::FAILED_TO_CREATE_SEMAPHORE);
	}
}

void Vulkan::updateMvpBuffers(float deltaSec)
{
	earthMvp.model = glm::rotate(earthMvp.model, glm::radians(30.0f) * deltaSec, glm::vec3(0.0f, 1.0f, 0.0f));

	earthMvp.view = glm::lookAt(pCamera->getPos(), pCamera->getTarget(), pCamera->getUp());

	// init projection matrix
	const float fov = 55.0f;
	const float zNear = 0.1f;
	const float zFar = 50.0f;
	earthMvp.proj = glm::perspective(glm::radians(fov), pSwapChain->getAspect(), zNear, zFar);

	pEarthMvpBuffer->updateData(&earthMvp, sizeof(earthMvp), 0);

	initSkyboxMvpMatrices();
	pSkyboxMvpBuffer->updateData(&skyboxMvp, sizeof(skyboxMvp), 0);

	lighting.cameraPos = pCamera->getPos();
	pLightingBuffer->updateData(&lighting.cameraPos, sizeof(lighting.cameraPos), offsetof(Lighting, cameraPos));
}





