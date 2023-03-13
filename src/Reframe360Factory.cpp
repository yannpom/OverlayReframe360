#include "global.h"
#include "Reframe360Factory.h"
#include <ofxsImageEffect.h>
#include <ofxsMultiThread.h>
#include <ofxsProcessing.h>
#include "Reframe360TransformInteract.h"
#include "define_utils.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>


#include "Parameters.h"


void OFX::Plugin::getPluginIDs(PluginFactoryArray& p_FactoryArray)
{
    static Reframe360Factory reframe360;
    p_FactoryArray.push_back(&reframe360);
}


class Reframe360OverlayDescriptor
    : public OFX::DefaultEffectOverlayDescriptor<Reframe360OverlayDescriptor, Reframe360TransformInteract>
{
};

void Reframe360Factory::describe(OFX::ImageEffectDescriptor& p_Desc)
{
    SPDLOG_INFO("describe");
    // Basic labels
    p_Desc.setLabels(kPluginName, kPluginName, kPluginName);
    p_Desc.setPluginGrouping(kPluginGrouping);
    p_Desc.setPluginDescription(kPluginDescription);

    // Add the supported contexts, only filter at the moment
    p_Desc.addSupportedContext(OFX::eContextFilter);
    p_Desc.addSupportedContext(OFX::eContextGeneral);

    // Add supported pixel depths
    p_Desc.addSupportedBitDepth(OFX::eBitDepthFloat);

    // Set a few flags
    p_Desc.setSingleInstance(true);
    p_Desc.setHostFrameThreading(false);
    p_Desc.setSupportsMultiResolution(false);
    p_Desc.setSupportsTiles(false);
    p_Desc.setTemporalClipAccess(false);
    p_Desc.setRenderTwiceAlways(false);
    p_Desc.setSupportsMultipleClipPARs(false);

    // Setup OpenCL and CUDA render capability flags
    p_Desc.setSupportsOpenCLRender(true);
    p_Desc.setSupportsMetalRender(true);

    p_Desc.setOverlayInteractDescriptor(new Reframe360OverlayDescriptor);
}


static void init_log() {
    static bool init = false;

    if (init)
        return;

    init = true;

    spdlog::set_level(spdlog::level::off);

#ifdef DEBUG
    static const char * LOG_PATTERN = "%^[%L %H:%M:%S.%f %t %s:%# %!] %v%$";
    const char* home_dir = std::getenv("HOME");

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();    
    console_sink->set_pattern(LOG_PATTERN);

    auto file_info_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(std::string{home_dir} + "/reframe.info.log", true);
    file_info_sink->set_pattern(LOG_PATTERN);
    file_info_sink->set_level(spdlog::level::info);

    auto file_debug_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(std::string{home_dir} + "/reframe.debug.log", true);
    file_debug_sink->set_pattern(LOG_PATTERN);
    file_debug_sink->set_level(spdlog::level::debug);

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(console_sink);
    sinks.push_back(file_info_sink);
    sinks.push_back(file_debug_sink);
    auto logger = std::make_shared<spdlog::logger>("name", begin(sinks), end(sinks));
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::debug); 

    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::debug); 
#endif
}

void Reframe360Factory::load()
{
    init_log();
    SPDLOG_INFO("Load");
}
void Reframe360Factory::unload()
{
    SPDLOG_INFO("Unload");
}

void Reframe360Factory::describeInContext(OFX::ImageEffectDescriptor& p_Desc, OFX::ContextEnum /*p_Context*/)
{
    SPDLOG_INFO("describeInContext");
    // Source clip only in the filter context
    // Create the mandated source clip
    OFX::ClipDescriptor* srcClip = p_Desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(false);
    srcClip->setIsMask(false);

    // Create the mandated output clip
    OFX::ClipDescriptor* dstClip = p_Desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->setSupportsTiles(false);

    // Make some pages and to things in
    OFX::PageParamDescriptor* page = p_Desc.definePageParam("Controls");

    // ########################################################################################################################
    OFX::GroupParamDescriptor* mainGroup = p_Desc.defineGroupParam("mainParams");
    mainGroup->setLabel("Global Parameters");
    OFX::CustomParamDescriptor* json_param = p_Desc.defineCustomParam("json");
    json_param->setAnimates(false);
    page->addChild(*json_param);

    OFX::StringParamDescriptor* help_param = p_Desc.defineStringParam("help");
    help_param->setLabel("Help");
    help_param->setAnimates(false);
    help_param->setIsPersistant(false);
    help_param->setDefault("Activate Open FX Overlay");
    help_param->setEnabled(false);
    help_param->setStringType(OFX::eStringTypeMultiLine);
    page->addChild(*help_param);
}

OFX::ImageEffect* Reframe360Factory::createInstance(OfxImageEffectHandle p_Handle, OFX::ContextEnum /*p_Context*/)
{
    SPDLOG_INFO("createInstance");
    return new Reframe360(p_Handle);
}



std::string Reframe360Factory::paramIdForCam(std::string baseName, int cam)
{
    std::stringstream ss;
    ss << baseName << "_hidden_" << cam;
    return ss.str();
}
