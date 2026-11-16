#pragma once
// A2 RACK v2 - original four-knob waveshaping DSP, independent of JUCE.
// This is intentionally not based on, or derived from, any proprietary plugin DSP.
#include <algorithm>
#include <cmath>

namespace a2
{
inline float clamp (float value, float low, float high) noexcept
{
    return std::max (low, std::min (high, value));
}

inline float dbToGain (float dB) noexcept
{
    return std::pow (10.0f, dB / 20.0f);
}

// Triangular foldback at +/- threshold. The result remains in [-threshold,threshold].
// Uses fmod() so large input peaks never cause unbounded loops.
inline float foldback (float input, float threshold) noexcept
{
    threshold = std::max (0.05f, threshold);
    const float period = 4.0f * threshold;
    float position = std::fmod (input + threshold, period);
    if (position < 0.0f) position += period;
    return threshold - std::fabs (position - 2.0f * threshold);
}

inline float processSample (float input, float foldAmount, float driveAmount,
                            float slopeAmount, float outputGain) noexcept
{
    if (! std::isfinite (input)) return 0.0f;
    const float fold = clamp (foldAmount * 0.1f, 0.0f, 1.0f);
    const float drive = clamp (driveAmount * 0.1f, 0.0f, 1.0f);
    const float hardness = clamp (slopeAmount * 0.1f, 0.0f, 1.0f);

    // DRIVE: 0..10 -> 0..30 dB before clipping.
    float signal = input * dbToGain (drive * 30.0f);

    // W-FOLD: lower fold threshold and blend in the reflected harmonic signal.
    // 0 = clean, 10 = full wavefolding. Normalise the reflected signal so quiet
    // sources don't lose their entire volume when fold amount increases.
    if (fold > 0.0001f)
    {
        const float threshold = 1.0f - 0.85f * fold;
        const float folded = foldback (signal, threshold) / threshold;
        signal = signal + (folded - signal) * fold;
    }

    // SLOPE: morph soft-saturation into a hard limiter with no hidden gain.
    const float soft = std::tanh (signal * 1.6f) / std::tanh (1.6f);
    const float hard = clamp (signal, -1.0f, 1.0f);
    float shaped = soft + (hard - soft) * hardness;
    shaped = clamp (shaped, -1.0f, 1.0f);
    shaped *= outputGain;
    return std::isfinite (shaped) ? shaped : 0.0f;
}
}
