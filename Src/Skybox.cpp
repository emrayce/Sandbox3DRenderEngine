#include "Skybox.h"

Skybox::Skybox()
{
}

Skybox::Skybox(std::vector<std::string> faceLocations)
{
	// Shader Setup
	skyShader = new Shader();
	skyShader->CreateFromFiles("Shaders/skybox.vert", "Shaders/skybox.frag");

	uniformProjection = skyShader->GetProjectionLocation();
	uniformView = skyShader->GetViewLocation();

	// Texture Setup
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

	int width, height, bitDepth;

	for (size_t i = 0; i < 6; i++)
	{
		unsigned char* texData = stbi_load(faceLocations[i].c_str(), &width, &height, &bitDepth, 0);
		if (!texData)
		{
			printf("Failed to find: %s. %s\n", faceLocations[i].c_str(), stbi_failure_reason());
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
		stbi_image_free(texData);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Mesh Setup
	unsigned int skyboxIndices[] = {
		// front
		0, 1, 2,
		2, 1, 3,
		//right
		2, 3, 5,
		5, 3, 7,
		//back
		5, 7, 4,
		4, 7, 6,
		//left
		4, 6, 0,
		0, 6, 1,
		// top
		4, 0, 5,
		5, 0, 2,
		// bottom
		1, 6, 3,
		3, 6, 7
	};

	float skyboxVertices[] = {
		-1.0f, 1.0f, -1.0f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, -1.0f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, -1.0f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, -1.0f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,

		-1.0f, 1.0f, 1.0f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, 1.0f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f,		0.0f, 0.0f,		0.0f, 0.0f, 0.0f
	};

	skyMesh = new Mesh();
	skyMesh->CreateMesh(skyboxVertices, skyboxIndices, 64, 36);
}

void Skybox::DrawSkybox(glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
{
	// get rid of the translate info that it stored in the 4th column
	// that way the skybvox will consider the viewMatrix as its origin
	viewMatrix = glm::mat4(glm::mat3(viewMatrix));
	// Don't take depth value into account
	// if we do we'll we only see the skybox as it right in front of the camera
	// And all that behind won't appear
	glDepthMask(GL_FALSE);

	skyShader->UseShader();
	
	glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(viewMatrix));

	// we know that GL_TEXTURE0 will be a samplerCube and there will be only one texture so it's ok
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

	// no need to use Set texture because it's 0 by default so already linked to GL_TEXTURE0

	skyShader->Validate();

	skyMesh->RenderMesh();

	glDepthMask(GL_TRUE);
}
