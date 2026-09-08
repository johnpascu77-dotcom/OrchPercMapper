#pragma once

#include <array>

// Pure logic for the Layer-2 shared player-pool allocator described in
// Docs/OrchPercMapper_Design.md §3. No JUCE dependency - deterministic,
// real-time safe (no allocation), independently testable.
//
// Models 3-4 real percussionists sharing 12 instruments (the 5 pitched
// mallets + the 7 unpitched instruments; Timpani is excluded by
// construction - it is simply never passed to this allocator, since it has
// its own dedicated player and never competes for a pool slot).
//
// NOTE on architecture (see Design doc §4): this logic answers "given who
// currently wants to play, who actually gets a player, respecting a hold
// time." It does *not* yet address how state gets shared across the several
// separate plugin instances that will need it (one OrchPercMapper instance
// per unpitched-instrument track for note mapping, one shared instance for
// CC arbitration) - that is a distinct, harder problem (most likely an
// OrchMerge-style Hub/Sender IPC link) tracked separately, not solved here.
namespace opmp
{

enum class Instrument
{
    // Pitched mallets: notes stay in OrchNoteMapper; only their gate CC is
    // arbitrated here.
    glockenspiel = 0,
    xylophone,
    marimba,
    vibraphone,
    tubularBells,

    // Unpitched instruments: both note-identity mapping (see
    // OrchPercMapperNoteLogic) and gate-CC arbitration happen here.
    bassDrum,
    snareDrum,
    cymbals,
    piatti,
    tamTam,
    tambourine,
    triangle,

    count
};

constexpr int numPoolInstruments = static_cast<int> (Instrument::count);
constexpr int maxPoolSize = 8; // generous fixed upper bound; real pools are 3-4

struct PoolConfig
{
    int poolSize = 3;           // 3-4 real players sharing the 12 instruments
    double minHoldBeats = 4.0;  // one default constant to start (see Design doc §3);
                                // adjustable later per instrument pair if real use
                                // shows a single constant doesn't hold up

    bool isValid() const noexcept
    {
        return poolSize > 0 && poolSize <= maxPoolSize && minHoldBeats >= 0.0;
    }
};

// A newly-requested instrument is settled immediately against whatever slots
// are free at that instant, so two requests generally resolve in arrival
// order (matching how MIDI CC events actually arrive), not by instrument
// identity. Instrument enum order only acts as a tie-break in the one case
// where it can't be avoided: several instruments already marked requested
// before a slot frees purely via advance() (e.g. after state restore), with
// no setRequested() call between them to establish an order. Simple and
// deterministic; revisit if real use disagrees.
class PoolAllocator
{
public:
    explicit PoolAllocator (PoolConfig configIn);

    // Call whenever an instrument's Layer-1 eligibility (the OrchConductor
    // gate CC arbitrated here) changes state. currentBeats is the host's
    // current beat position.
    void setRequested (Instrument instrument, bool requested, double currentBeats);

    // Re-evaluates hold-time expiry and pending requests without a
    // request-state change on any instrument, so a slot freed purely by the
    // passage of time gets handed to a waiting instrument promptly. Cheap;
    // safe to call every block.
    void advance (double currentBeats);

    // True only while `instrument` is both currently requested and holding a
    // pool slot - this is what should drive the arbitrated gate CC sent
    // onward to that instrument's OrchGate.
    bool isActive (Instrument instrument) const noexcept;

    // True while `instrument` is requested but has not yet been granted a
    // slot (pool full, nothing eligible to free yet).
    bool isWaiting (Instrument instrument) const noexcept;

    int numOccupiedSlots() const noexcept;

private:
    struct Slot
    {
        bool occupied = false;
        Instrument occupant = Instrument::glockenspiel;
        double assignedAtBeats = 0.0;
    };

    void settle (double currentBeats);
    bool trySeatInstrument (Instrument instrument, double currentBeats);
    int findSlotIndexFor (Instrument instrument) const noexcept;

    PoolConfig config;
    std::array<Slot, maxPoolSize> slots {};
    std::array<bool, numPoolInstruments> requestedFlags {};
};

} // namespace opmp
