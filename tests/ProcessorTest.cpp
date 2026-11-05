// Awk BK - pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PluginProcessor.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>

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

void feedTone (AwkBKProcessor& processor, double sampleRate, double freq, int totalSamples, int block, bool checkBlock)
{
    juce::AudioBuffer<float> buffer (2, block);
    constexpr double pi = 3.14159265358979323846;

    for (int offset = 0; offset + block <= totalSamples; offset += block)
    {
        for (int i = 0; i < block; ++i)
        {
            const float sample = 0.5f * (float) std::sin (2.0 * pi * freq * (double) (offset + i) / sampleRate);
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, sample);
        }

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
        const float left = buffer.getSample (0, 0);
        const float right = buffer.getSample (1, 0);
        processor.processBlock (buffer, midi);

        if (checkBlock && offset == 0)
        {
            check (midi.isEmpty(), "processBlock clears MIDI");
            check (std::fabs (buffer.getSample (0, 0) - left) < 1.0e-6f, "processBlock leaves the left sample unchanged");
            check (std::fabs (buffer.getSample (1, 0) - right) < 1.0e-6f, "processBlock leaves the right sample unchanged");
        }
    }
}

void testProcessorThreadAndLock()
{
    constexpr double sampleRate = 48000.0;
    constexpr int block = 32;
    const double fSharp3 = 440.0 * std::pow (2.0, (54.0 - 69.0) / 12.0);
    const double a2 = 110.0;

    AwkBKProcessor processor;
    check (processor.getName() == "Awk BK", "processor name is Awk BK");

    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (juce::AudioChannelSet::stereo());
    layout.outputBuses.add (juce::AudioChannelSet::stereo());
    check (processor.setBusesLayout (layout), "stereo bus layout is accepted");

    processor.prepareToPlay (sampleRate, block);
    check (processor.getTotalNumInputChannels() == 2, "prepare exposes a stereo input");

    const auto initial = processor.getReadout();
    check (! initial.voiced && initial.midiNote < 0, "readout stays empty until the analysis thread has audio");

    const auto copyStarted = std::chrono::steady_clock::now();
    feedTone (processor, sampleRate, fSharp3, (int) sampleRate, block, true);
    const auto copyMs = std::chrono::duration_cast<std::chrono::milliseconds> (
                            std::chrono::steady_clock::now() - copyStarted)
                            .count();
    std::cout << "     processBlock one second of 32-sample blocks in " << copyMs << " ms\n";
    check (copyMs < 5000, "processBlock stays off the pitch and tempo detectors");

    bool found = false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds (5);
    while (std::chrono::steady_clock::now() < deadline)
    {
        feedTone (processor, sampleRate, fSharp3, block * 32, block, false);
        const auto readout = processor.getReadout();
        if (readout.voiced && readout.midiNote == 54)
        {
            found = true;
            std::cout << "     thread pitch hz=" << readout.frequencyHz
                      << " midi=" << readout.midiNote
                      << " cents=" << readout.cents
                      << "\n";
            break;
        }
        std::this_thread::sleep_for (std::chrono::milliseconds (20));
    }
    check (found, "F#3 appears after the analysis thread runs");

    if (found)
    {
        auto* lock = processor.apvts.getParameter ("lock");
        check (lock != nullptr, "lock parameter exists");
        if (lock != nullptr)
            lock->setValueNotifyingHost (1.0f);
        check (processor.apvts.getRawParameterValue ("lock")->load() >= 0.5f, "lock parameter turns on");

        feedTone (processor, sampleRate, fSharp3, block * 8, block, false);
        std::this_thread::sleep_for (std::chrono::milliseconds (150));
        const auto frozen = processor.getReadout();
        check (frozen.locked, "readout reports Lock");
        check (frozen.voiced && frozen.midiNote == 54, "Lock keeps the current note");

        const auto lockedUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds (700);
        while (std::chrono::steady_clock::now() < lockedUntil)
        {
            feedTone (processor, sampleRate, a2, block * 16, block, false);
            std::this_thread::sleep_for (std::chrono::milliseconds (20));
        }

        const auto still = processor.getReadout();
        std::cout << "     locked midi=" << still.midiNote
                  << " hz=" << still.frequencyHz
                  << " bpm=" << still.bpm
                  << "\n";
        check (still.locked, "Lock stays on while audio changes");
        check (still.midiNote == frozen.midiNote, "Lock freezes the pitch");
        check (std::fabs (still.frequencyHz - frozen.frequencyHz) < 0.5f, "Lock freezes the frequency");
        if (frozen.haveTempo)
            check (std::fabs (still.bpm - frozen.bpm) < 0.01f, "Lock freezes the BPM");
    }

    const auto stopStarted = std::chrono::steady_clock::now();
    processor.releaseResources();
    const auto stopMs = std::chrono::duration_cast<std::chrono::milliseconds> (
                            std::chrono::steady_clock::now() - stopStarted)
                            .count();
    std::cout << "     releaseResources " << stopMs << " ms\n";
    check (stopMs < 2000, "releaseResources returns promptly");
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    testProcessorThreadAndLock();

    if (gFailures != 0)
    {
        std::cerr << gFailures << " processor test(s) failed\n";
        return 1;
    }
    std::cout << "processor tests passed\n";
    return 0;
}
