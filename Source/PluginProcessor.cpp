#include "PluginProcessor.h"
#include "PluginEditor.h"

CrateDiggerAudioProcessor::CrateDiggerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
#endif
{
}

CrateDiggerAudioProcessor::~CrateDiggerAudioProcessor() {}

const juce::String CrateDiggerAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool CrateDiggerAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool CrateDiggerAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool CrateDiggerAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double CrateDiggerAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int CrateDiggerAudioProcessor::getNumPrograms() {
    return 1; // NB: some hosts don't like this being 0
}

int CrateDiggerAudioProcessor::getCurrentProgram() { return 0; }

void CrateDiggerAudioProcessor::setCurrentProgram(int index) {}

const juce::String CrateDiggerAudioProcessor::getProgramName(int index) {
    return {};
}

void CrateDiggerAudioProcessor::changeProgramName(int index,
                                                  const juce::String &newName) {
}

void CrateDiggerAudioProcessor::prepareToPlay(double sampleRate,
                                              int samplesPerBlock) {}

void CrateDiggerAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CrateDiggerAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void CrateDiggerAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                             juce::MidiBuffer &midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

bool CrateDiggerAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *CrateDiggerAudioProcessor::createEditor() {
    return new CrateDiggerAudioProcessorEditor(*this);
}

void CrateDiggerAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {}

void CrateDiggerAudioProcessor::setStateInformation(const void *data,
                                                    int sizeInBytes) {}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new CrateDiggerAudioProcessor();
}
