#pragma once

#include "OrchPercMapperPoolLogic.h"

// CC <-> Instrument mapping for the Arbiter role (Design doc §4/§7).
//
// The 5 mallets' CCs are real and already live in OrchConductor (confirmed
// from OrchConductorProcessor.cpp / the factory library JSON): Glockenspiel
// 44, Xylophone 45, Marimba 46, Vibraphone 47, Tubular Bells 48 (Timpani 43
// is deliberately absent here - excluded from the pool by design).
//
// The 7 unpitched instruments' CCs are a PROPOSAL, not yet built in
// OrchConductor - that plugin's percussion section still only has 6 rows
// (Timpani + 5 mallets); extending it to 13 rows to add these is separate,
// not-yet-done work. CC56-62 is chosen because CC20-55 is already fully
// occupied (35 orchestral instruments + Harp/Piano) and CC56-64 is the
// remaining part of OrchConductor's own documented "MPL collision zone"
// (20-64) that its input-passthrough logic already treats as reserved/
// blocked - safe to extend into without touching that logic.
//
// Deliberately a hardcoded default for now, not a UI-adjustable parameter -
// re-taught by hand (a small code change) if OrchConductor's actual
// assignment ends up different once that side is built, matching this
// family's usual "opinionated default, adjustable later" pattern.
namespace opmp
{

// -1 for Instrument::count or any instrument this map doesn't cover.
int getPoolGateCcNumber (Instrument instrument) noexcept;

// Instrument::count if ccNumber isn't one of the 12 pool CCs.
Instrument getInstrumentForPoolGateCc (int ccNumber) noexcept;

} // namespace opmp
