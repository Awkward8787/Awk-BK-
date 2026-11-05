// Awk BK — pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

namespace awk
{

// Krumhansl–Schmuckler key profiles over a decaying pitch-class histogram.
class KeyEstimator
{
public:
    struct Result
    {
        int pitchClass = -1;
        bool minor = false;
        float confidence = 0.0f;
        float score = 0.0f;
        bool valid = false;
    };

    void reset() noexcept;
    void decay (float factor) noexcept;
    void add (int pitchClass, float weight) noexcept;
    float energy() const noexcept { return total; }

    Result estimate() const noexcept;

private:
    float hist[12] {};
    float total = 0.0f;
};

} // namespace awk
