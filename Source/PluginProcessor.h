#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class LaPautaAVCompresorAudioProcessor : public juce::AudioProcessor {
public:
    LaPautaAVCompresorAudioProcessor();
    ~LaPautaAVCompresorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parámetros expuestos para la GUI y automatización
    juce::AudioProcessorValueTreeState parameters;

    // Medidores que lee la GUI (se escriben desde el hilo de audio)
    std::atomic<float> vuLevel { -100.0f };        // nivel de salida en unidades VU (0 VU = -18 dBFS RMS)
    std::atomic<float> gainReductionDb { 0.0f };   // reducción de ganancia máxima del último bloque (dB, positivo)

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float getInputGainDb() const;

    // Punteros directos a los parámetros (seguros para el hilo de audio)
    std::atomic<float>* gainParam      = nullptr;
    std::atomic<float>* padParam       = nullptr;
    std::atomic<float>* lineMicParam   = nullptr;
    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ratioParam     = nullptr;
    std::atomic<float>* outputParam    = nullptr;
    std::atomic<float>* attackFastParam = nullptr;
    std::atomic<float>* bypassParam    = nullptr;

    juce::SmoothedValue<float> inputGainSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;

    double currentSampleRate = 44100.0;
    float grState = 0.0f;   // reducción de ganancia suavizada (dB)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaPautaAVCompresorAudioProcessor)
};
