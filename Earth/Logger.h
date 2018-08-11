#pragma once

#include <iostream>
#include <vulkan/vulkan.h>

// ����������� ������
#define LOGGER_FATAL(msg)														\
{																				\
	std::string message = (msg);												\
	Logger::fatal(Logger::VALIDATION_LAYERS_NOT_AVAILABLE, __FILE__, __LINE__);	\
}

class Logger
{
public:
	static const std::string VALIDATION_LAYERS_NOT_AVAILABLE;
	static const std::string INSTANCE_EXTENSIONS_NOT_AVAILABLE;
	static const std::string FAILED_TO_CREATE_INSTANCE;
	static const std::string FAILED_TO_CREATE_CALLBACK;
	static const std::string FAILED_TO_CREATE_SURFACE;

	static void infoValidationLayers(bool enabled);  // ����� ���������� � ����� ��������

	static void fatal(std::string message, std::string file, int line);  // ����������� ������

	// ������� ��� ������ ��������� ����� ��������
	static VKAPI_ATTR VkBool32 VKAPI_CALL validationLayerCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char *layerPrefix,
		const char *msg,
		void *userData
	);

private:
	static void printInfo();

	static std::string getFilename(std::string path);  // ��������� ����� �� ������� ����
};

