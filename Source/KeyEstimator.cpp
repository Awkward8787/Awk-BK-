// Awk BK - pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "KeyEstimator.h"

#include <algorithm>
#include <cmath>

namespace awk
{

namespace
{
// Krumhansl & Kessler (1982), tonic at index 0.
constexpr float kMajor[12] = {
    6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f
};
constexpr float kMinor[12] = {
    6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f
};

float pearson (const float* hist, const float* profile, int tonic) noexcept
{
    double meanH = 0.0;
    double meanP = 0.0;
    for (int i = 0; i < 12; ++i)
    {
        meanH += hist[i];
        meanP += profile[i];
    }
    meanH /= 12.0;
    meanP /= 12.0;

    double num = 0.0;
    double denH = 0.0;
    double denP = 0.0;
    for (int pc = 0; pc < 12; ++pc)
    {
        const double h = hist[pc] - meanH;
        const double p = profile[(pc - tonic + 12) % 12] - meanP;
        num += h * p;
        denH += h * h;
        denP += p * p;
    }
    if (denH < 1.0e-12 || denP < 1.0e-12)
        return 0.0f;
    return (float) (num / std::sqrt (denH * denP));
}
} // namespace

void KeyEstimator::reset() noexcept
{
    for (float& bin : hist)
        bin = 0.0f;
    total = 0.0f;
}

void KeyEstimator::decay (float factor) noexcept
{
    const float k = std::clamp (factor, 0.0f, 1.0f);
    total = 0.0f;
    for (float& bin : hist)
    {
        bin *= k;
        total += bin;
    }
}

void KeyEstimator::add (int pitchClass, float weight) noexcept
{
    if (pitchClass < 0 || pitchClass > 11 || ! (weight > 0.0f) || ! std::isfinite (weight))
        return;
    hist[pitchClass] += weight;
    total += weight;
}

KeyEstimator::Result KeyEstimator::estimate() const noexcept
{
    Result result;
    if (total < 0.25f)
        return result;

    float best = -2.0f;
    float second = -2.0f;
    int bestPc = 0;
    bool bestMinor = false;

    for (int tonic = 0; tonic < 12; ++tonic)
    {
        const float majorScore = pearson (hist, kMajor, tonic);
        const float minorScore = pearson (hist, kMinor, tonic);
        const float score = std::max (majorScore, minorScore);
        const bool minor = minorScore > majorScore;

        if (score > best)
        {
            second = best;
            best = score;
            bestPc = tonic;
            bestMinor = minor;
        }
        else if (score > second)
        {
            second = score;
        }
    }

    int active = 0;
    for (float bin : hist)
        if (bin > 0.15f)
            ++active;

    // One or two pitch classes cannot support a key. Scale the margin by how
    // many classes are actually present so a single held note does not read 100%.
    const float diversity = std::clamp ((active - 2) / 3.0f, 0.0f, 1.0f);
    result.pitchClass = bestPc;
    result.minor = bestMinor;
    result.score = best;
    const float margin = best - std::max (second, 0.0f);
    result.confidence = std::clamp ((margin / 0.16f) * diversity, 0.0f, 1.0f);
    result.valid = best > 0.25f && active >= 3 && result.confidence >= 0.15f;
    return result;
}

} // namespace awk
