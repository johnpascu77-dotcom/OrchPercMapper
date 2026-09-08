# OrchPercMapper — Design

Date: 2026-09-08
Status: **Concept only — nothing built yet.** This doc captures the conceptual
discussion that closed before any code; the open question in §5 needs an
answer before Phase 1 scaffolding starts.
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

## 4. Chain position (note path, unpitched instruments)

```
Shared motive/MIDI source → OrchNoteFilter → OrchPercMapper → OrchGate → Instrument
```

Same slot OrchNoteMapper occupies for every other instrument — this is a
drop-in replacement for the note-identity step, not a new stage.

## 5. Open question: where does Layer 2 actually sit?

This is the one real design fork left, and it needs an answer before Phase 1
starts, because it changes the plugin's I/O shape.

The note-identity mapping (§2) only ever sees the 7 unpitched instruments'
stream — the 5 pitched mallets keep flowing through OrchNoteMapper as already
built. But the player-pool allocator (§3) is scoped across all **12**
instruments (mallets included). OrchPercMapper can't arbitrate a pool that
includes instruments whose notes never pass through it.

Two ways to resolve this:

- **(A) Split responsibilities.** OrchPercMapper only ever handles the 7
  unpitched instruments — both mapping *and* their share of the pool. The 5
  pitched mallets keep using OrchGate's plain per-instrument gate, ungated by
  the shared pool. Simpler to build, but doesn't actually deliver "any of the
  3-4 players can cover any of the 12 instruments" — mallets would be exempt
  from the realism constraint the whole discussion was about.
- **(B) OrchPercMapper arbitrates all 12 at the control-CC level.** It sits
  between OrchConductor and all 12 OrchGate instances (5 mallet + 7 unpitched)
  as a CC-only pass-through/arbiter — reading each instrument's Layer-1
  eligibility CC and re-emitting an arbitrated, pool-respecting gate CC to
  each of the 12 OrchGates — while separately still doing note-identity
  mapping only for the 7 unpitched instruments in the note path. This
  actually delivers the full 12-instrument pool but means the plugin has two
  distinct jobs wired through two different signal paths (note stream for 7,
  control CCs for 12), which is more surface area to build and reason about.

(B) is the only one that matches what was actually asked for; (A) is the
smaller build if the mallet instruments turning out ungated in practice isn't
a real problem. Needs the user's call before Phase 1 scaffolding.

## 6. Not yet decided / not yet started

- Plugin type (MIDI effect vs. instrument-with-MIDI-out) — likely MIDI effect,
  matching OrchNoteMapper/OrchHarp, but not confirmed.
- Exact hold-time default value.
- Plugin short code (Opmp or similar) and CMake/JUCE scaffolding.
- GitHub repo creation/publish — ask before making it public.
