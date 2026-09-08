# OrchPercMapper — Design

Date: 2026-09-08
Status: **Both roles now wired for real - nothing left in processBlock is
pure pass-through.** `OrchPercMapperPoolLogic` (21 assertions),
`OrchPercMapperNoteLogic` (40+ assertions), `OrchPercMapperCcMap` (6
assertions), and the new `OrchPercMapperNoteCollapseLogic` (18 assertions)
are all pure logic, tested, and wired into `processBlock`. The `Role`
parameter (Note Mapper / Arbiter, default Note Mapper) picks between: the
**Arbiter** running the pool allocator for real (§7), or **NoteMapper**
rewriting every incoming note onto its instrument's confirmed Iconica
destination key via a many-to-one hold-count collapse, gated by a new
"Instrument" parameter (§8). Full VST3 build clean, no warnings.
Repo: `C:\AudioDev\Repos\OrchPercMapper` (public).
GitHub `johnpascu77-dotcom/OrchPercMapper` — created and pushed.
Plugin code: `Opmp`.

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

**Real problem surfaced while building the pool logic, now resolved (§7):**
the 7 unpitched instruments each need their own OrchPercMapper instance on
their own track (note mapping is inherently per-track, same as OrchNoteMapper
today), but the pool allocator needs ONE shared view of all 12 instruments'
state - a `PoolAllocator` living inside 7 separate, independent plugin
instances can't see each other's requests. Initially this looked like it
might need an OrchMerge-style Hub/Sender IPC link between instances; it
doesn't - see §7 for why, and for the actual (much simpler) resolution: a
`Role` parameter and one shared Arbiter instance using ordinary Bitwig
routing, no custom transport at all.

## 5. Instrument-track-level questions (settled)

- **Plugin type**: MIDI effect (`IS_MIDI_EFFECT TRUE`), matching
  OrchNoteMapper/OrchHarp.
- **Plugin short code**: `Opmp`.
- GitHub repo created and pushed: `johnpascu77-dotcom/OrchPercMapper`.

## 6. Deliberately not yet solved by OrchMerge's Hub/Sender pattern

Before concluding §7, it's worth recording why that pattern - this
ecosystem's own proven answer to "N tracks' MIDI needs to reach one place" -
doesn't actually apply here, since it was the first instinct:

OrchMerge needed a custom socket transport (`OrchMergeLink`, Sender/Hub
roles, a ppq-aware emit buffer, careful note-off lifecycle tracking keyed by
`(senderUid, channel, note)`) because its problem is **fundamentally a
many-to-one, dense, timing-critical MERGE**: up to 25 tracks' overlapping
note streams, at audio-block rate, where losing simultaneity or misrouting a
note-off is a real musical correctness bug (see OrchMerge_Design.md §3-4,
including a real, still-only-partially-resolved "phantom note-on from an
idle Sender" bug and a "held notes never released" transport-stop bug found
live). None of that applies here: the pool allocator's inputs are 12 sparse,
low-frequency CC *state changes* (an instrument's eligibility flips on a
preset/combi change, not every block), with no note-off lifecycle and no
ppq-precision timing requirement at all. Reaching for the same heavyweight
machinery for a much simpler problem would have imported all of OrchMerge's
real hard-won complexity (and its still-open bugs) for no benefit.

## 7. The actual resolution: a Role parameter + ordinary Bitwig routing

**No custom IPC needed.** This rig already has a documented, *proven* pattern
for exactly this shape of problem - one shared source's output needing to
reach several downstream tracks merged with each track's own content -
because it's the same problem OrchConductor's own CC output already solves
today for every instrument. From `OrchConductor/Docs/OrchConductor_MC_Integration_And_Narrative_Scan_Design.md`
§12 (real, live-tested Bitwig lessons, not speculation):

> Note Receiver needs an empty second Note-FX layer - a lone Note Receiver
> *replaces* the track input; add an empty Layer 2 and MPL notes + OC's CC
> merge. This is the fan-out mechanism.
>
> Stale track taps... motivates a section MIDI-bus topology (3 bus tracks tap
> OC, instrument tracks tap their bus).

That is: Bitwig's **Note Receiver** device (in a second Note FX layer, so it
merges with rather than replaces the track's own input) is the ecosystem's
already-working answer to "many tracks need to read one shared source," and
a **bus-track topology** (an intermediate track taps the shared source once;
downstream tracks tap the bus instead of the source directly) is already the
established pattern for inserting exactly this kind of intermediary. A
Percussion Arbiter bus track is a direct extension of a pattern already in
production, not a new idea.

**Resolved design:**

- `OrchPercMapperAudioProcessor::Role` (APVTS `AudioParameterChoice`,
  "Note Mapper" / "Arbiter", **default Note Mapper** - same "the safe
  default never binds a shared resource" convention as OrchMerge's own
  Sender-default role param).
- **One Arbiter instance**, on a new "Percussion Pool" bus track that taps
  OrchConductor directly (Note Receiver + empty Layer 2, per the pattern
  above). It owns the one `PoolAllocator` for all 12 non-Timpani percussion
  instruments.
- The 12 relevant OrchGate instances, and the 7 unpitched instruments'
  NoteMapper-role tracks, re-point their own Note Receiver from OrchConductor
  to the Percussion Pool bus track instead - same mechanism, one hop further
  downstream. Every other instrument's routing (woodwinds, brass, strings,
  Harp, Piano, Timpani) is untouched.
- **CC↔instrument mapping** (`OrchPercMapperCcMap.h/.cpp`): the 5 mallets'
  CCs are real and already live in OrchConductor (Glockenspiel 44, Xylophone
  45, Marimba 46, Vibraphone 47, Tubular Bells 48). The 7 unpitched
  instruments' CCs are a **proposal** (56-62), since OrchConductor's
  percussion section doesn't have rows for them yet (separate, not-yet-done
  work - extending that section from 6 to 13 rows, per §3). Hardcoded for
  now, not a UI-adjustable parameter; a small code change if OrchConductor's
  eventual real assignment differs.
- **`processArbiterBlock`**: for each incoming CC matching one of the 12 pool
  CCs, treat it as `PoolAllocator::setRequested()` and consume it (not
  forwarded raw); everything else (every other CC, all notes) passes through
  completely unchanged. Emits the arbitrated gate CC for all 12 instruments
  **only on change**, not every block - a resend-on-every-tick CC stream
  would be needless spam; OrchConductor itself only sends on an explicit
  preset/combi change, and the Arbiter follows the same convention.
  `PoolAllocator::advance()` runs every block regardless of CC input, so an
  instrument waiting purely on hold-time expiry gets granted promptly rather
  than only on the next incoming request.

**Verified**: `OrchPercMapperCcMapCheck` (6 assertions: all 12 CCs unique,
round-trip correctly, Timpani/unrelated CCs correctly excluded) and the full
`OrchPercMapper_VST3` build, both green. The Arbiter's `processBlock` path
itself doesn't yet have a JUCE-linked integration test (mirroring
OrchConductor's own `ProcessorMidiRegressionCheck`) - worth adding once this
is closer to a real Bitwig smoke test, but its only real logic
(`PoolAllocator`, `CcMap`) is already covered in isolation, and the block
routing around them is thin, mechanical composition.

**Remaining rig work (not code):**

- Actually build the "Percussion Pool" bus track + Note Receiver rewiring in
  Bitwig, and confirm the Arbiter's arbitrated CC actually reaches a real
  OrchGate instance.
- Extend OrchConductor's percussion section to 13 rows so the 7 unpitched
  instruments' CCs (56-62) actually exist upstream (currently only proposed
  in this repo's `CcMap`, not real anywhere yet).

## 8. NoteMapper role: wired

One instance per unpitched-instrument track, same as OrchNoteMapper's own
per-instrument pattern. A new **"Instrument"** APVTS choice parameter (the 7
unpitched instruments, `OrchPercMapperAudioProcessor::getUnpitchedInstrumentChoices()`)
picks which one this instance represents.

**Deliberately does not gate participation itself.** The Arbiter's
arbitrated CC for this instrument reaches its downstream OrchGate instance
directly (via the same Note Receiver mechanism, §7) - OrchGate already owns
every gating concern (hard gate, participation amount, safe note-off,
keyswitch passthrough) for every instrument in this ecosystem, and
duplicating any of that here would just be two sources of truth for the same
decision. NoteMapper's only job is note *identity*.

**The real new problem here: many incoming pitches collapse onto one
destination note.** Every note this instance sees maps to the *same* fixed
Iconica key (`OrchPercMapperNoteLogic::getUnpitchedHitDestinationNote()`),
so a plain per-message note-on/note-off passthrough would be wrong: two
overlapping incoming notes of different original pitches would incorrectly
retrigger the destination, and a note-off for whichever one happens to
release first would incorrectly cut off a still-sounding destination note
while the other incoming note is still held. `OrchPercMapperNoteCollapseLogic.h`
(`NoteHoldCollapser`, pure, header-only) tracks a hold count instead: a
note-on only actually triggers the destination when the count goes 0→1, a
note-off only actually releases it when the count returns to 0, and a stray
unmatched note-off is safely ignored rather than going negative.

Also carries the same "held notes never released" safety net OrchMerge
needed for a related reason (its Sender/Hub transport-stop bug, §6): on the
host's playing→stopped edge, force-release whatever's held and emit an
explicit note-off, so a note truly held at the exact instant of Stop can't
leave the destination stuck sounding into the next take.

**Verified**: `OrchPercMapperNoteCollapseLogicCheck` (18 assertions - single
note-on/off, overlapping notes not retriggering, a stray note-off never
going negative, force-release with and without something actually held).
Full VST3 build clean, no warnings.

**Not yet done:**

- Exact hold-time default value (`PoolConfig::minHoldBeats`, currently a
  placeholder 4.0 beats, Arbiter role) - not tuned against any real material
  yet; not yet a UI-adjustable parameter either.
- Variant selection (Hit vs. Roll/alternate) based on the actual incoming
  performance - `getUnpitchedRollOrAlternateDestinationNote()` exists and is
  tested, but nothing calls it yet; every note currently maps to the Hit
  variant only.
- No real Bitwig smoke test yet for either role - everything so far is
  pure-logic-level and clean-build verification, not live-confirmed.
