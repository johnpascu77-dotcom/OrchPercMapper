#include "OrchPercMapperNoteLogic.h"

namespace opmp
{

bool isUnpitchedInstrument (Instrument instrument) noexcept
{
    switch (instrument)
    {
        case Instrument::bassDrum:
        case Instrument::snareDrum:
        case Instrument::cymbals:
        case Instrument::piatti:
        case Instrument::tamTam:
        case Instrument::tambourine:
        case Instrument::triangle:
            return true;

        default:
            return false;
    }
}

int getUnpitchedHitDestinationNote (Instrument instrument) noexcept
{
    switch (instrument)
    {
        case Instrument::bassDrum:  return 35; // Gran Cassa Hit    (B0)
        case Instrument::snareDrum: return 36; // Snare 1 Hit       (C1)
        case Instrument::cymbals:   return 46; // Cymb 18" Hit      (A#1)
        case Instrument::piatti:    return 42; // Piatti Med Hit    (F#1)
        case Instrument::tamTam:    return 51; // Tam Tam Hit       (D#2)
        case Instrument::tambourine:return 53; // Tambourine Hit    (F2)
        case Instrument::triangle:  return 55; // Triangle Hit      (G2)

        default:
            return -1;
    }
}

int getUnpitchedRollOrAlternateDestinationNote (Instrument instrument) noexcept
{
    switch (instrument)
    {
        case Instrument::bassDrum:  return 34; // Gran Cassa Roll   (A#0)
        case Instrument::snareDrum: return 37; // Snare 1 Roll      (C#1)
        case Instrument::cymbals:   return 48; // Cymb 18" Roll     (C2)
        case Instrument::piatti:    return 44; // Piatti Large Hit  (G#1) - no Roll voice
        case Instrument::tamTam:    return 52; // Tam Tam Roll      (E2)
        case Instrument::tambourine:return 54; // Tambourine Roll   (F#2)
        case Instrument::triangle:  return 56; // Triangle Roll     (G#2)

        default:
            return -1;
    }
}

} // namespace opmp
