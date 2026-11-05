// Awk BK — pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "TempoDetector.h"

#include <algorithm>
#include <cmath>

namespace awk
{

namespace
{
constexpr float kBpmMin = 60.0f;
constexpr float kBpmMax = 200.0f;
constexpr double kHopSeconds = 0.010;
}

void TempoDetector::prepare (int maxFrames)
{
    frameCapacity = std::max (64, maxFrames);
    energy.assign ((size_t) frameCapacity, 0.0f);
    onset.assign ((size_t) frameCapacity, 0.0f);
    acf.assign ((size_t) frameCapacity, 0.0f);
}

TempoDetector::Result TempoDetector::detect (const float* samples, int numSamples,
                                             double sampleRate, float sensitivity)
{
    Result result;
    if (samples == nullptr || sampleRate < 1000.0 || numSamples < (int) sampleRate)
        return result;

    const int hop = std::max (1, (int) std::lround (sampleRate * kHopSeconds));
    const int frames = numSamples / hop;
    if (frames < 48)
        return result;

    if (frames > frameCapacity)
        prepare (frames);

    for (int i = 0; i < frames; ++i)
    {
        const float* p = samples + (i * hop);
        double acc = 0.0;
        for (int k = 0; k < hop; ++k)
            acc += (double) p[k] * (double) p[k];
        energy[(size_t) i] = (float) std::sqrt (acc / (double) hop);
    }

    onset[0] = 0.0f;
    for (int i = 1; i < frames; ++i)
    {
        const float d = energy[(size_t) i] - energy[(size_t) (i - 1)];
        onset[(size_t) i] = d > 0.0f ? d : 0.0f;
    }

    // Spread each onset across a few frames so a tempo that falls between
    // hop boundaries still correlates at its true period, not only at 2x.
    float carry = 0.0f;
    constexpr float kOnsetDecay = 0.85f;
    for (int i = 0; i < frames; ++i)
    {
        carry = std::max (onset[(size_t) i], carry * kOnsetDecay);
        onset[(size_t) i] = carry;
    }

    float maxOnset = 0.0f;
    for (int i = 0; i < frames; ++i)
        maxOnset = std::max (maxOnset, onset[(size_t) i]);

    const float sens = std::clamp (sensitivity, 0.0f, 1.0f);
    const float gate = maxOnset * (0.40f * (1.0f - sens));
    double mean = 0.0;
    for (int i = 0; i < frames; ++i)
    {
        if (onset[(size_t) i] < gate)
            onset[(size_t) i] = 0.0f;
        mean += onset[(size_t) i];
    }
    mean /= (double) frames;
    for (int i = 0; i < frames; ++i)
        onset[(size_t) i] -= (float) mean;

    const double hopSec = (double) hop / sampleRate;
    int lagMin = (int) std::floor ((60.0 / (double) kBpmMax) / hopSec);
    int lagMax = (int) std::ceil ((60.0 / (double) kBpmMin) / hopSec);
    lagMin = std::clamp (lagMin, 2, frames / 2 - 2);
    lagMax = std::clamp (lagMax, lagMin + 1, frames / 2 - 1);

    for (int lag = 0; lag <= lagMax; ++lag)
        acf[(size_t) lag] = 0.0f;

    float bestVal = -1.0e9f;
    int bestLag = lagMin;
    for (int lag = lagMin; lag <= lagMax; ++lag)
    {
        double sum = 0.0;
        const int count = frames - lag;
        for (int i = 0; i < count; ++i)
            sum += (double) onset[(size_t) i] * (double) onset[(size_t) (i + lag)];
        const float value = (float) (sum / (double) count);
        acf[(size_t) lag] = value;
        if (value > bestVal)
        {
            bestVal = value;
            bestLag = lag;
        }
    }

    // If half the winning lag is almost as strong, the winner was a subharmonic
    // (half tempo). Walk toward the faster tempo while the evidence holds.
    for (int pass = 0; pass < 3; ++pass)
    {
        const int half = bestLag / 2;
        if (half < lagMin)
            break;

        int refined = half;
        const int lo = std::max (lagMin, half - 2);
        const int hi = std::min (lagMax, half + 2);
        for (int lag = lo; lag <= hi; ++lag)
            if (acf[(size_t) lag] > acf[(size_t) refined])
                refined = lag;

        if (acf[(size_t) refined] > bestVal * 0.82f)
        {
            bestLag = refined;
            bestVal = acf[(size_t) refined];
        }
        else
        {
            break;
        }
    }

    if (bestVal <= 0.0f)
        return result;

    float delta = 0.0f;
    if (bestLag > lagMin && bestLag < lagMax)
    {
        const float y0 = acf[(size_t) (bestLag - 1)];
        const float y1 = acf[(size_t) bestLag];
        const float y2 = acf[(size_t) (bestLag + 1)];
        const float denom = y0 - 2.0f * y1 + y2;
        if (std::fabs (denom) > 1.0e-9f)
            delta = 0.5f * (y0 - y2) / denom;
        delta = std::clamp (delta, -0.5f, 0.5f);
    }

    const float lagF = (float) bestLag + delta;
    const float bpm = (float) (60.0 / ((double) lagF * hopSec));
    if (bpm < kBpmMin - 1.0f || bpm > kBpmMax + 1.0f)
        return result;

    double absSum = 0.0;
    int counted = 0;
    for (int lag = lagMin; lag <= lagMax; ++lag)
    {
        absSum += std::fabs (acf[(size_t) lag]);
        ++counted;
    }
    const float meanAbs = counted > 0 ? (float) (absSum / (double) counted) : 0.0f;
    const float confidence = std::clamp ((bestVal - meanAbs) / (bestVal + 1.0e-6f), 0.0f, 1.0f);

    result.bpm = bpm;
    result.confidence = confidence;
    result.valid = confidence >= 0.20f;
    return result;
}

} // namespace awk
