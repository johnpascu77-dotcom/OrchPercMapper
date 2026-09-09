#include "OrchPercMapperPoolLogic.h"

#include <cassert>

namespace opmp
{

PoolAllocator::PoolAllocator (PoolConfig configIn)
    : config (configIn)
{
    assert (config.isValid());
}

int PoolAllocator::findSlotIndexFor (Instrument instrument) const noexcept
{
    for (int i = 0; i < config.poolSize; ++i)
        if (slots[static_cast<size_t> (i)].occupied
            && slots[static_cast<size_t> (i)].occupant == instrument)
            return i;

    return -1;
}

bool PoolAllocator::trySeatInstrument (Instrument instrument, double currentTimeSeconds)
{
    for (int i = 0; i < config.poolSize; ++i)
    {
        auto& slot = slots[static_cast<size_t> (i)];

        if (! slot.occupied)
        {
            slot.occupied = true;
            slot.occupant = instrument;
            slot.assignedAtSeconds = currentTimeSeconds;
            return true;
        }
    }

    return false;
}

void PoolAllocator::settle (double currentTimeSeconds)
{
    // 1. Free any occupied slot whose occupant is no longer requested and
    //    whose minimum hold time has elapsed. A slot whose occupant is
    //    still requested is never forced out, no matter how long it has
    //    been held - the pool only ever grants on demand, it never evicts
    //    an instrument that's still wanted.
    for (int i = 0; i < config.poolSize; ++i)
    {
        auto& slot = slots[static_cast<size_t> (i)];

        if (slot.occupied
            && ! requestedFlags[static_cast<size_t> (slot.occupant)]
            && (currentTimeSeconds - slot.assignedAtSeconds) >= config.minHoldSeconds)
        {
            slot.occupied = false;
        }
    }

    // 2. Seat every currently-requested instrument that isn't already
    //    holding a slot, in stable enum order (deterministic tie-break when
    //    more than one instrument is waiting for the same freed slot).
    for (int i = 0; i < numPoolInstruments; ++i)
    {
        const auto instrument = static_cast<Instrument> (i);

        if (! requestedFlags[static_cast<size_t> (i)])
            continue;

        if (findSlotIndexFor (instrument) >= 0)
            continue; // already seated

        trySeatInstrument (instrument, currentTimeSeconds);
    }
}

void PoolAllocator::setRequested (Instrument instrument, bool requested, double currentTimeSeconds)
{
    requestedFlags[static_cast<size_t> (instrument)] = requested;
    settle (currentTimeSeconds);
}

void PoolAllocator::advance (double currentTimeSeconds)
{
    settle (currentTimeSeconds);
}

bool PoolAllocator::isActive (Instrument instrument) const noexcept
{
    return requestedFlags[static_cast<size_t> (instrument)]
        && findSlotIndexFor (instrument) >= 0;
}

bool PoolAllocator::isWaiting (Instrument instrument) const noexcept
{
    return requestedFlags[static_cast<size_t> (instrument)]
        && findSlotIndexFor (instrument) < 0;
}

bool PoolAllocator::isRequested (Instrument instrument) const noexcept
{
    return requestedFlags[static_cast<size_t> (instrument)];
}

int PoolAllocator::numOccupiedSlots() const noexcept
{
    int count = 0;

    for (int i = 0; i < config.poolSize; ++i)
        if (slots[static_cast<size_t> (i)].occupied)
            ++count;

    return count;
}

} // namespace opmp
