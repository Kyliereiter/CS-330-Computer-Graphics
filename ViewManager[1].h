/////////////////////////////////////////////////////////////////////////////////
// ViewManager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "camera.h"

// GLFW library
#include "GLFW/glfw3.h"

class ViewManager
{
public:
	// constructor / destructor
	ViewManager(ShaderManager* pShaderManager);
	~ViewManager();

	// create the initial OpenGL display window
	GLFWwindow* CreateDisplayWindow(const char* windowTitle);

	// prepare the view/projection matrices each frame
	void PrepareSceneView();

	// mouse callbacks for interaction with the 3D scene
	static void Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos);
	static void Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset);

private:
	// pointer to shader manager object
	ShaderManager* m_pShaderManager;

	// active OpenGL display window
	GLFWwindow* m_pWindow;

	// process keyboard events for interaction with the 3D scene
	void ProcessKeyboardEvents();
};
