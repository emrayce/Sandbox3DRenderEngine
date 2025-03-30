#include "Window.h"

#include <iostream>

Window::Window()
{
	mainWindow = nullptr;
	width = 800;
	height = 600;
	bufferWidth = 0;
	bufferHeight = 0;
	lastX = 0;
	lastY = 0;
	xChange = 0;
	yChange = 0;
	mouseFirstMoved = true;

	for (size_t i = 0; i < 1024; ++i)
	{
		keys[i] = 0;
	}
}

Window::Window(GLint windowWidth, GLint windowHeight)
{
	mainWindow = nullptr;
	width = windowWidth;
	height = windowHeight;
	bufferWidth = 0;
	bufferHeight = 0;
	lastX = 0;
	lastY = 0;
	xChange = 0;
	yChange = 0;
	mouseFirstMoved = true;


	for (size_t i = 0; i < 1024; ++i)
	{
		keys[i] = 0;
	}
}

int Window::Initialise()
{
	// Initialise GLFW
	if (!glfwInit())
	{
		std::cout << "GLFW Initialisation failed!" << std::endl;
		glfwTerminate();
		return 1;
	}

	// Setup GLFW window properties
	// OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Core profile = No backwards Compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Allow forard comaptibility
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

	mainWindow = glfwCreateWindow(width, height, "Test Window", NULL, NULL);
	if (!mainWindow)
	{
		std::cout << "GLFW window creation failed!" << std::endl;
		glfwTerminate();
		return 1;
	}

	// Get Buffer size information
	glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);

	// Set context for GLEW to use
	glfwMakeContextCurrent(mainWindow);

	// Handle key + mouse input
	CreateCallbacks();
	glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Allow modern extension features
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		std::cout << "GLEW initialisation failed!" << std::endl;

		glfwDestroyWindow(mainWindow);
		glfwTerminate();
		return 1;
	}

	// Enable depth test to determine which triangle are behind other
	glEnable(GL_DEPTH_TEST);

	// Setup Viewport size
	glViewport(0, 0, bufferWidth, bufferHeight);

	glfwSetWindowUserPointer(mainWindow, this);

	return 0;
}

void Window::CreateCallbacks()
{
	glfwSetKeyCallback(mainWindow, HandleKeys);
	glfwSetCursorPosCallback(mainWindow, HandleMouse);
}

GLfloat Window::getXChange()
{
	GLfloat theChange = xChange;
	xChange = 0.0f;
	return theChange;
}

GLfloat Window::getYChange()
{
	GLfloat theChange = yChange;
	yChange = 0.0f;
	return theChange;
}

void Window::HandleKeys(GLFWwindow* window, int key, int code, int action, int mode)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			theWindow->keys[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			theWindow->keys[key] = false;
		}
	}
}

void Window::HandleMouse(GLFWwindow* window, double xPos, double yPos)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (theWindow->mouseFirstMoved)
	{
		theWindow->lastX = xPos;
		theWindow->lastY = yPos;
		theWindow->mouseFirstMoved = false;
	}

	theWindow->xChange = xPos - theWindow->lastX;
	theWindow->yChange = theWindow->lastY - yPos;

	theWindow->lastX = xPos;
	theWindow->lastY = yPos;
}


Window::~Window()
{
	glfwDestroyWindow(mainWindow);
	glfwTerminate();
}