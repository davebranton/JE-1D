#include "public.sdk/source/main/pluginfactory.h"
#include "pluginprocessor.h"
#include "plugincontroller.h"
#include "pluginids.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.cpp"

#define stringPluginName "JE1DEQ"

// Define company information
#define stringCompanyName "Your Company"
#define stringCompanyWeb "www.yourcompany.com"
#define stringCompanyEmail "info@yourcompany.com"

//-----------------------------------------------------------------------------
// VST3 Plugin Definitions
//-----------------------------------------------------------------------------

BEGIN_FACTORY_DEF(stringCompanyName, stringCompanyWeb, stringCompanyEmail)

    // Register the JE1DEQ processor
    DEF_CLASS2(INLINE_UID_FROM_FUID(Steinberg::JE1DEQ::JE1DEQProcessorUID),
               PClassInfo::kManyInstances,
               kVstAudioEffectClass,
               stringPluginName,
               Vst::kDistributable,
               Vst::PlugType::kFxEQ,
               FULL_VERSION_STR,
               kVstVersionString,
               Steinberg::JE1DEQ::JE1DEQProcessor::createInstance)

    // Register the JE1DEQ controller
    DEF_CLASS2(INLINE_UID_FROM_FUID(Steinberg::JE1DEQ::JE1DEQControllerUID),
               PClassInfo::kManyInstances,
               kVstComponentControllerClass,
               stringPluginName "Controller",
               0,
               "",
               FULL_VERSION_STR,
               kVstVersionString,
               Steinberg::JE1DEQ::JE1DEQController::createInstance)

END_FACTORY
