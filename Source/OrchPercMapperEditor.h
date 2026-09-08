#pragma once

#include <JuceHeader.h>
#include "OrchPercMapperProcessor.h"

class OrchPercMapperAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit OrchPercMapperAudioProcessorEditor (OrchPercMapperAudioProcessor&);
    ~OrchPercMapperAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OrchPercMapperAudioProcessor& audioProcessor;

    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchPercMapperAudioProcessorEditor)
};
