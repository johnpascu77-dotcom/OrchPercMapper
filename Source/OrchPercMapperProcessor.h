#pragma once

#include <array>
#include <JuceHeader.h>

#include "OrchPercMapperCcMap.h"
#include "OrchPercMapperNoteCollapseLogic.h"
#include "OrchPercMapperPoolLogic.h"

// See Docs/OrchPercMapper_Design.md for the full design.
//
// Two roles, one plugin (Design doc §4/§7):
//   - NoteMapper (default): sits on one of the 7 unpitched-instrument
//     tracks, in OrchNoteMapper's usual slot, one instance per instrument
//     (its "Instrument" parameter picks which of the 7). Rewrites every
//     incoming note's pitch onto that instrument's confirmed Iconica
//     destination note (OrchPercMapperNoteLogic) - never gates
//     participation itself, that stays OrchGate's job downstream, reading
//     the Arbiter's arbitrated CC directly.
//   - Arbiter: ONE shared instance sitting downstream of OrchConductor
//     (a new percussion bus track, matching this rig's existing "N bus
//     tracks tap OC, instrument tracks tap their bus" pattern - no custom
//     IPC needed, see Design doc §7). Owns the one PoolAllocator for all 12
//     non-Timpani percussion instruments.
class OrchPercMapperAudioProcessor final : public juce::AudioProcessor
{
public:
    enum class Role
    {
        noteMapper = 0,
        arbiter
    };

    OrchPercMapperAudioProcessor();
    ~OrchPercMapperAudioProcessor() override;

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

    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // The 7 unpitched instruments, in the order the "Instrument" choice
    // parameter presents them (opmp::Instrument's own bassDrum..triangle
    // order).
    static juce::StringArray getUnpitchedInstrumentChoices();

    Role getRole() const noexcept;
    opmp::Instrument getSelectedUnpitchedInstrument() const noexcept;

    // For the editor's status display.
    bool isPoolInstrumentActive (opmp::Instrument instrument) const noexcept;
    bool isPoolInstrumentRequested (opmp::Instrument instrument) const noexcept;
    bool isPoolInstrumentWaiting (opmp::Instrument instrument) const noexcept;
    int getLastEmittedGateValue (opmp::Instrument instrument) const noexcept;
    int getNumOccupiedPoolSlots() const noexcept;
    int getNoteMapperHeldCount() const noexcept;

private:
    void processArbiterBlock (juce::MidiBuffer& midiMessages, double currentBeats);
    void processNoteMapperBlock (juce::MidiBuffer& midiMessages, bool hostIsPlaying);

    double readCurrentBeats() const;
    bool readHostIsPlaying() const;

    juce::AudioProcessorValueTreeState parameters;
    juce::AudioParameterChoice* roleParameter = nullptr;
    juce::AudioParameterChoice* instrumentParameter = nullptr;

    // Arbiter-role state. PoolConfig is currently a fixed default (see
    // Design doc §7 - hold time isn't tuned against real material yet), not
    // a plugin parameter.
    opmp::PoolAllocator poolAllocator { opmp::PoolConfig {} };

    // -1 = never emitted yet, so the first settle always sends the current
    // state at least once. Otherwise the last value actually sent, so a
    // resend only happens on change (not every block).
    std::array<int, opmp::numPoolInstruments> lastEmittedGateValues {};

    // NoteMapper-role state: every incoming pitch collapses onto one
    // destination note, so "is the destination currently sounding" is a
    // hold count, not a plain per-pitch passthrough (see
    // OrchPercMapperNoteCollapseLogic.h).
    opmp::NoteHoldCollapser noteCollapser;
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchPercMapperAudioProcessor)
};
