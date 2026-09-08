# OrchPercMapper — Design

Date: 2026-09-08
Status: **Concept closed, Phase 1 not started.** This doc captures the
conceptual discussion that closed before any code. The design fork in §5 is
resolved: **(B)** — OrchPercMapper arbitrates all 12 non-Timpani percussion
instruments' CC gates, in addition to mapping note identity for the 7
unpitched ones.
Repo: `C:\AudioDev\Repos\OrchPercMapper` (git initialised, no remote yet).
GitHub `johnpascu77-dotcom/OrchPercMapper` (public, like the rest of the Orch
family) — not yet created; ask before publishing.
Plugin code: TBD (3-4 letter code following the family convention — Ocpn,
Ohrp, Ocap, Omrg — something like `Opmp`).

## 1. Purpose

The percussion family is growing beyond what OrchNoteMapper and OrchGate were
designed for. Today the rig has one pitched percussion instrument (Timpani)
and OrchNoteMapper/OrchConductor are about to gain five more pitched
instruments (Glockenspiel, Xylophone, Marimba, Vibraphone, Tubular Bells) and
seven unpitched ones (Cymbals, Piatti, Bass Drum, Snare Drum, Tam-Tam,
Tambourine, Triangle).

The five new pitched instruments fit the existing architecture cleanly —
OrchNoteMapper already has full range-based instrument presets for all five
(practical sounding min/max, octave-fold), and OrchConductor's CC map already
reserves CC44-48 for them alongside Timpani's CC43. That part is mostly
wiring/testing once the real tracks exist, not new design.

The seven unpitched instruments don't fit the existing architecture, for two
independent reasons:

1. **Note identity, not note range.** OrchNoteMapper's whole model is a
   continuous playable range + octave-fold per instrument. A drum map is the
   opposite shape: one name, several fixed, musically unrelated destination
   keys (hit vs. roll vs. mute), and choosing among them is a musical decision,
   not a range-adaptation one.
2. **A real orchestra doesn't have 12 more players.** Beyond the dedicated
   timpanist, 3-4 real percussionists cover all 12 remaining instruments
   (5 pitched + 7 unpitched) by physically moving between them, and a mallet
   change takes real time. Nothing in the current chain enforces "at most
   3-4 of these 12 can be active at once, with a minimum hold time before
   reassignment" — OrchGate's per-instrument gate has no concept of a shared,
   limited pool.

OrchPercMapper exists to own both of these, for the instruments that need it.

## 2. Note-identity mapping (unpitched instruments only)

For the 7 unpitched instruments, OrchPercMapper replaces OrchNoteMapper's role
with a fixed lookup table: incoming note → destination key, no range math, no
octave-fold. Reuse the *shape* of OrchNoteMapper's existing keyswitch-remap
mechanism (`mapLowKeyswitchNoteToDestination` et al. in
`OrchNoteMapper/Source/PluginProcessor.h`) rather than its range logic — that
mechanism is already a fixed note→note lookup, which is exactly what this is.

**Default mapping: Iconica Sketch's own Percussion Map**, per the layout the
user supplied (Steinberg Iconica Sketch, "Percussion Map" keyswitch grid).
This is an explicitly opinionated, single-library default — re-pointable by
hand for any other sample library, not a claim of universal compatibility.
Confirmed instrument coverage in that map (hit-type variants in parentheses):

- Bass Drum → "Gran Cassa" (Roll, Hit)
- Snare Drum → "Snare 1/2/3" (Hit, Roll — several distinct snare voices)
- Cymbals → "Cymb 18"/20"" (Roll, Hit)
- Piatti → "Piatti" (Hi/Med Hit, Large Hit)
- Tam-Tam → "Tam Tam" (Hit, Roll)
- Tambourine → "Tambourine" (Hit, Roll)
- Triangle → "Triangle" (Hit, Mute)

The map also carries auxiliary voices outside this project's 7-instrument
scope (congas, bongos, wood blocks, bell tree, chimes, cowbell, vibraslap,
castanets, toms) — out of scope for now, available later if the instrument
list grows.

Variant selection (which of an instrument's several destination keys a given
incoming note resolves to — hit vs. roll, etc.) is new territory none of the
existing plugins handle; this is where that logic lives.

## 3. Participation: the shared player-pool allocator

Two layers, so a flat CC gate and a realistic performance constraint don't
fight each other:

- **Layer 1 — OrchConductor CC (per-instrument, coarse).** Unchanged pattern
  from every other instrument in the ecosystem: "is this instrument allowed
  to participate in this passage at all," a compositional decision. This
  extends OrchConductor's existing percussion section array (currently 6
  rows: Timpani + 5 mallets) to 13 rows to add coarse CCs for the 7 unpitched
  instruments — a clean fit, since (unlike Harp/Piano) percussion is already
  a proper array-driven section with no structural gap.
- **Layer 2 — OrchPercMapper's shared-pool allocator.** Among whatever is
  currently Layer-1-enabled, enforce a hard cap of *P* simultaneously-active
  instruments, with a minimum hold time per assignment so a player who just
  picked up mallets for Marimba can't be reassigned to Triangle one beat
  later. A newly-requested instrument that would exceed the cap waits until a
  player's hold-time expires and frees up, rather than pre-empting mid-phrase.

Settled parameters (confirmed in discussion):

- **Pool scope: 12 instruments** — the 5 pitched mallets (Glockenspiel,
  Xylophone, Marimba, Vibraphone, Tubular Bells) plus the 7 unpitched
  instruments. **Timpani is excluded** — its own dedicated player, never
  competes for a pool slot.
- **Assignment model: fully flexible.** Any of the 3-4 players can cover any
  of the 12 instruments interchangeably (per the user's real-world
  performance-practice experience) — no fixed "mallet specialist vs. unpitched
  specialist" split.
- **Pool size** *P*: 3-4, exposed as a plugin parameter rather than hardcoded
  (real ensembles vary).
- **Hold time**: one default constant to start (exact value TBD — something
  in the "at least a bar" range is the working assumption), adjustable later
  per instrument pair if real use shows the single constant doesn't hold up.
  Not scoped further until it's actually in front of real material.

## 4. Chain position — two signal paths, one plugin

**Resolved (2026-09-08): (B).** OrchPercMapper has two distinct jobs wired
through two different signal paths, because the note-identity mapping (§2)
only ever sees the 7 unpitched instruments, but the player-pool allocator
(§3) is scoped across all 12 non-Timpani instruments (mallets included) —
option (A), scoping the pool to only the 7 unpitched instruments, would have
left the 5 mallets ungated and not actually delivered "any of the 3-4 players
can cover any of the 12 instruments."

**Note path (7 unpitched instruments only)** — drop-in replacement for
OrchNoteMapper's role, same slot:

```
Shared motive/MIDI source → OrchNoteFilter → OrchPercMapper (note map) → OrchGate → Instrument
```

**Control path (all 12 non-Timpani instruments)** — OrchPercMapper sits
between OrchConductor and all 12 OrchGate instances (5 mallet + 7 unpitched)
as a CC arbiter: it reads each instrument's Layer-1 eligibility CC coming
from OrchConductor and re-emits an arbitrated, pool-respecting gate CC to
each of the 12 OrchGates. The 5 mallet instruments' *notes* never pass
through OrchPercMapper (they keep flowing through OrchNoteMapper as already
built) — only their gate CC does, for arbitration purposes:

```
OrchConductor --CC (12 instruments)--> OrchPercMapper (pool arbiter) --arbitrated CC--> OrchGate x12 --> Instruments
```

Implication for Phase 1 scaffolding: this is not a single MIDI-in/MIDI-out
effect like OrchNoteMapper/OrchHarp. It needs a note-stream I/O pair for the
7 unpitched instruments' mapping *and* a CC-level pass-through/arbitration
path that also reaches the 5 mallet instruments' OrchGate CCs — two jobs, not
one, sharing the same plugin instance's internal pool state.

## 5. Not yet decided / not yet started

- Plugin type (MIDI effect vs. instrument-with-MIDI-out) — likely MIDI effect,
  matching OrchNoteMapper/OrchHarp, but not confirmed.
- Exact hold-time default value.
- Plugin short code (Opmp or similar) and CMake/JUCE scaffolding.
- GitHub repo creation/publish — ask before making it public.
