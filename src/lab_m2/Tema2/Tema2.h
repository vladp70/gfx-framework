#pragma once

#include <string>

#include "components/simple_scene.h"
#include "core/gpu/frame_buffer.h"


namespace m2
{
    class Tema2 : public gfxc::SimpleScene
    {
     public:
        Tema2();
        ~Tema2();

        void Init() override;

     private:
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;

        void OnInputUpdate(float deltaTime, int mods) override;
        void OnKeyPress(int key, int mods) override;
        void OnKeyRelease(int key, int mods) override;
        void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
        void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
        void OnWindowResize(int width, int height) override;

        void OpenDialog();
        void OnFileSelected(const std::string &fileName);

        // Processing effects
        void GrayScale(Texture2D* originalImage, Texture2D* processedImage);
        void Blur(Texture2D* originalImage, Texture2D* processedImage);
        void Frontier(Texture2D* originalImage, Texture2D* processedImage);
        void RemoveAllMatches(Texture2D* processedImage, Texture2D* processedWatermark,Texture2D* result);
        float CheckMatchPercentage(unsigned char* imageData, unsigned char* elementData, int x, int y);
        float CountWhitePixels(Texture2D* processedWatermark);
        void HighlightMatch(Texture2D* originalImage, Texture2D* watermark, int x, int y);
        void RemoveMatch(Texture2D* originalImage, Texture2D* watermark, int x, int y);
        void SaveImage(const std::string &fileName);

     private:
        Texture2D *watermark;
        Texture2D *grayscaleWatermark;
        Texture2D *processedWatermark;
        Texture2D *originalImage;
        Texture2D *grayscaleImage;
        Texture2D *processedImage;
        Texture2D *resultImage;
        Texture2D *displayImage;
        float numWhitePixelsWatermark;
        int blurRadius;
        float binarizationThreshold;
        float matchThreshold;
        bool imageProcessed;
        glm::ivec2 imageSize;
        glm::ivec2 elementSize;
        int imageRowSize;
        unsigned int imageChannels;
        unsigned int elementChannels;

        std::string currentFilename;
        int outputMode;
        bool gpuProcessing;
        bool saveScreenToImage;
    };
}   // namespace m2
