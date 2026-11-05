// Awk BK - pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PitchDetector.h"
#include "NoteUtils.h"

#include <algorithm>
#include <cmath>

namespace awk
{

// YIN fundamental-frequency estimator:
// de Cheveigné, A. and Kawahara, H. (2002). YIN, a fundamental frequency
// estimator for speech and music. The Journal of the Acoustical Society of
// America, 111(4), 1917–1930.

void PitchDetector::prepare (int maxSamples)
{
    capacity = std::max (64, maxSamples);
    const int tauMax = capacity / 2;
    diff.assign ((size_t) tauMax + 1, 0.0f);
    cmnd.assign ((size_t) tauMax + 1, 1.0f);
}

PitchDetector::Result PitchDetector::detect (const float* samples, int numSamples,
                                             double sampleRate, float yinThreshold,
                                             float rmsGate)
{
    Result result;
    if (samples == nullptr || numSamples < 64 || sampleRate < 1000.0)
        return result;

    if (numSamples > capacity)
        prepare (numSamples);

    const int tauLimit = numSamples / 2;
    double energy = 0.0;
    for (int i = 0; i < numSamples; ++i)
        energy += (double) samples[i] * (double) samples[i];

    const float rms = (float) std::sqrt (energy / (double) numSamples);
    if (rms < rmsGate)
        return result;

    // Fixed-window difference function. W = tauLimit, buffer length = 2W.
    diff[0] = 1.0f;
    for (int tau = 1; tau < tauLimit; ++tau)
    {
        double sum = 0.0;
        for (int i = 0; i < tauLimit; ++i)
        {
            const double d = (double) samples[i] - (double) samples[i + tau];
            sum += d * d;
        }
        diff[(size_t) tau] = (float) sum;
    }

    cmnd[0] = 1.0f;
    double running = 0.0;
    for (int tau = 1; tau < tauLimit; ++tau)
    {
        running += diff[(size_t) tau];
        cmnd[(size_t) tau] = running > 1.0e-12
                                 ? (float) (diff[(size_t) tau] * (double) tau / running)
                                 : 1.0f;
    }

    const int tauMin = std::clamp ((int) std::lround (sampleRate / 2000.0), 2, tauLimit - 2);
    const int tauMax = std::clamp ((int) std::lround (sampleRate / 40.0), tauMin + 1, tauLimit - 2);

    const float threshold = std::clamp (yinThreshold, 0.01f, 0.5f);
    int best = -1;
    for (int tau = tauMin; tau <= tauMax; ++tau)
    {
        if (cmnd[(size_t) tau] < threshold)
        {
            while (tau + 1 <= tauMax && cmnd[(size_t) (tau + 1)] < cmnd[(size_t) tau])
                ++tau;
            best = tau;
            break;
        }
    }

    if (best < 0)
        return result;

    const float y0 = cmnd[(size_t) (best - 1)];
    const float y1 = cmnd[(size_t) best];
    const float y2 = cmnd[(size_t) (best + 1)];
    const float denom = 2.0f * (2.0f * y1 - y2 - y0);
    float delta = 0.0f;
    if (std::fabs (denom) > 1.0e-12f)
        delta = (y2 - y0) / denom;
    delta = std::clamp (delta, -0.5f, 0.5f);

    const float period = (float) best + delta;
    if (period < 1.0f)
        return result;

    const float hz = (float) (sampleRate / (double) period);
    if (hz < 40.0f || hz > 2000.0f)
        return result;

    const NoteName note = noteFromHz (hz);
    result.frequencyHz = hz;
    result.confidence = std::clamp (1.0f - y1, 0.0f, 1.0f);
    result.voiced = true;
    result.midiNote = note.midi;
    result.cents = note.cents;
    return result;
}

} // namespace awk
