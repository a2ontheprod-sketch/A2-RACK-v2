#include "../Source/ShapeDSP.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    assert (a2::foldback (0.0f, 1.0f) == 0.0f);
    assert (std::abs (a2::foldback (1.0f, 1.0f) - 1.0f) < 0.00001f);
    assert (std::abs (a2::foldback (2.0f, 1.0f)) < 0.00001f);
    assert (a2::processSample (0.0f, 10, 10, 10, 1) == 0.0f);
    assert (a2::processSample (NAN, 0, 0, 0, 1) == 0.0f);
    for (float fold : { 0.0f, 2.5f, 5.0f, 10.0f })
        for (float drive : { 0.0f, 3.0f, 7.0f, 10.0f })
            for (float slope : { 0.0f, 5.0f, 10.0f })
                for (int i = -10000; i <= 10000; ++i)
                {
                    const auto input = i / 2500.0f;
                    const auto output = a2::processSample (input, fold, drive, slope, 1.0f);
                    assert (std::isfinite (output));
                    assert (output >= -1.00001f && output <= 1.00001f);
                }
    std::cout << "A2 DSP sanity tests passed.\n";
}
