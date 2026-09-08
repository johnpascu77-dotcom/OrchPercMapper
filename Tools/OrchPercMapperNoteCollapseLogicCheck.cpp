#include "OrchPercMapperNoteCollapseLogic.h"

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

using opmp::NoteHoldCollapser;

void testSingleNoteOnOff()
{
    NoteHoldCollapser collapser;

    check (collapser.noteOn(), "single note: first note-on triggers the destination");
    check (collapser.getHeldCount() == 1, "single note: held count is 1");
    check (collapser.noteOff(), "single note: matching note-off releases the destination");
    check (collapser.getHeldCount() == 0, "single note: held count back to 0");
}

void testOverlappingNotesDoNotRetrigger()
{
    NoteHoldCollapser collapser;

    check (collapser.noteOn(), "overlap: first note-on triggers");
    check (! collapser.noteOn(), "overlap: second overlapping note-on does NOT retrigger");
    check (collapser.getHeldCount() == 2, "overlap: held count is 2 after two note-ons");

    check (! collapser.noteOff(), "overlap: first note-off does NOT release (one still held)");
    check (collapser.getHeldCount() == 1, "overlap: held count is 1 after one note-off");

    check (collapser.noteOff(), "overlap: second note-off releases (last one held)");
    check (collapser.getHeldCount() == 0, "overlap: held count back to 0");
}

void testStrayNoteOffIsIgnoredNotNegative()
{
    NoteHoldCollapser collapser;

    check (! collapser.noteOff(), "stray note-off with nothing held does not trigger a release");
    check (collapser.getHeldCount() == 0, "stray note-off: held count stays at 0, never negative");

    // Confirm a subsequent real note-on still works normally afterward.
    check (collapser.noteOn(), "stray note-off: a real note-on afterward still triggers normally");
}

void testForceRelease()
{
    NoteHoldCollapser heldCase;
    heldCase.noteOn();
    heldCase.noteOn();

    check (heldCase.forceRelease(), "force release: emits a note-off when something was held");
    check (heldCase.getHeldCount() == 0, "force release: held count cleared to 0");

    NoteHoldCollapser emptyCase;
    check (! emptyCase.forceRelease(), "force release: emits nothing when nothing was held");

    // After a forced release, a fresh note-on should behave like a clean start.
    check (heldCase.noteOn(), "force release: a fresh note-on afterward triggers normally");
}

int main()
{
    testSingleNoteOnOff();
    testOverlappingNotesDoNotRetrigger();
    testStrayNoteOffIsIgnoredNotNegative();
    testForceRelease();

    if (failures > 0)
    {
        std::cerr << "\n" << failures << " check(s) failed.\n";
        return 1;
    }

    std::cout << "\nAll OrchPercMapper note-collapse checks passed.\n";
    return 0;
}
