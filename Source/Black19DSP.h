#pragma once
#include <array>
#include <cmath>
#include <algorithm>

namespace bbk {

// 19-tap Type-I linear-phase FIR, optimized minimum-peak-sidelobe design for
// Fs = 192 kHz, passband 0-20 kHz, stopband 76-96 kHz.
// IDENTICAL coefficients to the originally supplied JUCE project - unmodified.
static constexpr int kNumTaps = 19;
static constexpr int kGroupDelay = 9; // (19-1)/2

static constexpr std::array<double, kNumTaps> kBlack19Taps {
    -9.34962125e-05,
    -1.20196379e-03,
    -2.25009410e-03,
     6.05204782e-03,
     1.67181305e-02,
    -1.55743260e-02,
    -6.75386164e-02,
     2.60820961e-02,
     3.03136636e-01,
     4.69239072e-01,
     3.03136636e-01,
     2.60820961e-02,
    -6.75386164e-02,
    -1.55743260e-02,
     1.67181305e-02,
     6.05204782e-03,
    -2.25009410e-03,
    -1.20196379e-03,
    -9.34962125e-05
};

struct ChannelState
{
    std::array<double, kNumTaps> history {};
    int writeIndex = 0;

    void clear()
    {
        history.fill (0.0);
        writeIndex = 0;
    }
};

// Linear ramp smoother, mirrors juce::SmoothedValue<Linear> behaviour used in
// the original JUCE version: reset(sr, seconds) then setTargetValue/getNextValue.
struct LinearSmoother
{
    double current = 0.0;
    double target = 0.0;
    double increment = 0.0;
    int stepsRemaining = 0;

    void reset (double sampleRate, double timeSeconds)
    {
        rampLengthSamples = std::max (1, (int) std::round (sampleRate * timeSeconds));
    }

    void setCurrentAndTargetValue (double v)
    {
        current = target = v;
        stepsRemaining = 0;
    }

    void setTargetValue (double v)
    {
        if (v == target)
            return;
        target = v;
        stepsRemaining = rampLengthSamples;
        increment = (target - current) / (double) stepsRemaining;
    }

    double getNextValue()
    {
        if (stepsRemaining <= 0)
        {
            current = target;
            return current;
        }
        --stepsRemaining;
        current += increment;
        if (stepsRemaining == 0)
            current = target;
        return current;
    }

    int rampLengthSamples = 1;
};

// One convolution + crossfade step, in double precision throughout, matching
// the originally supplied and validated JUCE DSP exactly.
inline double processSample (ChannelState& state, double x, double mix)
{
    state.history[(size_t) state.writeIndex] = x;

    double wet = 0.0;
    for (int k = 0; k < kNumTaps; ++k)
    {
        int idx = state.writeIndex - k;
        if (idx < 0) idx += kNumTaps;
        wet += kBlack19Taps[(size_t) k] * state.history[(size_t) idx];
    }

    int dryIndex = state.writeIndex - kGroupDelay;
    if (dryIndex < 0) dryIndex += kNumTaps;
    const double dryAligned = state.history[(size_t) dryIndex];

    const double y = dryAligned + mix * (wet - dryAligned);

    ++state.writeIndex;
    if (state.writeIndex == kNumTaps) state.writeIndex = 0;

    return y;
}

} // namespace bbk
