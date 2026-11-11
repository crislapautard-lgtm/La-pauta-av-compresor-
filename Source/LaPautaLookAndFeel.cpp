#include "LaPautaLookAndFeel.h"

#include <cmath>

namespace
{
    using juce::Colour;

    // Interruptor de palanca con tuerca hexagonal. "on" = palanca hacia arriba con luz roja.
    void drawSwitch (juce::Graphics& g, juce::Rectangle<float> bounds, bool on)
    {
        const auto c = bounds.getCentre();

        if (on)
        {
            juce::ColourGradient glow (Colour (0xffff2a2a).withAlpha (0.8f), c.x, c.y,
                                       Colour (0xffff2a2a).withAlpha (0.0f), c.x + 30.0f, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (c.x - 30.0f, c.y - 30.0f, 60.0f, 60.0f);
        }

        // Tuerca hexagonal
        juce::Path hex;
        hex.addPolygon (c, 6, 17.0f, juce::MathConstants<float>::pi / 6.0f);

        juce::ColourGradient nut (Colour (0xfff2f2f2), c.x - 14.0f, c.y - 14.0f,
                                  Colour (0xff6c6e73), c.x + 14.0f, c.y + 14.0f, false);
        g.setGradientFill (nut);
        g.fillPath (hex);
        g.setColour (Colour (0xff35373b));
        g.strokePath (hex, juce::PathStrokeType (1.2f));

        // Base circular interior
        juce::ColourGradient base (Colour (0xff5b5d62), c.x - 8.0f, c.y - 8.0f,
                                   Colour (0xffd5d6d9), c.x + 8.0f, c.y + 8.0f, false);
        g.setGradientFill (base);
        g.fillEllipse (c.x - 10.0f, c.y - 10.0f, 20.0f, 20.0f);
        g.setColour (Colour (0xff2c2e31));
        g.drawEllipse (c.x - 10.0f, c.y - 10.0f, 20.0f, 20.0f, 1.0f);

        // Palanca
        const float dir = on ? -1.0f : 1.0f;
        const juce::Point<float> tip (c.x, c.y + dir * 24.0f);

        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.drawLine (c.x + 2.0f, c.y + 2.0f, tip.x + 2.0f, tip.y + 2.0f, 7.0f);

        g.setColour (Colour (0xffa9abb0));
        g.drawLine (c.x, c.y, tip.x, tip.y, 7.0f);
        g.setColour (Colour (0xfff4f4f5));
        g.drawLine (c.x - 1.2f, c.y, tip.x - 1.2f, tip.y, 2.2f);

        // Punta esférica
        juce::ColourGradient ball (Colour (0xffffffff), tip.x - 2.5f, tip.y - 2.5f,
                                   Colour (0xff6e7075), tip.x + 7.0f, tip.y + 7.0f, true);
        g.setGradientFill (ball);
        g.fillEllipse (tip.x - 7.0f, tip.y - 7.0f, 14.0f, 14.0f);
        g.setColour (Colour (0xff36383c));
        g.drawEllipse (tip.x - 7.0f, tip.y - 7.0f, 14.0f, 14.0f, 1.0f);
    }

    // Botón cuadrado con luz roja (encendido = brillante).
    void drawSquare (juce::Graphics& g, juce::Rectangle<float> bounds, bool on, bool down)
    {
        auto r = juce::Rectangle<float> (46.0f, 30.0f).withCentre (bounds.getCentre());

        if (on)
        {
            const auto c = r.getCentre();
            juce::ColourGradient glow (Colour (0xffff3b30).withAlpha (0.55f), c.x, c.y,
                                       Colour (0xffff3b30).withAlpha (0.0f), c.x + 36.0f, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (c.x - 36.0f, c.y - 36.0f, 72.0f, 72.0f);
        }

        // Marco oscuro
        g.setColour (Colour (0xff2a2b2d));
        g.fillRoundedRectangle (r.expanded (3.0f), 5.0f);

        if (down)
            r = r.translated (0.0f, 1.0f);

        const Colour top    = on ? Colour (0xffff6a5c) : Colour (0xff8c1c18);
        const Colour bottom = on ? Colour (0xffd0140c) : Colour (0xff5a0d0a);

        juce::ColourGradient body (top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (r, 3.0f);

        // Brillo superior
        auto shine = r.reduced (4.0f, 2.0f);
        auto shineTop = shine.removeFromTop (shine.getHeight() * 0.4f);
        g.setColour (juce::Colours::white.withAlpha (on ? 0.35f : 0.12f));
        g.fillRoundedRectangle (shineTop, 2.0f);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
    }
}

LaPautaLookAndFeel::LaPautaLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, Colour (0xffb3221c));
}

void LaPautaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float startAngle, float endAngle,
                                           juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const auto centre = bounds.getCentre();
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 10.0f;
    const float angle  = startAngle + sliderPos * (endAngle - startAngle);

    const int ticks = (int) slider.getProperties().getWithDefault ("ticks", 10);
    const int major = juce::jmax (1, (int) slider.getProperties().getWithDefault ("major", 1));

    // Marcas de la escala alrededor de la perilla
    for (int i = 0; i <= ticks; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / (float) juce::jmax (1, ticks);
        const bool isMajor = (i % major) == 0;
        const float r1 = radius + 3.0f;
        const float r2 = radius + (isMajor ? 9.5f : 6.0f);
        const float s = std::sin (a);
        const float c = std::cos (a);

        g.setColour (Colour (0xff2b2b2b));
        g.drawLine (centre.x + r1 * s, centre.y - r1 * c,
                    centre.x + r2 * s, centre.y - r2 * c,
                    isMajor ? 1.6f : 1.0f);
    }

    // Sombra proyectada
    for (int i = 0; i < 4; ++i)
    {
        const float grow = (float) i;
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.fillEllipse (centre.x - radius - grow + 1.0f, centre.y - radius - grow + 4.0f,
                       (radius + grow) * 2.0f, (radius + grow) * 2.0f);
    }

    // Aro exterior de acero
    juce::ColourGradient ring (Colour (0xfff4f4f4), centre.x - radius * 0.7f, centre.y - radius * 0.9f,
                               Colour (0xff585a5e), centre.x + radius * 0.7f, centre.y + radius * 0.9f, false);
    g.setGradientFill (ring);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.2f);

    // Moleteado del borde
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    for (int i = 0; i < 48; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 48.0f;
        const float s = std::sin (a);
        const float c = std::cos (a);
        g.drawLine (centre.x + radius * 0.89f * s, centre.y - radius * 0.89f * c,
                    centre.x + radius * 0.99f * s, centre.y - radius * 0.99f * c, 1.0f);
    }

    // Cara superior cepillada
    const float faceR = radius * 0.84f;
    juce::ColourGradient face (Colour (0xfffbfbfb), centre.x - faceR * 0.8f, centre.y - faceR * 0.8f,
                               Colour (0xff9a9ca0), centre.x + faceR * 0.8f, centre.y + faceR * 0.8f, false);
    face.addColour (0.5, Colour (0xffd9dadd));
    g.setGradientFill (face);
    g.fillEllipse (centre.x - faceR, centre.y - faceR, faceR * 2.0f, faceR * 2.0f);

    for (int k = 1; k <= 10; ++k)
    {
        const float rr = faceR * (float) k / 10.0f;
        g.setColour ((k % 2 == 0 ? juce::Colours::white : juce::Colours::black).withAlpha (k % 2 == 0 ? 0.12f : 0.05f));
        g.drawEllipse (centre.x - rr, centre.y - rr, rr * 2.0f, rr * 2.0f, 0.6f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawEllipse (centre.x - faceR, centre.y - faceR, faceR * 2.0f, faceR * 2.0f, 1.0f);

    // Indicador
    const float s = std::sin (angle);
    const float c = std::cos (angle);
    juce::Path pointer;
    pointer.startNewSubPath (centre.x + radius * 0.30f * s, centre.y - radius * 0.30f * c);
    pointer.lineTo (centre.x + radius * 0.80f * s, centre.y - radius * 0.80f * c);

    g.setColour (Colour (0xff1d1d1d));
    g.strokePath (pointer, juce::PathStrokeType (juce::jmax (2.5f, radius * 0.06f),
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

void LaPautaLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted);

    const auto bounds = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();
    const auto style = button.getProperties().getWithDefault ("style", "switch").toString();

    if (style == "square")
        drawSquare (g, bounds, on, shouldDrawButtonAsDown);
    else
        drawSwitch (g, bounds, on);
}
