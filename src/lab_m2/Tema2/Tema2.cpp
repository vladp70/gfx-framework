#include "lab_m2/Tema2/Tema2.h"

#include <vector>
#include <iostream>
#include <omp.h>

#include "pfd/portable-file-dialogs.h"

using namespace std;
using namespace m2;

#define THREAD_NUM 8

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
    numWhitePixelsWatermark = 0;
    blurRadius = 1;
    binarizationThreshold = 25.f;
    matchThreshold = 0.95f;
}


Tema2::~Tema2()
{
}


void Tema2::Init()
{
    // Process watermark
    watermark = TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "test_images", "watermark.png"), nullptr, "watermark", true, true);
    grayscaleWatermark = TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "test_images", "watermark.png"), nullptr, "gswatermark", true, true);
    processedWatermark = TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "test_images", "watermark.png"), nullptr, "reswatermark", true, true);
    elementSize = glm::ivec2(watermark->GetWidth(), watermark->GetHeight());
    elementChannels = processedWatermark->GetNrChannels();
    Frontier(watermark, processedWatermark);
    numWhitePixelsWatermark = CountWhitePixels(processedWatermark);
    cout << "White pixels in watermark: " << numWhitePixelsWatermark << endl;

    // Load default texture for imagine processing and process main image
    currentFilename = PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "test_images", "star.png");
    OnFileSelected(currentFilename);

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

    auto textureImage = (gpuProcessing == true) ? originalImage : displayImage;
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

        glReadPixels(0, 0, displayImage->GetWidth(), displayImage->GetHeight(), format, GL_UNSIGNED_BYTE, displayImage->GetImageData());
        displayImage->UploadNewData(displayImage->GetImageData());
        SaveImage("shader_processing_" + std::to_string(outputMode));

        float aspectRatio = static_cast<float>(displayImage->GetWidth()) / displayImage->GetHeight();
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
        // Init texture
        currentFilename = fileName;
        std::cout << fileName << endl;
        originalImage = TextureManager::LoadTexture(currentFilename, nullptr, "image", true, true);
        grayscaleImage = TextureManager::LoadTexture(currentFilename, nullptr, "gsimage", true, true);
        processedImage = TextureManager::LoadTexture(currentFilename, nullptr, "newImage", true, true);
        resultImage = TextureManager::LoadTexture(currentFilename, nullptr, "resultImage", true, true);
        displayImage = originalImage;

        // Calculate stats
        imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());
        imageChannels = processedImage->GetNrChannels();
        imageRowSize = imageChannels * imageSize.x;

        // Process image
        GrayScale(originalImage, grayscaleImage);
        Frontier(originalImage, processedImage);
        imageProcessed = false;

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

    glm::ivec2 currentImageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());

    for (int i = 0; i < currentImageSize.y; i++)
    {
        for (int j = 0; j < currentImageSize.x; j++)
        {
            int offset = channels * (i * currentImageSize.x + j);

            // Reset save image data
            char value = static_cast<char>(data[offset + 0] * 0.2f + data[offset + 1] * 0.71f + data[offset + 2] * 0.07f);
            memset(&newData[offset], value, 3);
        }
    }

    processedImage->UploadNewData(newData);
}

void Tema2::Blur(Texture2D* originalImage, Texture2D* processedImage)
{
    unsigned int channels = originalImage->GetNrChannels();
    unsigned char* data = originalImage->GetImageData();
    unsigned char* newData = processedImage->GetImageData();

    if (channels < 3)
        return;

    glm::ivec2 currentImageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());
    int offset = channels * (1 * currentImageSize.x + 1);
    int idx;
    float samples = pow((2 * blurRadius + 1), 2);

    for (int i = blurRadius; i < currentImageSize.y - blurRadius; ++i)
    {
        offset = channels * i * currentImageSize.x;
        for (int j = blurRadius; j < currentImageSize.x - blurRadius; ++j)
        {
            offset += channels;

            // Apply blur
            float r, g, b;
            r = g = b = 0;
            for (int di = -blurRadius; i <= blurRadius; i++)
            {
                for (int dj = -blurRadius; j <= blurRadius; j++)
                {
                    idx = channels * ((i + di) * currentImageSize.x + (j + dj));
                    r += static_cast<float>(data[idx]);
                    g += static_cast<float>(data[idx + 1]);
                    b += static_cast<float>(data[idx + 2]);
                }
            }

            memset(&newData[offset], static_cast<unsigned char>(r / samples), 1);
            memset(&newData[offset+1], static_cast<unsigned char>(g / samples), 1);
            memset(&newData[offset+2], static_cast<unsigned char>(b / samples), 1);
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

    glm::ivec2 currentImageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());
    
    #pragma omp parallel shared(currentImageSize, channels, data, newData)
    for (int i = 1; i < currentImageSize.y - 1; ++i)
    {
        for (int j = 1; j < currentImageSize.x - 1; ++j)
        {
            int idx;
            int offset = channels * (i * currentImageSize.x + j);

            // Apply grayscale and Sobel
            float vals[9];

            for (int di = -1; di <= 1; di++)
            {
                for (int dj = -1; dj <= 1; dj++)
                {
                    idx = channels * ((i + di) * currentImageSize.x + (j + dj));
                    vals[3 * (di + 1) + (dj + 1)] =
                        0.21f * data[idx] + 0.71f * data[idx + 1] + 0.07f * data[idx + 2];
                }
            }

            float dx = abs(-vals[0] + vals[2] - 2 * vals[3] + 2 * vals[5] - vals[6] + vals[8]);
            float dy = abs(vals[0] + 2 * vals[1] + vals[2] - vals[6] - 2 * vals[7] - vals[8]);

            float d = sqrt(dx*dx + dy*dy);

            // Binarization based on threshold
            unsigned char value;
            if (d > binarizationThreshold)
            {
                value = 255;
            }
            else
            {
                value = 0;
            }

            // Set the processed pixel value
            #pragma omp critical
            memset(&newData[offset], value, 3);
        }
    }

    int offset;

    // Process borders
    for (int i = 0; i < currentImageSize.y; i++)
    {
        for (int j = 0; j < currentImageSize.x; j+= currentImageSize.x - 1)
        {
            offset = channels * (i * currentImageSize.x + j);
            memset(&newData[offset], static_cast<unsigned char>(0), 3);
        }
    }
    for (int i = 0; i < currentImageSize.y; i+= currentImageSize.y - 1)
    {
        for (int j = 0; j < currentImageSize.x; j ++)
        {
            offset = channels * (i * currentImageSize.x + j);
            memset(&newData[offset], static_cast<unsigned char>(0), 3);
        }
    }

    processedImage->UploadNewData(newData);
}

float Tema2::CountWhitePixels(Texture2D* processedWatermark) {
    unsigned int channels = processedWatermark->GetNrChannels();
    unsigned char* data = processedWatermark->GetImageData();

    if (channels < 3)
        return 0.0f;

    glm::ivec2 elementSize = glm::ivec2(processedWatermark->GetWidth(), processedWatermark->GetHeight());

    int offset = 0;
    int whitePixels = 0;

    for (int i = 0; i < elementSize.y; ++i)
    {
        for (int j = 0; j < elementSize.x; ++j)
        {
            if (data[offset] == 255)
                whitePixels++;

            offset += channels;
        }
    }

    return static_cast<float>(whitePixels);
}

float Tema2::CheckMatchPercentage(unsigned char* data, unsigned char* elementData, int x, int y) {
    if (imageChannels < 3)
        return 0.0f;

    int startOffset = imageChannels * (y * imageSize.x + x);
    int offset = 0;
    int elementOffset = 0;
    int numMatches = 0;
    int i, j;

    for (i = y; i < y + elementSize.y; ++i)
    {
        startOffset = imageChannels * (i * imageSize.x + x);
        offset = startOffset;
        for (j = x; j < x + elementSize.x; ++j)
        {
            if (elementData[elementOffset] == 255 && data[offset] == 255) {
                numMatches++;
            }

            elementOffset += elementChannels;
            offset += imageChannels;
        }
        startOffset += imageRowSize;
    }
    
    if (numMatches) {
        return numMatches / numWhitePixelsWatermark;
    }
    else
        return 0.0f;
}

void Tema2::HighlightMatch(Texture2D* originalImage, Texture2D* watermark, int x, int y) {
    unsigned char* newData = originalImage->GetImageData();
    unsigned int channels = originalImage->GetNrChannels();
    unsigned char* elementData = watermark->GetImageData();
    unsigned int elementChannels = watermark->GetNrChannels();

    if (channels < 3)
        return;

    int startOffset = channels * (y * imageSize.x + x);
    int rowSize = channels * imageSize.x;
    int elementOffset = 0;
    int offset = 0;

    for (int i = y; i < y + elementSize.y; ++i)
    {
        offset = startOffset;
        for (int j = x; j < x + elementSize.x; ++j)
        {
            memset(&newData[offset], static_cast<unsigned char>(255.0f), 1);
            memset(&newData[offset + 1], static_cast<unsigned char>(0.0f), 2);

            elementOffset += elementChannels;
            offset += channels;
        }
        startOffset += rowSize;
    }
    
    originalImage->UploadNewData(newData);
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
            for (int rgb = 0; rgb < 3; rgb++) {
                newData[offset + rgb] = newData[offset + rgb] - elementData[elementOffset + rgb];
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
    int numMatches = 0;
    unsigned int channels = processedImage->GetNrChannels();
    unsigned char* data = processedImage->GetImageData();
    unsigned char* elementData = processedWatermark->GetImageData();
    unsigned char* newData = result->GetImageData();
    float match = 0;
    int i, j;

    if (channels < 3)
        return;
    else
        cout << "Image has " << channels << " channels." << endl;

    for (i = 0; i < imageSize.y - elementSize.y; ++i)
    {
        for (j = 0; j < imageSize.x - elementSize.x; ++j)
        {
            match = CheckMatchPercentage(data, elementData, j, i);

            if (match > matchThreshold) {
                ++numMatches;
                cout << "Found match " << match <<" at: " << j << ", " << i << endl;
                RemoveMatch(result, watermark, j, i);
                j += elementSize.x;
            }
            else if (match < 0.1f) {
                j += elementSize.x / 2;
            }
        }
    }

    cout << "Found " << numMatches << " matches total." << endl;
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

        switch (outputMode)
        {
        case 0:
            displayImage = originalImage;
            break;
        case 1:
            displayImage = grayscaleImage;
            break;
        case 2:
            displayImage = processedImage;
            break;
        case 3:
            if (!imageProcessed) {
                RemoveAllMatches(processedImage, processedWatermark, resultImage);
                imageProcessed = true;
            }
            displayImage = resultImage;
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
