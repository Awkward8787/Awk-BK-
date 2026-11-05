// Awk BK - pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "GoldControls.h"
#include "PluginProcessor.h"

class AwkBKEditor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit AwkBKEditor (AwkBKProcessor&);
    ~AwkBKEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Layout
    {
        juce::Rectangle<int> header;
        juce::Rectangle<int> glass;
        juce::Rectangle<int> controls;
        juce::Rectangle<int> footer;
        juce::Rectangle<float> bpm;
        juce::Rectangle<float> pitch;
        juce::Rectangle<float> wave;
        juce::Rectangle<int> tap;
        juce::Rectangle<int> lock;
        juce::Rectangle<int> sens;
        juce::Rectangle<int> window;
        juce::Rectangle<int> mode;
    };

    Layout makeLayout() const;
    void timerCallback() override;
    void drawChassis (juce::Graphics& g) const;
    void drawHeader (juce::Graphics& g, juce::Rectangle<int> header) const;
    void drawGlass (juce::Graphics& g, const Layout& layout, const awk::Readout& readout) const;
    void drawBpm (juce::Graphics& g, juce::Rectangle<float> area, const awk::Readout& readout) const;
    void drawPitch (juce::Graphics& g, juce::Rectangle<float> area, const awk::Readout& readout) const;
    void drawWave (juce::Graphics& g, juce::Rectangle<float> area) const;
    void drawFooter (juce::Graphics& g, juce::Rectangle<int> footer) const;

    AwkBKProcessor& processor;
    juce::Image logo;
    awk::GoldTapButton tapButton;
    awk::GoldToggle lockToggle { "LOCK", false };
    awk::GoldKnob sensKnob { "SENS" };
    awk::GoldKnob windowKnob { "WINDOW" };
    awk::GoldToggle modeToggle { "MODE", true };

    std::unique_ptr<juce::SliderParameterAttachment> sensAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> windowAttachment;
    std::unique_ptr<juce::ButtonParameterAttachment> lockAttachment;
    std::unique_ptr<juce::ButtonParameterAttachment> modeAttachment;
    std::unique_ptr<juce::TooltipWindow> tooltip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AwkBKEditor)
};
