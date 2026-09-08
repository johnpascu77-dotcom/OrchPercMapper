# OrchPercMapper — Design

Date: 2026-09-08
Status: **Phase 0 (skeleton) + both pure-logic pieces built and tested, not
yet wired into processBlock.** Design fork in §5 resolved as **(B)**.
`OrchPercMapperPoolLogic` (Layer-2 allocator, 21 assertions green) and
`OrchPercMapperNoteLogic` (confirmed Iconica destination-note table, 40+
assertions green) both exist as pure logic and are linked into the plugin
target, but nothing in `OrchPercMapperProcessor::processBlock` calls either
one yet — it is still pure MIDI pass-through pending the next phase (wiring
processBlock, plus designing the note→pool-request bridge and the
cross-instance CC-arbitration transport for Layer 2 - see §4/§6).
Repo: `C:\AudioDev\Repos\OrchPercMapper` (public).
GitHub `johnpascu77-dotcom/OrchPercMapper` — created and pushed.
Plugin code: `Opmp`.
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
**Confirmed 2026-09-08** against a full screenshot of the real map, with the
user confirming the octave-naming convention (Bitwig: C3 = MIDI 60). Exact
destination notes, implemented in `OrchPercMapperNoteLogic.h/.cpp`
(`getUnpitchedHitDestinationNote` / `getUnpitchedRollOrAlternateDestinationNote`),
14 confirmed unique across all 7 instruments (verified by
`OrchPercMapperNoteLogicCheck`):

| Instrument | Hit (default) | Roll / alternate |
|---|---|---|
| Bass Drum | Gran Cassa Hit — B0 = **35** | Gran Cassa Roll — A#0 = **34** |
| Snare Drum | Snare 1 Hit — C1 = **36** | Snare 1 Roll — C#1 = **37** |
| Cymbals | Cymb 18" Hit — A#1 = **46** | Cymb 18" Roll — C2 = **48** |
| Piatti | Piatti Med Hit — F#1 = **42** | Piatti Large Hit — G#1 = **44** (no Roll voice) |
| Tam-Tam | Tam Tam Hit — D#2 = **51** | Tam Tam Roll — E2 = **52** |
| Tambourine | Tambourine Hit — F2 = **53** | Tambourine Roll — F#2 = **54** |
| Triangle | Triangle Hit — G2 = **55** | Triangle Roll — G#2 = **56** |

Snare Drum (of 3 voices: 1/2/3) and Cymbals (of 2 sizes: 18"/20") each had
more than one candidate in the map for "the" primary Hit; Snare 1 and 18"
were picked as defaults — swap freely, they're just as re-pointable as
everything else here. Only the Hit variant is wired into
`getUnpitchedHitDestinationNote()` and used anywhere yet; the Roll/alternate
note is recorded and tested but not yet consumed by any selection logic (see
below).

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

**Real open problem surfaced while building the pool logic (§6): this can't
be "one plugin instance doing two jobs."** The 7 unpitched instruments each
need their own OrchPercMapper instance on their own track (note mapping is
inherently per-track, same as OrchNoteMapper today). But the pool allocator
needs ONE shared view of all 12 instruments' state - a `PoolAllocator` living
inside 7 separate, independent plugin instances can't see each other's
requests. This needs either a single dedicated "arbiter" instance that all
12 instruments' gate CCs route through (matching OrchConductor's own
single-global-instance role), an OrchMerge-style Hub/Sender IPC link between
instances, or something else - not yet decided. See §6.

## 5. Instrument-track-level questions (settled)

- **Plugin type**: MIDI effect (`IS_MIDI_EFFECT TRUE`), matching
  OrchNoteMapper/OrchHarp - built into the Phase 0 skeleton.
- **Plugin short code**: `Opmp`, CMake/JUCE scaffolding built (mirrors
  OrchHarp's CMakeLists.txt structure).
- GitHub repo created and pushed: `johnpascu77-dotcom/OrchPercMapper`.

## 6. Not yet decided / not yet started

- **The cross-instance pool-state problem from §4** - the biggest remaining
  open question, and harder than anything solved so far. `PoolAllocator`
  itself (Source/OrchPercMapperPoolLogic.h/.cpp) is transport-agnostic pure
  logic; it doesn't yet know or care how its `setRequested()`/`advance()`
  calls would actually reach it across plugin instances. Needs its own
  dedicated design pass, likely modelled on OrchMerge's Hub/Sender pattern -
  worth reading OrchMerge's known history first (Hub Margin timing, the
  reopened stuck-note/phantom-note-on bug) before assuming that pattern
  transfers cleanly.
- Exact hold-time default value (`PoolConfig::minHoldBeats`, currently a
  placeholder 4.0 beats) - not tuned against any real material yet.
- Variant selection (Hit vs. Roll/alternate) based on the actual incoming
  performance - `getUnpitchedRollOrAlternateDestinationNote()` exists and is
  tested, but nothing calls it; every incoming note currently would map to
  the Hit variant only, once processBlock is wired up.
- `processBlock` itself does nothing yet beyond pass-through - neither piece
  of pure logic built so far is called from the real MIDI path.
