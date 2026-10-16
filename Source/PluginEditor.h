#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Manopola disegnata sopra la grafica.
//  - Round:   la manopola e' gia' nella foto, si disegna solo l'indice che ruota
//  - Chicken: selettore a "becco" disegnato interamente (la foto e' stata ripulita)
//==============================================================================
class PanelKnob : public juce::Slider
{
public:
    enum class Style { Round, Chicken };

    PanelKnob (Style s, const juce::String& labelText);

    void setGeometry (juce::Point<float> centreInPanel, float radius,
                      float startDeg, float endDeg, float boxHalfSize);
    void setTicks (int numTicks, float r1, float r2) { ticks = numTicks; tickR1 = r1; tickR2 = r2; }
    void setChickenSize (float bodyRadius, float pointerLength) { bodyR = bodyRadius; pointerL = pointerLength; }

    juce::String getBubbleText();
    juce::Point<float> getCentreInPanel() const { return centre; }
    float getKnobRadius() const { return radius; }

    std::function<void (PanelKnob&, bool)> onActivity;

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit  (const juce::MouseEvent&) override;
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;
    void valueChanged() override;

private:
    float currentAngle();
    void paintRound (juce::Graphics&, juce::Point<float> c, float angle);
    void paintChicken (juce::Graphics&, juce::Point<float> c, float angle);

    Style style;
    juce::String label;
    juce::Point<float> centre;
    float radius = 50.0f;
    float startRad = 0.0f, endRad = 1.0f;
    int ticks = 0;
    float tickR1 = 0, tickR2 = 0;
    float bodyR = 22.0f, pointerL = 46.0f;
};

//==============================================================================
class PanelToggle : public juce::Button
{
public:
    PanelToggle();
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
private:
    juce::Image offImage;
};

//==============================================================================
class ValueBubble : public juce::Component
{
public:
    void show (const juce::String& text, juce::Point<float> anchor, float knobRadius);
    void paint (juce::Graphics&) override;
private:
    juce::String text;
};

//==============================================================================
class Panel : public juce::Component
{
public:
    Panel();
    void paint (juce::Graphics&) override;
    void setLampOn (bool on) { if (lampOn != on) { lampOn = on; repaint(); } }
    void setLogoLevel (float level);
private:
    juce::Image background, lampOff, logoLit;
    float logoLevel = 1.0f;
    bool lampOn = true;
};

//==============================================================================
class PlaceboAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit PlaceboAudioProcessorEditor (PlaceboAudioProcessor&);
    ~PlaceboAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int panelW = 1657;
    static constexpr int panelH = 520;

private:
    void timerCallback() override;
    void knobActivity (PanelKnob&, bool active);

    PlaceboAudioProcessor& audioProcessor;
    Panel panel;

    PanelKnob lowBoost  { PanelKnob::Style::Round,   "Low Boost" };
    PanelKnob lowAtten  { PanelKnob::Style::Round,   "Low Atten" };
    PanelKnob highBoost { PanelKnob::Style::Round,   "High Boost" };
    PanelKnob highAtten { PanelKnob::Style::Round,   "High Atten" };
    PanelKnob output    { PanelKnob::Style::Round,   "Output" };
    PanelKnob bandwidth { PanelKnob::Style::Round,   "Bandwidth" };
    PanelKnob drive     { PanelKnob::Style::Round,   "Drive" };
    PanelKnob lowFreq   { PanelKnob::Style::Chicken, "Low Freq" };
    PanelKnob highFreq  { PanelKnob::Style::Chicken, "High Freq" };
    PanelKnob attenSel  { PanelKnob::Style::Chicken, "Atten Sel" };

    PanelToggle eqInSwitch;
    ValueBubble bubble;
    PanelKnob* activeKnob = nullptr;
    float logoLevel = 1.0f;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SliderAtt>> sliderAttachments;
    std::unique_ptr<ButtonAtt> eqInAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaceboAudioProcessorEditor)
};
