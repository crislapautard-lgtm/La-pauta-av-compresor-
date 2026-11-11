#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
    // Curva del compresor con rodilla suave (6 dB). Devuelve la reducción en dB (>= 0).
    float computeGainReductionDb (float levelDb, float thresholdDb, float ratio)
    {
        constexpr float knee = 6.0f;
        const float over  = levelDb - thresholdDb;
        const float slope = 1.0f - 1.0f / ratio;

        if (2.0f * over < -knee)
            return 0.0f;

        if (2.0f * std::abs (over) <= knee)
        {
            const float x = over + knee * 0.5f;
            return slope * x * x / (2.0f * knee);
        }

        return slope * over;
    }

    float timeToCoef (float ms, double sampleRate)
    {
        return std::exp (-1.0f / (0.001f * ms * (float) sampleRate));
    }
}

LaPautaAVCompresorAudioProcessor::LaPautaAVCompresorAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    gainParam       = parameters.getRawParameterValue ("gain");
    padParam        = parameters.getRawParameterValue ("pad");
    lineMicParam    = parameters.getRawParameterValue ("linemic");
    thresholdParam  = parameters.getRawParameterValue ("threshold");
    ratioParam      = parameters.getRawParameterValue ("ratio");
    outputParam     = parameters.getRawParameterValue ("output");
    attackFastParam = parameters.getRawParameterValue ("attackfast");
    bypassParam     = parameters.getRawParameterValue ("bypass");
}

LaPautaAVCompresorAudioProcessor::~LaPautaAVCompresorAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout LaPautaAVCompresorAudioProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    using FloatAttrs = juce::AudioParameterFloatAttributes;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // --- Sección PREAMP / INPUT ---
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "gain", 1 }, "Gain",
                                                                   Range (0.0f, 24.0f, 0.1f), 0.0f,
                                                                   FloatAttrs().withLabel ("dB")));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "linemic", 1 }, "Line/Mic (Mic = +12 dB)", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "pad", 1 }, "Pad -20 dB", false));

    // --- Sección OPTICAL COMPRESSOR ---
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "threshold", 1 }, "Threshold",
                                                                   Range (-40.0f, 0.0f, 0.1f), -12.0f,
                                                                   FloatAttrs().withLabel ("dB")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "ratio", 1 }, "Ratio",
                                                                   Range (1.0f, 10.0f, 0.1f), 3.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "output", 1 }, "Output",
                                                                   Range (-24.0f, 24.0f, 0.1f), 0.0f,
                                                                   FloatAttrs().withLabel ("dB")));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "attackfast", 1 }, "Attack Fast", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return { params.begin(), params.end() };
}

const juce::String LaPautaAVCompresorAudioProcessor::getName() const { return JucePlugin_Name; }
bool LaPautaAVCompresorAudioProcessor::acceptsMidi() const { return false; }
bool LaPautaAVCompresorAudioProcessor::producesMidi() const { return false; }
bool LaPautaAVCompresorAudioProcessor::isMidiEffect() const { return false; }
double LaPautaAVCompresorAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int LaPautaAVCompresorAudioProcessor::getNumPrograms() { return 1; }
int LaPautaAVCompresorAudioProcessor::getCurrentProgram() { return 0; }
void LaPautaAVCompresorAudioProcessor::setCurrentProgram (int) {}
const juce::String LaPautaAVCompresorAudioProcessor::getProgramName (int) { return {}; }
void LaPautaAVCompresorAudioProcessor::changeProgramName (int, const juce::String&) {}

float LaPautaAVCompresorAudioProcessor::getInputGainDb() const
{
    float db = gainParam->load();

    if (lineMicParam->load() > 0.5f)
        db += 12.0f;          // MIC: más ganancia de preamp

    if (padParam->load() > 0.5f)
        db -= 20.0f;          // PAD: atenuador de entrada

    return db;
}

void LaPautaAVCompresorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate = sampleRate;

    inputGainSmoothed.reset (sampleRate, 0.02);
    outputGainSmoothed.reset (sampleRate, 0.02);
    inputGainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getInputGainDb()));
    outputGainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

    grState = 0.0f;
    vuLevel.store (-100.0f);
    gainReductionDb.store (0.0f);
}

void LaPautaAVCompresorAudioProcessor::releaseResources() {}

bool LaPautaAVCompresorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void LaPautaAVCompresorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalIn    = getTotalNumInputChannels();
    const int totalOut   = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, numSamples);

    const int numChannels = juce::jmin (totalIn, totalOut, 2);

    if (numChannels == 0 || numSamples == 0)
        return;

    float maxGrDb = 0.0f;

    if (bypassParam->load() > 0.5f)
    {
        // Bypass: el audio pasa sin tocar
        grState = 0.0f;
    }
    else
    {
        const float thresholdDb = thresholdParam->load();
        const float ratio       = juce::jmax (1.0f, ratioParam->load());
        const bool  fastAttack  = attackFastParam->load() > 0.5f;
        const float gainDb      = gainParam->load();
        const bool  micOn       = lineMicParam->load() > 0.5f;

        // Saturación tipo válvula: más ganancia de preamp = más armónicos
        const float drive    = 1.0f + (gainDb / 24.0f) * 2.0f + (micOn ? 0.5f : 0.0f);
        const float invDrive = 1.0f / drive;

        // Tiempos del compresor óptico
        const float attackCoef  = timeToCoef (fastAttack ? 5.0f : 30.0f, currentSampleRate);
        const float relSlowCoef = timeToCoef (800.0f, currentSampleRate);
        const float relFastCoef = timeToCoef (120.0f, currentSampleRate);

        inputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (getInputGainDb()));
        outputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

        float* data[2] = { nullptr, nullptr };
        for (int ch = 0; ch < numChannels; ++ch)
            data[ch] = buffer.getWritePointer (ch);

        for (int s = 0; s < numSamples; ++s)
        {
            const float inGain  = inputGainSmoothed.getNextValue();
            const float outGain = outputGainSmoothed.getNextValue();

            // 1) Preamp + saturación
            float peak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float x = data[ch][s] * inGain;
                x = std::tanh (drive * x) * invDrive;
                data[ch][s] = x;
                peak = juce::jmax (peak, std::abs (x));
            }

            // 2) Detector (enlazado estéreo) y cálculo de reducción
            const float levelDb  = juce::Decibels::gainToDecibels (peak, -100.0f);
            const float grTarget = computeGainReductionDb (levelDb, thresholdDb, ratio);

            // 3) Ballistics: ataque fijo, release dependiente del programa (más rápido con más compresión)
            const float depth   = juce::jlimit (0.0f, 1.0f, grState / 12.0f);
            const float relCoef = relSlowCoef + depth * (relFastCoef - relSlowCoef);
            const float coef    = (grTarget > grState) ? attackCoef : relCoef;
            grState = grTarget + coef * (grState - grTarget);
            maxGrDb = juce::jmax (maxGrDb, grState);

            // 4) Aplicar compresión y ganancia de salida
            const float g = std::exp (-grState * 0.11512925f) * outGain;   // ln(10)/20
            for (int ch = 0; ch < numChannels; ++ch)
                data[ch][s] *= g;
        }
    }

    gainReductionDb.store (maxGrDb);

    // Medidor VU: RMS de salida (0 VU = -18 dBFS)
    float meanSquare = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float rms = buffer.getRMSLevel (ch, 0, numSamples);
        meanSquare += rms * rms;
    }
    meanSquare /= (float) numChannels;

    const float rmsDb = juce::Decibels::gainToDecibels (std::sqrt (meanSquare), -100.0f);
    vuLevel.store (rmsDb + 18.0f);
}

bool LaPautaAVCompresorAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* LaPautaAVCompresorAudioProcessor::createEditor() { return new LaPautaAVCompresorAudioProcessorEditor (*this); }

void LaPautaAVCompresorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LaPautaAVCompresorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LaPautaAVCompresorAudioProcessor();
}
