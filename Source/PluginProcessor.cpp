#include "PluginProcessor.h"
#include "PluginEditor.h"

LaPautaAVCompresorAudioProcessor::LaPautaAVCompresorAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

LaPautaAVCompresorAudioProcessor::~LaPautaAVCompresorAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout LaPautaAVCompresorAudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Controles inspirados en la sección óptica y de ganancia del hardware
    params.push_back (std::make_unique<juce::AudioParameterFloat>("gain", "Gain", juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("compression", "Compression", juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>("output", "Output", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

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

void LaPautaAVCompresorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    // Inicializar filtros o variables de procesamiento DSP aquí
}

void LaPautaAVCompresorAudioProcessor::releaseResources() {}

bool LaPautaAVCompresorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void LaPautaAVCompresorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // --- Aquí irá la lógica DSP de saturación por válvulas y compresión óptica ---
}

bool LaPautaAVCompresorAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* LaPautaAVCompresorAudioProcessor::createEditor() { return new LaPautaAVCompresorAudioProcessorEditor (*this); }

void LaPautaAVCompresorAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LaPautaAVCompresorAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new LaPautaAVCompresorAudioProcessor();
}
