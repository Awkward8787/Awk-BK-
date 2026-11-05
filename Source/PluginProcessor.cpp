// Awk BK — pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "NoteUtils.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sensitivity", 1 },
        "Sensitivity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.32f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "window", 1 },
        "Window",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.50f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "lock", 1 },
        "Lock",
        false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "fileMode", 1 },
        "File Mode",
        false));

    return layout;
}

float clamp01 (float value) noexcept
{
    return std::clamp (value, 0.0f, 1.0f);
}
} // namespace

AwkBKProcessor::AwkBKProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    sensParam = apvts.getRawParameterValue ("sensitivity");
    windowParam = apvts.getRawParameterValue ("window");
    lockParam = apvts.getRawParameterValue ("lock");
    fileModeParam = apvts.getRawParameterValue ("fileMode");
}

AwkBKProcessor::~AwkBKProcessor()
{
    stopAnalysisThread();
}

void AwkBKProcessor::prepareToPlay (double sampleRate, int)
{
    stopAnalysisThread();
    currentSampleRate = sampleRate > 1000.0 ? sampleRate : 48000.0;
    const int ring = std::max (16384, (int) std::ceil (currentSampleRate * 12.0));
    mono.assign ((size_t) ring, 0.0f);
    monoWrite.store (0);
    ringSeq.store (0);
    samplesSeen.store (0);
    scratch.assign ((size_t) ring, 0.0f);
    analysis.assign (4096, 0.0f);

    pitchDetector.prepare (4096);
    tempoDetector.prepare ((int) std::ceil (12.0 / 0.010) + 16);

    holdSamples = 0;
    pitchReady = false;
    haveTempo = false;
    smoothLogHz = 0.0f;
    smoothBpm = 0.0f;
    keyEstimator.reset();

    waveDecim = std::max (1, (int) std::lround (currentSampleRate * 0.0025));
    waveCount = 0;
    waveAccum = 0.0f;
    waveform.fill (0.0f);
    waveWrite.store (0);
    waveFilled.store (0);

    voicedAtomic.store (0);
    midiAtomic.store (-1);
    haveKeyAtomic.store (0);
    haveTempoAtomic.store (0);
    pitchConfAtomic.store (0.0f);
    startAnalysisThread();
}

void AwkBKProcessor::releaseResources()
{
    stopAnalysisThread();
}

bool AwkBKProcessor::isLocked() const noexcept
{
    return lockParam != nullptr && lockParam->load() >= 0.5f;
}

void AwkBKProcessor::startAnalysisThread()
{
    stopAnalysisThread();
    analysisStop.store (false);
    analysisThread = std::thread ([this] { analysisLoop(); });
}

void AwkBKProcessor::stopAnalysisThread()
{
    if (! analysisThread.joinable())
        return;
    analysisStop.store (true);
    analysisThread.join();
    analysisStop.store (false);
}

void AwkBKProcessor::analysisLoop()
{
    using clock = std::chrono::steady_clock;
    auto nextPitch = clock::now();
    auto nextTempo = clock::now();
    auto tapHoldUntil = clock::time_point {};

    while (! analysisStop.load (std::memory_order_acquire))
    {
        if (isLocked())
        {
            std::this_thread::sleep_for (std::chrono::milliseconds (20));
            continue;
        }

        const auto serial = tapSerial.load (std::memory_order_acquire);
        if (serial != seenTapSerial)
        {
            seenTapSerial = serial;
            smoothBpm = tapBpm.load (std::memory_order_relaxed);
            haveTempo = true;
            tapHoldUntil = clock::now() + std::chrono::seconds (3);
            bpmAtomic.store (smoothBpm);
            haveTempoAtomic.store (1);
        }

        const float sensitivity = sensParam != nullptr ? sensParam->load() : 0.32f;
        const float windowAmount = windowParam != nullptr ? windowParam->load() : 0.5f;
        const bool fileMode = fileModeParam != nullptr && fileModeParam->load() >= 0.5f;
        const auto now = clock::now();

        if (now >= nextPitch)
        {
            runPitch (sensitivity, windowAmount, fileMode);
            const auto pitchHop = std::chrono::milliseconds (fileMode ? 120 : 60);
            nextPitch = clock::now() + pitchHop;
        }

        if (now >= nextTempo && clock::now() >= tapHoldUntil)
        {
            runTempo (sensitivity, windowAmount, fileMode);
            const auto tempoHop = std::chrono::milliseconds (fileMode ? 400 : 220);
            nextTempo = clock::now() + tempoHop;
        }

        std::this_thread::sleep_for (std::chrono::milliseconds (8));
    }
}

bool AwkBKProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();
    if (input != output)
        return false;
    return input == juce::AudioChannelSet::mono()
        || input == juce::AudioChannelSet::stereo();
}

void AwkBKProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    midiMessages.clear();
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    // Input channels are left untouched. Extra outputs (none in the layouts we
    // accept) are cleared so they cannot carry garbage.
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    lockedAtomic.store (isLocked() ? 1 : 0);
    fileModeAtomic.store (fileModeParam != nullptr && fileModeParam->load() >= 0.5f ? 1 : 0);

    if (numIn <= 0 || numSamples <= 0 || mono.empty())
        return;

    auto* const* data = buffer.getArrayOfReadPointers();
    const int cap = (int) mono.size();

    // Odd sequence value means a block write is in progress. The analysis
    // thread copies only between even values, so this callback never waits.
    ringSeq.fetch_add (1, std::memory_order_acq_rel);
    int write = monoWrite.load (std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        float mixed = 0.0f;
        for (int ch = 0; ch < numIn; ++ch)
            mixed += data[ch][i];
        mixed /= (float) numIn;

        mono[(size_t) write] = mixed;
        write = (write + 1) % cap;

        waveAccum += mixed;
        if (++waveCount >= waveDecim)
        {
            const int idx = waveWrite.load (std::memory_order_relaxed);
            waveform[(size_t) idx] = waveAccum / (float) waveCount;
            const int next = (idx + 1) % kWavePoints;
            waveWrite.store (next, std::memory_order_release);
            const int filled = waveFilled.load (std::memory_order_relaxed);
            if (filled < kWavePoints)
                waveFilled.store (filled + 1, std::memory_order_relaxed);
            waveAccum = 0.0f;
            waveCount = 0;
        }
    }

    monoWrite.store (write, std::memory_order_release);
    samplesSeen.fetch_add (numSamples, std::memory_order_release);
    ringSeq.fetch_add (1, std::memory_order_release);
}

void AwkBKProcessor::copyLatest (int count, float* dest) const
{
    const int cap = (int) mono.size();
    if (cap <= 0 || count <= 0)
        return;
    count = std::min (count, cap);

    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const auto seq1 = ringSeq.load (std::memory_order_acquire);
        if ((seq1 & 1u) != 0u)
            continue;

        int start = monoWrite.load (std::memory_order_acquire) - count;
        while (start < 0)
            start += cap;

        for (int i = 0; i < count; ++i)
            dest[i] = mono[(size_t) ((start + i) % cap)];

        const auto seq2 = ringSeq.load (std::memory_order_acquire);
        if (seq1 == seq2)
            return;
    }
}

void AwkBKProcessor::runPitch (float sensitivity, float windowAmount, bool fileMode)
{
    if (isLocked())
        return;
    const float sens = clamp01 (sensitivity);
    const float window = clamp01 (windowAmount);
    int desired = (int) std::lround (awk::lerp (2048.0, 8192.0, (double) window));
    if (fileMode)
        desired = std::min (16384, desired * 2);

    int factor = 1;
    while (desired / factor > 4096)
        ++factor;
    desired -= desired % factor;

    const int hop = std::max (desired / 2, (int) std::lround (currentSampleRate * (fileMode ? 0.12 : 0.06)));

    if (desired < 1024 || samplesSeen.load() < desired || (int) scratch.size() < desired)
        return;

    copyLatest (desired, scratch.data());
    const int outCount = desired / factor;
    for (int i = 0; i < outCount; ++i)
    {
        double sum = 0.0;
        for (int k = 0; k < factor; ++k)
            sum += scratch[(size_t) (i * factor + k)];
        analysis[(size_t) i] = (float) (sum / (double) factor);
    }

    const float yinThreshold = awk::lerp (0.07f, 0.30f, sens);
    const float rmsGate = awk::lerp (2.0e-3f, 2.0e-5f, sens);
    const auto pitch = pitchDetector.detect (analysis.data(), outCount,
                                              currentSampleRate / (double) factor,
                                              yinThreshold, rmsGate);

    if (isLocked())
        return;

    if (pitch.voiced && pitch.frequencyHz > 0.0f)
    {
        const float logHz = std::log (pitch.frequencyHz);
        if (! pitchReady)
        {
            smoothLogHz = logHz;
            pitchReady = true;
        }
        else
        {
            const float semis = std::fabs (12.0f * (logHz - smoothLogHz) / std::log (2.0f));
            float alpha = awk::lerp (0.62f, 0.20f, window);
            if (fileMode)
                alpha *= 0.55f;
            if (semis > 1.0f)
                alpha = 1.0f;
            smoothLogHz += (logHz - smoothLogHz) * alpha;
        }

        holdSamples = (int) std::lround (currentSampleRate * 0.40);
        const float hz = std::exp (smoothLogHz);
        const awk::NoteName note = awk::noteFromHz (hz);

        frequencyAtomic.store (hz);
        midiAtomic.store (note.midi);
        centsAtomic.store (note.cents);
        pitchConfAtomic.store (pitch.confidence);
        voicedAtomic.store (1);

        if (note.pitchClass >= 0 && note.midi >= 0)
        {
            const float dt = (float) hop / (float) currentSampleRate;
            const float timeConstant = fileMode ? 14.0f : 7.0f;
            keyEstimator.decay (std::exp (-dt / timeConstant));
            keyEstimator.add (note.pitchClass, std::max (pitch.confidence, 0.05f));
            const auto key = keyEstimator.estimate();
            if (key.valid)
            {
                keyPcAtomic.store (key.pitchClass);
                keyMinorAtomic.store (key.minor ? 1 : 0);
                keyConfAtomic.store (key.confidence);
                haveKeyAtomic.store (1);
            }
        }
    }
    else if (holdSamples > 0)
    {
        holdSamples -= hop;
        if (holdSamples <= 0)
        {
            voicedAtomic.store (0);
            midiAtomic.store (-1);
            pitchConfAtomic.store (0.0f);
        }
    }
}

void AwkBKProcessor::runTempo (float sensitivity, float windowAmount, bool fileMode)
{
    if (isLocked())
        return;
    const float window = clamp01 (windowAmount);
    float seconds = awk::lerp (2.5f, 8.0f, window);
    if (fileMode)
        seconds = std::min (12.0f, seconds * 1.6f);

    int count = (int) std::lround (currentSampleRate * (double) seconds);
    count = std::min (count, (int) scratch.size());

    if (count < (int) currentSampleRate || samplesSeen.load() < count)
        return;

    copyLatest (count, scratch.data());
    const auto tempo = tempoDetector.detect (scratch.data(), count, currentSampleRate, clamp01 (sensitivity));
    if (isLocked() || ! tempo.valid)
        return;

    if (! haveTempo)
        smoothBpm = tempo.bpm;
    else
    {
        float beta = fileMode ? 0.14f : 0.34f;
        const float ratio = smoothBpm > 1.0f ? tempo.bpm / smoothBpm : 1.0f;
        if (ratio > 1.45f || ratio < 0.69f)
            beta = tempo.confidence > 0.72f ? 1.0f : beta * 0.25f;
        smoothBpm += (tempo.bpm - smoothBpm) * beta;
    }

    haveTempo = true;
    bpmAtomic.store (smoothBpm);
    haveTempoAtomic.store (1);
}

void AwkBKProcessor::tap()
{
    if (isLocked())
        return;

    const double now = juce::Time::getMillisecondCounterHiRes();
    const double gap = lastTapMs > 0.0 ? now - lastTapMs : 0.0;

    if (lastTapMs > 0.0 && gap < 80.0)
        return;

    if (lastTapMs <= 0.0 || gap > 2000.0)
        tapIntervals.clear();
    else
        tapIntervals.push_back (gap);

    if ((int) tapIntervals.size() > 6)
        tapIntervals.erase (tapIntervals.begin());

    lastTapMs = now;

    if (tapIntervals.empty())
        return;

    double sum = 0.0;
    for (double interval : tapIntervals)
        sum += interval;

    const float bpm = std::clamp ((float) (60000.0 / (sum / (double) tapIntervals.size())), 40.0f, 240.0f);
    tapBpm.store (bpm, std::memory_order_relaxed);
    tapSerial.fetch_add (1, std::memory_order_release);
}

awk::Readout AwkBKProcessor::getReadout() const
{
    awk::Readout readout;
    readout.bpm = bpmAtomic.load();
    readout.haveTempo = haveTempoAtomic.load() != 0;
    readout.frequencyHz = frequencyAtomic.load();
    readout.midiNote = midiAtomic.load();
    readout.cents = centsAtomic.load();
    readout.pitchConfidence = pitchConfAtomic.load();
    readout.voiced = voicedAtomic.load() != 0;
    readout.keyPitchClass = keyPcAtomic.load();
    readout.keyMinor = keyMinorAtomic.load() != 0;
    readout.keyConfidence = keyConfAtomic.load();
    readout.haveKey = haveKeyAtomic.load() != 0;
    readout.locked = lockedAtomic.load() != 0;
    readout.fileMode = fileModeAtomic.load() != 0;
    return readout;
}

int AwkBKProcessor::newestWaveformIndex() const noexcept
{
    // copyWaveform starts at the write cursor, so the newest sample lands
    // on the last index once any meter point exists.
    if (waveFilled.load (std::memory_order_acquire) <= 0)
        return -1;
    return kWavePoints - 1;
}

void AwkBKProcessor::copyWaveform (float* dest, int numPoints) const
{
    if (dest == nullptr || numPoints <= 0)
        return;

    const int write = waveWrite.load (std::memory_order_acquire);
    const int count = std::min (numPoints, kWavePoints);
    for (int i = 0; i < count; ++i)
        dest[i] = waveform[(size_t) ((write + i) % kWavePoints)];
    for (int i = count; i < numPoints; ++i)
        dest[i] = 0.0f;
}

juce::AudioProcessorEditor* AwkBKProcessor::createEditor()
{
    return new AwkBKEditor (*this);
}

void AwkBKProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AwkBKProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AwkBKProcessor();
}
