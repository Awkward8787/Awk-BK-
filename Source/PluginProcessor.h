// Awk BK — pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "KeyEstimator.h"
#include "PitchDetector.h"
#include "Readout.h"
#include "TempoDetector.h"

#include <array>
#include <atomic>
#include <vector>

class AwkBKProcessor : public juce::AudioProcessor
{
public:
    AwkBKProcessor();
    ~AwkBKProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Awk BK"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Message thread. Ignored by the readouts while Lock is on.
    void tap();

    awk::Readout getReadout() const;
    void copyWaveform (float* dest, int numPoints) const;

    juce::AudioProcessorValueTreeState apvts;

private:
    void runPitch (float sensitivity, float windowAmount, bool fileMode, bool locked);
    void runTempo (float sensitivity, float windowAmount, bool fileMode, bool locked);
    void copyLatest (int count, float* dest) const;

    static constexpr int kWavePoints = 256;

    std::atomic<float>* sensParam = nullptr;
    std::atomic<float>* windowParam = nullptr;
    std::atomic<float>* lockParam = nullptr;
    std::atomic<float>* fileModeParam = nullptr;

    double currentSampleRate = 48000.0;
    std::vector<float> mono;
    int monoWrite = 0;
    std::int64_t samplesSeen = 0;
    std::vector<float> scratch;
    std::vector<float> analysis;

    awk::PitchDetector pitchDetector;
    awk::TempoDetector tempoDetector;
    awk::KeyEstimator keyEstimator;

    int samplesUntilPitch = 0;
    int samplesUntilTempo = 0;
    int holdSamples = 0;
    int tapHoldSamples = 0;
    bool pitchReady = false;
    bool haveTempo = false;
    float smoothLogHz = 0.0f;
    float smoothBpm = 0.0f;

    int waveDecim = 128;
    int waveCount = 0;
    float waveAccum = 0.0f;
    std::array<float, kWavePoints> waveform {};
    std::atomic<int> waveWrite { 0 };

    std::atomic<float> bpmAtomic { 0.0f };
    std::atomic<int> haveTempoAtomic { 0 };
    std::atomic<float> frequencyAtomic { 0.0f };
    std::atomic<int> midiAtomic { -1 };
    std::atomic<float> centsAtomic { 0.0f };
    std::atomic<float> pitchConfAtomic { 0.0f };
    std::atomic<int> voicedAtomic { 0 };
    std::atomic<int> keyPcAtomic { -1 };
    std::atomic<int> keyMinorAtomic { 0 };
    std::atomic<float> keyConfAtomic { 0.0f };
    std::atomic<int> haveKeyAtomic { 0 };
    std::atomic<int> lockedAtomic { 0 };
    std::atomic<int> fileModeAtomic { 0 };

    double lastTapMs = 0.0;
    std::vector<double> tapIntervals;
    std::atomic<float> tapBpm { 0.0f };
    std::atomic<std::uint32_t> tapSerial { 0 };
    std::uint32_t seenTapSerial = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AwkBKProcessor)
};
