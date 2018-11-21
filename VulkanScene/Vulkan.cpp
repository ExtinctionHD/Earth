#include <set>
#include <array>
#include "ShaderModule.h"
#include "AssimpModel.h"
#include "FinalRenderPass.h"
#include "DepthRenderPass.h"
#include "GeometryRenderPass.h"
#include "LightingRenderPass.h"

#include "Vulkan.h"

// public:

Vulkan::Vulkan(HINSTANCE hInstance, HWND hWnd, VkExtent2D frameExtent)
{
	std::vector<const char*> requiredLayers = ENABLE_VALIDATION_LAYERS ?
		VALIDATION_LAYERS : std::vector<const char*>();

	pInstance = new Instance(requiredLayers, EXTENTIONS);
	pSurface = new Surface(pInstance->getInstance(),hInstance, hWnd);
	pDevice = new Device(pInstance->getInstance(), pSurface->getSurface(), requiredLayers);
	pSwapChain = new SwapChain(pDevice, pSurface->getSurface(), frameExtent);

	createRenderPasses();

	pScene = new Scene(pDevice, pSwapChain->getExtent());
	pDescriptorPool = new DescriptorPool(pDevice, pScene->getBufferCount(), pScene->getTextureCount(), pScene->getDescriptorSetCount());

	pScene->prepareSceneRendering(pDescriptorPool, renderPasses);

	initGraphicsCommands();

	createSemaphore(pDevice->device, imageAvailable);
	createSemaphore(pDevice->device, renderingFinished);
}

Vulkan::~Vulkan()
{
	vkDeviceWaitIdle(pDevice->device);

	vkDestroySemaphore(pDevice->device, imageAvailable, nullptr);
	vkDestroySemaphore(pDevice->device, renderingFinished, nullptr);

	delete(pScene);
	delete(pDescriptorPool);
    for (auto renderPass : renderPasses)
    {
		delete renderPass.second;
    }
	delete(pSwapChain);
	delete(pDevice);
	delete(pSurface);
	delete(pInstance);
}

void Vulkan::drawFrame()
{
	pScene->updateScene();

	if (minimized)
	{
		return;
	}

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		pDevice->device,
		pSwapChain->getSwapchain(),
		(std::numeric_limits<uint64_t>::max)(),
		imageAvailable,
		VK_NULL_HANDLE,
		&imageIndex
	);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		resize(pSwapChain->getExtent());
		return;
	}

    assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

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
	assert(result == VK_SUCCESS);

	std::vector<VkSwapchainKHR> swapChains{ pSwapChain->getSwapchain() };
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
		resize(pSwapChain->getExtent());
	}
	else
	{
		assert(result == VK_SUCCESS);
	}
}

void Vulkan::resize(VkExtent2D newExtent)
{
	if (minimized)
	{
		return;
	}

	vkDeviceWaitIdle(pDevice->device);

	pSwapChain->recreate(newExtent);

	renderPasses.at(FINAL)->recreate(pSwapChain->getExtent());
	renderPasses.at(GEOMETRY)->recreate(pSwapChain->getExtent());
	renderPasses.at(LIGHTING)->recreate(pSwapChain->getExtent());

	pScene->resizeExtent(pSwapChain->getExtent());

	initGraphicsCommands();
}

void Vulkan::keyDownCallback(int key)
{
	pScene->getController()->keyDownCallback(key);
}

void Vulkan::keyUpCallback(int key)
{
	pScene->getController()->keyUpCallback(key);
}

// private:

void Vulkan::createRenderPasses()
{
	const VkExtent2D DEPTH_MAP_EXTENT = { 4096, 4096 };

	renderPasses.insert({ DEPTH, new DepthRenderPass(pDevice, DEPTH_MAP_EXTENT) });
	renderPasses.insert({ GEOMETRY, new GeometryRenderPass(pDevice, pSwapChain->getExtent()) });
	renderPasses.insert({ LIGHTING, new LightingRenderPass(pDevice, pSwapChain) });
	renderPasses.insert({ FINAL, new FinalRenderPass(pDevice, pSwapChain) });

    for (auto renderPass : renderPasses)
    {
		renderPass.second->create();
    }
}

void Vulkan::initGraphicsCommands()
{
	// return old command buffers to pool
	if (!graphicCommands.empty())
	{
		vkFreeCommandBuffers(pDevice->device, pDevice->commandPool, graphicCommands.size(), graphicCommands.data());
	}

	graphicCommands.resize(pSwapChain->getImageCount());

	VkCommandBufferAllocateInfo allocInfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	// sType;
		nullptr,										// pNext;
		pDevice->commandPool,							// commandPool;
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,				// level;
		graphicCommands.size(),							// commandBufferCount;
	};

	VkResult result = vkAllocateCommandBuffers(pDevice->device, &allocInfo, graphicCommands.data());
	assert(result == VK_SUCCESS);

	for (size_t i = 0; i < graphicCommands.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// sType;
			nullptr,										// pNext;
			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,	// flags;
			nullptr,										// pInheritanceInfo;
		};

		result = vkBeginCommandBuffer(graphicCommands[i], &beginInfo);
		assert(result == VK_SUCCESS);

		beginDepthRenderPass(graphicCommands[i]);

		beginGeometryRenderPass(graphicCommands[i]);

		beginLightingRenderPass(graphicCommands[i], i);

		result = vkEndCommandBuffer(graphicCommands[i]);
		assert(result == VK_SUCCESS);
	}
}

void Vulkan::beginDepthRenderPass(VkCommandBuffer commandBuffer)
{
	VkClearValue clearValue;
	clearValue.depthStencil = { 1.0f, 0 };

	VkRect2D renderArea{
		{ 0, 0 },                           // offset
		renderPasses.at(DEPTH)->getExtent() // extent
	};

	VkRenderPassBeginInfo renderPassBeginInfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	    // sType;
		nullptr,									    // pNext;
		renderPasses.at(DEPTH)->getRenderPass(),		// renderPass;
		renderPasses.at(DEPTH)->getFramebuffers()[0],   // framebuffer;
		renderArea,									    // renderArea;
		1,							                    // clearValueCount;
		&clearValue							            // pClearValues;
	};

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	pScene->renderDepth(commandBuffer);

	vkCmdEndRenderPass(commandBuffer);
}

void Vulkan::beginGeometryRenderPass(VkCommandBuffer commandBuffer)
{
	std::array<VkClearValue, 4> clearValues{};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[3].depthStencil = { 1.0f, 0 };

	VkRect2D renderArea{
		{ 0, 0 },                               // offset
		renderPasses.at(GEOMETRY)->getExtent()  // extent
	};

	VkRenderPassBeginInfo renderPassBeginInfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,           // sType;
		nullptr,                                            // pNext;
		renderPasses.at(GEOMETRY)->getRenderPass(),         // renderPass;
		renderPasses.at(GEOMETRY)->getFramebuffers()[0],    // framebuffer;
		renderArea,                                         // renderArea;
		clearValues.size(),                                 // clearValueCount;
		clearValues.data()                                  // pClearValues;
	};

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	pScene->renderGeometry(commandBuffer);

	vkCmdEndRenderPass(commandBuffer);
}

void Vulkan::beginLightingRenderPass(VkCommandBuffer commandBuffer, uint32_t index)
{
	std::array<VkClearValue, 1> clearValues{};
	clearValues[0].color = CLEAR_COLOR;

	VkRect2D renderArea{
		{ 0, 0 },                               // offset
		renderPasses.at(LIGHTING)->getExtent()  // extent
	};

	VkRenderPassBeginInfo renderPassBeginInfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,               // sType;
		nullptr,                                                // pNext;
		renderPasses.at(LIGHTING)->getRenderPass(),             // renderPass;
		renderPasses.at(LIGHTING)->getFramebuffers()[index],    // framebuffer;
		renderArea,                                             // renderArea;
		clearValues.size(),                                     // clearValueCount;
		clearValues.data()                                      // pClearValues;
	};

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	pScene->renderLighting(commandBuffer);

	vkCmdEndRenderPass(commandBuffer);
}

void Vulkan::beginFinalRenderPass(VkCommandBuffer commandBuffer, uint32_t index)
{
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = CLEAR_COLOR;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRect2D renderArea{
		{ 0, 0 },                           // offset
		renderPasses.at(FINAL)->getExtent() // extent
	};

	VkRenderPassBeginInfo renderPassBeginInfo{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,           // sType;
		nullptr,                                            // pNext;
		renderPasses.at(FINAL)->getRenderPass(),            // renderPass;
		renderPasses.at(FINAL)->getFramebuffers()[index],   // framebuffer;
		renderArea,                                         // renderArea;
		clearValues.size(),                                 // clearValueCount;
		clearValues.data()                                  // pClearValues;
	};

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	pScene->renderFinal(commandBuffer);

	vkCmdEndRenderPass(commandBuffer);
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
	assert(result == VK_SUCCESS);
}





