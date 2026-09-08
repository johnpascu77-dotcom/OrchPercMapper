#include "OrchPercMapperEditor.h"

OrchPercMapperAudioProcessorEditor::OrchPercMapperAudioProcessorEditor (OrchPercMapperAudioProcessor& processor)
    : AudioProcessorEditor (&processor), audioProcessor (processor)
{
    roleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (roleLabel);

    roleBox.addItemList ({ "Note Mapper", "Arbiter" }, 1);
    addAndMakeVisible (roleBox);

    roleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.getParameters(), "role", roleBox);

    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (200, 200, 200));
    statusLabel.setFont (juce::FontOptions (13.0f));
    addAndMakeVisible (statusLabel);

    updateStatus();
    startTimerHz (10);

    setSize (480, 140);
}

OrchPercMapperAudioProcessorEditor::~OrchPercMapperAudioProcessorEditor()
{
    stopTimer();
}

void OrchPercMapperAudioProcessorEditor::timerCallback()
{
    updateStatus();
}

void OrchPercMapperAudioProcessorEditor::updateStatus()
{
    if (audioProcessor.getRole() == OrchPercMapperAudioProcessor::Role::arbiter)
    {
        statusLabel.setText (
            "Arbiter: " + juce::String (audioProcessor.getNumOccupiedPoolSlots())
                + " pool slot(s) occupied. Sits downstream of OrchConductor on a "
                  "percussion bus track (see Docs/OrchPercMapper_Design.md \xC2\xA7 4/7) - "
                  "only one instance of this role should run at a time.",
            juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText (
            "Note Mapper: pass-through only as of this phase - the confirmed Iconica "
            "destination-note table exists but isn't wired into processBlock yet.",
            juce::dontSendNotification);
    }
}

void OrchPercMapperAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (30, 32, 36));
}

void OrchPercMapperAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto roleRow = area.removeFromTop (28);
    roleLabel.setBounds (roleRow.removeFromLeft (50));
    roleBox.setBounds (roleRow.removeFromLeft (150));

    area.removeFromTop (12);
    statusLabel.setBounds (area);
}
