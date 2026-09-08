#include "OrchPercMapperProcessor.h"
#include "OrchPercMapperEditor.h"
#include "OrchPercMapperNoteLogic.h"

namespace
{
    constexpr const char* roleParameterId = "role";
    constexpr const char* instrumentParameterId = "instrument";

    // The 7 unpitched instruments, in opmp::Instrument's own bassDrum..
    // triangle order - index 0 of this list is opmp::Instrument::bassDrum.
    constexpr int firstUnpitchedInstrumentIndex = static_cast<int> (opmp::Instrument::bassDrum);
}

juce::StringArray OrchPercMapperAudioProcessor::getUnpitchedInstrumentChoices()
{
    return { "Bass Drum", "Snare Drum", "Cymbals", "Piatti", "Tam-Tam", "Tambourine", "Triangle" };
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

    // Only meaningful in NoteMapper role - which of the 7 unpitched
    // instruments this instance represents, same "one instrument per
    // instance" pattern as OrchNoteMapper's own Instrument Preset.
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { instrumentParameterId, 1 },
        "Instrument",
        OrchPercMapperAudioProcessor::getUnpitchedInstrumentChoices(),
        0));

    return { params.begin(), params.end() };
}

OrchPercMapperAudioProcessor::OrchPercMapperAudioProcessor()
    : AudioProcessor (BusesProperties()),
      parameters (*this, nullptr, "OrchPercMapperState", createParameterLayout())
{
    roleParameter = dynamic_cast<juce::AudioParameterChoice*> (parameters.getParameter (roleParameterId));
    instrumentParameter = dynamic_cast<juce::AudioParameterChoice*> (parameters.getParameter (instrumentParameterId));

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

opmp::Instrument OrchPercMapperAudioProcessor::getSelectedUnpitchedInstrument() const noexcept
{
    const int index = instrumentParameter != nullptr ? instrumentParameter->getIndex() : 0;
    return static_cast<opmp::Instrument> (firstUnpitchedInstrumentIndex + index);
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

bool OrchPercMapperAudioProcessor::readHostIsPlaying() const
{
    if (auto* transport = const_cast<OrchPercMapperAudioProcessor*> (this)->getPlayHead())
        if (const auto position = transport->getPosition())
            return position->getIsPlaying();

    return false;
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

void OrchPercMapperAudioProcessor::processNoteMapperBlock (juce::MidiBuffer& midiMessages, bool hostIsPlaying)
{
    const int destinationNote = opmp::getUnpitchedHitDestinationNote (getSelectedUnpitchedInstrument());

    // Should never happen (the Instrument parameter only ever offers the 7
    // unpitched choices), but never emit an invalid note number if it does.
    if (destinationNote < 0)
    {
        wasPlaying = hostIsPlaying;
        return;
    }

    juce::MidiBuffer remapped;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            // Every incoming pitch collapses onto the one destination note -
            // an overlapping second note-on shouldn't retrigger it (see
            // OrchPercMapperNoteCollapseLogic.h).
            if (noteCollapser.noteOn())
                remapped.addEvent (
                    juce::MidiMessage::noteOn (message.getChannel(), destinationNote, message.getVelocity()),
                    metadata.samplePosition);

            continue;
        }

        if (message.isNoteOff())
        {
            if (noteCollapser.noteOff())
                remapped.addEvent (
                    juce::MidiMessage::noteOff (message.getChannel(), destinationNote, message.getVelocity()),
                    metadata.samplePosition);

            continue;
        }

        // Anything else (CCs, etc.) passes through unchanged - this role
        // only ever rewrites note-on/note-off pitch, participation gating
        // is OrchGate's job downstream.
        remapped.addEvent (message, metadata.samplePosition);
    }

    // Mirrors OrchMerge's own releaseAllHeld() fix for the same class of
    // bug: a note still logically held at the exact instant the host stops
    // could otherwise never see its eventual note-off, leaving the
    // destination note stuck sounding into the next take.
    if (wasPlaying && ! hostIsPlaying && noteCollapser.forceRelease())
        remapped.addEvent (juce::MidiMessage::noteOff (1, destinationNote), 0);

    wasPlaying = hostIsPlaying;

    midiMessages.swapWith (remapped);
}

void OrchPercMapperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    if (getRole() == Role::arbiter)
        processArbiterBlock (midiMessages, readCurrentBeats());
    else
        processNoteMapperBlock (midiMessages, readHostIsPlaying());
}

bool OrchPercMapperAudioProcessor::isPoolInstrumentActive (opmp::Instrument instrument) const noexcept
{
    return poolAllocator.isActive (instrument);
}

int OrchPercMapperAudioProcessor::getNumOccupiedPoolSlots() const noexcept
{
    return poolAllocator.numOccupiedSlots();
}

int OrchPercMapperAudioProcessor::getNoteMapperHeldCount() const noexcept
{
    return noteCollapser.getHeldCount();
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
