#pragma once


static OFX::DoubleParamDescriptor* defineParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                          const std::string& p_Label,
                                          const std::string& p_Hint, OFX::GroupParamDescriptor* p_Parent, float min,
                                          float max, float default_value, float hardmin = INT_MIN, float hardmax = INT_MAX)
{
    OFX::DoubleParamDescriptor* param = p_Desc.defineDoubleParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default_value);
    param->setRange(hardmin, hardmax);
    param->setIncrement(0.1);
    param->setDisplayRange(min, max);
    param->setDoubleType(OFX::eDoubleTypePlain);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}

static OFX::DoubleParamDescriptor* defineAngleParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                               const std::string& p_Label,
                                               const std::string& p_Hint, OFX::GroupParamDescriptor* p_Parent, float min,
                                               float max, float default_value, float hardmin = INT_MIN,
                                               float hardmax = INT_MAX)
{
    OFX::DoubleParamDescriptor* param = p_Desc.defineDoubleParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default_value);
    param->setRange(hardmin, hardmax);
    param->setIncrement(0.1);
    param->setDisplayRange(min, max);
    param->setDoubleType(OFX::eDoubleTypeAngle);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}

static OFX::IntParamDescriptor* defineIntParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                          const std::string& p_Label,
                                          const std::string& p_Hint, OFX::GroupParamDescriptor* p_Parent, int min, int max,
                                          int default_value, int hardmin = INT_MIN, int hardmax = INT_MAX)
{
    OFX::IntParamDescriptor* param = p_Desc.defineIntParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default_value);
    param->setRange(hardmin, hardmax);
    param->setDisplayRange(min, max);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}

static OFX::ChoiceParamDescriptor* defineChoiceParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                                const std::string& p_Label,
                                                const std::string& p_Hint, OFX::GroupParamDescriptor* p_Parent, int default_value,
                                                std::string p_ChoiceLabels[], int choiceCount)
{
    OFX::ChoiceParamDescriptor* param = p_Desc.defineChoiceParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default_value);

    for (int i = 0; i < choiceCount; ++i)
    {
        auto choice = p_ChoiceLabels[i];

        param->appendOption(choice, choice);
    }

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}

static OFX::BooleanParamDescriptor* defineBooleanParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                                  const std::string& p_Label,
                                                  const std::string& p_Hint, OFX::GroupParamDescriptor* p_Parent,
                                                  bool default_value)
{
    OFX::BooleanParamDescriptor* param = p_Desc.defineBooleanParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);
    param->setDefault(default_value);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}

static OFX::PushButtonParamDescriptor* defineButtonParam(OFX::ImageEffectDescriptor& p_Desc, const std::string& p_Name,
                                                    const std::string& p_Label,
                                                    const std::string& p_Hint, OFX::GroupParamDescriptor* p_Parent)
{
    OFX::PushButtonParamDescriptor* param = p_Desc.definePushButtonParam(p_Name);

    param->setLabels(p_Label, p_Label, p_Label);
    param->setScriptName(p_Name);
    param->setHint(p_Hint);

    if (p_Parent)
    {
        param->setParent(*p_Parent);
    }

    return param;
}