#define STB_IMAGE_IMPLEMENTATION

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "CommonValues.h"

#include "Camera.h"
#include "DirectionalLight.h"
#include "Material.h"
#include "Mesh.h"
#include "Model.h"
#include "PointLight.h"
#include "Shader.h"
#include "Skybox.h"
#include "Texture.h"
#include "Window.h"

// Window dimensions
const float toRadians = 3.14159265f / 180.0f;

// Improvement : Get rid of a maximum of global variables
// Improvement handle all uniform variable in the shader class
GLuint uniformProjection = 0, uniformModel = 0, uniformView, uniformEyePosition = 0,
uniformSpecularIntensity = 0, uniformShininess = 0,
uniformDirectionalLightTransform = 0, uniformOmniLightPos = 0, uniformFarPlane = 0;

unsigned int pointLightCount = 0;
unsigned int spotLightCount = 0;

Window mainWindow;
std::vector<Mesh*> meshList;

std::vector<Shader> shaderList;
Shader directionalShadowShader;
Shader omniShadowShader;

Camera camera;

Texture brickTexture;
Texture dirtTexture;
Texture plainTexture;

Material shinyMaterial;
Material dullMaterial;

Model turtle;

DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];

Skybox skybox;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;

GLfloat turtuleAngle = 0.0f;


// Vertex Shader
static const char* vShader = "Shaders/shader.vert";

// Fragment Shader
static const char* fShader = "Shaders/shader.frag";


// On calcule le vecteur normal moyen en prenant en prenant en compte tous le svecteurs normaux des triangles adjacents (Ici on a une pyramide donc ils sont tous adjacents)
void calcAverageNormals(unsigned int* indices, unsigned int indiceCount, GLfloat* vertices, unsigned int verticeCount, unsigned int vLength, unsigned int normalOffset)
{
	// += 3 because we're doing triangle by triangle 
	for (size_t i = 0; i < indiceCount; i+=3)
	{
		unsigned int in0 = indices[i] * vLength;
		unsigned int in1 = indices[i + 1] * vLength;
		unsigned int in2 = indices[i + 2] * vLength;
		// On cr�e 2 vecteurs qui correspondent � 2 c�t�s du triangle.Afin de calculer leur produit vectoriel pour obtenir le vecteur normal du triangle
		glm::vec3 v1(vertices[in1] - vertices[in0], vertices[in1 + 1] - vertices[in0 + 1], vertices[in1 + 2] - vertices[in0 + 2]);
		glm::vec3 v2(vertices[in2] - vertices[in0], vertices[in2 + 1] - vertices[in0 + 1], vertices[in2 + 2] - vertices[in0 + 2]);
		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		// Adding the offset to get the part that is interesting to us
		// eg: If we don't care about UV and we want to go directly to nx then offset will be 5
		in0 += normalOffset; in1 += normalOffset; in2 += normalOffset;
		// We overwrite by adding the result from the previous calculated normal
		vertices[in0] += normal.x; vertices[in0 + 1] += normal.y; vertices[in0 + 2] += normal.z;
		vertices[in1] += normal.x; vertices[in1 + 1] += normal.y; vertices[in1 + 2] += normal.z;
		vertices[in2] += normal.x; vertices[in2 + 1] += normal.y; vertices[in2 + 2] += normal.z;
	}

	// Go to all average normal vectors that we just calculated and normalize them
	for (size_t i = 0; i < verticeCount / vLength; i++)
	{
		unsigned int nOffset = i * vLength + normalOffset;
		glm::vec3 vec(vertices[nOffset], vertices[nOffset + 1], vertices[nOffset + 2]);
		vec = glm::normalize(vec);
		vertices[nOffset] = vec.x; vertices[nOffset + 1] = vec.y; vertices[nOffset + 2] = vec.z;
	}
}

void CreateObjects()
{
	unsigned int indices[] = {
		0, 3, 1,
		1, 3, 2,
		2, 3, 0,
		0, 1, 2
	};

	GLfloat vertices[] = {
	//   x,     y,	  z,	u,    v,    nx,   ny,   nz
		-1.0f, -1.0f, -0.6f,	0.0f, 0.0f,	0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 1.0f,	0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, -0.6f,	1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,	0.5f, 1.0f, 0.0f, 0.0f, 0.0f
	};

	unsigned int floorIndices[] = {
		0, 2, 1,
		1, 2, 3
	};

	GLfloat floorVertices[] = {
		-10.0f, 0.0f, -10.0f,	0.0f, 0.0f,		0.0f, -1.0f, 0.0f,
		10.0f, 0.0f, -10.0f,	10.0f, 0.0f,	0.0f, -1.0f, 0.0f,
		-10.0f, 0.0f, 10.0f,	0.0f, 10.0f,	0.0f, -1.0f, 0.0f,
		10.0f, 0.0f, 10.0f,		10.0f, 10.0f,	0.0f, -1.0f, 0.0f
	};

	calcAverageNormals(indices, 12, vertices, 32, 8, 5);

	Mesh* obj1 = new Mesh();
	obj1->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(obj1);

	Mesh* obj2 = new Mesh();
	obj2->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(obj2);

	Mesh* obj3 = new Mesh();
	obj3->CreateMesh(floorVertices, floorIndices, 32, 6);
	meshList.push_back(obj3);

}

void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(*shader1);

	directionalShadowShader.CreateFromFiles("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
	omniShadowShader.CreateFromFiles("Shaders/omni_shadow_map.vert", "Shaders/omni_shadow_map.geom", "Shaders/omni_shadow_map.frag");
}

void RenderScene()
{
	// Apply transformation for the firt object
	glm::mat4 model(1.0f);

	model = glm::translate(model, glm::vec3(0.0f, 0.0f, -2.5f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	brickTexture.UseTexture();
	shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[0]->RenderMesh();

	// Apply transformation for the second object
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 4.0f, -2.5f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	dirtTexture.UseTexture();
	dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[1]->RenderMesh();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	dirtTexture.UseTexture();
	dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[2]->RenderMesh();

	turtuleAngle += 0.1f;
	if (turtuleAngle > 360.0f)
	{
		turtuleAngle = 0.0f;
	}

	model = glm::mat4(1.0f);
	model = glm::rotate(model, turtuleAngle * toRadians, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::translate(model, glm::vec3(7.0f, -0.5f, 0.0f));
	model = glm::rotate(model, 90 * toRadians, glm::vec3(-1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	turtle.RenderModel();
}

// Handle rendering for the creation of the shadowmap
void DirectionalShadowMapPass(DirectionalLight* light)
{
	directionalShadowShader.UseShader();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();
	glClear(GL_DEPTH_BUFFER_BIT);

	uniformModel = directionalShadowShader.GetModelLocation();
	glm::mat4 lTransform = light->CalculateLightTransform();
	directionalShadowShader.SetDirectionalLightTransform(&lTransform);

	directionalShadowShader.Validate();
	// Render the scene from the view from the light and write only depth
	RenderScene();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OmniShadowMapPass(PointLight* light)
{
	omniShadowShader.UseShader();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();
	glClear(GL_DEPTH_BUFFER_BIT);

	uniformModel = omniShadowShader.GetModelLocation();
	uniformOmniLightPos = omniShadowShader.GetOmniLightPosLocation();
	uniformFarPlane = omniShadowShader.GetFarPlaneLocation();

	glUniform3f(uniformOmniLightPos, light->GetPosition().x, light->GetPosition().y, light->GetPosition().z);
	glUniform1f(uniformFarPlane, light->GetFarPlane());
	omniShadowShader.SetOmniLightMatrices(light->CalculateLightTransform());

	omniShadowShader.Validate();
	// Render the scene from the view from the light and write only depth
	RenderScene();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass(glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
	glViewport(0, 0, 1366, 768);
	// Clear window
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	skybox.DrawSkybox(viewMatrix, projectionMatrix);

	shaderList[0].UseShader();

	uniformModel = shaderList[0].GetModelLocation();
	uniformProjection = shaderList[0].GetProjectionLocation();
	uniformView = shaderList[0].GetViewLocation();
	uniformEyePosition = shaderList[0].GetEyePositionLocation();
	uniformSpecularIntensity = shaderList[0].GetSpecularIntensityLocation();
	uniformShininess = shaderList[0].GetShininessLocation();



	glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniform3f(uniformEyePosition, camera.GetCameraPosition().x, camera.GetCameraPosition().y, camera.GetCameraPosition().z);

	shaderList[0].SetDirectionalLight(&mainLight);
	shaderList[0].SetPointLights(pointLights, pointLightCount, 3, 0);
	shaderList[0].SetSpotLights(spotLights, spotLightCount, 3 + pointLightCount, pointLightCount);
	glm::mat4 lTransform = mainLight.CalculateLightTransform();
	shaderList[0].SetDirectionalLightTransform(&lTransform);

	// We need to ensure that every texture has its own teture unit AND that there is no different type of texture set up with the same texture unit.
	// The problem is that by default the texture unit associated is 0.
	// We use GL_TEXTURE_CUBE_MAP for omnishadowmap for point and spot lights.
	// As we don't use the maximum of lights there is existent but unused shadowmap created bound to GL_TEXTURE0
	// There are several way to deal with that problem but for an idiotic way we just shift everything by one
	// So directional shadowmap 1->2
	mainLight.GetShadowMap()->Read(GL_TEXTURE2);
	shaderList[0].SetTexture(1);
	shaderList[0].SetDirectionalShadowMap(2);

	glm::vec3 lowerLight = camera.GetCameraPosition();
	lowerLight.y -= 0.3f;
	spotLights[0].SetFlash(lowerLight, camera.getCameraDirection());

	shaderList[0].Validate();
	// Render the map from the view of the camera andrender with color
	RenderScene();
}

int main()
{
	mainWindow = Window(1366, 768);
	mainWindow.Initialise();

	CreateObjects();
	CreateShaders();

	camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -0.0f, 0.0f, 5.0f, 0.5f);

	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTextureA();
	dirtTexture = Texture("Textures/dirt.png");
	dirtTexture.LoadTextureA();
	plainTexture = Texture("Textures/plain.png");
	plainTexture.LoadTextureA();

	shinyMaterial = Material(1.0f, 256);
	dullMaterial = Material(0.0f, 1);

	turtle = Model();
	turtle.LoadModel("Models/turtle.obj");

	mainLight = DirectionalLight(2048, 2048,
								1.0f, 0.5f, 0.3f,
								0.1f, 0.5f,
								-10.0f, -12.0f, 18.5f);

	pointLights[0] = PointLight(1024, 1024,
								0.01f, 100.0f,
								0.0f, 0.0f, 1.0f,
								0.1f, 0.6f,
								5.0f, 7.0f, 0.0f,
								0.3f, 0.1f, 0.1f);
	//pointLightCount++;
	pointLights[1] = PointLight(1024, 1024,
								0.01f, 100.0f, 
								0.0f, 1.0f, 0.0f,
								0.1f, 0.6f,
								-5.0f, 7.0f, 0.0f,
								0.3f, 0.1f, 0.1f);
	//pointLightCount++;

	spotLights[0] = SpotLight(1024, 1024,
							0.01f, 100.0f, 
							1.0f, 1.0f, 1.0f,
							0.0f, 2.0f,
							0.0f, 0.0f, 0.0f,
							0.0f, -1.0f, 0.0f,
							0.3f, 0.1f, 0.1f,
							20.0f);
	spotLightCount++;
	spotLights[1] = SpotLight(1024, 1024,
							0.01f, 100.0f, 
							1.0f, 1.0f, 1.0f,
							0.0f, 1.0f,
							0.0f, -1.5f, 0.0f,
							-100.0f, -1.0f, 0.0f,
							1.0f, 0.0f, 0.0f,
							20.0f);
	//spotLightCount++;

	// load skybox textures
	// beware of the order when we push them
	std::vector<std::string> skyboxFaces;
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_rt.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_lf.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_up.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_dn.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_bk.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_ft.tga");

	skybox = Skybox(skyboxFaces);

	glm::mat4 projection = glm::perspective(glm::radians(60.0f), (GLfloat)mainWindow.GetBufferWidth() / (GLfloat)mainWindow.GetBufferHeight(), 0.01f, 100.0f);

	// Loop until window closed
	while (!mainWindow.GetShouldClose())
	{
		GLfloat now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		// Get and Handle user input events
		glfwPollEvents();

		// Get input
		camera.KeyControl(mainWindow.getKeys(), deltaTime);
		camera.MouseControl(mainWindow.getXChange(), mainWindow.getYChange());

		if (mainWindow.getKeys()[GLFW_KEY_L])
		{
			spotLights[0].Toggle();
			mainWindow.getKeys()[GLFW_KEY_L] = false;
		}

		// Create the directionalShadowMap
		DirectionalShadowMapPass(&mainLight);
		// Create the omniShadowmap for each pointLights and spotLights
		for (size_t i = 0; i < pointLightCount; i++)
		{
			OmniShadowMapPass(&pointLights[i]);
		}
		for (size_t i = 0; i < spotLightCount; i++)
		{
			OmniShadowMapPass(&spotLights[i]);
		}
		// Render from the camera
		RenderPass(projection, camera.CalculateViewMatrix());
		
		glUseProgram(0);

		mainWindow.SwapBuffers();
	}

	return 0;
}