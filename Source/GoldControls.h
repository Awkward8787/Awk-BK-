// Awk BK - pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "AwkPalette.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>

namespace awk
{

inline juce::Font goldFont (float size, bool bold = false, float kerning = 0.0f)
{
    auto font = juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    font.setExtraKerningFactor (kerning);
    return font;
}

class GoldKnob : public juce::Slider
{
public:
    explicit GoldKnob (const juce::String& name)
        : juce::Slider (name), label (name)
    {
        setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                             juce::MathConstants<float>::pi * 2.8f,
                             true);
        setDoubleClickReturnValue (true, name == "SENS" ? 0.32 : 0.50);
        setMouseDragSensitivity (160);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto labelArea = bounds.removeFromBottom (22.0f);
        const float diameter = juce::jmin (78.0f, bounds.getWidth(), bounds.getHeight());
        const auto knob = bounds.withSizeKeepingCentre (diameter, diameter);

        g.setColour (palette::gold);
        g.drawLine (knob.getCentreX(), knob.getY() - 9.0f, knob.getCentreX(), knob.getY() - 2.0f, 1.5f);

        juce::ColourGradient body (palette::knobTop, knob.getCentreX(), knob.getY(),
                                   palette::knobBot, knob.getCentreX(), knob.getBottom(), false);
        g.setGradientFill (body);
        g.fillEllipse (knob.reduced (2.0f));

        g.setColour (palette::goldDeep.withAlpha (0.9f));
        g.drawEllipse (knob.reduced (2.5f), 2.0f);
        g.setColour (palette::gold.withAlpha (0.95f));
        g.drawEllipse (knob.reduced (1.0f), 1.25f);

        const auto rotary = getRotaryParameters();
        const float angle = juce::jmap ((float) getValue(),
                                         (float) getMinimum(),
                                         (float) getMaximum(),
                                         rotary.startAngleRadians,
                                         rotary.endAngleRadians);
        const auto centre = knob.getCentre();
        const float radius = diameter * 0.34f;
        const float tipX = centre.x + std::sin (angle) * radius;
        const float tipY = centre.y - std::cos (angle) * radius;
        g.setColour (palette::goldBright);
        g.drawLine (centre.x, centre.y, tipX, tipY, 2.2f);
        g.fillEllipse (tipX - 3.2f, tipY - 3.2f, 6.4f, 6.4f);

        g.setColour (palette::gold);
        g.setFont (goldFont (12.0f, false, 0.22f));
        g.drawFittedText (label, labelArea.toNearestInt(), juce::Justification::centred, 1);
    }

private:
    juce::String label;
};

class GoldToggle : public juce::Button
{
public:
    GoldToggle (const juce::String& name, bool liveFileCaption)
        : juce::Button (name), showLiveFile (liveFileCaption)
    {
        setClickingTogglesState (true);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto labelArea = bounds.removeFromTop (18.0f);
        bounds.removeFromTop (6.0f);
        auto switchArea = bounds.removeFromTop (28.0f).withSizeKeepingCentre (64.0f, 26.0f);

        g.setColour (palette::gold);
        g.setFont (goldFont (12.0f, false, 0.24f));
        g.drawFittedText (getButtonText(), labelArea.toNearestInt(), juce::Justification::centred, 1);

        const bool on = getToggleState();
        if (on)
        {
            g.setColour (palette::gold.withAlpha (0.28f));
            g.fillRoundedRectangle (switchArea.expanded (4.0f), 16.0f);
        }

        g.setColour (on ? juce::Colour (0xff3a2410) : palette::ink);
        g.fillRoundedRectangle (switchArea, 13.0f);
        g.setColour ((on || highlighted) ? palette::gold : palette::goldDim);
        g.drawRoundedRectangle (switchArea, 13.0f, 1.3f);

        const auto track = switchArea.reduced (3.0f);
        const float thumbSize = track.getHeight();
        juce::Rectangle<float> thumb = on ? track.withX (track.getRight() - thumbSize).withWidth (thumbSize)
                                          : track.withWidth (thumbSize);
        if (down)
            thumb = thumb.reduced (1.0f);

        g.setColour (on ? palette::goldBright : palette::gold.withAlpha (highlighted ? 1.0f : 0.8f));
        g.fillEllipse (thumb);

        if (showLiveFile)
        {
            auto caption = bounds.withTrimmedTop (4.0f);
            auto liveArea = caption.removeFromLeft (caption.getWidth() * 0.42f);
            auto slashArea = caption.removeFromLeft (caption.getWidth() * 0.28f);
            g.setFont (goldFont (11.0f, false, 0.06f));
            g.setColour (on ? palette::goldDim : palette::goldBright);
            g.drawFittedText ("LIVE", liveArea.toNearestInt(), juce::Justification::centredRight, 1);
            g.setColour (palette::goldDim);
            g.drawFittedText ("/", slashArea.toNearestInt(), juce::Justification::centred, 1);
            g.setColour (on ? palette::goldBright : palette::goldDim);
            g.drawFittedText ("FILE", caption.toNearestInt(), juce::Justification::centredLeft, 1);
        }
    }

private:
    bool showLiveFile = false;
};

class GoldTapButton : public juce::Button
{
public:
    GoldTapButton()
        : juce::Button ("TAP")
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (down ? palette::gold.withAlpha (0.20f) : palette::ink.withAlpha (0.72f));
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (down || highlighted ? palette::goldBright : palette::gold);
        g.drawRoundedRectangle (bounds, 8.0f, down ? 2.0f : 1.45f);

        if (down)
        {
            g.setColour (palette::gold.withAlpha (0.18f));
            g.fillRoundedRectangle (bounds.reduced (3.0f), 6.0f);
        }

        g.setColour (palette::goldBright);
        g.setFont (goldFont (15.0f, true, 0.24f));
        g.drawFittedText ("TAP", bounds.toNearestInt(), juce::Justification::centred, 1);
    }
};

} // namespace awk
