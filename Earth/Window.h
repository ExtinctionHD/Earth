#pragma once

#include <GLFW/glfw3.h>

class Window
{
public:
	int width = 1280;
	int height = 720;

	void init();  // ������������� ���������� � �������� ����

	void mainLoop();  // ���� ������ ����������

	operator GLFWwindow*();  // ���������� � ����������� ����
private:
	GLFWwindow * window;  // ���������� ����
};

