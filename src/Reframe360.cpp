#include <ofxsImageEffect.h>
#include <ofxsMultiThread.h>
#include <ofxsProcessing.h>
#include <ctime>
#include <unistd.h>
#include <list>



#include "Reframe360Factory.h"
#include "Reframe360.h"
#include "ImageScaler.h"
#include "MathUtil.h"
#include "Parameters.h"
#include "utils.h"
//#include "XTouchMini.h"
#include <spdlog/fmt/ostr.h>
#include <signal.h>
#include <spdlog/stopwatch.h>

void pitchMatrix(float pitch, float* out)
{
    out[0] = 1.0;
    out[1] = 0;
    out[2] = 0;
    out[3] = 0;
    out[4] = cos(pitch);
    out[5] = -sin(pitch);
    out[6] = 0;
    out[7] = sin(pitch);
    out[8] = cos(pitch);
}

void yawMatrix(float yaw, float* out)
{
    out[0] = cos(yaw);
    out[1] = 0;
    out[2] = sin(yaw);
    out[3] = 0;
    out[4] = 1.0;
    out[5] = 0;
    out[6] = -sin(yaw);
    out[7] = 0;
    out[8] = cos(yaw);
}

void rollMatrix(float roll, float* out)
{
    out[0] = cos(roll);
    out[1] = -sin(roll);
    out[2] = 0;
    out[3] = sin(roll);
    out[4] = cos(roll);
    out[5] = 0;
    out[6] = 0;
    out[7] = 0;
    out[8] = 1.0;
}


void matMul(const float* y, const float* p, float* outmat)
{
    outmat[0] = p[0] * y[0] + p[3] * y[1] + p[6] * y[2];
    outmat[1] = p[1] * y[0] + p[4] * y[1] + p[7] * y[2];
    outmat[2] = p[2] * y[0] + p[5] * y[1] + p[8] * y[2];
    outmat[3] = p[0] * y[3] + p[3] * y[4] + p[6] * y[5];
    outmat[4] = p[1] * y[3] + p[4] * y[4] + p[7] * y[5];
    outmat[5] = p[2] * y[3] + p[5] * y[4] + p[8] * y[5];
    outmat[6] = p[0] * y[6] + p[3] * y[7] + p[6] * y[8];
    outmat[7] = p[1] * y[6] + p[4] * y[7] + p[7] * y[8];
    outmat[8] = p[2] * y[6] + p[5] * y[7] + p[8] * y[8];
}


Reframe360::Reframe360(OfxImageEffectHandle p_Handle)
    : ImageEffect(p_Handle)
    , params(this)
    , _dirty(false)
{
    //reframe_360_singleton = this;
    m_DstClip = fetchClip(kOfxImageEffectOutputClipName);
    m_SrcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);

    // m_Shutter = fetchDoubleParam("shutter");
    // m_Samples = fetchIntParam("samples");
    // m_Bilinear = fetchBooleanParam("bilinear");
    // m_noMotionBlur = fetchBooleanParam("noMotionBlur");
    // m_mbRendering = fetchChoiceParam("mbRendering");

    _json_param = fetchCustomParam("json");

    SPDLOG_DEBUG("About to load JSON");
    loadJson();

    SPDLOG_DEBUG("JSON loaded");

    // OfxPointD size = getProjectSize();
    // printf("size: %f %f\n", size.x, size.y);
    // OfxPointD offset = getProjectOffset();
    // printf("offset: %f %f\n", offset.x, offset.y);
    // OfxPointD extent = getProjectExtent();
    // printf("extent: %f %f\n", extent.x, extent.y);
    // // double getProjectPixelAspectRatio(void) const;
    // double duration = getEffectDuration();
    // printf("duration: %f\n", duration);
    // double framerate = getFrameRate();
    // printf("framerate: %f\n", framerate);

    _xtouchmini = NULL;
    //_xtouchmini = new XTouchMini(*this);
}

void Reframe360::loadJson() {
    std::string json;
    _json_param->getValue(json);
    params.loadJson(json);
}

void Reframe360::render(const OFX::RenderArguments& p_Args)
{
    if ((m_DstClip->getPixelDepth() == OFX::eBitDepthFloat) && (m_DstClip->getPixelComponents() == OFX::ePixelComponentRGBA)) {
        ImageScaler imageScaler(*this);
        setupAndProcess(imageScaler, p_Args);
    } else {
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
}

void Reframe360::changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName)
{
    //printf("Reframe360::changedParam: reason %d time %f name %s\n", p_Args.reason, p_Args.time, p_ParamName.c_str());
    // eChangeUserEdit,    /**< @brief A user actively editted something in the plugin, eg: changed the value of an integer param on an interface */
    // eChangePluginEdit,  /**< @brief The plugin's own code changed something in the instance, eg: a callback on on param settting the value of another */
    // eChangeTime         /**< @brief The current value of a parameter has changed because the param animates and the current time has changed */

    SPDLOG_DEBUG("reason {} time {} name {}", p_Args.reason, p_Args.time, p_ParamName);
    _time = p_Args.time;


    if (p_Args.reason == OFX::eChangeUserEdit) {
        // if (_time == 750) {
        //     sleep(60);
        //     raise(SIGINT);
        // }

        // CTRL+Z 
        std::string json_host;
        _json_param->getValue(json_host);
        const std::string & json = params.getJson();
        _dirty = json != json_host;
        //loadJson();
    }
}

void Reframe360::changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName)
{
    SPDLOG_INFO("clip name {}", p_ClipName);
}


void Reframe360::setupAndProcess(ImageScaler& p_ImageScaler, const OFX::RenderArguments& p_Args)
{
    //spdlog::stopwatch sw;
    double time = p_Args.time;

    // Get the dst image
    std::auto_ptr<OFX::Image> dst(m_DstClip->fetchImage(time));
    OFX::BitDepthEnum dstBitDepth = dst->getPixelDepth();
    OFX::PixelComponentEnum dstComponents = dst->getPixelComponents();

    // Get the src image
    std::auto_ptr<OFX::Image> src(m_SrcClip->fetchImage(time));
    OFX::BitDepthEnum srcBitDepth = src->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src->getPixelComponents();

    const OfxRectI& bounds = src->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;
    //SPDLOG_DEBUG("src->getBounds {} {}", width, height);

    // Check to see if the bit depth and number of components are the same
    if ((srcBitDepth != dstBitDepth) || (srcComponents != dstComponents)) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
    }

    bool bilinear;
    switch (globalMotionBlurRendering) {
        case MB_RENDERING_AUTO:
            bilinear = width * height >= MOTION_BLUR_AUTO_PIXELS;
            break;
        case MB_RENDERING_ON:
            bilinear = true;
            break;
        case MB_RENDERING_OFF:
            bilinear = false;
            break;
    }

    float mb_shutter = params.getMotionBlur();
    int mb_samples = bilinear && mb_shutter > 0 ? 50 : 1;
    
    SPDLOG_DEBUG("RENDER time {} mb_samples {} mb_shutter {} bilinear {}", time, mb_samples, mb_shutter, bilinear);

    float* fovs = (float*)malloc(sizeof(float) * mb_samples);
    float* tinyplanets = (float*)malloc(sizeof(float) * mb_samples);
    float* rectilinears = (float*)malloc(sizeof(float) * mb_samples);
    float* rotmats = (float*)malloc(sizeof(float) * mb_samples * 9);
    // Yaw needs to be inverted in multi threaded rendering... not sure why
    bool invertYaw = !p_Args.isEnabledCudaRender && !p_Args.isEnabledOpenCLRender && !p_Args.isEnabledMetalRender;

    for (int i = 0; i < mb_samples; i++)
    {
        float offset = 0;
        if (mb_samples > 1)
        {
            offset = fitRange((float)i, 0, mb_samples - 1.0f, -.5f, .5f);
            offset *= mb_shutter / 360;
        }

        const ParamValues & v = params.getValuesAtTime(time, offset);
        //SPDLOG_INFO("time {} params {}", time, values);

        float pitchMat[9];
        float yawMat[9];
        float rollMat[9];
        float pitchRollMat[9];
        float rotMat[9];

        pitchMatrix(-v.pitch / 180 * (float)M_PI, pitchMat);
        yawMatrix((invertYaw?-v.yaw:v.yaw) / 180 * (float)M_PI, yawMat);
        rollMatrix(-v.roll / 180 * (float)M_PI, rollMat);

        matMul(pitchMat, rollMat, pitchRollMat);
        matMul(yawMat, pitchRollMat, rotMat);

        memcpy(&rotmats[i * 9], rotMat, sizeof(float) * 9);

        fovs[i] = v.fov;
        tinyplanets[i] = v.tiny_planet;
        rectilinears[i] = v.rectilinear;
    }

    // log fov
    for (int i=0; i<mb_samples; ++i) {
        fovs[i] = 0.01f * powf(10, fovs[i]);
    }

    // Set the images
    p_ImageScaler.setDstImg(dst.get());
    p_ImageScaler.setSrcImg(src.get());

    // Setup OpenCL and CUDA Render arguments
    p_ImageScaler.setGPURenderArgs(p_Args);

    // Set the render window
    p_ImageScaler.setRenderWindow(p_Args.renderWindow);

    // Set the scales
    p_ImageScaler.setParams(rotmats, fovs, tinyplanets, rectilinears, mb_samples, bilinear);

    // Call the base class process member, this will call the derived templated process code
    p_ImageScaler.process();

    //SPDLOG_INFO("Render time {}", sw);
}


void Reframe360::refresh() {
    SPDLOG_INFO("Save JSON to param");
    _dirty = false;
    _json_param->setValue(params.getJson());
}

// void Reframe360::processMidiQueue() {
//     if (!_xtouchmini)
//         return;
//     _xtouchmini->processQueue();
// }



// void Reframe360::purgeCaches(void) { SPDLOG_INFO("x"); }
// void Reframe360::syncPrivateData(void) { SPDLOG_INFO("x"); }
// void Reframe360::beginSequenceRender(const OFX::BeginSequenceRenderArguments &args) { SPDLOG_INFO("x"); }
// void Reframe360::endSequenceRender(const OFX::EndSequenceRenderArguments &args) { SPDLOG_INFO("x"); }
// bool Reframe360::isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime) {
//     SPDLOG_INFO("x");
//     return true;
//     }
// bool Reframe360::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) { SPDLOG_INFO("x"); return true; }
// void Reframe360::getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois) { SPDLOG_INFO("x"); }
// void Reframe360::getFramesNeeded(const OFX::FramesNeededArguments &args, OFX::FramesNeededSetter &frames) { SPDLOG_INFO("x"); }
// void Reframe360::getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences) { SPDLOG_INFO("x"); }
// void Reframe360::beginEdit(void) { SPDLOG_INFO("x"); }
// void Reframe360::endEdit(void) { SPDLOG_INFO("x"); }
// void Reframe360::beginChanged(OFX::InstanceChangeReason reason) { SPDLOG_INFO("x"); }
// void Reframe360::endChanged(OFX::InstanceChangeReason reason) { SPDLOG_INFO("x"); }



