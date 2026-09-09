#include "OrchPercMapperEditor.h"
#include "OrchPercMapperCcMap.h"

namespace
{
    // opmp::Instrument's own glockenspiel..triangle order.
    const char* const allPoolInstrumentNames[opmp::numPoolInstruments] =
    {
        "Glockenspiel", "Xylophone", "Marimba", "Vibraphone", "Tubular Bells",
        "Bass Drum", "Snare Drum", "Cymbals", "Piatti", "Tam-Tam", "Tambourine", "Triangle"
    };
}

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

    statusLabel.setJustificationType (juce::Justification::topLeft);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour::fromRGB (200, 200, 200));
    statusLabel.setFont (juce::FontOptions (13.0f));
    addAndMakeVisible (statusLabel);

    updateStatus();
    startTimerHz (10);

    setSize (480, 420);
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
        statusLabel.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));

        // Read every field from ONE snapshot taken atomically on the audio
        // thread - never mix-and-match separate live calls here, that's
        // exactly what produced torn, impossible-looking rows before
        // (requested but neither active nor waiting).
        const auto snapshot = audioProcessor.getArbiterDiagnosticsSnapshot();

        juce::String text;
        text << "Arbiter: " << snapshot.occupiedSlots
             << " pool slot(s) occupied. Only one instance of this role should run at a time.\n\n";
        text << "Instrument      CC  req  active  waiting  last-sent\n";

        for (int i = 0; i < opmp::numPoolInstruments; ++i)
        {
            const auto instrument = static_cast<opmp::Instrument> (i);
            const int cc = opmp::getPoolGateCcNumber (instrument);
            const auto index = static_cast<size_t> (i);

            juce::String name (allPoolInstrumentNames[i]);
            text << name.paddedRight (' ', 15)
                 << juce::String (cc).paddedLeft (' ', 3) << "  "
                 << (snapshot.requested[index] ? "Y  " : ".  ")
                 << "  "
                 << (snapshot.active[index] ? "Y     " : ".     ")
                 << "  "
                 << (snapshot.waiting[index] ? "Y      " : ".      ")
                 << "  "
                 << juce::String (snapshot.lastSent[index]) << "\n";
        }

        statusLabel.setText (text, juce::dontSendNotification);
    }
    else
    {
        statusLabel.setFont (juce::FontOptions (13.0f));

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
