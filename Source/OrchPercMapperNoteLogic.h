#pragma once

#include "OrchPercMapperPoolLogic.h"

// Default note-identity mapping for the 7 unpitched percussion instruments
// (Design doc §2). No JUCE dependency - deterministic, real-time safe,
// independently testable.
//
// Transcribed from Steinberg Iconica Sketch's own "Percussion Map" and
// confirmed against Bitwig's octave-naming convention (C3 = MIDI 60,
// pitch class offsets C=0 .. B=11: MIDI = 60 + (octave-3)*12 + pitchClass).
// This is an explicitly opinionated, single-library default - re-point by
// hand for any other sample library, not a claim of universal correctness.
//
// Each instrument maps to ONE primary "Hit" destination note for now.
// Choosing among an instrument's other voices (Roll, an alternate size,
// Mute) based on the actual incoming performance is real, not-yet-built
// logic (it would need duration/onset analysis, not just note identity -
// see Design doc §2) - getRollOrAlternateDestinationNote() exposes the
// confirmed secondary-variant note for whenever that selection logic gets
// built, but nothing calls it yet.
namespace opmp
{

// True for the 7 unpitched instruments this table covers; false for the 5
// pitched mallets (their mapping lives in OrchNoteMapper, not here) and for
// Instrument::count.
bool isUnpitchedInstrument (Instrument instrument) noexcept;

// Primary "Hit" destination MIDI note (0-127), or -1 for a mallet
// instrument / Instrument::count (out of scope for this table).
//
//   Bass Drum  -> Gran Cassa Hit    (B0  = 35)
//   Snare Drum -> Snare 1 Hit       (C1  = 36)  [of 3 snare voices in the
//                                                map; 1 chosen as default]
//   Cymbals    -> Cymb 18" Hit      (A#1 = 46)  [of 2 sizes in the map; 18"
//                                                chosen as default]
//   Piatti     -> Piatti Med Hit    (F#1 = 42)  [of 2 sizes in the map; Med
//                                                chosen as default]
//   Tam-Tam    -> Tam Tam Hit       (D#2 = 51)
//   Tambourine -> Tambourine Hit    (F2  = 53)
//   Triangle   -> Triangle Hit      (G2  = 55)
int getUnpitchedHitDestinationNote (Instrument instrument) noexcept;

// The confirmed secondary-variant note for the same instrument: Roll for
// everything except Piatti, which has no Roll in the map (Large Hit
// instead). -1 for a mallet instrument / Instrument::count.
//
//   Bass Drum  -> Gran Cassa Roll   (A#0 = 34)
//   Snare Drum -> Snare 1 Roll      (C#1 = 37)
//   Cymbals    -> Cymb 18" Roll     (C2  = 48)
//   Piatti     -> Piatti Large Hit  (G#1 = 44)  [no Roll voice for Piatti]
//   Tam-Tam    -> Tam Tam Roll      (E2  = 52)
//   Tambourine -> Tambourine Roll   (F#2 = 54)
//   Triangle   -> Triangle Roll     (G#2 = 56)
int getUnpitchedRollOrAlternateDestinationNote (Instrument instrument) noexcept;

} // namespace opmp
