#include "OrchPercMapperProcessor.h"
#include "OrchPercMapperEditor.h"

OrchPercMapperAudioProcessor::OrchPercMapperAudioProcessor()
    : AudioProcessor (BusesProperties())
{
}

OrchPercMapperAudioProcessor::~OrchPercMapperAudioProcessor() = default;

void OrchPercMapperAudioProcessor::prepareToPlay (double, int)
{
}

void OrchPercMapperAudioProcessor::releaseResources()
{
}

bool OrchPercMapperAudioProcessor::isBusesLayoutSupported (const BusesLayout&) const
{
    return true;
}

void OrchPercMapperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    buffer.clear();

    // Phase 0: pass MIDI through unchanged. Note-identity mapping and
    // pool-arbitration logic land in a later phase - see
    // Docs/OrchPercMapper_Design.md.
}

bool OrchPercMapperAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OrchPercMapperAudioProcessor::createEditor()
{
    return new OrchPercMapperAudioProcessorEditor (*this);
}

const juce::String OrchPercMapperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OrchPercMapperAudioProcessor::acceptsMidi() const
{
    return true;
}

bool OrchPercMapperAudioProcessor::producesMidi() const
{
    return true;
}

bool OrchPercMapperAudioProcessor::isMidiEffect() const
{
    return true;
}

double OrchPercMapperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OrchPercMapperAudioProcessor::getNumPrograms()
{
    return 1;
}

int OrchPercMapperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OrchPercMapperAudioProcessor::setCurrentProgram (int)
{
}

const juce::String OrchPercMapperAudioProcessor::getProgramName (int)
{
    return {};
}

void OrchPercMapperAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void OrchPercMapperAudioProcessor::getStateInformation (juce::MemoryBlock&)
{
}

void OrchPercMapperAudioProcessor::setStateInformation (const void*, int)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrchPercMapperAudioProcessor();
}
