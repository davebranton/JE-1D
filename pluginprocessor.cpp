#include "pluginprocessor.h"
#include "plugincontroller.h"
#include "pluginids.h"
#include "version.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

// For tanh function
#include <cmath>

namespace Steinberg {
namespace JE1DEQ {

JE1DEQProcessor::JE1DEQProcessor()
: mHFRolloffOn(false)
, mHFRolloffAmount(0.0f)
, mLowCutOn(false)
, mVolume(1.0f)        // Initialize volume to 0dB (unity gain)
, mMaxLevel(0.0f)
, mClipping(false)
, mSampleRate(44100.0f)
{
    // Register parameters
    setControllerClass(JE1DEQControllerUID);
}

JE1DEQProcessor::~JE1DEQProcessor()
{
    // Nothing to clean up
}

tresult PLUGIN_API JE1DEQProcessor::initialize(FUnknown* context)
{
    // Call parent implementation
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    // Set up input and output bus configuration (stereo)
    addAudioInput(STR16("AudioInput"), Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("AudioOutput"), Vst::SpeakerArr::kStereo);

    return kResultOk;
}

tresult PLUGIN_API JE1DEQProcessor::terminate()
{
    return AudioEffect::terminate();
}

tresult PLUGIN_API JE1DEQProcessor::setActive(TBool state)
{
    if (state)
    {
        // Initialize filters when plugin becomes active
        updateFilters();
    }
    else
    {
        // Reset filters when plugin becomes inactive
        mLowShelf1.reset();
        mHighShelf.reset();
        mPeakFilter1.reset();
        mPeakFilter2.reset();
        mLowShelf2.reset();
        mHFRolloff.reset();
        mLowCut.reset();
        
        // Reset clipping indicators
        mMaxLevel = 0.0f;
        mClipping = false;
    }
    
    return AudioEffect::setActive(state);
}

void JE1DEQProcessor::updateFilters()
{
    // Set up filters based on the screenshot - with slightly reduced Q values for stability
    mLowShelf1.makeLowShelf(mSampleRate, 3710.0f, 0.5f, -20.0f);         // 3710Hz Low Shelf, Q=0.5, -20dB
    mHighShelf.makeHighShelf(mSampleRate, 10000.0f, 0.4f, 20.0f);        // 10000Hz High Shelf, Q=0.4, +20dB
    mPeakFilter1.makePeakFilter(mSampleRate, 8200.0f, 1.8f, 5.0f);       // 8200Hz Bell, Q slightly reduced from 2.0 to 1.8
    mPeakFilter2.makePeakFilter(mSampleRate, 50.0f, 0.3f, -10.0f);       // 50Hz Bell, Q=0.3, -10dB
    mLowShelf2.makeLowShelf(mSampleRate, 1300.0f, 0.9f, -7.0f);          // 1300Hz Low Shelf, Q slightly reduced from 1.0 to 0.9
    
    // Set up optional filters with safer Q values
    mHFRolloff.makeLowPass(mSampleRate, 10000.0f, 0.8f);  // High frequency rolloff, reduced Q
    mLowCut.makeHighPass(mSampleRate, 60.0f, 0.7071f);    // Low frequency cut
}

tresult PLUGIN_API JE1DEQProcessor::setupProcessing(Vst::ProcessSetup& setup)
{
    // Store sample rate
    mSampleRate = setup.sampleRate;
    
    // Make sure sample rate is valid
    if (mSampleRate <= 0)
        mSampleRate = 44100.0f; // Default to 44.1kHz if invalid
    
    // Update filters with new sample rate
    updateFilters();
    
    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API JE1DEQProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
    // Support 32-bit float processing
    if (symbolicSampleSize == Vst::kSample32)
        return kResultOk;
    
    return kResultFalse;
}

tresult PLUGIN_API JE1DEQProcessor::process(Vst::ProcessData& data)
{
    // Check if there are parameter changes
    if (data.inputParameterChanges)
    {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numParamsChanged; i++)
        {
            Vst::IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData(i);
            if (paramQueue)
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();
                
                if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultOk)
                {
                    switch (paramQueue->getParameterId())
                    {
                        case kHFRolloffOnParam:
                            mHFRolloffOn = (value > 0.5f);
                            break;
                            
                        case kHFRolloffAmountParam:
                            mHFRolloffAmount = (float)value;
                            break;
                            
                        case kLowCutOnParam:
                            mLowCutOn = (value > 0.5f);
                            break;
                            
                        case kVolumeParam:
                            mVolume = (float)value;
                            break;
                    }
                }
            }
        }
    }
    
    // Process audio
    if (data.numInputs == 0 || data.numOutputs == 0 || !data.inputs[0].channelBuffers32 || !data.outputs[0].channelBuffers32)
    {
        return kResultOk;
    }
    
    // Get audio buffers
    Vst::AudioBusBuffers& inputBus = data.inputs[0];
    Vst::AudioBusBuffers& outputBus = data.outputs[0];
    
    // Get channel count
    int32 numChannels = std::min(inputBus.numChannels, outputBus.numChannels);
    uint32 sampleFrames = data.numSamples;
    
    // Reset max level for this block
    float blockMaxLevel = 0.0f;
    
    // Process each channel
    for (int32 channel = 0; channel < numChannels; channel++)
    {
        float* inputChannel = inputBus.channelBuffers32[channel];
        float* outputChannel = outputBus.channelBuffers32[channel];
        
        // Process each sample
        for (uint32 sample = 0; sample < sampleFrames; sample++)
        {
            float input = inputChannel[sample];
            
            // Check for invalid values
            if (!std::isfinite(input))
                input = 0.0f;
                
            // Apply volume control before EQ (with safety clamping)
            float output = input * std::max(0.0f, std::min(2.0f, mVolume));
            
            // Apply EQ filters
            output = mPeakFilter2.process(output);   // 50Hz Bell first (low to high)
            output = mLowShelf2.process(output);    // 1300Hz Low Shelf
            output = mLowShelf1.process(output);    // 3710Hz Low Shelf
            output = mPeakFilter1.process(output);  // 8200Hz Bell
            output = mHighShelf.process(output);    // 10000Hz High Shelf
            
            // Apply optional filters if enabled
            if (mLowCutOn)
            {
                output = mLowCut.process(output);
            }
            
            if (mHFRolloffOn)
            {
                // Apply HF rolloff using a low-pass filter
                // The mHFRolloffAmount controls how much of the filtered (low-passed) signal to use
                float rolloffAmount = std::max(0.0f, std::min(1.0f, mHFRolloffAmount));
                // Process through low-pass filter (which will attenuate high frequencies)
                float filteredOutput = mHFRolloff.process(output);
                // Mix between the original and filtered signal based on the rolloff amount
                output = filteredOutput * rolloffAmount + output * (1.0f - rolloffAmount);
            }
            
            // Store the result (with safety check for NaN/Inf)
            if (!std::isfinite(output))
                output = 0.0f;
                
            // Apply soft limiting to prevent harsh clipping
            if (output > 0.9f)
                output = 0.9f + (1.0f - 0.9f) * tanh((output - 0.9f) / (1.0f - 0.9f));
            else if (output < -0.9f)
                output = -0.9f + (-1.0f + 0.9f) * tanh((output + 0.9f) / (1.0f - 0.9f));
                
            outputChannel[sample] = output;
            
            // Track maximum level for clipping detection
            float absOutput = std::abs(output);
            if (absOutput > blockMaxLevel)
            {
                blockMaxLevel = absOutput;
            }
        }
    }
    
    // Update max level and check for clipping
    mMaxLevel = std::max(mMaxLevel, blockMaxLevel);
    
    // Set clipping flag if level exceeds 0dB
    bool newClippingState = (blockMaxLevel >= 1.0f);
    if (newClippingState)
    {
        mClipping = true;
        
        // Send a parameter change notification for the clipping state if needed
        if (data.outputParameterChanges)
        {
            int32 index = 0;
            Vst::IParamValueQueue* paramQueue = data.outputParameterChanges->addParameterData(kClipIndicatorParam, index);
            if (paramQueue)
            {
                paramQueue->addPoint(0, 1.0, index);
            }
        }
    }
    else if (mClipping && data.outputParameterChanges) // Reset clip indicator when needed
    {
        // Only send reset message when the current block isn't clipping but overall clip flag is set
        int32 index = 0;
        Vst::IParamValueQueue* paramQueue = data.outputParameterChanges->addParameterData(kClipIndicatorParam, index);
        if (paramQueue)
        {
            paramQueue->addPoint(0, 0.0, index); // Reset clip indicator
        }
    }
    
    return kResultOk;
}

tresult PLUGIN_API JE1DEQProcessor::setState(IBStream* state)
{
    if (!state)
        return kResultFalse;
    
    IBStreamer streamer(state, kLittleEndian);
    
    // Read parameters
    int32 temp;
    if (streamer.readInt32(temp) == false)
        return kResultFalse;
    mHFRolloffOn = temp > 0;
    
    if (streamer.readFloat(mHFRolloffAmount) == false)
        return kResultFalse;
    
    int32 lowCutOnTemp;
    if (streamer.readInt32(lowCutOnTemp) == false)
        return kResultFalse;
    mLowCutOn = lowCutOnTemp > 0;
    
    // Read volume parameter (for backward compatibility, defaults to 1.0 if not in state)
    if (streamer.readFloat(mVolume) == false)
        mVolume = 1.0f;
    
    // Update filters with new state
    updateFilters();
    
    return kResultOk;
}

tresult PLUGIN_API JE1DEQProcessor::getState(IBStream* state)
{
    if (!state)
        return kResultFalse;
    
    IBStreamer streamer(state, kLittleEndian);
    
    // Write parameters
    streamer.writeInt32(mHFRolloffOn ? 1 : 0);
    streamer.writeFloat(mHFRolloffAmount);
    streamer.writeInt32(mLowCutOn ? 1 : 0);
    streamer.writeFloat(mVolume);
    
    return kResultOk;
}

}}
