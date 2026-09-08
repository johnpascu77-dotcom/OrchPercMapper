#include "OrchPercMapperEditor.h"

OrchPercMapperAudioProcessorEditor::OrchPercMapperAudioProcessorEditor (OrchPercMapperAudioProcessor& processor)
    : AudioProcessorEditor (&processor), audioProcessor (processor)
{
    statusLabel.setText ("OrchPercMapper - Phase 0 skeleton (pass-through, no mapping/pool logic yet)",
                          juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (220, 220, 220));
    addAndMakeVisible (statusLabel);

    setSize (480, 120);
}

OrchPercMapperAudioProcessorEditor::~OrchPercMapperAudioProcessorEditor() = default;

void OrchPercMapperAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (30, 32, 36));
}

void OrchPercMapperAudioProcessorEditor::resized()
{
    statusLabel.setBounds (getLocalBounds().reduced (12));
}
