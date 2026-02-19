/////////////////////////////////////////////////////////////////////////////////
// ViewManager.cpp
// ===============
// Manage the viewing of 3D objects within the viewport
//
// AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
// Updated for CS-330 Milestone Three:
// - WASD + QE camera movement
// - Mouse look (cursor)
// - Mouse scroll adjusts movement speed
// - P = perspective, O = orthographic
//
// Notes:
// - This version does NOT rely on Camera::ProcessMouseMovement.
// - It updates the camera Front vector directly using yaw/pitch.
// - It will work even if your Camera class is minimal.
/////////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with the 3D scene
	Camera* g_pCamera = nullptr;

	// mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// yaw/pitch for mouse-look (degrees)
	float gYaw = -90.0f;
	float gPitch = 0.0f;

	// time between current frame and last frame
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// tuning
	float gMouseSensitivity = 0.35f; // was 0.10f, increased so mouse look is noticeable
	float gBaseMoveSpeed = 3.5f;
	float gSpeedMultiplier = 1.0f;

	// projection toggle
	bool bOrthographicProjection = false;
	float gOrthoScale = 3.5f;

	// key edge detection so toggles happen once per press
	bool gPrevPDown = false;
	bool gPrevODown = false;
}

/***********************************************************
 *  ViewManager()
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;

	g_pCamera = new Camera();

	// default camera view parameters (perspective mode)
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);

	// Normalize the front vector so movement and mouse look feel correct
	g_pCamera->Front = glm::normalize(glm::vec3(0.0f, -0.25f, -1.0f));

	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80.0f;
}

/***********************************************************
 *  ~ViewManager()
 ***********************************************************/
ViewManager::~ViewManager()
{
	m_pShaderManager = NULL;
	m_pWindow = NULL;

	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);

	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}

	glfwMakeContextCurrent(window);

	// capture mouse for camera look
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// callbacks
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// enable blending for supporting transparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;
	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	if (g_pCamera == nullptr)
	{
		return;
	}

	// Optional: disable mouse-look in orthographic mode
	if (bOrthographicProjection)
	{
		return;
	}

	float xpos = (float)xMousePos;
	float ypos = (float)yMousePos;

	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed

	gLastX = xpos;
	gLastY = ypos;

	// apply sensitivity
	xoffset *= gMouseSensitivity;
	yoffset *= gMouseSensitivity;

	// update yaw/pitch
	gYaw += xoffset;
	gPitch += yoffset;

	// clamp pitch to prevent flipping
	if (gPitch > 89.0f) gPitch = 89.0f;
	if (gPitch < -89.0f) gPitch = -89.0f;

	// calculate front vector from yaw/pitch
	glm::vec3 front;
	front.x = cos(glm::radians(gYaw)) * cos(glm::radians(gPitch));
	front.y = sin(glm::radians(gPitch));
	front.z = sin(glm::radians(gYaw)) * cos(glm::radians(gPitch));

	g_pCamera->Front = glm::normalize(front);
}

/***********************************************************
 *  Mouse_Scroll_Callback()
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	// scroll up increases speed, scroll down decreases speed
	gSpeedMultiplier += (float)yOffset * 0.1f;

	// clamp multiplier
	if (gSpeedMultiplier < 0.2f) gSpeedMultiplier = 0.2f;
	if (gSpeedMultiplier > 4.0f) gSpeedMultiplier = 4.0f;
}

/***********************************************************
 *  ProcessKeyboardEvents()
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	if (m_pWindow == NULL || g_pCamera == nullptr)
	{
		return;
	}

	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// Projection toggle (edge-triggered)
	bool pDown = glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS;
	bool oDown = glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS;

	if (pDown && !gPrevPDown)
	{
		bOrthographicProjection = false;
	}
	if (oDown && !gPrevODown)
	{
		bOrthographicProjection = true;
	}

	gPrevPDown = pDown;
	gPrevODown = oDown;

	// movement amount this frame
	float velocity = gBaseMoveSpeed * gSpeedMultiplier * gDeltaTime;

	// WASD + QE movement (allowed in both modes, but orthographic view is usually fixed)
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->Position += g_pCamera->Front * velocity;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->Position -= g_pCamera->Front * velocity;
	}

	glm::vec3 right = glm::normalize(glm::cross(g_pCamera->Front, g_pCamera->Up));
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->Position -= right * velocity;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->Position += right * velocity;
	}

	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->Position -= g_pCamera->Up * velocity;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->Position += g_pCamera->Up * velocity;
	}
}

/***********************************************************
 *  PrepareSceneView()
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	if (m_pWindow == NULL || g_pCamera == nullptr)
	{
		return;
	}

	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = (float)glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process keyboard input
	ProcessKeyboardEvents();

	// view matrix
	if (!bOrthographicProjection)
	{
		view = g_pCamera->GetViewMatrix();
	}
	else
	{
		// Orthographic view: look directly at the mug and keep it centered
		glm::vec3 target = glm::vec3(0.0f, 0.85f, -2.8f);
		glm::vec3 orthoCamPos = target + glm::vec3(0.0f, 1.5f, 6.0f);

		view = glm::lookAt(
			orthoCamPos,
			target,
			glm::vec3(0.0f, 1.0f, 0.0f));
	}

	// projection matrix
	if (!bOrthographicProjection)
	{
		projection = glm::perspective(
			glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f,
			100.0f);
	}
	else
	{
		float aspect = (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT;

		projection = glm::ortho(
			-gOrthoScale * aspect,
			gOrthoScale * aspect,
			-gOrthoScale,
			gOrthoScale,
			0.1f,
			100.0f);
	}

	// send to shader
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ViewName, view);
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}