#pragma once

#include "global.h"
#include <ofxsImageEffect.h>


class Reframe360Factory : public OFX::PluginFactoryHelper<Reframe360Factory>
{
public:
    Reframe360Factory()
    : OFX::PluginFactoryHelper<Reframe360Factory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor)
    {}

    virtual void load();
    virtual void unload();
    virtual void describe(OFX::ImageEffectDescriptor& p_Desc);
    virtual void describeInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum p_Context);
    virtual OFX::ImageEffect* createInstance(OfxImageEffectHandle p_Handle, OFX::ContextEnum p_Context);
    std::string paramIdForCam(std::string baseName, int cam);
};
