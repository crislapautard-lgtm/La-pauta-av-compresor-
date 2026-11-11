#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <utility>
#include <vector>
#include "PluginProcessor.h"
#include "LaPautaLookAndFeel.h"

//==============================================================================
// Vúmetro analógico con aguja (lee el nivel de salida del Processor)
class VuMeter : public juce::Component,
                private juce::Timer {
public:
    explicit VuMeter (std::atomic<float>& levelSource);
    ~VuMeter() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    std::atomic<float>& source;
    float needlePos = 0.0f;   // 0..1 sobre la escala del VU

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VuMeter)
};

//==============================================================================
class LaPautaAVCompresorAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    LaPautaAVCompresorAudioProcessorEditor (LaPautaAVCompresorAudioProcessor&);
    ~LaPautaAVCompresorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct KnobSpec {
        juce::Slider* slider;
        float cx, cy, radius;
        juce::String name;
        std::vector<std::pair<float, juce::String>> labels;   // posición 0..1 + texto
    };

    void addKnob (juce::Slider& slider, float cx, float cy, float radius,
                  const juce::String& name, int ticks, int major, double defaultValue,
                  std::vector<std::pair<float, juce::String>> labels);
    void addToggle (juce::ToggleButton& button, const char* style);
    void rebuildBackground();

    LaPautaAVCompresorAudioProcessor& audioProcessor;
    LaPautaLookAndFeel lookAndFeel;

    juce::Slider gainSlider, thresholdSlider, ratioSlider, outputSlider;
    juce::ToggleButton lineMicButton, padButton, bypassButton, attackButton;
    VuMeter vuMeter;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> gainAttachment, thresholdAttachment, ratioAttachment, outputAttachment;
    std::unique_ptr<ButtonAttachment> lineMicAttachment, padAttachment, bypassAttachment, attackAttachment;

    std::vector<KnobSpec> knobs;
    juce::Image brushedBackground;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaPautaAVCompresorAudioProcessorEditor)
};
