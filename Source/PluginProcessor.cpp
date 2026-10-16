#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::String knobText (float v, int)   { return juce::String (v, 1); }
    juce::String dbText (float v, int)
    {
        if (std::abs (v) < 0.05f) return "0.0 dB";
        return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB";
    }
}

//==============================================================================
PlaceboAudioProcessor::PlaceboAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PLACEBO", createParameterLayout())
{
    pLowBoost  = apvts.getRawParameterValue ("lowBoost");
    pLowAtten  = apvts.getRawParameterValue ("lowAtten");
    pLowFreq   = apvts.getRawParameterValue ("lowFreq");
    pHighBoost = apvts.getRawParameterValue ("highBoost");
    pBandwidth = apvts.getRawParameterValue ("bandwidth");
    pHighFreq  = apvts.getRawParameterValue ("highFreq");
    pHighAtten = apvts.getRawParameterValue ("highAtten");
    pAttenSel  = apvts.getRawParameterValue ("attenSel");
    pDrive     = apvts.getRawParameterValue ("drive");
    pOutput    = apvts.getRawParameterValue ("output");
    pEqIn      = apvts.getRawParameterValue ("eqIn");
    pBypass    = apvts.getRawParameterValue ("bypass");
    bypassParam = apvts.getParameter ("bypass");
}

juce::AudioProcessorValueTreeState::ParameterLayout PlaceboAudioProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    auto knob = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                    NormalisableRange<float> (0.0f, 10.0f, 0.01f), def,
                    AudioParameterFloatAttributes().withStringFromValueFunction (knobText));
    };

    params.push_back (knob ("lowBoost", "Low Boost", 0.0f));
    params.push_back (knob ("lowAtten", "Low Atten", 0.0f));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "lowFreq", 1 }, "Low Frequency",
                        StringArray { "20 Hz", "30 Hz", "60 Hz", "100 Hz", "200 Hz" }, 3));

    params.push_back (knob ("highBoost", "High Boost", 0.0f));
    params.push_back (knob ("bandwidth", "Bandwidth", 5.0f));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "highFreq", 1 }, "High Frequency",
                        StringArray { "3 kHz", "4 kHz", "5 kHz", "8 kHz", "10 kHz", "12 kHz", "16 kHz" }, 4));

    params.push_back (knob ("highAtten", "High Atten", 0.0f));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "attenSel", 1 }, "Atten Sel",
                        StringArray { "5 kHz", "10 kHz", "20 kHz" }, 1));

    params.push_back (knob ("drive", "Drive", 0.0f));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
                        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                        AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (dbText)));

    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { "eqIn", 1 }, "EQ In", true));
    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Bypass", false));

    return { params.begin(), params.end() };
}

bool PlaceboAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

//==============================================================================
void PlaceboAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;

    for (auto* s : { &sLowBoost, &sLowAtten, &sHighBoost, &sHighAtten, &sBandwidth, &sEqIn })
        s->reset (sampleRate, 0.05);
    sDrive.reset (sampleRate, 0.05);
    sOutputGain.reset (sampleRate, 0.05);
    sBypassMix.reset (sampleRate, 0.02);
    sBypassMix.setCurrentAndTargetValue (pBypass->load() > 0.5f ? 1.0 : 0.0);

    sLowBoost.setCurrentAndTargetValue  (pLowBoost->load());
    sLowAtten.setCurrentAndTargetValue  (pLowAtten->load());
    sHighBoost.setCurrentAndTargetValue (pHighBoost->load());
    sHighAtten.setCurrentAndTargetValue (pHighAtten->load());
    sBandwidth.setCurrentAndTargetValue (pBandwidth->load());
    sEqIn.setCurrentAndTargetValue      (pEqIn->load() > 0.5f ? 1.0 : 0.0);
    sDrive.setCurrentAndTargetValue     (pDrive->load() / 10.0);
    sOutputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain ((double) pOutput->load()));

    placebo::computeTransformer (sampleRate, prototype);
    for (auto& c : channels)
    {
        c.reset();
        placebo::computeTransformer (sampleRate, c);
    }

    needsCoeffUpdate = true;
    updateEq (0);
}

static inline void copyCoeffs (placebo::Biquad& dst, const placebo::Biquad& src)
{
    dst.b0 = src.b0; dst.b1 = src.b1; dst.b2 = src.b2;
    dst.a1 = src.a1; dst.a2 = src.a2;
}

void PlaceboAudioProcessor::updateEq (int numSamplesToSkip)
{
    placebo::Params p;
    p.lowBoost  = numSamplesToSkip > 0 ? sLowBoost.skip (numSamplesToSkip)  : sLowBoost.getCurrentValue();
    p.lowAtten  = numSamplesToSkip > 0 ? sLowAtten.skip (numSamplesToSkip)  : sLowAtten.getCurrentValue();
    p.highBoost = numSamplesToSkip > 0 ? sHighBoost.skip (numSamplesToSkip) : sHighBoost.getCurrentValue();
    p.highAtten = numSamplesToSkip > 0 ? sHighAtten.skip (numSamplesToSkip) : sHighAtten.getCurrentValue();
    p.bandwidth = numSamplesToSkip > 0 ? sBandwidth.skip (numSamplesToSkip) : sBandwidth.getCurrentValue();
    p.eqIn      = numSamplesToSkip > 0 ? sEqIn.skip (numSamplesToSkip)      : sEqIn.getCurrentValue();

    p.lowFreqHz  = lowFreqTable  [(size_t) juce::jlimit (0, 4, (int) pLowFreq->load())];
    p.highFreqHz = highFreqTable [(size_t) juce::jlimit (0, 6, (int) pHighFreq->load())];
    p.attenSelHz = attenSelTable [(size_t) juce::jlimit (0, 2, (int) pAttenSel->load())];

    const bool changed = needsCoeffUpdate
        || p.lowBoost != lastParams.lowBoost   || p.lowAtten != lastParams.lowAtten
        || p.highBoost != lastParams.highBoost || p.highAtten != lastParams.highAtten
        || p.bandwidth != lastParams.bandwidth || p.eqIn != lastParams.eqIn
        || p.lowFreqHz != lastParams.lowFreqHz || p.highFreqHz != lastParams.highFreqHz
        || p.attenSelHz != lastParams.attenSelHz;

    if (! changed)
        return;

    placebo::computeEq (p, currentSampleRate, prototype);
    for (auto& c : channels)
    {
        copyCoeffs (c.lowBoost,  prototype.lowBoost);
        copyCoeffs (c.lowAtten,  prototype.lowAtten);
        copyCoeffs (c.highBoost, prototype.highBoost);
        copyCoeffs (c.highAtten, prototype.highAtten);
    }

    lastParams = p;
    needsCoeffUpdate = false;
}

template <typename SampleType>
void PlaceboAudioProcessor::processSamples (juce::AudioBuffer<SampleType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    for (int ch = numIn; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    if (numCh <= 0 || numSamples <= 0)
        return;

    sLowBoost.setTargetValue  (pLowBoost->load());
    sLowAtten.setTargetValue  (pLowAtten->load());
    sHighBoost.setTargetValue (pHighBoost->load());
    sHighAtten.setTargetValue (pHighAtten->load());
    sBandwidth.setTargetValue (pBandwidth->load());
    sEqIn.setTargetValue      (pEqIn->load() > 0.5f ? 1.0 : 0.0);
    sDrive.setTargetValue     (pDrive->load() / 10.0);
    sOutputGain.setTargetValue (juce::Decibels::decibelsToGain ((double) pOutput->load()));

    const bool bypassOn = pBypass->load() > 0.5f;
    sBypassMix.setTargetValue (bypassOn ? 1.0 : 0.0);
    hostBypassed.store (bypassOn);

    SampleType* data[2] = { buffer.getWritePointer (0),
                            numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    constexpr int subBlock = 16;   // i coefficienti seguono le manopole ogni 16 campioni
    int pos = 0;
    while (pos < numSamples)
    {
        const int len = juce::jmin (subBlock, numSamples - pos);
        updateEq (len);

        for (int i = 0; i < len; ++i)
        {
            const double drive = sDrive.getNextValue();
            const double gain  = sOutputGain.getNextValue();
            const double byp   = sBypassMix.getNextValue();   // dissolvenza in/out del bypass

            for (int ch = 0; ch < numCh; ++ch)
            {
                auto& c = channels[(size_t) ch];
                const double dry = (double) data[ch][pos + i];
                double x = c.process (dry);
                x = placebo::TubeStage::process (x, drive);
                x = c.transformer (x) * gain;
                data[ch][pos + i] = (SampleType) (x + (dry - x) * byp);
            }
        }
        pos += len;
    }
}

void PlaceboAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    processSamples (buffer);
}

void PlaceboAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    processSamples (buffer);
}

void PlaceboAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    hostBypassed.store (true);
    juce::AudioProcessor::processBlockBypassed (buffer, midi);
}

void PlaceboAudioProcessor::processBlockBypassed (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midi)
{
    hostBypassed.store (true);
    juce::AudioProcessor::processBlockBypassed (buffer, midi);
}

//==============================================================================
juce::AudioProcessorEditor* PlaceboAudioProcessor::createEditor()
{
    return new PlaceboAudioProcessorEditor (*this);
}

void PlaceboAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PlaceboAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlaceboAudioProcessor();
}
