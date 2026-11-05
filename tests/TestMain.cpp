// Awk BK — pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "KeyEstimator.h"
#include "NoteUtils.h"
#include "PitchDetector.h"
#include "TempoDetector.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#ifndef AWK_PROCESSOR_SOURCE
 #define AWK_PROCESSOR_SOURCE "Source/PluginProcessor.cpp"
#endif

namespace
{
int gFailures = 0;

void check (bool ok, const std::string& message)
{
    if (ok)
    {
        std::cout << "ok   " << message << "\n";
        return;
    }
    std::cerr << "FAIL " << message << "\n";
    ++gFailures;
}

std::vector<float> sine (int n, double sampleRate, double freq, float amp = 0.5f)
{
    std::vector<float> out ((size_t) n);
    constexpr double pi = 3.14159265358979323846;
    for (int i = 0; i < n; ++i)
        out[(size_t) i] = amp * (float) std::sin (2.0 * pi * freq * (double) i / sampleRate);
    return out;
}

std::vector<float> harmonic (int n, double sampleRate, double freq)
{
    auto fundamental = sine (n, sampleRate, freq, 0.55f);
    auto second = sine (n, sampleRate, freq * 2.0, 0.28f);
    auto third = sine (n, sampleRate, freq * 3.0, 0.14f);
    for (int i = 0; i < n; ++i)
        fundamental[(size_t) i] += second[(size_t) i] + third[(size_t) i];
    return fundamental;
}

std::vector<float> clicks (double sampleRate, double bpm, double seconds)
{
    const int n = (int) std::lround (sampleRate * seconds);
    std::vector<float> out ((size_t) n, 0.0f);
    const double interval = 60.0 / bpm;
    const int burst = std::max (8, (int) std::lround (sampleRate * 0.004));
    constexpr double pi = 3.14159265358979323846;
    for (double t = 0.12; t < seconds - 0.05; t += interval)
    {
        const int start = (int) std::lround (t * sampleRate);
        for (int i = 0; i < burst && start + i < n; ++i)
        {
            const float env = 1.0f - (float) i / (float) burst;
            const float tone = (float) std::sin (2.0 * pi * 1000.0 * (double) i / sampleRate);
            out[(size_t) (start + i)] += env * tone;
        }
    }
    return out;
}

void expectPitch (const char* label, const std::vector<float>& buffer, double sampleRate,
                  int expectedMidi, float expectedCents, float centTolerance)
{
    awk::PitchDetector detector;
    detector.prepare ((int) buffer.size());
    const auto result = detector.detect (buffer.data(), (int) buffer.size(), sampleRate, 0.15f, 1.0e-4f);
    const awk::NoteName note = awk::noteFromHz (result.frequencyHz);

    std::cout << "     " << label
              << " voiced=" << result.voiced
              << " hz=" << result.frequencyHz
              << " note=" << awk::formatNote (note)
              << " cents=" << result.cents
              << " conf=" << result.confidence
              << "\n";

    check (result.voiced, std::string (label) + " is voiced");
    check (result.midiNote == expectedMidi, std::string (label) + " midi " + std::to_string (result.midiNote)
                                                 + " expected " + std::to_string (expectedMidi));
    check (std::fabs (result.cents - expectedCents) <= centTolerance,
           std::string (label) + " cents " + std::to_string (result.cents)
               + " expected " + std::to_string (expectedCents));
}

void testAudioCallbackDoesNotAnalyze()
{
    std::ifstream input (AWK_PROCESSOR_SOURCE);
    std::string text ((std::istreambuf_iterator<char> (input)), {});
    check (input.good() || ! text.empty(), std::string ("read ") + AWK_PROCESSOR_SOURCE);

    const auto blockAt = text.find ("void AwkBKProcessor::processBlock");
    const auto nextAt = text.find ("void AwkBKProcessor::copyLatest");
    check (blockAt != std::string::npos && nextAt != std::string::npos && nextAt > blockAt,
           "processBlock is present");
    if (blockAt == std::string::npos || nextAt == std::string::npos || nextAt <= blockAt)
        return;

    const auto block = text.substr (blockAt, nextAt - blockAt);
    const char* banned[] = { "runPitch", "runTempo", "pitchDetector", "tempoDetector", ".detect(" };
    for (const char* word : banned)
        check (block.find (word) == std::string::npos,
               std::string ("processBlock does not reference ") + word);

    check (block.find ("midiMessages.clear()") != std::string::npos, "processBlock clears MIDI");
    check (text.find ("std::thread") != std::string::npos, "analysis is started on a std::thread");
    check (text.find ("void AwkBKProcessor::analysisLoop()") != std::string::npos,
           "YIN and tempo run from analysisLoop");
    const auto loopAt = text.find ("void AwkBKProcessor::analysisLoop()");
    const auto loopEnd = text.find ("bool AwkBKProcessor::isBusesLayoutSupported");
    if (loopAt != std::string::npos && loopEnd != std::string::npos && loopEnd > loopAt)
    {
        const auto loop = text.substr (loopAt, loopEnd - loopAt);
        check (loop.find ("isLocked()") != std::string::npos, "analysisLoop skips work while locked");
        check (loop.find ("runPitch") != std::string::npos, "analysisLoop owns pitch");
        check (loop.find ("runTempo") != std::string::npos, "analysisLoop owns tempo");
    }
}

void expectTempo (const char* label, double sampleRate, double bpm, float tolerance)
{
    const auto buffer = clicks (sampleRate, bpm, 6.0);
    awk::TempoDetector detector;
    detector.prepare (2000);
    const auto result = detector.detect (buffer.data(), (int) buffer.size(), sampleRate, 0.6f);
    std::cout << "     " << label
              << " valid=" << result.valid
              << " bpm=" << result.bpm
              << " conf=" << result.confidence
              << "\n";
    check (result.valid, std::string (label) + " is valid");
    check (std::fabs (result.bpm - (float) bpm) <= tolerance,
           std::string (label) + " bpm " + std::to_string (result.bpm)
               + " expected " + std::to_string (bpm));
}
} // namespace

int main()
{
    constexpr double sr = 48000.0;
    const double fSharp3 = 440.0 * std::pow (2.0, (54.0 - 69.0) / 12.0);
    const double detuned = fSharp3 * std::pow (2.0, 7.0 / 1200.0);

    const auto exact = awk::noteFromHz ((float) fSharp3);
    check (exact.midi == 54 && std::string (awk::formatNote (exact)) == "F#3",
           "noteFromHz F#3 maps to midi 54");
    check (std::fabs (exact.cents) < 0.05f, "exact F#3 is within 0.05 cents");

    const auto a4 = awk::noteFromHz (440.0f);
    check (a4.midi == 69 && std::string (awk::formatNote (a4)) == "A4", "440 Hz is A4");

    expectPitch ("F#3 sine", sine (8192, sr, fSharp3), sr, 54, 0.0f, 2.0f);
    expectPitch ("F#3 +7 cents", sine (8192, sr, detuned), sr, 54, 7.0f, 2.0f);
    expectPitch ("A3 harmonic", harmonic (8192, sr, 220.0), sr, 57, 0.0f, 8.0f);
    expectPitch ("A4 at 44.1k", sine (8192, 44100.0, 440.0), 44100.0, 69, 0.0f, 2.0f);

    {
        awk::PitchDetector detector;
        std::vector<float> silent (4096, 0.0f);
        detector.prepare (4096);
        const auto result = detector.detect (silent.data(), 4096, sr, 0.15f, 1.0e-4f);
        check (! result.voiced && result.midiNote < 0, "silence is unvoiced");
    }

    testAudioCallbackDoesNotAnalyze();

    expectTempo ("140 BPM", sr, 140.0, 1.0f);
    expectTempo ("90 BPM", sr, 90.0, 1.0f);
    expectTempo ("120 BPM", 44100.0, 120.0, 1.0f);
    expectTempo ("174 BPM", sr, 174.0, 1.5f);

    {
        awk::KeyEstimator key;
        for (int i = 0; i < 8; ++i)
            key.add (6, 1.0f); // F# tonic
        for (int pc : { 8, 9, 11, 1, 2, 4 })
            key.add (pc, 1.0f); // F# natural minor: G# A B C# D E
        const auto result = key.estimate();
        std::cout << "     F# minor key pc=" << result.pitchClass
                  << " minor=" << result.minor
                  << " conf=" << result.confidence
                  << " score=" << result.score
                  << "\n";
        check (result.valid, "F# minor estimate is valid");
        check (result.pitchClass == 6 && result.minor, "histogram maps to F# minor");
        check (result.confidence >= 0.45f, "F# minor confidence is at least 45%");
    }

    {
        awk::KeyEstimator key;
        for (int i = 0; i < 8; ++i)
            key.add (0, 1.0f); // C
        for (int pc : { 2, 4, 5, 7, 9, 11 })
            key.add (pc, 1.0f);
        const auto result = key.estimate();
        std::cout << "     C major key pc=" << result.pitchClass
                  << " minor=" << result.minor
                  << " conf=" << result.confidence
                  << "\n";
        check (result.valid && result.pitchClass == 0 && ! result.minor, "C major scale maps to C major");
    }

    {
        awk::KeyEstimator key;
        const auto result = key.estimate();
        check (! result.valid, "empty histogram has no key");
    }

    {
        awk::KeyEstimator key;
        for (int i = 0; i < 12; ++i)
            key.add (6, 1.0f);
        const auto result = key.estimate();
        check (! result.valid, "a single held note does not claim a key");
    }

    {
        std::mt19937 rng (1);
        std::uniform_real_distribution<float> noise (-0.2f, 0.2f);
        std::vector<float> buffer (48000 * 4);
        for (float& s : buffer)
            s = noise (rng);
        awk::TempoDetector detector;
        detector.prepare (2000);
        const auto result = detector.detect (buffer.data(), (int) buffer.size(), sr, 0.5f);
        std::cout << "     noise valid=" << result.valid << " conf=" << result.confidence << "\n";
        check (! result.valid || result.confidence < 0.55f, "noise is not a confident tempo");
    }

    if (gFailures != 0)
    {
        std::cerr << gFailures << " test(s) failed\n";
        return 1;
    }
    std::cout << "all tests passed\n";
    return 0;
}
