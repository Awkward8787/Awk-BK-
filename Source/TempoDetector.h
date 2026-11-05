// Awk BK - pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <vector>

namespace awk
{

// Onset-energy autocorrelation. Allocates in prepare(), not in detect().
class TempoDetector
{
public:
    struct Result
    {
        float bpm = 0.0f;
        float confidence = 0.0f;
        bool valid = false;
    };

    void prepare (int maxFrames);

    // sensitivity 0..1 lowers the onset gate as it rises.
    Result detect (const float* samples, int numSamples, double sampleRate, float sensitivity);

private:
    std::vector<float> energy;
    std::vector<float> onset;
    std::vector<float> acf;
    int frameCapacity = 0;
};

} // namespace awk
