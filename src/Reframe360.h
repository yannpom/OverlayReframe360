#pragma once

#include <ofxsImageEffect.h>
#include <ofxsInteract.h>
#include <vector>

#include "ImageScaler.h"
#include "RemoteControlProtocol.hpp"
#include "Parameters.h"


class XTouchMini;

class Reframe360 : public OFX::ImageEffect, public RemoteControlProtocol
{
public:
    explicit Reframe360(OfxImageEffectHandle p_Handle);

    /* Override the render */
    virtual void render(const OFX::RenderArguments& p_Args);

    /* Override is identity */
    //virtual bool isIdentity(const OFX::IsIdentityArguments& p_Args, OFX::Clip*& p_IdentityClip, double& p_IdentityTime);

    /* Override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ParamName);

    /* Override changed clip */
    virtual void changedClip(const OFX::InstanceChangedArgs& p_Args, const std::string& p_ClipName);

    /* Set up and run a processor */
    void setupAndProcess(ImageScaler& p_ImageScaler, const OFX::RenderArguments& p_Args);


    void refresh();
    //void processMidiQueue();

    void loadJson();



    // virtual void purgeCaches(void);
    // virtual void syncPrivateData(void);
    // virtual void beginSequenceRender(const OFX::BeginSequenceRenderArguments &args);
    // virtual void endSequenceRender(const OFX::EndSequenceRenderArguments &args);
    // virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod);
    // virtual void getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois);
    // virtual void getFramesNeeded(const OFX::FramesNeededArguments &args, OFX::FramesNeededSetter &frames);
    // virtual void getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences);
    // virtual void beginEdit(void);
    // virtual void endEdit(void);
    // virtual void beginChanged(OFX::InstanceChangeReason reason);
    // virtual void endChanged(OFX::InstanceChangeReason reason);


//private:

    // Does not own the following pointers
    OFX::Clip* m_DstClip;
    OFX::Clip* m_SrcClip;

    Params params;

    OFX::CustomParam* _json_param;

    double _time;
    XTouchMini * _xtouchmini;
    bool _dirty;


};


