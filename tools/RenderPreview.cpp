// Awk BK - pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace
{
void renderSineAndClicks (AwkBKProcessor& processor)
{
    constexpr double sampleRate = 48000.0;
    constexpr int block = 512;
    constexpr double seconds = 7.0;
    const double fSharp3 = 440.0 * std::pow (2.0, (54.0 - 69.0) / 12.0);
    const double interval = 60.0 / 140.0;
    constexpr double pi = 3.14159265358979323846;

    processor.prepareToPlay (sampleRate, block);
    const int total = (int) std::lround (sampleRate * seconds);
    juce::AudioBuffer<float> buffer (2, block);
    float maxDiff = 0.0f;

    for (int offset = 0; offset < total; offset += block)
    {
        const int n = std::min (block, total - offset);
        buffer.clear();
        for (int i = 0; i < n; ++i)
        {
            const int index = offset + i;
            const double time = (double) index / sampleRate;
            float sample = 0.45f * (float) std::sin (2.0 * pi * fSharp3 * time);
            const double intoClick = std::fmod (time, interval);
            if (intoClick < 0.004)
            {
                const float env = 1.0f - (float) (intoClick / 0.004);
                sample += env * (float) std::sin (2.0 * pi * 1000.0 * intoClick);
            }
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, sample);
        }

        std::vector<float> expected ((size_t) n);
        for (int i = 0; i < n; ++i)
            expected[(size_t) i] = buffer.getSample (0, i);

        juce::MidiBuffer midi;
        processor.processBlock (buffer, midi);

        for (int i = 0; i < n; ++i)
        {
            maxDiff = std::max (maxDiff, std::fabs (buffer.getSample (0, i) - expected[(size_t) i]));
            maxDiff = std::max (maxDiff, std::fabs (buffer.getSample (1, i) - expected[(size_t) i]));
        }
    }

    std::cout << "passthrough max diff " << maxDiff << "\n";
    if (maxDiff > 0.0f)
        std::cout << "passthrough FAILED\n";

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds (3);
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto readout = processor.getReadout();
        if (readout.voiced && readout.haveTempo)
            break;
        std::this_thread::sleep_for (std::chrono::milliseconds (20));
    }
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String outputPath = argc > 1 ? juce::String (argv[1]) : juce::String ("awk-bk-ui.png");

    AwkBKProcessor processor;
    renderSineAndClicks (processor);

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (editor->getWidth(), editor->getHeight());

    juce::Image image (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
    juce::Graphics graphics (image);
    editor->paintEntireComponent (graphics, true);

    juce::File output (outputPath);
    output.deleteFile();
    juce::FileOutputStream stream (output);
    if (! stream.openedOk())
        return 1;

    juce::PNGImageFormat png;
    if (! png.writeImageToStream (image, stream))
        return 1;

    std::cout << "wrote " << output.getFullPathName() << "\n";
    const auto readout = processor.getReadout();
    std::cout << "bpm " << readout.bpm
              << " haveTempo " << readout.haveTempo
              << " midi " << readout.midiNote
              << " cents " << readout.cents
              << " voiced " << readout.voiced
              << "\n";
    return 0;
}
