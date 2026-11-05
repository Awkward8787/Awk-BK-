// Awk BK - pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

namespace awk
{

struct Readout
{
    float bpm = 0.0f;
    bool haveTempo = false;
    float frequencyHz = 0.0f;
    int midiNote = -1;
    float cents = 0.0f;
    float pitchConfidence = 0.0f;
    bool voiced = false;
    int keyPitchClass = -1;
    bool keyMinor = false;
    float keyConfidence = 0.0f;
    bool haveKey = false;
    bool locked = false;
    bool fileMode = false;
};

} // namespace awk
