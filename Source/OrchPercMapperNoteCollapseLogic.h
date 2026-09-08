#pragma once

// Every incoming pitch for a given unpitched instrument collapses onto ONE
// fixed destination note (Design doc §2/§8) - a NoteMapper-role instance
// doesn't preserve pitch identity, it just decides *whether the destination
// note should currently be sounding*. That needs a hold count, not a plain
// note-on -> note-off passthrough: if two overlapping incoming notes (of
// whatever original pitch) both map to the same destination, the second
// note-on shouldn't retrigger it, and the destination should only actually
// release once every incoming note that's holding it has been released.
//
// No JUCE dependency - pure, real-time safe, independently testable.
namespace opmp
{

class NoteHoldCollapser
{
public:
    // Call on every incoming note-on (regardless of its original pitch -
    // they all map to the same destination). Returns true only when the
    // destination should actually receive a note-on (the count went from 0
    // to 1); false means the destination is already sounding and this
    // incoming note-on should be counted but not re-triggered.
    bool noteOn() noexcept
    {
        ++heldCount;
        return heldCount == 1;
    }

    // Call on every incoming note-off. Returns true only when the
    // destination should actually receive a note-off (the count reached 0);
    // false means other incoming notes are still holding it. Never lets the
    // count go negative - an unmatched/stray note-off is simply ignored
    // rather than corrupting later counting.
    bool noteOff() noexcept
    {
        if (heldCount <= 0)
            return false;

        --heldCount;
        return heldCount == 0;
    }

    // Unconditionally clears the hold state (e.g. on a transport
    // playing->stopped edge, mirroring OrchMerge's own releaseAllHeld()
    // fix for the same class of "held notes never released" bug). Returns
    // true if a note-off should be emitted for the destination (it was
    // actually held).
    bool forceRelease() noexcept
    {
        const bool wasHeld = heldCount > 0;
        heldCount = 0;
        return wasHeld;
    }

    int getHeldCount() const noexcept { return heldCount; }

private:
    int heldCount = 0;
};

} // namespace opmp
