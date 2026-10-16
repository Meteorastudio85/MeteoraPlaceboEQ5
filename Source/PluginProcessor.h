#pragma once

#include <JuceHeader.h>
#include <array>
#include "PlaceboDSP.h"

class PlaceboAudioProcessor : public juce::AudioProcessor
{
public:
    PlaceboAudioProcessor();
    ~PlaceboAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&,  juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    // Bypass dell'host: il plugin espone il proprio parametro di bypass,
    // cosi' sa sempre se e' acceso o spento (serve al logo illuminato)
    void processBlockBypassed (juce::AudioBuffer<float>&,  juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    bool isPluginActive() const noexcept
    {
        return ! hostBypassed.load() && ! (pBypass != nullptr && pBypass->load() > 0.5f);
    }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Tabelle dei selettori (anche usate dall'editor)
    static constexpr std::array<double, 5> lowFreqTable   { 20.0, 30.0, 60.0, 100.0, 200.0 };
    static constexpr std::array<double, 7> highFreqTable  { 3000.0, 4000.0, 5000.0, 8000.0, 10000.0, 12000.0, 16000.0 };
    static constexpr std::array<double, 3> attenSelTable  { 5000.0, 10000.0, 20000.0 };

private:
    template <typename SampleType>
    void processSamples (juce::AudioBuffer<SampleType>& buffer);

    void updateEq (int numSamplesToSkip);

    std::atomic<float>* pLowBoost  = nullptr;
    std::atomic<float>* pLowAtten  = nullptr;
    std::atomic<float>* pLowFreq   = nullptr;
    std::atomic<float>* pHighBoost = nullptr;
    std::atomic<float>* pBandwidth = nullptr;
    std::atomic<float>* pHighFreq  = nullptr;
    std::atomic<float>* pHighAtten = nullptr;
    std::atomic<float>* pAttenSel  = nullptr;
    std::atomic<float>* pDrive     = nullptr;
    std::atomic<float>* pOutput    = nullptr;
    std::atomic<float>* pEqIn      = nullptr;
    std::atomic<float>* pBypass    = nullptr;
    juce::AudioProcessorParameter* bypassParam = nullptr;
    std::atomic<bool> hostBypassed { false };

    juce::SmoothedValue<double> sLowBoost, sLowAtten, sHighBoost, sHighAtten, sBandwidth, sEqIn;
    juce::SmoothedValue<double> sDrive, sOutputGain, sBypassMix;

    double currentSampleRate = 44100.0;
    placebo::Params lastParams;
    bool needsCoeffUpdate = true;

    placebo::EqChannel prototype;
    std::array<placebo::EqChannel, 2> channels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaceboAudioProcessor)
};
