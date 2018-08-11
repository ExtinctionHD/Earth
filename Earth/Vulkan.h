#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>

#include "VkDeleter.h"

class Vulkan
{
public:
	void init();

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
};

