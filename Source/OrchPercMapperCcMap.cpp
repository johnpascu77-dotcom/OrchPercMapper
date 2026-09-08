#include "OrchPercMapperCcMap.h"

namespace opmp
{

int getPoolGateCcNumber (Instrument instrument) noexcept
{
    switch (instrument)
    {
        // Real, already live in OrchConductor.
        case Instrument::glockenspiel: return 44;
        case Instrument::xylophone:    return 45;
        case Instrument::marimba:      return 46;
        case Instrument::vibraphone:   return 47;
        case Instrument::tubularBells: return 48;

        // Proposed, not yet built in OrchConductor - see header comment.
        case Instrument::bassDrum:     return 56;
        case Instrument::snareDrum:    return 57;
        case Instrument::cymbals:      return 58;
        case Instrument::piatti:       return 59;
        case Instrument::tamTam:       return 60;
        case Instrument::tambourine:   return 61;
        case Instrument::triangle:     return 62;

        default:
            return -1;
    }
}

Instrument getInstrumentForPoolGateCc (int ccNumber) noexcept
{
    for (int i = 0; i < numPoolInstruments; ++i)
    {
        const auto instrument = static_cast<Instrument> (i);

        if (getPoolGateCcNumber (instrument) == ccNumber)
            return instrument;
    }

    return Instrument::count;
}

} // namespace opmp
