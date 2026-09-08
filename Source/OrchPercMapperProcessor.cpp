#include "OrchPercMapperProcessor.h"
#include "OrchPercMapperEditor.h"

namespace
{
    constexpr const char* roleParameterId = "role";
}

juce::AudioProcessorValueTreeState::ParameterLayout OrchPercMapperAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Default to NoteMapper - the safe choice, matching this ecosystem's own
    // convention (OrchMerge's role param also defaults to the "just relay my
    // own track, never bind anything shared" role) so a freshly-dropped
    // instance never accidentally tries to act as the one shared Arbiter.
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { roleParameterId, 1 },
        "Role",
        juce::StringArray { "Note Mapper", "Arbiter" },
        0));

    return { params.begin(), params.end() };
}

OrchPercMapperAudioProcessor::OrchPercMapperAudioProcessor()
    : AudioProcessor (BusesProperties()),
      parameters (*this, nullptr, "OrchPercMapperState", createParameterLayout())
{
    roleParameter = dynamic_cast<juce::AudioParameterChoice*> (parameters.getParameter (roleParameterId));

    lastEmittedGateValues.fill (-1);
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

OrchPercMapperAudioProcessor::Role OrchPercMapperAudioProcessor::getRole() const noexcept
{
    return roleParameter != nullptr && roleParameter->getIndex() == 1
        ? Role::arbiter
        : Role::noteMapper;
}

double OrchPercMapperAudioProcessor::readCurrentBeats() const
{
    if (auto* transport = const_cast<OrchPercMapperAudioProcessor*> (this)->getPlayHead())
    {
        if (const auto position = transport->getPosition())
        {
            if (const auto ppq = position->getPpqPosition())
                return *ppq;
        }
    }

    return 0.0;
}

void OrchPercMapperAudioProcessor::processArbiterBlock (juce::MidiBuffer& midiMessages, double currentBeats)
{
    juce::MidiBuffer passthrough;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isController())
        {
            const auto instrument = opmp::getInstrumentForPoolGateCc (message.getControllerNumber());

            if (instrument != opmp::Instrument::count)
            {
                // One of the 12 pool CCs: consumed as a participation
                // request, not forwarded raw - the arbitrated value (below)
                // is what actually reaches the downstream OrchGate/
                // NoteMapper instances.
                poolAllocator.setRequested (instrument, message.getControllerValue() > 0, currentBeats);
                continue;
            }
        }

        // Every other instrument's CC, and any note data, passes through
        // unchanged - the Arbiter only ever touches the 12 pool CCs.
        passthrough.addEvent (message, metadata.samplePosition);
    }

    // Re-evaluate hold-time expiry even when no CC arrived this block, so a
    // waiting instrument gets granted as soon as it's eligible, not only on
    // the next incoming request.
    poolAllocator.advance (currentBeats);

    // Emit the arbitrated gate CC only on change, not every block - a CC
    // stream should stay sparse, matching how OrchConductor itself only
    // sends on an explicit preset/combi change rather than continuously.
    for (int i = 0; i < opmp::numPoolInstruments; ++i)
    {
        const auto instrument = static_cast<opmp::Instrument> (i);
        const int value = poolAllocator.isActive (instrument) ? 127 : 0;

        if (lastEmittedGateValues[static_cast<size_t> (i)] != value)
        {
            const int cc = opmp::getPoolGateCcNumber (instrument);
            passthrough.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
            lastEmittedGateValues[static_cast<size_t> (i)] = value;
        }
    }

    midiMessages.swapWith (passthrough);
}

void OrchPercMapperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    if (getRole() == Role::arbiter)
        processArbiterBlock (midiMessages, readCurrentBeats());

    // NoteMapper role: still pure pass-through as of this phase. The
    // confirmed Iconica note-identity table (OrchPercMapperNoteLogic) is
    // built and tested but not yet wired here - see Design doc §6.
}

bool OrchPercMapperAudioProcessor::isPoolInstrumentActive (opmp::Instrument instrument) const noexcept
{
    return poolAllocator.isActive (instrument);
}

int OrchPercMapperAudioProcessor::getNumOccupiedPoolSlots() const noexcept
{
    return poolAllocator.numOccupiedSlots();
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

void OrchPercMapperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void OrchPercMapperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrchPercMapperAudioProcessor();
}
