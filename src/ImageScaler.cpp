#include "ImageScaler.h"
#include "MathUtil.h"

#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

extern void RunMetalKernel(void* p_CmdQ, int p_Width, int p_Height, float* p_Fov, float* p_Tinyplanet, float* p_Rectilinear, const float* p_Input, float* p_Output,float* p_RotMat, int p_Samples, bool p_Bilinear);

void ImageScaler::processImagesMetal()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());

    static int n = 0;
    int i = n++;
    RunMetalKernel(_pMetalCmdQ, width, height, _fov, _tinyplanet, _rectilinear, input, output, _rotMat, _samples, _bilinear);
}

extern void RunOpenCLKernel(void* p_CmdQ, int p_Width, int p_Height, float* p_Fov, float* p_Tinyplanet,
                            float* p_Rectilinear, const float* p_Input, float* p_Output, float* p_RotMat, int p_Samples,
                            bool p_Bilinear);

void ImageScaler::processImagesOpenCL()
{
    const OfxRectI& bounds = _srcImg->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;

    float* input = static_cast<float*>(_srcImg->getPixelData());
    float* output = static_cast<float*>(_dstImg->getPixelData());

    RunOpenCLKernel(_pOpenCLCmdQ, width, height, _fov, _tinyplanet, _rectilinear, input, output, _rotMat, _samples,
                    _bilinear);
}


void ImageScaler::multiThreadProcessImages(OfxRectI p_ProcWindow)
{
    int width = p_ProcWindow.x2 - p_ProcWindow.x1;
    int height = p_ProcWindow.y2 - p_ProcWindow.y1;

    for (int i = 0; i < _samples; i++)
    {
        mat3 rotMat;
        rotMat[0][0] = _rotMat[i * 9 + 0];
        rotMat[0][1] = _rotMat[i * 9 + 1];
        rotMat[0][2] = _rotMat[i * 9 + 2];
        rotMat[1][0] = _rotMat[i * 9 + 3];
        rotMat[1][1] = _rotMat[i * 9 + 4];
        rotMat[1][2] = _rotMat[i * 9 + 5];
        rotMat[2][0] = _rotMat[i * 9 + 6];
        rotMat[2][1] = _rotMat[i * 9 + 7];
        rotMat[2][2] = _rotMat[i * 9 + 8];

        float fov = _fov[i];
        float aspect = (float)width / (float)height;

        for (int y = p_ProcWindow.y1; y < p_ProcWindow.y2; ++y)
        {
            if (_effect.abort()) break;

            float* dstPix = static_cast<float*>(_dstImg->getPixelAddress(p_ProcWindow.x1, y));

            for (int x = p_ProcWindow.x1; x < p_ProcWindow.x2; ++x)
            {
                vec2 uv = vec2((float)x / width, (float)y / height);

                vec3 dir = vec3(0, 0, 0);
                dir.x = (uv.x * 2) - 1;
                dir.y = (uv.y * 2) - 1;
                dir.y /= aspect;
                dir.z = fov;

                vec3 tinyplanet = tinyPlanetSph(dir);
                tinyplanet = normalize(tinyplanet);

                tinyplanet = rotMat * tinyplanet;
                vec3 rectdir = rotMat * dir;

                rectdir = normalize(rectdir);

                dir = mix(fisheyeDir(dir, rotMat), tinyplanet, _tinyplanet[i]);
                dir = mix(dir, rectdir, _rectilinear[i]);

                vec2 iuv = polarCoord(dir);
                iuv = repairUv(iuv);

                iuv.x *= (width - 1);
                iuv.y *= (height - 1);

                int x_new = p_ProcWindow.x1 + (int)iuv.x;
                int y_new = p_ProcWindow.y1 + (int)iuv.y;

                if ((x_new < width) && (y_new < height))
                {
                    float* srcPix = static_cast<float*>(_srcImg ? _srcImg->getPixelAddress(x_new, y_new) : 0);
                    vec4 interpCol;

                    // do we have a source image to scale up
                    if (srcPix)
                    {
                        if (_bilinear)
                        {
                            interpCol = linInterpCol(iuv, _srcImg, p_ProcWindow, width, height);
                        }
                        else
                        {
                            interpCol = vec4(srcPix[0], srcPix[1], srcPix[2], srcPix[3]);
                        }
                    }
                    else
                    {
                        interpCol = vec4(0, 0, 0, 1.0);
                    }

                    if (i == 0)
                    {
                        dstPix[0] = 0;
                        dstPix[1] = 0;
                        dstPix[2] = 0;
                        dstPix[3] = 0;
                    }

                    dstPix[0] += interpCol.x / _samples;
                    dstPix[1] += interpCol.y / _samples;
                    dstPix[2] += interpCol.z / _samples;
                    dstPix[3] += interpCol.w / _samples;
                }

                // increment the dst pixel
                dstPix += 4;

                continue;
            }
        }
    }
}


void ImageScaler::setSrcImg(OFX::Image* p_SrcImg)
{
    _srcImg = p_SrcImg;
}

void ImageScaler::setParams(float* p_RotMat, float* p_Fov, float* p_Tinyplanet, float* p_Rectilinear, int p_Samples, bool p_Bilinear)
{
    _rotMat = p_RotMat;
    _fov = p_Fov;
    _tinyplanet = p_Tinyplanet;
    _rectilinear = p_Rectilinear;
    _samples = p_Samples;
    _bilinear = p_Bilinear;
}