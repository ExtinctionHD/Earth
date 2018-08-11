#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#include "VkDeleter.h"

class Vulkan
{
public:
	void init(GLFWwindow *window);

private:
	// ���� �������� Vulkan
	const std::vector<const char *> validationLayers =
	{
		"VK_LAYER_LUNARG_standard_validation"  // ����� ����������� �����
	};

#ifdef _DEBUG  // ���� ������� ���������� ������ � ���������� ������
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif

	VkDeleter<VkInstance> instance{ vkDestroyInstance };  // �������� Vulkan

	// callback ����� ��������
	VkDeleter<VkDebugReportCallbackEXT> callback{ instance, vkDestroyDebugReportCallbackEXT };

	// ������ ����������� ��� �����������
	VkDeleter<VkSurfaceKHR> surface{ instance, vkDestroySurfaceKHR };

	void createInstance();  // �������� ����������

	// �������� ��������� ����������� ����� � ����������
	static bool checkInstanceLayerSupport(std::vector<const char *> requiredLayers);
	static bool checkInstanceExtensionSupport(std::vector<const char *> requiredExtensions);

	std::vector<const char *> getRequiredExtensions();  // ��������� ����������� ����������

	// ������� �� ���������� (EXT) ���������� ��������� ����� ��������������
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

	void createDebugCallback();  // �������� ��������� ������ ����� ��������

	void createSurface(GLFWwindow *window);  // �������� ����������� ������ ����
};

