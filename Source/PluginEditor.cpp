#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    const juce::Colour cream  (0xffe8dcc0);
    const juce::Colour creamDim (0xc8d9caa6);

    float toRad (float deg) { return juce::degreesToRadians (deg); }
}

//==============================================================================
// PanelKnob
//==============================================================================
PanelKnob::PanelKnob (Style s, const juce::String& labelText)
    : style (s), label (labelText)
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setMouseDragSensitivity (style == Style::Chicken ? 160 : 260);
    setPopupDisplayEnabled (false, false, nullptr);
    setRepaintsOnMouseActivity (false);
}

void PanelKnob::setGeometry (juce::Point<float> c, float r, float startDeg, float endDeg, float half)
{
    centre = c;
    radius = r;
    // JUCE vuole angoli positivi: si aggiunge un giro intero
    startRad = toRad (startDeg) + juce::MathConstants<float>::twoPi;
    endRad   = toRad (endDeg)   + juce::MathConstants<float>::twoPi;
    setRotaryParameters (startRad, endRad, true);

    setBounds (juce::Rectangle<float> (half * 2.0f, half * 2.0f).withCentre (c).toNearestInt());
}

float PanelKnob::currentAngle()
{
    const auto prop = (float) valueToProportionOfLength (getValue());
    return startRad + prop * (endRad - startRad);
}

juce::String PanelKnob::getBubbleText()
{
    return label + "   " + getTextFromValue (getValue());
}

void PanelKnob::paint (juce::Graphics& g)
{
    // centro reale in coordinate locali (setBounds arrotonda all'intero)
    const auto c = centre - getPosition().toFloat();
    const float a = currentAngle();

    if (style == Style::Round)
        paintRound (g, c, a);
    else
        paintChicken (g, c, a);
}

void PanelKnob::paintRound (juce::Graphics& g, juce::Point<float> c, float a)
{
    const juce::Point<float> dir (std::sin (a), -std::cos (a));
    const auto p1 = c + dir * (radius * 0.40f);
    const auto p2 = c + dir * (radius * 0.88f);
    const float w = juce::jmax (2.0f, radius * 0.05f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawLine ({ p1, p2 }, w + 2.5f);
    g.setColour (cream);
    g.drawLine ({ p1, p2 }, w);
    g.setColour (cream.withAlpha (0.9f));
    g.fillEllipse (juce::Rectangle<float> (w * 1.6f, w * 1.6f).withCentre (p2));
}

void PanelKnob::paintChicken (juce::Graphics& g, juce::Point<float> c, float a)
{
    // Tacche (solo dove la foto le ha perse)
    if (ticks > 1)
    {
        g.setColour (creamDim);
        for (int i = 0; i < ticks; ++i)
        {
            const float t = startRad + (endRad - startRad) * (float) i / (float) (ticks - 1);
            const juce::Point<float> d (std::sin (t), -std::cos (t));
            g.drawLine ({ c + d * tickR1, c + d * tickR2 }, 1.3f);
        }
    }

    // Forma del selettore a becco, costruita verso l'alto e poi ruotata
    const float rb = bodyR, L = pointerL;
    const float wb = rb * 0.62f, wt = rb * 0.22f, tail = rb * 0.95f;

    juce::Path body;
    body.addEllipse (-rb, -rb, rb * 2.0f, rb * 2.0f);

    juce::Path pointer;
    pointer.startNewSubPath (-wb, 0.0f);
    pointer.lineTo (-wt, -L);
    pointer.quadraticTo (0.0f, -L - wt * 1.4f, wt, -L);
    pointer.lineTo (wb, 0.0f);
    pointer.lineTo (wb * 0.8f, tail);
    pointer.quadraticTo (0.0f, tail + rb * 0.35f, -wb * 0.8f, tail);
    pointer.closeSubPath();

    juce::Path knob;
    knob.addPath (body);
    knob.addPath (pointer);
    knob.setUsingNonZeroWinding (true);

    const auto xf = juce::AffineTransform::rotation (a).translated (c.x, c.y);
    knob.applyTransform (xf);

    // Ombra (luce dall'alto a sinistra come nella foto)
    juce::DropShadow (juce::Colours::black.withAlpha (0.70f), (int) (rb * 0.6f), { (int) (rb * 0.25f), (int) (rb * 0.38f) })
        .drawForPath (g, knob);

    // Bachelite nera con riflesso
    juce::ColourGradient grad (juce::Colour (0xff45403b), c.x - rb, c.y - rb,
                               juce::Colour (0xff070605), c.x + rb * 0.9f, c.y + rb * 1.1f, false);
    grad.addColour (0.45, juce::Colour (0xff1a1715));
    g.setGradientFill (grad);
    g.fillPath (knob);

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.strokePath (knob, juce::PathStrokeType (1.1f));

    // Cappello centrale
    const float rc = rb * 0.58f;
    juce::ColourGradient cap (juce::Colour (0xff2c2825), c.x - rc, c.y - rc,
                              juce::Colour (0xff0b0a09), c.x + rc, c.y + rc, false);
    g.setGradientFill (cap);
    g.fillEllipse (c.x - rc, c.y - rc, rc * 2.0f, rc * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawEllipse (c.x - rc, c.y - rc, rc * 2.0f, rc * 2.0f, 1.0f);

    // Linea indice
    const juce::Point<float> d (std::sin (a), -std::cos (a));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawLine ({ c + d * (rb * 0.75f), c + d * (L - 3.0f) }, 3.6f);
    g.setColour (cream);
    g.drawLine ({ c + d * (rb * 0.75f), c + d * (L - 3.0f) }, 1.8f);
}

void PanelKnob::mouseEnter (const juce::MouseEvent& e)
{
    juce::Slider::mouseEnter (e);
    if (onActivity) onActivity (*this, true);
}

void PanelKnob::mouseExit (const juce::MouseEvent& e)
{
    juce::Slider::mouseExit (e);
    if (! isMouseButtonDown() && onActivity) onActivity (*this, false);
}

void PanelKnob::mouseDown (const juce::MouseEvent& e)
{
    juce::Slider::mouseDown (e);
    if (onActivity) onActivity (*this, true);
}

void PanelKnob::mouseUp (const juce::MouseEvent& e)
{
    juce::Slider::mouseUp (e);
    if (! isMouseOver() && onActivity) onActivity (*this, false);
}

void PanelKnob::valueChanged()
{
    repaint();
    if ((isMouseOver() || isMouseButtonDown()) && onActivity)
        onActivity (*this, true);
}

//==============================================================================
// PanelToggle (levetta IN)
//==============================================================================
PanelToggle::PanelToggle() : juce::Button ("EQ In")
{
    setClickingTogglesState (true);
    offImage = juce::ImageCache::getFromMemory (BinaryData::toggle_off_png, BinaryData::toggle_off_pngSize);
}

void PanelToggle::paintButton (juce::Graphics& g, bool, bool)
{
    // ON = levetta della foto; OFF = levetta ribaltata
    if (! getToggleState() && offImage.isValid())
        g.drawImage (offImage, getLocalBounds().toFloat());
}

//==============================================================================
// ValueBubble
//==============================================================================
void ValueBubble::show (const juce::String& t, juce::Point<float> anchor, float knobRadius)
{
    text = t;
    const juce::Font f (juce::FontOptions (24.0f, juce::Font::bold));
    const int w = (int) std::ceil (f.getStringWidthFloat (text)) + 34;
    const int h = 40;

    auto r = juce::Rectangle<int> (w, h).withCentre ({ (int) anchor.x, (int) (anchor.y - knobRadius - 28.0f) });
    if (r.getY() < 2)   // sopra non c'e' spazio: sotto la manopola
        r.setY ((int) (anchor.y + knobRadius + 6.0f));
    r = r.constrainedWithin ({ 0, 0, PlaceboAudioProcessorEditor::panelW, PlaceboAudioProcessorEditor::panelH });

    setBounds (r);
    setVisible (true);
    repaint();
}

void ValueBubble::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xe0141312));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (cream.withAlpha (0.55f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    g.setColour (cream);
    g.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
    g.drawText (text, getLocalBounds(), juce::Justification::centred, false);
}

//==============================================================================
// Panel
//==============================================================================
Panel::Panel()
{
    background = juce::ImageCache::getFromMemory (BinaryData::background_jpg, BinaryData::background_jpgSize);
    lampOff    = juce::ImageCache::getFromMemory (BinaryData::lamp_off_png,   BinaryData::lamp_off_pngSize);
    logoLit    = juce::ImageCache::getFromMemory (BinaryData::logo_lit_png,   BinaryData::logo_lit_pngSize);
    setOpaque (true);
}

void Panel::setLogoLevel (float level)
{
    level = juce::jlimit (0.0f, 1.0f, level);
    if (std::abs (level - logoLevel) > 0.001f)
    {
        logoLevel = level;
        repaint (86, 82, 260, 260);   // solo la zona del logo
    }
}

void Panel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (background, getLocalBounds().toFloat());

    // Logo Meteora: scuro nella grafica, si illumina quando il plugin e' acceso (dissolvenza)
    if (logoLevel > 0.001f && logoLit.isValid())
    {
        g.setOpacity (logoLevel);
        g.drawImageAt (logoLit, 86, 82);
        g.setOpacity (1.0f);
    }

    // Spia rossa: accesa nella grafica; spenta = immagine sovrapposta
    if (! lampOn && lampOff.isValid())
        g.drawImageAt (lampOff, 1311, 247);
}

//==============================================================================
// Editor
//==============================================================================
PlaceboAudioProcessorEditor::PlaceboAudioProcessorEditor (PlaceboAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible (panel);
    panel.setBounds (0, 0, panelW, panelH);

    // posizioni in pixel della grafica (pannello 1657 x 520)
    lowBoost .setGeometry ({ 442.0f, 156.0f }, 66.0f, -145.0f, 145.0f, 68.0f);
    lowAtten .setGeometry ({ 681.0f, 156.0f }, 67.0f, -145.0f, 145.0f, 69.0f);
    highBoost.setGeometry ({ 988.0f, 156.0f }, 66.0f, -145.0f, 145.0f, 68.0f);
    highAtten.setGeometry ({ 1226.0f, 154.0f }, 65.0f, -145.0f, 145.0f, 67.0f);
    output   .setGeometry ({ 1443.0f, 154.0f }, 49.0f, -135.0f, 135.0f, 51.0f);
    bandwidth.setGeometry ({ 829.0f, 384.0f }, 63.0f, -118.0f, 118.0f, 65.0f);
    drive    .setGeometry ({ 203.0f, 404.0f }, 40.0f, -135.0f, 135.0f, 42.0f);

    // selettori a becco: posizioni sulle tacche della grafica
    lowFreq .setGeometry ({ 560.0f, 380.0f }, 60.0f, -60.0f, 60.0f, 80.0f);   // 20 30 60 100 200
    lowFreq .setChickenSize (24.0f, 50.0f);
    highFreq.setGeometry ({ 1092.0f, 383.0f }, 60.0f, -90.0f, 90.0f, 84.0f);  // 3 4 5 8 10 12 16
    highFreq.setChickenSize (24.0f, 58.0f);
    attenSel.setGeometry ({ 1313.0f, 397.0f }, 50.0f, -40.0f, 40.0f, 72.0f);  // 5 10 20
    attenSel.setChickenSize (20.0f, 56.0f);

    auto& apvts = audioProcessor.apvts;
    auto attach = [&] (PanelKnob& k, const char* id, double resetValue, bool hasReset)
    {
        panel.addAndMakeVisible (k);
        sliderAttachments.push_back (std::make_unique<SliderAtt> (apvts, id, k));
        if (hasReset) k.setDoubleClickReturnValue (true, resetValue);
        k.onActivity = [this] (PanelKnob& kn, bool on) { knobActivity (kn, on); };
    };

    attach (lowBoost,  "lowBoost",  0.0, true);
    attach (lowAtten,  "lowAtten",  0.0, true);
    attach (highBoost, "highBoost", 0.0, true);
    attach (highAtten, "highAtten", 0.0, true);
    attach (bandwidth, "bandwidth", 5.0, true);
    attach (drive,     "drive",     0.0, true);
    attach (output,    "output",    0.0, true);
    attach (lowFreq,   "lowFreq",   0.0, false);
    attach (highFreq,  "highFreq",  0.0, false);
    attach (attenSel,  "attenSel",  0.0, false);

    panel.addAndMakeVisible (eqInSwitch);
    eqInSwitch.setBounds (266, 348, 88, 88);
    eqInSwitch.setTooltip ("EQ In");
    eqInAttachment = std::make_unique<ButtonAtt> (apvts, "eqIn", eqInSwitch);

    panel.addChildComponent (bubble);
    bubble.setInterceptsMouseClicks (false, false);
    bubble.setAlwaysOnTop (true);

    setResizable (true, true);
    setResizeLimits (660, 207, panelW, panelH);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) panelW / (double) panelH);
    setSize (1100, 345);

    logoLevel = audioProcessor.isPluginActive() ? 1.0f : 0.0f;
    panel.setLogoLevel (logoLevel);

    startTimerHz (30);
}

PlaceboAudioProcessorEditor::~PlaceboAudioProcessorEditor()
{
    stopTimer();
}

void PlaceboAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void PlaceboAudioProcessorEditor::resized()
{
    const float scale = (float) getWidth() / (float) panelW;
    panel.setTransform (juce::AffineTransform::scale (scale));
}

void PlaceboAudioProcessorEditor::knobActivity (PanelKnob& k, bool active)
{
    if (active)
    {
        activeKnob = &k;
        bubble.show (k.getBubbleText(), k.getCentreInPanel(), k.getKnobRadius());
        bubble.toFront (false);
    }
    else if (activeKnob == &k)
    {
        activeKnob = nullptr;
        bubble.setVisible (false);
    }
}

void PlaceboAudioProcessorEditor::timerCallback()
{
    if (auto* v = audioProcessor.apvts.getRawParameterValue ("eqIn"))
        panel.setLampOn (v->load() > 0.5f);

    const float target = audioProcessor.isPluginActive() ? 1.0f : 0.0f;
    logoLevel += (target - logoLevel) * 0.25f;          // accensione/spegnimento morbido
    if (std::abs (target - logoLevel) < 0.01f) logoLevel = target;
    panel.setLogoLevel (logoLevel);
}
