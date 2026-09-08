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

    instrumentLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (instrumentLabel);

    instrumentBox.addItemList (OrchPercMapperAudioProcessor::getUnpitchedInstrumentChoices(), 1);
    addAndMakeVisible (instrumentBox);

    instrumentAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.getParameters(), "instrument", instrumentBox);

    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (200, 200, 200));
    statusLabel.setFont (juce::FontOptions (13.0f));
    addAndMakeVisible (statusLabel);

    updateStatus();
    startTimerHz (10);

    setSize (480, 160);
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
    const bool isArbiter = audioProcessor.getRole() == OrchPercMapperAudioProcessor::Role::arbiter;

    instrumentBox.setEnabled (! isArbiter);
    instrumentLabel.setEnabled (! isArbiter);

    if (isArbiter)
    {
        statusLabel.setText (
            "Arbiter: " + juce::String (audioProcessor.getNumOccupiedPoolSlots())
                + " pool slot(s) occupied. Sits downstream of OrchConductor on a "
                  "percussion bus track (see Docs/OrchPercMapper_Design.md \xC2\xA7 7) - "
                  "only one instance of this role should run at a time.",
            juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText (
            "Note Mapper: remaps every incoming note to this instrument's Iconica "
            "destination key (held count: " + juce::String (audioProcessor.getNoteMapperHeldCount())
                + "). Does not gate participation itself - reads the Arbiter's "
                  "arbitrated CC via OrchGate downstream, same as every other "
                  "instrument in this rig.",
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

    area.removeFromTop (8);

    auto instrumentRow = area.removeFromTop (28);
    instrumentLabel.setBounds (instrumentRow.removeFromLeft (80));
    instrumentBox.setBounds (instrumentRow.removeFromLeft (150));

    area.removeFromTop (12);
    statusLabel.setBounds (area);
}
