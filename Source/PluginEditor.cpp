#include "PluginProcessor.h"
#include "PluginEditor.h"

LaPautaAVCompresorAudioProcessorEditor::LaPautaAVCompresorAudioProcessorEditor (LaPautaAVCompresorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p) {
    
    // Configurar perillas (Sliders) estilo rack analógico
    auto configureSlider = [this](juce::Slider& slider) {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (slider);
    };

    configureSlider (gainSlider);
    configureSlider (compSlider);
    configureSlider (outputSlider);

    gainAttachment   = std::make_unique<SliderAttachment>(audioProcessor.parameters, "gain", gainSlider);
    compAttachment   = std::make_unique<SliderAttachment>(audioProcessor.parameters, "compression", compSlider);
    outputAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "output", outputSlider);

    setSize (600, 300);
}

LaPautaAVCompresorAudioProcessorEditor::~LaPautaAVCompresorAudioProcessorEditor() {}

void LaPautaAVCompresorAudioProcessorEditor::paint (juce::Graphics& g) {
    // Fondo metálico / aluminio cepillado característico del hardware
    g.fillAll (juce::Colour (0xffd0d0d0));

    // Panel central estilo vúmetro analógico / placa frontal
    g.setColour (juce::Colour (0xffe8e4dc));
    g.fillRoundedRectangle (150, 30, 300, 100, 8.0f);

    g.setColour (juce::Colours::black);
    juce::Font titleFont = juce::Font(juce::FontOptions(16.0f).withStyle("Bold"));
    g.setFont (titleFont);
    g.drawText ("LA PAUTA - AV COMPRESOR", 150, 45, 300, 30, juce::Justification::centred);

    juce::Font subFont = juce::Font(juce::FontOptions(11.0f));
    g.setFont (subFont);
    g.drawText ("OPTO-VALVE CHANNEL STRIP", 150, 70, 300, 20, juce::Justification::centred);
}

void LaPautaAVCompresorAudioProcessorEditor::resized() {
    gainSlider.setBounds (80, 140, 100, 120);
    compSlider.setBounds (250, 140, 100, 120);
    outputSlider.setBounds (420, 140, 100, 120);
}

void LaPautaAVCompresorAudioProcessorEditor::sliderValueChanged (juce::Slider*) {}
