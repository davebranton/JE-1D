#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include <cmath>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define a small value to prevent denormals
#define ANTI_DENORMAL 1.0e-20f

namespace Steinberg {
namespace JE1DEQ {

// Biquad Filter implementation
class BiquadFilter {
public:
    BiquadFilter() {
        reset();
    }

    void reset() {
        z1 = z2 = 0.0f;
    }

    void setCoefficients(float a0, float a1, float a2, float b1, float b2) {
        this->a0 = a0;
        this->a1 = a1;
        this->a2 = a2;
        this->b1 = b1;
        this->b2 = b2;
    }

    float process(float input) {
        // Add anti-denormal noise
        input += ANTI_DENORMAL;
        
        float output = input * a0 + z1;
        z1 = input * a1 + z2 - b1 * output;
        z2 = input * a2 - b2 * output;
        return output;
    }

    // Low shelf filter calculation
    void makeLowShelf(float sampleRate, float cutoffFreq, float Q, float gainDB) {
        float A = std::pow(10.0f, gainDB / 40.0f);
        float omega = 2.0f * M_PI * cutoffFreq / sampleRate;
        float alpha = std::sin(omega) / (2.0f * Q);
        float cosOmega = std::cos(omega);
        float beta = std::sqrt(A) / Q;

        float b0 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega + beta * std::sin(omega));
        float b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega);
        float b2 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega - beta * std::sin(omega));
        float a0 = (A + 1.0f) + (A - 1.0f) * cosOmega + beta * std::sin(omega);
        float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega);
        float a2 = (A + 1.0f) + (A - 1.0f) * cosOmega - beta * std::sin(omega);

        setCoefficients(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
    }

    // High shelf filter calculation
    void makeHighShelf(float sampleRate, float cutoffFreq, float Q, float gainDB) {
        float A = std::pow(10.0f, gainDB / 40.0f);
        float omega = 2.0f * M_PI * cutoffFreq / sampleRate;
        float alpha = std::sin(omega) / (2.0f * Q);
        float cosOmega = std::cos(omega);
        float beta = std::sqrt(A) / Q;

        float b0 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + beta * std::sin(omega));
        float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega);
        float b2 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - beta * std::sin(omega));
        float a0 = (A + 1.0f) - (A - 1.0f) * cosOmega + beta * std::sin(omega);
        float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega);
        float a2 = (A + 1.0f) - (A - 1.0f) * cosOmega - beta * std::sin(omega);

        setCoefficients(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
    }

    // Peaking/Bell filter calculation
    void makePeakFilter(float sampleRate, float cutoffFreq, float Q, float gainDB) {
        float A = std::pow(10.0f, gainDB / 40.0f);
        float omega = 2.0f * M_PI * cutoffFreq / sampleRate;
        float alpha = std::sin(omega) / (2.0f * Q);
        float cosOmega = std::cos(omega);

        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * cosOmega;
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * cosOmega;
        float a2 = 1.0f - alpha / A;

        setCoefficients(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
    }

    // High-pass filter calculation
    void makeHighPass(float sampleRate, float cutoffFreq, float Q) {
        float omega = 2.0f * M_PI * cutoffFreq / sampleRate;
        float alpha = std::sin(omega) / (2.0f * Q);
        float cosOmega = std::cos(omega);

        float b0 = (1.0f + cosOmega) / 2.0f;
        float b1 = -(1.0f + cosOmega);
        float b2 = (1.0f + cosOmega) / 2.0f;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * cosOmega;
        float a2 = 1.0f - alpha;

        setCoefficients(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
    }

    // Low-pass filter calculation
    void makeLowPass(float sampleRate, float cutoffFreq, float Q) {
        float omega = 2.0f * M_PI * cutoffFreq / sampleRate;
        float alpha = std::sin(omega) / (2.0f * Q);
        float cosOmega = std::cos(omega);

        float b0 = (1.0f - cosOmega) / 2.0f;
        float b1 = 1.0f - cosOmega;
        float b2 = (1.0f - cosOmega) / 2.0f;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * cosOmega;
        float a2 = 1.0f - alpha;

        setCoefficients(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
    }

private:
    float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};

// Parameter IDs
enum JE1DEQParams {
    kHFRolloffOnParam = 0,
    kHFRolloffAmountParam,
    kLowCutOnParam,
    kVolumeParam,        // Added volume control parameter
    kClipIndicatorParam, // Clip indicator
    kParamCount
};

// JE1DEQ Processor
class JE1DEQProcessor : public Vst::AudioEffect {
public:
    JE1DEQProcessor();
    ~JE1DEQProcessor() SMTG_OVERRIDE;

    // Create function
    static FUnknown* createInstance(void*) { return (Vst::IAudioProcessor*)new JE1DEQProcessor(); }

    // AudioEffect overrides
    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;
    tresult PLUGIN_API process(Vst::ProcessData& data) SMTG_OVERRIDE;
    tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
    tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;
    tresult PLUGIN_API setupProcessing(Vst::ProcessSetup& setup) SMTG_OVERRIDE;
    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE;

protected:
    // Core EQ filters - updated from screenshot
    BiquadFilter mLowShelf1;      // 3710Hz Low Shelf
    BiquadFilter mHighShelf;      // 10000Hz High Shelf
    BiquadFilter mPeakFilter1;    // 8200Hz Bell
    BiquadFilter mPeakFilter2;    // 50Hz Bell
    BiquadFilter mLowShelf2;      // 1300Hz Low Shelf
    
    // Optional filters - switchable
    BiquadFilter mHFRolloff;      // 10kHz+ high-pass with adjustable gain
    BiquadFilter mLowCut;         // 20-40Hz low cut
    
    // Parameters
    bool mHFRolloffOn;
    float mHFRolloffAmount;
    bool mLowCutOn;
    float mVolume;       // Added volume control parameter
    
    // Clipping detection
    float mMaxLevel;
    bool mClipping;
    
    // Processing state
    float mSampleRate;
    
    // Update the filter coefficients based on current settings
    void updateFilters();
};

}}
