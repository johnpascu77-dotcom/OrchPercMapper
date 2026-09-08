#pragma once

#include <JuceHeader.h>

// Phase 0 skeleton - no note-identity mapping or pool-arbitration logic yet.
// See Docs/OrchPercMapper_Design.md for the full design: this plugin will
// eventually (a) map the 7 unpitched percussion instruments' incoming notes
// onto fixed destination keys (default: Iconica Sketch's Percussion Map),
// and (b) arbitrate all 12 non-Timpani percussion instruments' OrchConductor
// gate CCs against a shared 3-4-player pool with a minimum hold time.
//
// For now this just passes MIDI through unchanged, to prove the build/
// install pipeline before any real logic is written.
class OrchPercMapperAudioProcessor final : public juce::AudioProcessor
{
public:
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

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchPercMapperAudioProcessor)
};
