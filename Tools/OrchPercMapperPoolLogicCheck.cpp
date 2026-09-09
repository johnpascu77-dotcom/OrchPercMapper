#include "OrchPercMapperPoolLogic.h"

#include <iostream>
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
}

using opmp::Instrument;
using opmp::PoolAllocator;
using opmp::PoolConfig;

void testBasicGrant()
{
    PoolConfig config;
    config.poolSize = 3;
    config.minHoldSeconds = 4.0;

    PoolAllocator pool (config);

    pool.setRequested (Instrument::marimba, true, 0.0);
    pool.setRequested (Instrument::snareDrum, true, 0.0);
    pool.setRequested (Instrument::triangle, true, 0.0);

    check (pool.isActive (Instrument::marimba), "basic grant: marimba active within pool size");
    check (pool.isActive (Instrument::snareDrum), "basic grant: snareDrum active within pool size");
    check (pool.isActive (Instrument::triangle), "basic grant: triangle active within pool size");
    check (pool.numOccupiedSlots() == 3, "basic grant: 3 slots occupied");
}

void testPoolFullMakesFourthWait()
{
    PoolConfig config;
    config.poolSize = 3;
    config.minHoldSeconds = 4.0;

    PoolAllocator pool (config);

    pool.setRequested (Instrument::marimba, true, 0.0);
    pool.setRequested (Instrument::snareDrum, true, 0.0);
    pool.setRequested (Instrument::triangle, true, 0.0);
    pool.setRequested (Instrument::tamTam, true, 0.0);

    check (! pool.isActive (Instrument::tamTam), "pool full: tamTam not active when pool already has 3");
    check (pool.isWaiting (Instrument::tamTam), "pool full: tamTam is waiting");
    check (pool.numOccupiedSlots() == 3, "pool full: still exactly 3 slots occupied");
}

void testHoldTimeBlocksImmediateReassignment()
{
    PoolConfig config;
    config.poolSize = 1;
    config.minHoldSeconds = 4.0;

    PoolAllocator pool (config);

    pool.setRequested (Instrument::marimba, true, 0.0);
    check (pool.isActive (Instrument::marimba), "hold time: marimba granted the only slot");

    // Marimba stops being requested almost immediately, and something else
    // wants the (now nominally free) slot right away. The player who just
    // sat down at Marimba can't be reassigned to Triangle a beat later.
    pool.setRequested (Instrument::marimba, false, 1.0);
    pool.setRequested (Instrument::triangle, true, 1.0);

    check (! pool.isActive (Instrument::triangle), "hold time: triangle NOT granted before hold time elapses");
    check (pool.isWaiting (Instrument::triangle), "hold time: triangle is waiting during the hold window");

    // Advance past the hold time (assigned at beat 0, hold = 4 beats).
    pool.advance (5.0);

    check (pool.isActive (Instrument::triangle), "hold time: triangle granted once hold time has elapsed");
}

void testStillWantedInstrumentIsNeverEvicted()
{
    PoolConfig config;
    config.poolSize = 1;
    config.minHoldSeconds = 4.0;

    PoolAllocator pool (config);

    pool.setRequested (Instrument::marimba, true, 0.0);

    // Something else wants the slot, but marimba is still requested - even
    // long after the hold time would otherwise allow reassignment, marimba
    // must not be evicted while it's still wanted. The pool only grants on
    // demand; it never steals from an instrument still in use.
    pool.setRequested (Instrument::triangle, true, 1.0);
    pool.advance (100.0);

    check (pool.isActive (Instrument::marimba), "no eviction: marimba stays active while still requested");
    check (pool.isWaiting (Instrument::triangle), "no eviction: triangle keeps waiting indefinitely");
}

void testQuickReturnBeforeHoldExpiryKeepsSlot()
{
    PoolConfig config;
    config.poolSize = 1;
    config.minHoldSeconds = 4.0;

    PoolAllocator pool (config);

    pool.setRequested (Instrument::marimba, true, 0.0);
    pool.setRequested (Instrument::marimba, false, 1.0); // brief drop
    pool.setRequested (Instrument::marimba, true, 1.5);  // back on, still within hold window

    check (pool.isActive (Instrument::marimba), "quick return: marimba keeps its slot uninterrupted");
    check (pool.numOccupiedSlots() == 1, "quick return: no extra slot ever allocated");
}

void testFirstRequesterWinsAFreeSlot()
{
    // Each setRequested() call settles immediately, matching how MIDI CC
    // events actually arrive one at a time (even within one audio block,
    // they have a relative order) - so two instruments requesting "at the
    // same beat" are still resolved strictly in call order, not by enum
    // index. This is deliberate: it's the simpler, more honest model of how
    // the plugin will actually be driven, not an arbitrary tie-break.
    PoolConfig config;
    config.poolSize = 1;
    config.minHoldSeconds = 0.0;

    PoolAllocator pool (config);

    pool.setRequested (Instrument::marimba, true, 0.0);
    pool.setRequested (Instrument::marimba, false, 0.0);
    pool.advance (0.0); // minHoldSeconds = 0, so the slot is free immediately

    // triangle (higher enum index than tamTam) requests first and should
    // win the single free slot despite the enum ordering.
    pool.setRequested (Instrument::triangle, true, 1.0);
    pool.setRequested (Instrument::tamTam, true, 1.0);

    check (pool.isActive (Instrument::triangle), "arrival order: first requester (triangle) wins the free slot");
    check (! pool.isActive (Instrument::tamTam), "arrival order: later requester (tamTam) waits instead");
}

void testSimultaneousAdvanceFallsBackToEnumOrder()
{
    // The one case where enum order genuinely applies: multiple instruments
    // are *already* both marked requested (e.g. restored from state) before
    // a slot frees up via advance() alone, with no setRequested() call to
    // establish an arrival order between them. settle()'s single scan pass
    // (in Instrument enum order) is what decides it here.
    PoolConfig config;
    config.poolSize = 1;
    config.minHoldSeconds = 4.0;

    PoolAllocator pool (config);

    pool.setRequested (Instrument::marimba, true, 0.0);
    pool.setRequested (Instrument::marimba, false, 1.0);

    // Both instruments become "requested" while the slot is still held by
    // marimba's hold time (not yet resolved to either of them).
    pool.setRequested (Instrument::triangle, true, 1.0);
    pool.setRequested (Instrument::tamTam, true, 1.0);

    check (pool.isWaiting (Instrument::triangle), "pre-advance: triangle still waiting during marimba's hold");
    check (pool.isWaiting (Instrument::tamTam), "pre-advance: tamTam still waiting during marimba's hold");

    pool.advance (5.0); // past marimba's hold time; both are already "requested"

    check (pool.isActive (Instrument::tamTam), "enum fallback: tamTam (earlier enum index) wins on advance()");
    check (! pool.isActive (Instrument::triangle), "enum fallback: triangle keeps waiting");
}

int main()
{
    testBasicGrant();
    testPoolFullMakesFourthWait();
    testHoldTimeBlocksImmediateReassignment();
    testStillWantedInstrumentIsNeverEvicted();
    testQuickReturnBeforeHoldExpiryKeepsSlot();
    testFirstRequesterWinsAFreeSlot();
    testSimultaneousAdvanceFallsBackToEnumOrder();

    if (failures > 0)
    {
        std::cerr << "\n" << failures << " check(s) failed.\n";
        return 1;
    }

    std::cout << "\nAll OrchPercMapper pool-allocator checks passed.\n";
    return 0;
}
