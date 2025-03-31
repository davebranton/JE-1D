#include "plugincontroller.h"
#include "pluginids.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"

namespace Steinberg {
namespace JE1DEQ {

JE1DEQController::JE1DEQController()
{
    // Nothing to initialize
}

JE1DEQController::~JE1DEQController()
{
    // Nothing to clean up
}

tresult PLUGIN_API JE1DEQController::initialize(FUnknown* context)
{
    tresult result = EditControllerEx1::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }
    
    // Register parameters
    
    // HF Rolloff On/Off switch
    parameters.addParameter(
        STR16("HF Rolloff"), // Title
        nullptr,             // Units
        1,                   // Step count (for switch: only 2 steps)
        0,                   // Default normalized value (off)
        Vst::ParameterInfo::kCanAutomate, // Flags
        kHFRolloffOnParam,   // Parameter ID
        0,                   // Unit ID
        STR16("HF Rolloff")  // Short title
    );
    
    // HF Rolloff Amount
    parameters.addParameter(
        STR16("Amount"),     // Title
        STR16("%"),          // Units
        99,                  // Step count
        0,                   // Default normalized value (0%)
        Vst::ParameterInfo::kCanAutomate, // Flags
        kHFRolloffAmountParam, // Parameter ID
        0,                   // Unit ID
        STR16("Amount")      // Short title
    );
    
    // Low Cut On/Off switch
    parameters.addParameter(
        STR16("Low Cut"),    // Title
        nullptr,             // Units
        1,                   // Step count (for switch: only 2 steps)
        0,                   // Default normalized value (off)
        Vst::ParameterInfo::kCanAutomate, // Flags
        kLowCutOnParam,      // Parameter ID
        0,                   // Unit ID
        STR16("Low Cut")     // Short title
    );
    
    // Volume control
    parameters.addParameter(
        STR16("Volume"),     // Title
        STR16("dB"),         // Units
        99,                  // Step count
        1.0,                 // Default normalized value (0dB)
        Vst::ParameterInfo::kCanAutomate, // Flags
        kVolumeParam,        // Parameter ID
        0,                   // Unit ID
        STR16("Vol")         // Short title
    );
    
    // Add indicator for clipping (read-only parameter)
    parameters.addParameter(
        STR16("Clip"),       // Title
        nullptr,             // Units
        1,                   // Step count (for indicator: only 2 states)
        0,                   // Default normalized value (not clipping)
        Vst::ParameterInfo::kIsReadOnly, // Flags - read-only parameter
        kClipIndicatorParam, // Parameter ID
        0,                   // Unit ID
        STR16("Clip")        // Short title
    );
    
    return kResultOk;
}

tresult PLUGIN_API JE1DEQController::terminate()
{
    return EditControllerEx1::terminate();
}

tresult PLUGIN_API JE1DEQController::setComponentState(IBStream* state)
{
    if (!state)
        return kResultFalse;
    
    IBStreamer streamer(state, kLittleEndian);
    
    // Read parameters from state
    int32 hfRolloffOn;
    float hfRolloffAmount;
    int32 lowCutOn;
    float volume;
    
    if (streamer.readInt32(hfRolloffOn) == false)
        return kResultFalse;
    
    if (streamer.readFloat(hfRolloffAmount) == false)
        return kResultFalse;
    
    if (streamer.readInt32(lowCutOn) == false)
        return kResultFalse;
        
    // Try to read volume parameter (for backward compatibility)
    bool volumeRead = streamer.readFloat(volume);
    
    // Set parameter values
    setParamNormalized(kHFRolloffOnParam, hfRolloffOn ? 1.0 : 0.0);
    setParamNormalized(kHFRolloffAmountParam, hfRolloffAmount);
    setParamNormalized(kLowCutOnParam, lowCutOn ? 1.0 : 0.0);
    
    // Set volume if it was in the state
    if (volumeRead)
    {
        setParamNormalized(kVolumeParam, volume);
    }
    
    return kResultOk;
}

}}
