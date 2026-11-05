// Awk BK - pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <cmath>
#include <string>

namespace awk
{

struct NoteName
{
    int midi = -1;
    int pitchClass = 0;
    int octave = 0;
    float cents = 0.0f;
    const char* name = "";
};

inline float midiToHz (float midi) noexcept
{
    return 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f);
}

// Nearest equal-tempered note, A4 = 440 Hz. Cents are in [-50, 50].
inline NoteName noteFromHz (float hz) noexcept
{
    NoteName note;
    if (! (hz > 0.0f) || ! std::isfinite (hz))
        return note;

    const float midiFloat = 69.0f + 12.0f * std::log2 (hz / 440.0f);
    const int midi = (int) std::lround (midiFloat);
    if (midi < 0 || midi > 127)
        return note;

    static const char* names[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    note.midi = midi;
    note.pitchClass = midi % 12;
    note.octave = midi / 12 - 1;
    note.cents = (midiFloat - (float) midi) * 100.0f;
    note.name = names[note.pitchClass];
    return note;
}

inline std::string formatNote (const NoteName& note)
{
    if (note.midi < 0 || note.name == nullptr || note.name[0] == '\0')
        return "--";
    return std::string (note.name) + std::to_string (note.octave);
}

inline const char* pitchClassName (int pitchClass) noexcept
{
    static const char* names[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    if (pitchClass < 0 || pitchClass > 11)
        return "--";
    return names[pitchClass];
}

template <typename T>
constexpr T lerp (T a, T b, T t) noexcept
{
    return a + (b - a) * t;
}

} // namespace awk
