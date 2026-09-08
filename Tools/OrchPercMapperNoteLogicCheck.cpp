#include "OrchPercMapperNoteLogic.h"

#include <iostream>
#include <set>
#include <string>

namespace
{
    int failures = 0;

    void check (bool condition, const std::string& label)
    {
        if (condition)
        {
            std::cout << "[PASS] " << label << "\n";
        }
        else
        {
            std::cerr << "[FAIL] " << label << "\n";
            ++failures;
        }
    }

    void checkInt (int got, int expected, const std::string& label)
    {
        check (got == expected, label + " (got " + std::to_string (got)
                                + ", expected " + std::to_string (expected) + ")");
    }
}

using opmp::Instrument;

void testMalletsAreOutOfScope()
{
    const Instrument mallets[] = {
        Instrument::glockenspiel, Instrument::xylophone, Instrument::marimba,
        Instrument::vibraphone, Instrument::tubularBells
    };

    for (const auto instrument : mallets)
    {
        check (! opmp::isUnpitchedInstrument (instrument), "mallet instrument is not unpitched");
        checkInt (opmp::getUnpitchedHitDestinationNote (instrument), -1, "mallet Hit note is -1 (out of scope)");
        checkInt (opmp::getUnpitchedRollOrAlternateDestinationNote (instrument), -1, "mallet Roll note is -1 (out of scope)");
    }

    checkInt (opmp::getUnpitchedHitDestinationNote (Instrument::count), -1, "Instrument::count Hit note is -1");
}

void testConfirmedHitNotes()
{
    // Confirmed against the real Iconica Sketch Percussion Map screenshot,
    // Bitwig octave convention (C3 = MIDI 60).
    checkInt (opmp::getUnpitchedHitDestinationNote (Instrument::bassDrum), 35, "Bass Drum Hit = Gran Cassa Hit (B0)");
    checkInt (opmp::getUnpitchedHitDestinationNote (Instrument::snareDrum), 36, "Snare Drum Hit = Snare 1 Hit (C1)");
    checkInt (opmp::getUnpitchedHitDestinationNote (Instrument::cymbals), 46, "Cymbals Hit = Cymb 18\" Hit (A#1)");
    checkInt (opmp::getUnpitchedHitDestinationNote (Instrument::piatti), 42, "Piatti Hit = Piatti Med Hit (F#1)");
    checkInt (opmp::getUnpitchedHitDestinationNote (Instrument::tamTam), 51, "Tam-Tam Hit = Tam Tam Hit (D#2)");
    checkInt (opmp::getUnpitchedHitDestinationNote (Instrument::tambourine), 53, "Tambourine Hit = Tambourine Hit (F2)");
    checkInt (opmp::getUnpitchedHitDestinationNote (Instrument::triangle), 55, "Triangle Hit = Triangle Hit (G2)");
}

void testConfirmedRollOrAlternateNotes()
{
    checkInt (opmp::getUnpitchedRollOrAlternateDestinationNote (Instrument::bassDrum), 34, "Bass Drum Roll = Gran Cassa Roll (A#0)");
    checkInt (opmp::getUnpitchedRollOrAlternateDestinationNote (Instrument::snareDrum), 37, "Snare Drum Roll = Snare 1 Roll (C#1)");
    checkInt (opmp::getUnpitchedRollOrAlternateDestinationNote (Instrument::cymbals), 48, "Cymbals Roll = Cymb 18\" Roll (C2)");
    checkInt (opmp::getUnpitchedRollOrAlternateDestinationNote (Instrument::piatti), 44, "Piatti alternate = Piatti Large Hit (G#1)");
    checkInt (opmp::getUnpitchedRollOrAlternateDestinationNote (Instrument::tamTam), 52, "Tam-Tam Roll = Tam Tam Roll (E2)");
    checkInt (opmp::getUnpitchedRollOrAlternateDestinationNote (Instrument::tambourine), 54, "Tambourine Roll = Tambourine Roll (F#2)");
    checkInt (opmp::getUnpitchedRollOrAlternateDestinationNote (Instrument::triangle), 56, "Triangle Roll = Triangle Roll (G#2)");
}

void testHitAndRollAreDistinctPerInstrument()
{
    const Instrument unpitched[] = {
        Instrument::bassDrum, Instrument::snareDrum, Instrument::cymbals, Instrument::piatti,
        Instrument::tamTam, Instrument::tambourine, Instrument::triangle
    };

    for (const auto instrument : unpitched)
    {
        check (opmp::getUnpitchedHitDestinationNote (instrument)
                   != opmp::getUnpitchedRollOrAlternateDestinationNote (instrument),
               "Hit and Roll/alternate notes differ for the same instrument");
    }
}

void testAllDestinationNotesAreUniqueAcrossInstruments()
{
    // A transcription error that accidentally aliased two instruments onto
    // the same destination key would be a real, silent musical bug - catch
    // it here rather than discovering it by ear.
    const Instrument unpitched[] = {
        Instrument::bassDrum, Instrument::snareDrum, Instrument::cymbals, Instrument::piatti,
        Instrument::tamTam, Instrument::tambourine, Instrument::triangle
    };

    std::set<int> seenNotes;
    bool allUnique = true;

    for (const auto instrument : unpitched)
    {
        for (const int note : { opmp::getUnpitchedHitDestinationNote (instrument),
                                 opmp::getUnpitchedRollOrAlternateDestinationNote (instrument) })
        {
            if (! seenNotes.insert (note).second)
                allUnique = false;
        }
    }

    check (allUnique, "all 14 Hit/Roll destination notes across the 7 unpitched instruments are unique");
    checkInt (static_cast<int> (seenNotes.size()), 14, "exactly 14 distinct destination notes recorded");
}

void testAllNotesInValidMidiRange()
{
    const Instrument unpitched[] = {
        Instrument::bassDrum, Instrument::snareDrum, Instrument::cymbals, Instrument::piatti,
        Instrument::tamTam, Instrument::tambourine, Instrument::triangle
    };

    for (const auto instrument : unpitched)
    {
        const int hit = opmp::getUnpitchedHitDestinationNote (instrument);
        const int roll = opmp::getUnpitchedRollOrAlternateDestinationNote (instrument);

        check (hit >= 0 && hit <= 127, "Hit note is a valid MIDI note number");
        check (roll >= 0 && roll <= 127, "Roll/alternate note is a valid MIDI note number");
    }
}

int main()
{
    testMalletsAreOutOfScope();
    testConfirmedHitNotes();
    testConfirmedRollOrAlternateNotes();
    testHitAndRollAreDistinctPerInstrument();
    testAllDestinationNotesAreUniqueAcrossInstruments();
    testAllNotesInValidMidiRange();

    if (failures > 0)
    {
        std::cerr << "\n" << failures << " check(s) failed.\n";
        return 1;
    }

    std::cout << "\nAll OrchPercMapper note-logic checks passed.\n";
    return 0;
}
