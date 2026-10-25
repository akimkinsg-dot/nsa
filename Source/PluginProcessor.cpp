#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VibeSparkProcessor::VibeSparkProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

VibeSparkProcessor::~VibeSparkProcessor()
{
}

//==============================================================================
const juce::String VibeSparkProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VibeSparkProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VibeSparkProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VibeSparkProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VibeSparkProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VibeSparkProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should at least be 1, even if you're not really implementing programs.
}

int VibeSparkProcessor::getCurrentProgram()
{
    return 0;
}

void VibeSparkProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String VibeSparkProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void VibeSparkProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void VibeSparkProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void VibeSparkProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VibeSparkProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void VibeSparkProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Process audio / MIDI here
    juce::ignoreUnused (midiMessages);
}

//==============================================================================
bool VibeSparkProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VibeSparkProcessor::createEditor()
{
    return new VibeSparkAudioProcessorEditor (*this);
}

//==============================================================================
void VibeSparkProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void VibeSparkProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

void VibeSparkProcessor::cutAll(int channel)
{
    juce::ignoreUnused (channel);
    // Method implementation for cutAll
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibeSparkProcessor();
}
