#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "Vulkan.h"

class Window
{
public:
	GLFWwindow * window;  // window descriptor

	Window();  // initialize library and create window

	~Window();  // detroy window

	void mainLoop();

	VkExtent2D getFrameExtent() const;  // window width and height in VKExtent2D structure

private:
	// initial window extent
	const int baseWidth = 1280;
	const int baseHeight = 720;

	static Vulkan* getVulkanPointer(GLFWwindow *window);

	static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
};

