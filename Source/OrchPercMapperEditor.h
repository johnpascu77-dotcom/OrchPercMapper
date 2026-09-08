#pragma once

#include <JuceHeader.h>
#include "OrchPercMapperProcessor.h"

class OrchPercMapperAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit OrchPercMapperAudioProcessorEditor (OrchPercMapperAudioProcessor&);
    ~OrchPercMapperAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateStatus();

    OrchPercMapperAudioProcessor& audioProcessor;

    juce::ComboBox roleBox;
    juce::Label roleLabel { {}, "Role:" };
    juce::ComboBox instrumentBox;
    juce::Label instrumentLabel { {}, "Instrument:" };
    juce::Label statusLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> roleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> instrumentAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrchPercMapperAudioProcessorEditor)
};
