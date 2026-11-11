#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class LaPautaAVCompresorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                               private juce::Slider::Listener {
public:
    LaPautaAVCompresorAudioProcessorEditor (LaPautaAVCompresorAudioProcessor&);
    ~LaPautaAVCompresorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void sliderValueChanged (juce::Slider* slider) override;

    LaPautaAVCompresorAudioProcessor& audioProcessor;

    juce::Slider gainSlider;
    juce::Slider compSlider;
    juce::Slider outputSlider;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<SliderAttachment> compAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaPautaAVCompresorAudioProcessorEditor)
};
