#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Estilo visual de hardware analógico: perillas de acero, interruptores
// de palanca con tuerca hexagonal y botones cuadrados con luz roja.
//
// Propiedades que lee de los componentes:
//   Slider:       "ticks" (int, divisiones de la escala) y "major" (int, cada cuántas marcas va una larga)
//   ToggleButton: "style" = "switch" (palanca) o "square" (botón cuadrado)
class LaPautaLookAndFeel : public juce::LookAndFeel_V4 {
public:
    LaPautaLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;
};
