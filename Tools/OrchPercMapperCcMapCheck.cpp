#include "OrchPercMapperCcMap.h"

#include <iostream>
#include <set>
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

int main()
{
    std::set<int> seenCcs;
    bool allUnique = true;
    bool allRoundTrip = true;

    for (int i = 0; i < opmp::numPoolInstruments; ++i)
    {
        const auto instrument = static_cast<Instrument> (i);
        const int cc = opmp::getPoolGateCcNumber (instrument);

        if (cc < 0 || cc > 127)
            allUnique = false; // also catches an unmapped entry, which would be a real bug here

        if (! seenCcs.insert (cc).second)
            allUnique = false;

        if (opmp::getInstrumentForPoolGateCc (cc) != instrument)
            allRoundTrip = false;
    }

    check (allUnique, "all 12 pool instruments have a unique, valid CC number");
    check (static_cast<int> (seenCcs.size()) == opmp::numPoolInstruments, "exactly 12 distinct CC numbers recorded");
    check (allRoundTrip, "every instrument's CC maps back to that same instrument");

    check (opmp::getPoolGateCcNumber (Instrument::count) == -1, "Instrument::count has no CC");
    check (opmp::getInstrumentForPoolGateCc (20) == Instrument::count, "an unrelated CC (20, Piccolo) is not a pool CC");
    check (opmp::getInstrumentForPoolGateCc (43) == Instrument::count, "Timpani's CC (43) is not a pool CC - excluded by design");

    if (failures > 0)
    {
        std::cerr << "\n" << failures << " check(s) failed.\n";
        return 1;
    }

    std::cout << "\nAll OrchPercMapper CC-map checks passed.\n";
    return 0;
}
