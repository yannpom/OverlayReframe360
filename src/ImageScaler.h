#pragma once

#include "ofxsProcessing.h"

class ImageScaler : public OFX::ImageProcessor
{
public:
    ImageScaler(OFX::ImageEffect& p_Instance) : OFX::ImageProcessor(p_Instance) {}

    virtual void processImagesOpenCL();
    virtual void processImagesMetal();

    virtual void multiThreadProcessImages(OfxRectI p_ProcWindow);

    void setSrcImg(OFX::Image* p_SrcImg);
    void setParams(float* p_RotMat, float* p_Fov, float* p_Tinyplanet, float* p_Rectilinear, int p_Samples,
                   bool p_Bilinear);

private:
    OFX::Image* _srcImg;
    float* _rotMat;
    float* _fov;
    float* _tinyplanet;
    float* _rectilinear;
    int _samples;
    bool _bilinear;
};