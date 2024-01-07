#include "lab_m2/Tema2/Tema2.h"

#include <vector>
#include <iostream>

#include "pfd/portable-file-dialogs.h"

using namespace std;
using namespace m2;


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */


Tema2::Tema2()
{
    outputMode = 0;
    gpuProcessing = false;
    saveScreenToImage = false;
    window->SetSize(600, 566);
    notNullWatermark = 18128;
}


Tema2::~Tema2()
{
}


void Tema2::Init()
{
    // Load default texture fore imagine processing
    currentFilename = PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "test_images", "star.png");
    watermark = TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "test_images", "watermark.png"), nullptr, "watermark", true, true);
    processedWatermark = TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "test_images", "watermark.png"), nullptr, "watermark", true, true);
    originalImage = TextureManager::LoadTexture(currentFilename, nullptr, "image", true, true);
    processedImage = TextureManager::LoadTexture(currentFilename, nullptr, "newImage", true, true);
    resultImage = TextureManager::LoadTexture(currentFilename, nullptr, "newImage", true, true);

    {
        Mesh* mesh = new Mesh("quad");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "quad.obj");
        mesh->UseMaterials(false);
        meshes[mesh->GetMeshID()] = mesh;
    }

    std::string shaderPath = PATH_JOIN(window->props.selfDir, SOURCE_PATH::M2, "Tema2", "shaders");

    // Create a shader program for particle system
    {
        Shader *shader = new Shader("ImageProcessing");
        shader->AddShader(PATH_JOIN(shaderPath, "VertexShader.glsl"), GL_VERTEX_SHADER);
        shader->AddShader(PATH_JOIN(shaderPath, "FragmentShader.glsl"), GL_FRAGMENT_SHADER);

        shader->CreateAndLink();
        shaders[shader->GetName()] = shader;
    }
}


void Tema2::FrameStart()
{
}


void Tema2::Update(float deltaTimeSeconds)
{
    ClearScreen();

    auto shader = shaders["ImageProcessing"];
    shader->Use();

    if (saveScreenToImage)
    {
        window->SetSize(originalImage->GetWidth(), originalImage->GetHeight());
    }

    int flip_loc = shader->GetUniformLocation("flipVertical");
    glUniform1i(flip_loc, saveScreenToImage ? 0 : 1);

    int screenSize_loc = shader->GetUniformLocation("screenSize");
    glm::ivec2 resolution = window->GetResolution();
    glUniform2i(screenSize_loc, resolution.x, resolution.y);

    int outputMode_loc = shader->GetUniformLocation("outputMode");
    glUniform1i(outputMode_loc, outputMode);

    int locTexture = shader->GetUniformLocation("textureImage");
    glUniform1i(locTexture, 0);

    auto textureImage = (gpuProcessing == true) ? originalImage : processedImage;
    textureImage->BindToTextureUnit(GL_TEXTURE0);

    RenderMesh(meshes["quad"], shader, glm::mat4(1));

    if (saveScreenToImage)
    {
        saveScreenToImage = false;

        GLenum format = GL_RGB;
        if (originalImage->GetNrChannels() == 4)
        {
            format = GL_RGBA;
        }

        glReadPixels(0, 0, originalImage->GetWidth(), originalImage->GetHeight(), format, GL_UNSIGNED_BYTE, processedImage->GetImageData());
        processedImage->UploadNewData(processedImage->GetImageData());
        SaveImage("shader_processing_" + std::to_string(outputMode));

        float aspectRatio = static_cast<float>(originalImage->GetWidth()) / originalImage->GetHeight();
        window->SetSize(static_cast<int>(600 * aspectRatio), 600);
    }
}


void Tema2::FrameEnd()
{
    DrawCoordinateSystem();
}


void Tema2::OnFileSelected(const std::string &fileName)
{
    if (fileName.size())
    {
        currentFilename = fileName;
        std::cout << fileName << endl;
        originalImage = TextureManager::LoadTexture(fileName, nullptr, "image", true, true);
        processedImage = TextureManager::LoadTexture(fileName, nullptr, "newImage", true, true);
        resultImage = TextureManager::LoadTexture(fileName, nullptr, "resultImage", true, true);
        watermark = TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "test_images", "watermark.png"), nullptr, "watermark", true, true);

        float aspectRatio = static_cast<float>(originalImage->GetHeight()) / originalImage->GetWidth();
        window->SetSize(600, static_cast<int>(600 * aspectRatio));
    }
}


void Tema2::GrayScale(Texture2D* originalImage, Texture2D* processedImage)
{
    unsigned int channels = originalImage->GetNrChannels();
    unsigned char* data = originalImage->GetImageData();
    unsigned char* newData = processedImage->GetImageData();

    if (channels < 3)
        return;

    glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());

    for (int i = 0; i < imageSize.y; i++)
    {
        for (int j = 0; j < imageSize.x; j++)
        {
            int offset = channels * (i * imageSize.x + j);

            // Reset save image data
            char value = static_cast<char>(data[offset + 0] * 0.2f + data[offset + 1] * 0.71f + data[offset + 2] * 0.07f);
            memset(&newData[offset], value, 3);
        }
    }

    processedImage->UploadNewData(newData);
}

void Tema2::Frontier(Texture2D* originalImage, Texture2D* processedImage)
{
    unsigned int channels = originalImage->GetNrChannels();
    unsigned char* data = originalImage->GetImageData();
    unsigned char* newData = processedImage->GetImageData();

    if (channels < 3)
        return;

    glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());

    int offset = channels * (1 * imageSize.x + 1);

    for (int i = 1; i < imageSize.y - 1; ++i)
    {
        offset = channels * i * imageSize.x;
        for (int j = 1; j < imageSize.x - 1; ++j)
        {
            offset += channels;

            // Apply grayscale
            float vals[9];

            for (int di = -1; di <= 1; di++)
            {
                for (int dj = -1; dj <= 1; dj++)
                {
                    int idx = channels * ((i + di) * imageSize.x + (j + dj));
                    vals[3 * (di + 1) + (dj + 1)] =
                        0.21f * data[idx] + 0.71f * data[idx + 1] + 0.07f * data[idx + 2];
                }
            }

            float dx = abs(-vals[0] + vals[2] - 2 * vals[3] + 2 * vals[5] - vals[6] + vals[8]);
            float dy = abs(vals[0] + 2 * vals[1] + vals[2] - vals[6] - 2 * vals[7] - vals[8]);

            float d = dx + dy;

            // Binarization based on threshold
            if (d > 0.9f)
            {
                d = 1.0f;
            }
            else
            {
                d = 0.0f;
            }

            // Set the processed pixel value
            memset(&newData[offset], static_cast<unsigned char>(d * 255.0f), 3);
        }
    }

    // Process borders
    for (int i = 0; i < imageSize.y; i++)
    {
        for (int j = 0; j < imageSize.x; j+=imageSize.x - 1)
        {
            int offset = channels * (i * imageSize.x + j);
            memset(&newData[offset], static_cast<unsigned char>(0), 3);
        }
    }
    for (int i = 0; i < imageSize.y; i+=imageSize.y - 1)
    {
        for (int j = 0; j < imageSize.x; j ++)
        {
            int offset = channels * (i * imageSize.x + j);
            memset(&newData[offset], static_cast<unsigned char>(0), 3);
        }
    }

    processedImage->UploadNewData(newData);
}

float Tema2::CheckMatchPercentage(Texture2D* processedImage, Texture2D* processedWatermark, int x, int y) {
    unsigned int channels = processedImage->GetNrChannels();
    unsigned int elementChannels = processedWatermark->GetNrChannels();
    unsigned char* data = processedImage->GetImageData();
    unsigned char* elementData = processedWatermark->GetImageData();

    if (channels < 3)
        return 0.0f;

    glm::ivec2 imageSize = glm::ivec2(processedImage->GetWidth(), processedImage->GetHeight());
    glm::ivec2 elementSize = glm::ivec2(processedWatermark->GetWidth(), processedWatermark->GetHeight());

    int startOffset = channels * (y * imageSize.x + x);
    int offset = 0;
    int rowSize = channels * imageSize.x;
    int elementOffset = 0;
    int numMatches = 0;

    for (int i = y; i < y + elementSize.y; ++i)
    {
        offset = startOffset;
        for (int j = x; j < x + elementSize.x; ++j)
        {
            if (elementData[elementOffset] && (data[offset] == elementData[elementOffset])) {
                ++numMatches;
            }

            elementOffset += elementChannels;
            offset += channels;
        }
        startOffset += rowSize;
    }
    
    if (numMatches) {
        return numMatches / notNullWatermark;
    }
    else
        return 0.0f;
}

void Tema2::RemoveMatch(Texture2D* originalImage, Texture2D* watermark, int x, int y) {
    unsigned char* newData = originalImage->GetImageData();
    unsigned int channels = originalImage->GetNrChannels();
    unsigned char* elementData = watermark->GetImageData();
    unsigned int elementChannels = watermark->GetNrChannels();

    if (channels < 3)
        return;

    glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());
    glm::ivec2 elementSize = glm::ivec2(watermark->GetWidth(), watermark->GetHeight());

    int startOffset = channels * (y * imageSize.x + x);
    int rowSize = channels * imageSize.x;
    int elementOffset = 0;
    int offset = 0;

    for (int i = y; i < y + elementSize.y; ++i)
    {
        offset = startOffset;
        for (int j = x; j < x + elementSize.x; ++j)
        {
            if (elementData[elementOffset] && newData[offset] == elementData[elementOffset]) {
                memset(&newData[offset], static_cast<unsigned char>(255.0f), 1);
                memset(&newData[offset + 1], static_cast<unsigned char>(0.0f), 2);
            }

            elementOffset += elementChannels;
            offset += channels;
        }
        startOffset += rowSize;
    }
    
    originalImage->UploadNewData(newData);
}

void Tema2::RemoveAllMatches(Texture2D* processedImage, Texture2D* processedWatermark, Texture2D* result)
{
    Frontier(originalImage, processedImage);
    Frontier(watermark, processedWatermark);

    int numMatches = 0;
    unsigned int channels = processedImage->GetNrChannels();
    unsigned char* data = processedImage->GetImageData();
    unsigned char* elementData = processedWatermark->GetImageData();
    unsigned char* newData = result->GetImageData();

    if (channels < 3)
        return;
    else
        cout << "Image has " << channels << " channels." << endl;

    glm::ivec2 imageSize = glm::ivec2(processedImage->GetWidth(), processedImage->GetHeight());
    glm::ivec2 elementSize = glm::ivec2(processedWatermark->GetWidth(), processedWatermark->GetHeight());

    for (int i = 0; i < imageSize.y - elementSize.y; ++i)
    {
        for (int j = 0; j < imageSize.x - elementSize.x; ++j)
        {
            float match = CheckMatchPercentage(processedImage, processedWatermark, j, i);

            if (match > 0.3f) {
                ++numMatches;
                cout << "Found match " << match <<" at: " << j << ", " << i << endl;
                RemoveMatch(processedImage, processedWatermark, j, i);
                j += elementSize.x;
            }
        }
    }

    cout << "Found " << numMatches << " matches total." << endl;
    //processedImage->UploadNewData(newData);
}


void Tema2::SaveImage(const std::string &fileName)
{
    cout << "Saving image! ";
    processedImage->SaveToFile((fileName + ".png").c_str());
    cout << "[Done]" << endl;
}


void Tema2::OpenDialog()
{
    std::vector<std::string> filters =
    {
        "Image Files", "*.png *.jpg *.jpeg *.bmp",
        "All Files", "*"
    };

    auto selection = pfd::open_file("Select a file", ".", filters).result();
    if (!selection.empty())
    {
        std::cout << "User selected file " << selection[0] << "\n";
        OnFileSelected(selection[0]);
    }
}


/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Tema2::OnInputUpdate(float deltaTime, int mods)
{
    // Treat continuous update based on input
}


void Tema2::OnKeyPress(int key, int mods)
{
    // Add key press event
    if (key == GLFW_KEY_F || key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE)
    {
        OpenDialog();
    }

    if (key == GLFW_KEY_E)
    {
        gpuProcessing = !gpuProcessing;
        if (gpuProcessing == false)
        {
            outputMode = 0;
        }
        cout << "Processing on GPU: " << (gpuProcessing ? "true" : "false") << endl;
    }

    if (key - GLFW_KEY_0 >= 0 && key <= GLFW_KEY_4)
    {
        outputMode = key - GLFW_KEY_0;
        OnFileSelected(currentFilename);

        switch (outputMode)
        {
        case 0:
            break;
        case 1:
            GrayScale(originalImage, processedImage);
            break;
        case 2:
            Frontier(originalImage, processedImage);
            Frontier(watermark, processedWatermark);
            break;
        case 3:
            RemoveAllMatches(processedImage, processedWatermark, resultImage);
            //processedImage = resultImage;
            break;
        default:
            break;
        }
    }

    if (key == GLFW_KEY_S && mods & GLFW_MOD_CONTROL)
    {
        SaveImage("screenshot" + std::to_string(outputMode));
    }
}


void Tema2::OnKeyRelease(int key, int mods)
{
    // Add key release event
}


void Tema2::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Add mouse move event
}


void Tema2::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button press event
}


void Tema2::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button release event
}


void Tema2::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
    // Treat mouse scroll event
}


void Tema2::OnWindowResize(int width, int height)
{
    // Treat window resize event
}
