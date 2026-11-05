// Awk BK - pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <vector>

namespace awk
{

// YIN (de Cheveigné & Kawahara, 2002). Monophonic f0.
// Buffers are allocated in prepare() so detect() does not allocate.
class PitchDetector
{
public:
    struct Result
    {
        float frequencyHz = 0.0f;
        float confidence = 0.0f;
        bool voiced = false;
        int midiNote = -1;
        float cents = 0.0f;
    };

    void prepare (int maxSamples);

    // yinThreshold is the CMNDF absolute threshold (about 0.10 strict, 0.20 loose).
    // rmsGate is the minimum signal RMS; quieter blocks are unvoiced.
    Result detect (const float* samples, int numSamples, double sampleRate,
                   float yinThreshold, float rmsGate) ;

private:
    std::vector<float> diff;
    std::vector<float> cmnd;
    int capacity = 0;
};

} // namespace awk
