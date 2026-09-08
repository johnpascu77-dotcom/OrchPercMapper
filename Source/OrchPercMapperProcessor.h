#pragma once

#include <array>
#include <JuceHeader.h>

#include "OrchPercMapperCcMap.h"
#include "OrchPercMapperPoolLogic.h"

// See Docs/OrchPercMapper_Design.md for the full design.
//
// Two roles, one plugin (Design doc §4/§7):
//   - NoteMapper (default): sits on one of the 7 unpitched-instrument
//     tracks, in OrchNoteMapper's usual slot. Still pure pass-through as of
//     this phase - the confirmed Iconica note-identity table
//     (OrchPercMapperNoteLogic) exists and is tested, but nothing in
//     processBlock calls it yet.
//   - Arbiter: ONE shared instance sitting downstream of OrchConductor
//     (a new percussion bus track, matching this rig's existing "N bus
//     tracks tap OC, instrument tracks tap their bus" pattern - no custom
//     IPC needed, see Design doc §4). Owns the one PoolAllocator for all 12
//     non-Timpani percussion instruments: consumes their raw eligibility CC
//     as a pool *request*, passes everything else through unchanged, and
//     emits the arbitrated gate CC (sent only on change, not every block)
//     for downstream OrchGate instances and the 7 NoteMapper-role instances
//     to read via a Note Receiver.
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

    Role getRole() const noexcept;

    // For the editor's status display.
    bool isPoolInstrumentActive (opmp::Instrument instrument) const noexcept;
    int getNumOccupiedPoolSlots() const noexcept;

private:
    void processArbiterBlock (juce::MidiBuffer& midiMessages, double currentBeats);

    double readCurrentBeats() const;

    juce::AudioProcessorValueTreeState parameters;
    juce::AudioParameterChoice* roleParameter = nullptr;

    // Arbiter-role state. PoolConfig is currently a fixed default (see
    // Design doc §6 - hold time isn't tuned against real material yet), not
    // a plugin parameter.
    opmp::PoolAllocator poolAllocator { opmp::PoolConfig {} };

    // -1 = never emitted yet, so the first settle always sends the current
    // state at least once. Otherwise the last value actually sent, so a
    // resend only happens on change (not every block).
    std::array<int, opmp::numPoolInstruments> lastEmittedGateValues {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchPercMapperAudioProcessor)
};
