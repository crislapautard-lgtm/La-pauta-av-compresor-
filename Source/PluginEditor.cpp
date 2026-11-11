#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
    using juce::Colour;

    constexpr int kWidth  = 900;
    constexpr int kHeight = 500;

    const float kStartAngle = juce::degreesToRadians (-135.0f);
    const float kEndAngle   = juce::degreesToRadians (135.0f);

    juce::Font makeFont (float size, bool bold, const juce::String& family = {})
    {
        auto options = juce::FontOptions (size);

        if (family.isNotEmpty())
            options = options.withName (family);

        if (bold)
            options = options.withStyle ("Bold");

        return juce::Font (options);
    }

    // Escala real de un VU: -20 VU en el extremo izquierdo, +3 VU en el derecho.
    float vuToPos (float vu)
    {
        vu = juce::jlimit (-20.0f, 3.0f, vu);
        const float lo = 0.1f;                                    // 10^(-20/20)
        const float hi = std::pow (10.0f, 3.0f / 20.0f);          // 10^(+3/20)
        return (std::pow (10.0f, vu / 20.0f) - lo) / (hi - lo);
    }

    void drawScrew (juce::Graphics& g, float cx, float cy)
    {
        const float r = 11.0f;

        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.fillEllipse (cx - r + 1.5f, cy - r + 2.5f, r * 2.0f, r * 2.0f);

        juce::ColourGradient grad (Colour (0xfff2f2f2), cx - r * 0.6f, cy - r * 0.6f,
                                   Colour (0xff6d6f74), cx + r, cy + r, true);
        g.setGradientFill (grad);
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour (Colour (0xff3c3e42));
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);

        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (juce::degreesToRadians (35.0f), cx, cy));
        g.setColour (Colour (0xff2a2b2e));
        g.fillRoundedRectangle (cx - r * 0.75f, cy - 1.6f, r * 1.5f, 3.2f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.fillRect (cx - r * 0.75f, cy + 1.6f, r * 1.5f, 0.8f);
        g.restoreState();
    }

    // Marco con título "grabado" en el borde superior (como en el panel de referencia)
    void drawSection (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title)
    {
        const auto font = makeFont (15.0f, true);

        juce::GlyphArrangement ga;
        ga.addLineOfText (font, title, 0.0f, 0.0f);
        const float tw = ga.getBoundingBox (0, -1, true).getWidth();

        const juce::Rectangle<float> gap (bounds.getCentreX() - tw * 0.5f - 8.0f, bounds.getY() - 12.0f, tw + 16.0f, 24.0f);

        g.saveState();
        g.excludeClipRegion (gap.toNearestInt());
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawRoundedRectangle (bounds.translated (1.0f, 1.0f), 12.0f, 1.5f);
        g.setColour (Colour (0xff3a3c40));
        g.drawRoundedRectangle (bounds, 12.0f, 1.5f);
        g.restoreState();

        g.setFont (font);
        g.setColour (Colour (0xff2e3033));
        g.drawText (title, gap, juce::Justification::centred);
    }
}

//==============================================================================
// VuMeter
//==============================================================================
VuMeter::VuMeter (std::atomic<float>& levelSource) : source (levelSource)
{
    setOpaque (false);
    startTimerHz (30);
}

VuMeter::~VuMeter()
{
    stopTimer();
}

void VuMeter::timerCallback()
{
    const float target = vuToPos (source.load());
    const float previous = needlePos;

    // Balística tipo VU: la aguja se mueve con inercia
    needlePos += (target - needlePos) * 0.22f;

    if (std::abs (needlePos - previous) > 0.0002f)
        repaint();
}

void VuMeter::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // Bisel metálico
    juce::ColourGradient bezel (Colour (0xff9a9da2), b.getX(), b.getY(),
                                Colour (0xff2f3134), b.getX(), b.getBottom(), false);
    g.setGradientFill (bezel);
    g.fillRoundedRectangle (b, 16.0f);
    g.setColour (Colour (0xff1b1c1e));
    g.fillRoundedRectangle (b.reduced (7.0f), 12.0f);

    // Cara color crema
    const auto face = b.reduced (12.0f);
    juce::ColourGradient faceGrad (Colour (0xfffff4d2), face.getCentreX(), face.getBottom() - 20.0f,
                                   Colour (0xffd3c79a), face.getCentreX() + face.getWidth() * 0.65f, face.getBottom() - 20.0f, true);
    g.setGradientFill (faceGrad);
    g.fillRoundedRectangle (face, 8.0f);

    g.saveState();
    juce::Path facePath;
    facePath.addRoundedRectangle (face, 8.0f);
    g.reduceClipRegion (facePath);

    const float pivotX = face.getCentreX();
    const float pivotY = face.getBottom() - 16.0f;
    const float rs     = pivotY - face.getY() - 34.0f;
    const float angMin = juce::degreesToRadians (-50.0f);
    const float angMax = juce::degreesToRadians (50.0f);

    auto posToAngle = [angMin, angMax] (float pos) { return angMin + pos * (angMax - angMin); };

    // Arco base
    juce::Path arc;
    arc.addCentredArc (pivotX, pivotY, rs, rs, 0.0f, angMin, angMax, true);
    g.setColour (Colour (0xff2a2622));
    g.strokePath (arc, juce::PathStrokeType (1.5f));

    // Zona roja (0 a +3 VU)
    juce::Path redArc;
    redArc.addCentredArc (pivotX, pivotY, rs - 2.5f, rs - 2.5f, 0.0f,
                          posToAngle (vuToPos (0.0f)), angMax, true);
    g.setColour (Colour (0xffb3221c));
    g.strokePath (redArc, juce::PathStrokeType (5.0f));

    // Marcas y números
    const int majors[] = { -20, -10, -7, -5, -3, 0, 1, 2, 3 };
    auto isMajor = [&majors] (int v)
    {
        for (int m : majors)
            if (m == v) return true;
        return false;
    };

    for (int vu = -20; vu <= 3; ++vu)
    {
        const bool major = isMajor (vu);

        if (! major && vu < -10 && vu != -15)
            continue;

        const float a = posToAngle (vuToPos ((float) vu));
        const float s = std::sin (a);
        const float c = std::cos (a);
        const float len = major ? 11.0f : 6.0f;

        g.setColour (vu > 0 ? Colour (0xffb3221c) : Colour (0xff2a2622));
        g.drawLine (pivotX + rs * s, pivotY - rs * c,
                    pivotX + (rs - len) * s, pivotY - (rs - len) * c,
                    major ? 1.8f : 1.0f);

        if (major)
        {
            const auto text = (vu > 0 ? "+" : "") + juce::String (vu);
            const float lr = rs + 13.0f;
            const juce::Point<float> pt (pivotX + lr * s, pivotY - lr * c);

            g.setFont (makeFont (11.0f, true));
            g.setColour (vu > 0 ? Colour (0xffb3221c) : Colour (0xff2a2622));
            g.drawText (text, juce::Rectangle<float> (30.0f, 14.0f).withCentre (pt), juce::Justification::centred);
        }
    }

    // Texto "VU"
    g.setFont (makeFont (22.0f, true, "Georgia"));
    g.setColour (Colour (0xff2a2622));
    g.drawText ("VU", juce::Rectangle<float> (60.0f, 26.0f).withCentre ({ pivotX, pivotY - 46.0f }), juce::Justification::centred);

    // Aguja con sombra
    const float a = posToAngle (needlePos);
    const float tipX = pivotX + (rs + 4.0f) * std::sin (a);
    const float tipY = pivotY - (rs + 4.0f) * std::cos (a);

    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawLine (pivotX + 2.0f, pivotY + 2.0f, tipX + 2.0f, tipY + 2.0f, 2.2f);
    g.setColour (Colour (0xff151515));
    g.drawLine (pivotX, pivotY, tipX, tipY, 2.2f);

    // Montura oscura de la aguja (joroba inferior)
    juce::ColourGradient hump (Colour (0xff6a6d72), pivotX - 14.0f, pivotY - 20.0f,
                               Colour (0xff1c1d1f), pivotX + 20.0f, pivotY + 30.0f, false);
    g.setGradientFill (hump);
    g.fillEllipse (pivotX - 38.0f, pivotY - 24.0f, 76.0f, 76.0f);
    g.setColour (Colour (0xff0e0f10));
    g.drawEllipse (pivotX - 38.0f, pivotY - 24.0f, 76.0f, 76.0f, 1.2f);

    g.restoreState();

    // Reflejo del cristal y borde interior
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.4f), 8.0f);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawRoundedRectangle (face, 8.0f, 1.5f);
}

//==============================================================================
// Editor
//==============================================================================
LaPautaAVCompresorAudioProcessorEditor::LaPautaAVCompresorAudioProcessorEditor (LaPautaAVCompresorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), vuMeter (p.vuLevel)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    auto pos = [] (float v, float lo, float hi) { return (v - lo) / (hi - lo); };

    // GAIN (0 a 24 dB) - perilla grande del preamp
    addKnob (gainSlider, 162.0f, 365.0f, 58.0f, "GAIN", 12, 2, 0.0,
             { { pos (0, 0, 24),  "0" },  { pos (4, 0, 24),  "4" },  { pos (8, 0, 24),  "8" },
               { pos (12, 0, 24), "12" }, { pos (16, 0, 24), "16" }, { pos (20, 0, 24), "20" },
               { pos (24, 0, 24), "24dB" } });

    // THRESHOLD (-40 a 0 dB)
    addKnob (thresholdSlider, 737.0f, 180.0f, 40.0f, "THRESHOLD", 8, 2, -12.0,
             { { pos (-40, -40, 0), "-40" }, { pos (-30, -40, 0), "-30" }, { pos (-20, -40, 0), "-20" },
               { pos (-10, -40, 0), "-10" }, { pos (0, -40, 0),   "0" } });

    // RATIO (1:1 a 10:1)
    addKnob (ratioSlider, 683.0f, 310.0f, 32.0f, "RATIO", 9, 3, 3.0,
             { { pos (1, 1, 10),  "1:1" },  { pos (4, 1, 10), "4:1" },
               { pos (7, 1, 10),  "7:1" },  { pos (10, 1, 10), "10:1" } });

    // OUTPUT (-24 a +24 dB)
    addKnob (outputSlider, 792.0f, 310.0f, 32.0f, "OUTPUT", 8, 2, 0.0,
             { { pos (-24, -24, 24), "-24" }, { pos (-12, -24, 24), "-12" }, { pos (0, -24, 24), "0" },
               { pos (12, -24, 24),  "+12" }, { pos (24, -24, 24),  "+24" } });

    addToggle (lineMicButton, "switch");
    addToggle (padButton, "switch");
    addToggle (bypassButton, "square");
    addToggle (attackButton, "square");

    addAndMakeVisible (vuMeter);

    auto& apvts = audioProcessor.parameters;
    gainAttachment      = std::make_unique<SliderAttachment> (apvts, "gain", gainSlider);
    thresholdAttachment = std::make_unique<SliderAttachment> (apvts, "threshold", thresholdSlider);
    ratioAttachment     = std::make_unique<SliderAttachment> (apvts, "ratio", ratioSlider);
    outputAttachment    = std::make_unique<SliderAttachment> (apvts, "output", outputSlider);

    lineMicAttachment = std::make_unique<ButtonAttachment> (apvts, "linemic", lineMicButton);
    padAttachment     = std::make_unique<ButtonAttachment> (apvts, "pad", padButton);
    bypassAttachment  = std::make_unique<ButtonAttachment> (apvts, "bypass", bypassButton);
    attackAttachment  = std::make_unique<ButtonAttachment> (apvts, "attackfast", attackButton);

    setResizable (false, false);
    setSize (kWidth, kHeight);
}

LaPautaAVCompresorAudioProcessorEditor::~LaPautaAVCompresorAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void LaPautaAVCompresorAudioProcessorEditor::addKnob (juce::Slider& s, float cx, float cy, float radius,
                                                      const juce::String& name, int ticks, int major,
                                                      double defaultValue,
                                                      std::vector<std::pair<float, juce::String>> labels)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (kStartAngle, kEndAngle, true);
    s.setPopupDisplayEnabled (true, false, this);
    s.setDoubleClickReturnValue (true, defaultValue);
    s.getProperties().set ("ticks", ticks);
    s.getProperties().set ("major", major);
    addAndMakeVisible (s);

    knobs.push_back ({ &s, cx, cy, radius, name, std::move (labels) });
}

void LaPautaAVCompresorAudioProcessorEditor::addToggle (juce::ToggleButton& button, const char* style)
{
    button.getProperties().set ("style", style);
    button.setClickingTogglesState (true);
    button.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (button);
}

void LaPautaAVCompresorAudioProcessorEditor::rebuildBackground()
{
    const int w = getWidth();
    const int h = getHeight();

    if (w <= 0 || h <= 0)
        return;

    brushedBackground = juce::Image (juce::Image::RGB, w, h, true);
    juce::Graphics g (brushedBackground);

    juce::ColourGradient base (Colour (0xffe4e5e7), 0.0f, 0.0f, Colour (0xffb4b6ba), 0.0f, (float) h, false);
    base.addColour (0.5, Colour (0xffd7d8db));
    g.setGradientFill (base);
    g.fillAll();

    // Textura de aluminio cepillado (líneas horizontales finas)
    juce::Random rng (1234);

    for (int yy = 0; yy < h; ++yy)
    {
        g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (rng.nextFloat() * 0.07f));
        g.drawHorizontalLine (yy, 0.0f, (float) w);
    }

    for (int i = 0; i < 900; ++i)
    {
        const int x0  = rng.nextInt (w);
        const int len = 40 + rng.nextInt (200);
        const int yy  = rng.nextInt (h);

        g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.05f));
        g.drawHorizontalLine (yy, (float) x0, (float) (x0 + len));
    }
}

void LaPautaAVCompresorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (brushedBackground, 0, 0);

    // Marco exterior
    g.setColour (Colour (0xff45474b));
    g.drawRect (getLocalBounds(), 2);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawRect (getLocalBounds().reduced (2), 1);

    // Tornillos
    drawScrew (g, 28.0f, 28.0f);
    drawScrew (g, (float) kWidth - 28.0f, 28.0f);
    drawScrew (g, 28.0f, (float) kHeight - 28.0f);
    drawScrew (g, (float) kWidth - 28.0f, (float) kHeight - 28.0f);

    // Título
    g.setColour (Colour (0xff2e3033));
    g.setFont (makeFont (30.0f, true));
    g.drawText ("LA PAUTA - AV COMPRESOR", 0, 20, kWidth, 44, juce::Justification::centred);

    // Secciones laterales
    drawSection (g, { 30.0f, 95.0f, 265.0f, 375.0f }, "PREAMP / INPUT");
    drawSection (g, { 605.0f, 95.0f, 265.0f, 375.0f }, "OPTICAL COMPRESSOR");

    // Panel central color crema
    const juce::Rectangle<float> centre (310.0f, 95.0f, 280.0f, 375.0f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (centre.translated (2.0f, 3.0f), 12.0f);

    juce::ColourGradient cream (Colour (0xfff4efe0), centre.getX(), centre.getY(),
                                Colour (0xffe0d9c3), centre.getX(), centre.getBottom(), false);
    g.setGradientFill (cream);
    g.fillRoundedRectangle (centre, 12.0f);
    g.setColour (Colour (0xff3a3c40));
    g.drawRoundedRectangle (centre, 12.0f, 1.5f);

    g.setColour (Colour (0xff2a2622));
    g.setFont (makeFont (42.0f, true, "Georgia"));
    g.drawText ("LA PAUTA", 310, 312, 280, 56, juce::Justification::centred);
    g.fillRect (350.0f, 380.0f, 200.0f, 1.5f);
    g.setFont (makeFont (15.0f, false));
    g.drawText ("OPTO-VALVE CHANNEL STRIP", 310, 388, 280, 24, juce::Justification::centred);

    // Números y nombres de las perillas
    for (const auto& k : knobs)
    {
        const float labelR = k.radius + 22.0f;

        g.setColour (Colour (0xff2a2b2e));
        g.setFont (makeFont (11.5f, false));

        for (const auto& l : k.labels)
        {
            const float a = kStartAngle + l.first * (kEndAngle - kStartAngle);
            const juce::Point<float> pt (k.cx + labelR * std::sin (a), k.cy - labelR * std::cos (a));
            g.drawText (l.second, juce::Rectangle<float> (46.0f, 14.0f).withCentre (pt), juce::Justification::centred);
        }

        const float nameY = k.cy + 0.707f * labelR + 10.0f;
        g.setFont (makeFont (15.0f, true));
        g.drawText (k.name, juce::Rectangle<float> (k.cx - 60.0f, nameY, 120.0f, 18.0f), juce::Justification::centred);
    }

    // Etiquetas de interruptores y botones
    g.setColour (Colour (0xff2a2b2e));
    g.setFont (makeFont (13.0f, true));
    g.drawText ("MIC",   70, 110, 60, 16, juce::Justification::centred);
    g.drawText ("LINE",  70, 198, 60, 16, juce::Justification::centred);
    g.drawText ("PAD",  195, 110, 60, 16, juce::Justification::centred);
    g.drawText ("-20dB", 195, 198, 60, 16, juce::Justification::centred);

    g.setFont (makeFont (12.5f, true));
    g.drawText ("BYPASS", 638, 438, 90, 16, juce::Justification::centred);
    g.drawText ("ATTACK", 747, 438, 90, 15, juce::Justification::centred);
    g.setFont (makeFont (11.5f, true));
    g.drawText ("FAST/SLOW", 747, 452, 90, 15, juce::Justification::centred);
}

void LaPautaAVCompresorAudioProcessorEditor::resized()
{
    // Cada perilla ocupa su radio + 10 px de margen para dibujar las marcas de la escala
    for (const auto& k : knobs)
    {
        const float half = k.radius + 10.0f;
        k.slider->setBounds (juce::Rectangle<float> (half * 2.0f, half * 2.0f)
                                 .withCentre ({ k.cx, k.cy })
                                 .toNearestInt());
    }

    lineMicButton.setBounds (70, 128, 60, 68);     // centro x = 100
    padButton.setBounds (195, 128, 60, 68);        // centro x = 225

    vuMeter.setBounds (320, 110, 260, 190);

    bypassButton.setBounds (653, 391, 60, 44);     // centro x = 683
    attackButton.setBounds (762, 391, 60, 44);     // centro x = 792

    rebuildBackground();
}
