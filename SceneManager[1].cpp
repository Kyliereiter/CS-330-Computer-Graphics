/////////////////////////////////////////////////////////////////////////////////
// SceneManager.cpp
// ===============
// Manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
/////////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <iostream>

// Global shader uniform names
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
	: m_pShaderManager(pShaderManager),
	m_basicMeshes(nullptr),
	m_loadedTextures(0)
{
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;

	DestroyGLTextures();

	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// Safety: prevent overflow if too many textures are loaded
	if (m_loadedTextures >= 16)
	{
		std::cout << "Texture limit reached (16). Could not load: " << filename << std::endl;
		return false;
	}

	// Indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// Try to parse the image data from the specified image file
	unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);

	// If the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image: " << filename
			<< ", width: " << width
			<< ", height: " << height
			<< ", channels: " << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// Set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// Set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Upload to GPU (format depends on channels)
		if (colorChannels == 3)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		}
		else if (colorChannels == 4)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		}
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels: " << filename << std::endl;
			stbi_image_free(image);
			glBindTexture(GL_TEXTURE_2D, 0);
			return false;
		}

		// Generate mipmaps
		glGenerateMipmap(GL_TEXTURE_2D);

		// Free local image data
		stbi_image_free(image);

		// Unbind texture
		glBindTexture(GL_TEXTURE_2D, 0);

		// Register the loaded texture and associate it with the tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image: " << filename << std::endl;
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots. There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		if (m_textureIDs[i].ID != 0)
		{
			glDeleteTextures(1, &m_textureIDs[i].ID);
			m_textureIDs[i].ID = 0;
			m_textureIDs[i].tag = "";
		}
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  FindTextureID()
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
		{
			index++;
		}
	}

	return textureID;
}

/***********************************************************
 *  FindTextureSlot()
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
		{
			index++;
		}
	}

	return textureSlot;
}

/***********************************************************
 *  FindMaterial()
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return false;
	}

	for (size_t i = 0; i < m_objectMaterials.size(); i++)
	{
		if (m_objectMaterials[i].tag.compare(tag) == 0)
		{
			material.ambientColor = m_objectMaterials[i].ambientColor;
			material.ambientStrength = m_objectMaterials[i].ambientStrength;
			material.diffuseColor = m_objectMaterials[i].diffuseColor;
			material.specularColor = m_objectMaterials[i].specularColor;
			material.shininess = m_objectMaterials[i].shininess;
			return true;
		}
	}

	return false;
}

/***********************************************************
 *  SetTransformations()
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	scale = glm::scale(scaleXYZ);

	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));

	translation = glm::translate(positionXYZ);

	// Order used by this template (matches class sample)
	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	glm::vec4 currentColor;
	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, 0);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		int textureSlot = FindTextureSlot(textureTag);

		// Safety: if tag not found, disable texture so it doesn't go black
		if (textureSlot < 0)
		{
			m_pShaderManager->setIntValue(g_UseTextureName, 0);
			return;
		}

		m_pShaderManager->setIntValue(g_UseTextureName, 1);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 ***********************************************************/
void SceneManager::SetShaderMaterial(std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;

		if (FindMaterial(materialTag, material))
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}
/***********************************************************
 *  SetShaderLights()
 *
 *  Sends scene light uniforms to the shader.
 ***********************************************************/
void SceneManager::SetShaderLights()
{
	// Camera position for specular highlights
	// Replace this with your real camera position variable if different
	m_pShaderManager->setVec3Value("viewPosition", glm::vec3(0.0f, 3.0f, 8.0f));

	// Make sure lighting is enabled when needed (set these near the draw call too)
	// m_pShaderManager->setBoolValue("bUseLighting", true);
	// m_pShaderManager->setBoolValue("bUseTexture", true);

	// ---------- Light 0, key point light (above and slightly in front) ----------
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(0.0f, 3.0f, 2.0f));

	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.10f, 0.10f, 0.10f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(0.95f, 0.90f, 0.80f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.00f, 1.00f, 1.00f));

	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.60f);

	// Attenuation (room-like falloff)
	m_pShaderManager->setFloatValue("lightSources[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].linear", 0.09f);
	m_pShaderManager->setFloatValue("lightSources[0].quadratic", 0.032f);

	// ---------- Light 1, fill point light (keeps plane from going black) ----------
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-3.0f, 2.0f, -2.0f));

	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.14f, 0.14f, 0.14f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.35f, 0.35f, 0.40f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.40f, 0.40f, 0.40f));

	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.20f);

	m_pShaderManager->setFloatValue("lightSources[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].linear", 0.09f);
	m_pShaderManager->setFloatValue("lightSources[1].quadratic", 0.032f);

	// ---------- Lights 2 and 3, disabled ----------
	for (int i = 2; i < 4; ++i)
	{
		std::string idx = std::to_string(i);

		m_pShaderManager->setVec3Value(("lightSources[" + idx + "].position").c_str(), glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(("lightSources[" + idx + "].ambientColor").c_str(), glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(("lightSources[" + idx + "].diffuseColor").c_str(), glm::vec3(0.0f));
		m_pShaderManager->setVec3Value(("lightSources[" + idx + "].specularColor").c_str(), glm::vec3(0.0f));

		m_pShaderManager->setFloatValue(("lightSources[" + idx + "].focalStrength").c_str(), 1.0f);
		m_pShaderManager->setFloatValue(("lightSources[" + idx + "].specularIntensity").c_str(), 0.0f);

		// Safe attenuation (doesn't matter since colors are zero)
		m_pShaderManager->setFloatValue(("lightSources[" + idx + "].constant").c_str(), 1.0f);
		m_pShaderManager->setFloatValue(("lightSources[" + idx + "].linear").c_str(), 0.0f);
		m_pShaderManager->setFloatValue(("lightSources[" + idx + "].quadratic").c_str(), 0.0f);
	}
}

/***********************************************************
 * PrepareScene()
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load textures (tags must match what you use later)
	CreateGLTexture("Resources/Textures/wood.png", "wood");
	CreateGLTexture("Resources/Textures/ceramic.png", "ceramic");

	// Bind all loaded textures to texture units (GL_TEXTURE0, GL_TEXTURE1, ...)
	BindGLTextures();

	// Load meshes
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 * RenderScene()
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Make sure the correct shader program is active each frame
	m_pShaderManager->use();

	// Send lights (including attenuation) and camera position to the shader
	SetShaderLights();

	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;

	/******************************************************************/
	// Desk / Floor (WOOD TEXTURE, TILED)
	/******************************************************************/
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");
	SetTextureUVScale(6.0f, 3.0f); // tiling technique, adjust to taste
	m_basicMeshes->DrawPlaneMesh();

	/******************************************************************/
	// Coffee Mug (2 shapes)
	/******************************************************************/
	const float mugX = 0.0f;
	const float mugZ = -2.8f;
	const float mugYaw = -20.0f;

	/********************/
	/* Mug Body (CERAMIC)
	/********************/
	scaleXYZ = glm::vec3(1.15f, 1.65f, 1.15f);
	const float bodyHalfHeight = scaleXYZ.y * 0.5f;

	// Put bottom of mug on the plane (y = 0)
	positionXYZ = glm::vec3(mugX, bodyHalfHeight, mugZ);

	SetTransformations(scaleXYZ, 0.0f, mugYaw, 0.0f, positionXYZ);

	// Texture on body
	SetShaderTexture("ceramic");
	SetTextureUVScale(2.0f, 2.0f);
	m_basicMeshes->DrawTaperedCylinderMesh();

	/********************/
	/* Handle (COLOR)
	/********************/
	scaleXYZ = glm::vec3(0.55f, 0.75f, 0.22f);

	const float xRotationDegrees = 0.0f;
	const float yRotationDegrees = mugYaw;
	const float zRotationDegrees = 90.0f;

	positionXYZ = glm::vec3(
		mugX + 0.98f,               // offset out from the mug body
		bodyHalfHeight + 0.45f,     // raise to mid-upper body
		mugZ + 0.08f                // small forward offset to avoid z-fighting
	);

	SetTransformations(scaleXYZ, xRotationDegrees, yRotationDegrees, zRotationDegrees, positionXYZ);

	// Solid color on handle (turns texture off inside SetShaderColor)
	SetShaderColor(0.98f, 0.55f, 0.15f, 1.0f);
	m_basicMeshes->DrawTorusMesh();
}