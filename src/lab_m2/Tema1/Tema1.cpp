#include "lab_m2/Tema1/Tema1.h"

#include <vector>
#include <iostream>

#include "stb/stb_image.h"

using namespace std;
using namespace m2;


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */

Tema1::Tema1()
{
	framebuffer_object = 0;
	color_texture = 0;
	depth_texture = 0;
	angle = 0;
	type = 0;

	framebuffer_size = 1024;

	//hmm -> end
	mirrorRot = glm::quat();
	mirrorRotSpeed = glm::radians(60.0f);
	mirrorPos = glm::vec3(0.f, 0, -2.f);
	mirrorVelocity = glm::vec3(5.f, 5, 5);

	nrParticles = 400;

	controlPoints[0] = glm::vec3(0, 0, 0);
	controlPoints[1] = glm::vec3(1, 6, 2);
	controlPoints[2] = glm::vec3(4, -2, 2);
	controlPoints[3] = glm::vec3(7, 8, 0);

	controlPoints[4] = glm::vec3(0, 0, 0);
	controlPoints[5] = glm::vec3(-1, 6, 0);
	controlPoints[6] = glm::vec3(-4, -2, 0);
	controlPoints[7] = glm::vec3(-7, 8, 0);

	controlPoints[8] = glm::vec3(0, 0, 0);
	controlPoints[9] = glm::vec3(1, 6, 0);
	controlPoints[10] = glm::vec3(5, 8, 0);
	controlPoints[11] = glm::vec3(7, -9, 0);

	controlPoints[12] = glm::vec3(0, 0, 0);
	controlPoints[13] = glm::vec3(-1, 2, 5);
	controlPoints[14] = glm::vec3(4, 5, -2);
	controlPoints[15] = glm::vec3(-7, 2, 7);

	controlPoints[16] = glm::vec3(0, 0, 0);
	controlPoints[17] = glm::vec3(4, 10, -10);
	controlPoints[18] = glm::vec3(2, 3, 3);
	controlPoints[19] = glm::vec3(-7, 8, 0);

	upVectors = {
	   glm::vec3(0.0f,-1.0f, 0.0f),
	   glm::vec3(0.0f,-1.0f, 0.0f),
	   glm::vec3(0.0f, 0.0f, 1.0f),
	   glm::vec3(0.0f, 0.0f, -1.0f),
	   glm::vec3(0.0f,-1.0f, 0.0f),
	   glm::vec3(0.0f,-1.0f, 0.0f) };


	generator_position = glm::vec3(0);
}

Tema1::~Tema1()
{
}

void Tema1::Init()
{
	auto camera = GetSceneCamera();
	camera->SetPositionAndRotation(glm::vec3(0, -1, 4), glm::quat(glm::vec3(RADIANS(10), 0, 0)));
	camera->Update();

	std::string texturePath = PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "cube");
	std::string shaderPath = PATH_JOIN(window->props.selfDir, SOURCE_PATH::M2, "Tema1", "shaders");

	// Init meshes
	{
		Mesh* mesh = new Mesh("cube");
		mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");
		mesh->UseMaterials(false);
		meshes[mesh->GetMeshID()] = mesh;
	}

	{
		Mesh* mesh = new Mesh("archer");
		mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "characters", "archer"), "Archer.fbx");
		mesh->UseMaterials(false);
		meshes[mesh->GetMeshID()] = mesh;
	}

	{
		Mesh* mesh = new Mesh("teapot");
		mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "teapot.obj");
		mesh->UseMaterials(false);
		meshes[mesh->GetMeshID()] = mesh;
	}

	{
		Mesh* mesh = new Mesh("quad");
		mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "quad.obj");
		mesh->UseMaterials(false);
		meshes[mesh->GetMeshID()] = mesh;
	}

	// Create a shader program for rendering cubemap texture
	{
		Shader* shader = new Shader("CubeMap");
		shader->AddShader(PATH_JOIN(shaderPath, "CubeMap.VS.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "CubeMap.FS.glsl"), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	// Create a shader program for standard rendering
	{
		Shader* shader = new Shader("ShaderNormal");
		shader->AddShader(PATH_JOIN(shaderPath, "Normal.VS.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "Normal.FS.glsl"), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	// Create a shader program for creating a cubemap
	{
		Shader* shader = new Shader("Framebuffer");
		shader->AddShader(PATH_JOIN(shaderPath, "Framebuffer.VS.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "Framebuffer.FS.glsl"), GL_FRAGMENT_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "Framebuffer.GS.glsl"), GL_GEOMETRY_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	// Create a shader program for particle system
	{
		Shader* shader = new Shader("Particle");
		shader->AddShader(PATH_JOIN(shaderPath, "Particle.VS.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "Particle.FS.glsl"), GL_FRAGMENT_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "Particle.GS.glsl"), GL_GEOMETRY_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	// Create a shader program for contour/outline rendering
	{
		Shader* shader = new Shader("Contour");
		shader->AddShader(PATH_JOIN(shaderPath, "Contour.VS.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "Contour.FS.glsl"), GL_FRAGMENT_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "Contour.GS.glsl"), GL_GEOMETRY_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	// Create a shader program for extra effects
	{
		Shader* shader = new Shader("Extra");
		shader->AddShader(PATH_JOIN(shaderPath, "Extra.VS.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "Extra.FS.glsl"), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	cubeMapTextureID = UploadCubeMapTexture(
		PATH_JOIN(texturePath, "pos_x.png"),
		PATH_JOIN(texturePath, "pos_y.png"),
		PATH_JOIN(texturePath, "pos_z.png"),
		PATH_JOIN(texturePath, "neg_x.png"),
		PATH_JOIN(texturePath, "neg_y.png"),
		PATH_JOIN(texturePath, "neg_z.png"));

	TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS), "characters", "archer", "Akai_E_Espiritu.fbm", "akai_diffuse.png");
	TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES), "particle2.png");

	// Create the framebuffer on which the scene is rendered from the perspective of the mesh
	// Texture size must be cubic
	CreateFramebuffer(framebuffer_size, framebuffer_size);
}

void Tema1::FrameStart()
{
}

void Tema1::Update(float deltaTimeSeconds)
{
	angle += 0.5f * deltaTimeSeconds;
	auto camera = GetSceneCamera();

	// Draw the scene in Framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);
	// Set the clear color for the color buffer
	glClearColor(0, 0, 0, 1);
	// Clears the color buffer (using the previously set color) and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, framebuffer_size, framebuffer_size);

	Shader* shader;
	Shader* old_shader;
	if (type == 1)
		shader = shaders["Contour"];
	else
		shader = shaders["Framebuffer"];
	shader->Use();

	glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

	glm::mat4 cubeView[6] =
	{
		glm::lookAt(mirrorPos, mirrorPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f)), // +X
		glm::lookAt(mirrorPos, mirrorPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f,-1.0f, 0.0f)), // -X
		glm::lookAt(mirrorPos, mirrorPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)), // +Y
		glm::lookAt(mirrorPos, mirrorPos + glm::vec3(0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f)), // -Y
		glm::lookAt(mirrorPos, mirrorPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f,-1.0f, 0.0f)), // +Z
		glm::lookAt(mirrorPos, mirrorPos + glm::vec3(0.0f, 0.0f,-1.0f), glm::vec3(0.0f,-1.0f, 0.0f)), // -Z
	};

	// Draw surroundings
	if (type != 1)
	{
		glm::mat4 modelMatrix = glm::scale(glm::mat4(1), glm::vec3(30));

		glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
		glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projection));

		glUniformMatrix4fv(glGetUniformLocation(shader->GetProgramID(), "viewMatrices"), 6, GL_FALSE, glm::value_ptr(cubeView[0]));

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTextureID);
		glUniform1i(glGetUniformLocation(shader->program, "texture_cubemap"), 1);

		glUniform1i(glGetUniformLocation(shader->program, "cube_draw"), 1);

		meshes["cube"]->Render();
	}

	// Draw archers
	if (type == 3) {
		old_shader = shader;
		shader = shaders["Extra"];
		shader->Use();
	}

	for (int i = 0; i < 5; i++)
	{
		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix *= glm::rotate(glm::mat4(1), angle + i * glm::radians(360.0f) / 5, glm::vec3(0, 1, 0));
		modelMatrix *= glm::translate(glm::mat4(1), glm::vec3(3, -1, 0));
		modelMatrix *= glm::rotate(glm::mat4(1), glm::radians(-90.0f), glm::vec3(0, 1, 0));
		modelMatrix *= glm::scale(glm::mat4(1), glm::vec3(0.01f));

		glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
		glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(shader->GetProgramID(), "viewMatrices"), 6, GL_FALSE, glm::value_ptr(cubeView[0]));


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureManager::GetTexture("Akai_E_Espiritu.fbm\\akai_diffuse.png")->GetTextureID());
		glUniform1i(glGetUniformLocation(shader->program, "texture_1"), 0);

		glUniform1i(glGetUniformLocation(shader->program, "cube_draw"), 0);

		glUniform1i(glGetUniformLocation(shader->program, "type"), type);

		meshes["archer"]->Render();
	}

	// Draw teapot
	{
		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix *= glm::translate(glm::mat4(1), glm::vec3(0, 1, 0));
		modelMatrix *= glm::rotate(glm::mat4(1), angle + glm::radians(360.0f) / 5, glm::vec3(0, 1, 0));
		modelMatrix *= glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(0, 0, 1));
		glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

		meshes["teapot"]->Render();
	}

	if (type == 3) {
		shader = old_shader;
		shader->Use();
	}

	if (type == 2)
	{
		glLineWidth(3);

		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquation(GL_FUNC_ADD);

		auto shader = shaders["Particle"];
		if (shader->GetProgramID())
		{
			shader->Use();

			TextureManager::GetTexture("particle2.png")->BindToTextureUnit(GL_TEXTURE0);
			particleEffect->Render(GetSceneCamera(), shader);

			glUniform3f(shader->GetUniformLocation("generator_position"), generator_position.x, generator_position.y, generator_position.z);
			glUniform1f(shader->GetUniformLocation("deltaTime"), deltaTimeSeconds);
			glUniform1f(shader->GetUniformLocation("offset"), 0.1);

			glUniformMatrix4fv(glGetUniformLocation(shader->GetProgramID(), "view_matrices"), 6, GL_FALSE, glm::value_ptr(cubeView[0]));
			glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "up_vectors"), 6, glm::value_ptr(upVectors[0]));
			glUniform3f(shader->GetUniformLocation("Position"), mirrorPos.x, mirrorPos.y, mirrorPos.z);
			glUniformMatrix4fv(shader->GetUniformLocation("Projection"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniform1i(shader->GetUniformLocation("nrParticles"), nrParticles);
			glUniform3fv(shader->GetUniformLocation("control_points"), 20, glm::value_ptr(controlPoints[0]));
		}

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, color_texture);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);



	// Reset drawing to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Clear the scene
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, window->GetResolution().x, window->GetResolution().y);

	// Draw the cubemap
	{
		Shader* shader = shaders["ShaderNormal"];
		shader->Use();

		glm::mat4 modelMatrix = glm::scale(glm::mat4(1), glm::vec3(30));

		glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
		glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTextureID);
		int loc_texture = shader->GetUniformLocation("texture_cubemap");
		glUniform1i(loc_texture, 0);

		meshes["cube"]->Render();
	}

	// Draw the archers
	for (int i = 0; i < 5; i++)
	{
		Shader* shader = shaders["Simple"];
		shader->Use();

		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix *= glm::rotate(glm::mat4(1), angle + i * glm::radians(360.0f) / 5, glm::vec3(0, 1, 0));
		modelMatrix *= glm::translate(glm::mat4(1), glm::vec3(3, -1, 0));
		modelMatrix *= glm::rotate(glm::mat4(1), glm::radians(-90.0f), glm::vec3(0, 1, 0));
		modelMatrix *= glm::scale(glm::mat4(1), glm::vec3(0.01f));

		glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
		glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureManager::GetTexture("Akai_E_Espiritu.fbm\\akai_diffuse.png")->GetTextureID());
		glUniform1i(glGetUniformLocation(shader->program, "texture_1"), 0);

		meshes["archer"]->Render();
	}

	// Draw teapot
	{
		Shader* shader = shaders["Simple"];
		shader->Use();
		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix *= glm::translate(glm::mat4(1), glm::vec3(0, 2, 0));
		modelMatrix *= glm::rotate(glm::mat4(1), angle + glm::radians(360.0f) / 5, glm::vec3(0, 1, 0));
		modelMatrix *= glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(0, 0, 1));
		glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		meshes["teapot"]->Render();
	}

	// Draw the reflection on the mesh
	{
		Shader* shader = shaders["CubeMap"];
		shader->Use();

		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix *= glm::translate(glm::mat4(1.0f), mirrorPos);
		modelMatrix *= glm::toMat4(mirrorRot);
		modelMatrix *= glm::scale(glm::mat4(1.0f), glm::vec3(3, 3, 1));

		glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1.0f), mirrorPos) * camera->GetViewMatrix()));
		glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));

		auto cameraPosition = camera->m_transform->GetWorldPosition();

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, color_texture);
		int loc_texture2 = shader->GetUniformLocation("texture_cubemap");
		glUniform1i(loc_texture2, 1);

		int loc_camera = shader->GetUniformLocation("camera_position");
		glUniform3f(loc_camera, cameraPosition.x, cameraPosition.y, cameraPosition.z);

		meshes["quad"]->Render();
	}
}

void Tema1::FrameEnd()
{
	// DrawCoordinateSystem();
}

unsigned int Tema1::UploadCubeMapTexture(const std::string& pos_x, const std::string& pos_y, const std::string& pos_z, const std::string& neg_x, const std::string& neg_y, const std::string& neg_z)
{
	int width, height, chn;

	unsigned char* data_pos_x = stbi_load(pos_x.c_str(), &width, &height, &chn, 0);
	unsigned char* data_pos_y = stbi_load(pos_y.c_str(), &width, &height, &chn, 0);
	unsigned char* data_pos_z = stbi_load(pos_z.c_str(), &width, &height, &chn, 0);
	unsigned char* data_neg_x = stbi_load(neg_x.c_str(), &width, &height, &chn, 0);
	unsigned char* data_neg_y = stbi_load(neg_y.c_str(), &width, &height, &chn, 0);
	unsigned char* data_neg_z = stbi_load(neg_z.c_str(), &width, &height, &chn, 0);

	unsigned int textureID = 0;
	// Create and bind the texture
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (GLEW_EXT_texture_filter_anisotropic) {
		float maxAnisotropy;

		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load texture information for each face
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_pos_x);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_pos_y);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_pos_z);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_neg_x);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_neg_y);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_neg_z);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	if (GetOpenGLError() == GL_INVALID_OPERATION)
	{
		cout << "\t[NOTE] : For students : DON'T PANIC! This error should go away when completing the tasks." << std::endl;
	}

	// Free memory
	SAFE_FREE(data_pos_x);
	SAFE_FREE(data_pos_y);
	SAFE_FREE(data_pos_z);
	SAFE_FREE(data_neg_x);
	SAFE_FREE(data_neg_y);
	SAFE_FREE(data_neg_z);

	return textureID;
}

void Tema1::CreateFramebuffer(int width, int height)
{
	//  Generate and bind the framebuffer
	glGenFramebuffers(1, &framebuffer_object);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);

	// Generate and bind the color texture
	glGenTextures(1, &color_texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, color_texture);

	// Initialize the color textures
	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	}

	if (color_texture) {
		//cubemap params
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		if (GLEW_EXT_texture_filter_anisotropic) {
			float maxAnisotropy;

			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// Bind the color textures to the framebuffer as a color attachments
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_texture, 0);

		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		std::vector<GLenum> draw_textures;
		draw_textures.push_back(GL_COLOR_ATTACHMENT0);
		glDrawBuffers(draw_textures.size(), &draw_textures[0]);

	}

	// Generate and bind the depth texture
	glGenTextures(1, &depth_texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depth_texture);

	// Initialize the depth textures
	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}

	if (depth_texture) {
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture, 0);
	}

	glCheckFramebufferStatus(GL_FRAMEBUFFER);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void m2::Tema1::ResetParticle()
{
	particleEffect = new ParticleEffect<Particle>();
	particleEffect->Generate(nrParticles, true);

	auto particleSSBO = particleEffect->GetParticleBuffer();
	Particle* data = const_cast<Particle*>(particleSSBO->GetBuffer());

	for (unsigned int i = 0; i < nrParticles; i++)
	{
		float lifetime = 4 + 10.0f * (rand() % 100 / 100.0f);
		float delay = (rand() % 100 / 100.0f) * 3.0f;

		data[i].initialLifetime = lifetime;
		data[i].initialDelay = delay;
	}

	particleSSBO->SetBufferData(data);
}

void Tema1::OnInputUpdate(float deltaTime, int mods)
{
	// Treat continuous update based on input
	if (!window->MouseHold(GLFW_MOUSE_BUTTON_RIGHT))
	{
		if (window->KeyHold(GLFW_KEY_W))
			mirrorRot *= glm::angleAxis(-mirrorRotSpeed * deltaTime, glm::vec3(1, 0, 0));
		if (window->KeyHold(GLFW_KEY_A))
			mirrorRot *= glm::angleAxis(-mirrorRotSpeed * deltaTime, glm::vec3(0, 1, 0));
		if (window->KeyHold(GLFW_KEY_S))
			mirrorRot *= glm::angleAxis(mirrorRotSpeed * deltaTime, glm::vec3(1, 0, 0));
		if (window->KeyHold(GLFW_KEY_D))
			mirrorRot *= glm::angleAxis(mirrorRotSpeed * deltaTime, glm::vec3(0, 1, 0));
		if (window->KeyHold(GLFW_KEY_Q))
			mirrorRot *= glm::angleAxis(mirrorRotSpeed * deltaTime, glm::vec3(0, 0, 1));
		if (window->KeyHold(GLFW_KEY_E))
			mirrorRot *= glm::angleAxis(-mirrorRotSpeed * deltaTime, glm::vec3(0, 0, 1));
		mirrorRot = glm::normalize(mirrorRot);

		if (window->KeyHold(GLFW_KEY_I))
			mirrorPos += mirrorVelocity * glm::vec3(0, 0, -1) * deltaTime;
		if (window->KeyHold(GLFW_KEY_J))
			mirrorPos += mirrorVelocity * glm::vec3(-1, 0, 0) * deltaTime;
		if (window->KeyHold(GLFW_KEY_K))
			mirrorPos += mirrorVelocity * glm::vec3(0, 0, 1) * deltaTime;
		if (window->KeyHold(GLFW_KEY_L))
			mirrorPos += mirrorVelocity * glm::vec3(1, 0, 0) * deltaTime;
		if (window->KeyHold(GLFW_KEY_U))
			mirrorPos += mirrorVelocity * glm::vec3(0, -1, 0) * deltaTime;
		if (window->KeyHold(GLFW_KEY_O))
			mirrorPos += mirrorVelocity * glm::vec3(0, 1, 0) * deltaTime;
	}
}

void Tema1::OnKeyPress(int key, int mods)
{
	// Add key press event
	if (key == GLFW_KEY_1)
	{
		type = 0;
	}

	if (key == GLFW_KEY_2)
	{
		type = 1;
	}

	if (key == GLFW_KEY_3)
	{
		type = 2;
		ResetParticle();
	}

	if (key == GLFW_KEY_4)
	{
		//type = 3;
	}
}

void Tema1::OnKeyRelease(int key, int mods)
{
	// Add key release event
}

void Tema1::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
	// Add mouse move event
}

void Tema1::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
	// Add mouse button press event
}

void Tema1::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
	// Add mouse button release event
}

void Tema1::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
	// Treat mouse scroll event
}

void Tema1::OnWindowResize(int width, int height)
{
	// Treat window resize event
}
