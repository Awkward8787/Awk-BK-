// Awk BK — pitch and BPM detector
// Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PluginEditor.h"

#include "NoteUtils.h"

#include <BinaryData.h>
#include <cmath>

namespace
{
juce::String formatCents (float cents)
{
    int rounded = (int) std::lround (cents);
    rounded = juce::jlimit (-50, 50, rounded);
    const int mag = std::abs (rounded);
    auto text = (rounded < 0 ? "-" : "+") + juce::String (mag).paddedLeft ('0', 2);
    return text + " cents";
}

juce::String formatNoteText (int midi)
{
    if (midi < 0 || midi > 127)
        return "--";
    return juce::String (awk::pitchClassName (midi % 12)) + juce::String (midi / 12 - 1);
}

void drawScrew (juce::Graphics& g, float x, float y)
{
    juce::Rectangle<float> screw (x, y, 11.0f, 11.0f);
    g.setColour (awk::palette::goldDeep);
    g.fillEllipse (screw);
    g.setColour (awk::palette::gold);
    g.drawEllipse (screw, 1.1f);
    g.drawLine (screw.getX() + 2.0f, screw.getCentreY(), screw.getRight() - 2.0f, screw.getCentreY(), 1.0f);
}
} // namespace

AwkBKEditor::AwkBKEditor (AwkBKProcessor& proc)
    : AudioProcessorEditor (&proc), processor (proc)
{
    setOpaque (true);
    setResizable (false, false);

    logo = juce::ImageFileFormat::loadFrom (BinaryData::AwkBeatsLogo_png,
                                            BinaryData::AwkBeatsLogo_pngSize);

    addAndMakeVisible (tapButton);
    addAndMakeVisible (lockToggle);
    addAndMakeVisible (sensKnob);
    addAndMakeVisible (windowKnob);
    addAndMakeVisible (modeToggle);

    tapButton.onClick = [this] { processor.tap(); };
    tapButton.setTooltip ("Tap the tempo. Ignored while Lock is on.");
    lockToggle.setTooltip ("Freeze the pitch, key, and BPM readouts.");
    sensKnob.setTooltip ("Detection sensitivity. Higher accepts weaker pitch and softer onsets.");
    windowKnob.setTooltip ("Analysis window. Longer windows are steadier and slower to react.");
    modeToggle.setTooltip ("Live follows the input. File uses a longer window for bounced audio.");

    auto attachSlider = [this] (const char* paramId, juce::Slider& slider)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (processor.apvts.getParameter (paramId));
        jassert (param != nullptr);
        return std::make_unique<juce::SliderParameterAttachment> (*param, slider);
    };
    auto attachButton = [this] (const char* paramId, juce::Button& button)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (processor.apvts.getParameter (paramId));
        jassert (param != nullptr);
        return std::make_unique<juce::ButtonParameterAttachment> (*param, button);
    };

    sensAttachment = attachSlider ("sensitivity", sensKnob);
    windowAttachment = attachSlider ("window", windowKnob);
    lockAttachment = attachButton ("lock", lockToggle);
    modeAttachment = attachButton ("fileMode", modeToggle);

    tooltip = std::make_unique<juce::TooltipWindow> (this, 700);
    setSize (720, 760);
    startTimerHz (30);
}

AwkBKEditor::~AwkBKEditor()
{
    stopTimer();
}

void AwkBKEditor::timerCallback()
{
    repaint();
}

AwkBKEditor::Layout AwkBKEditor::makeLayout() const
{
    Layout layout;
    const auto bounds = getLocalBounds();
    layout.header = { 0, 0, bounds.getWidth(), 92 };
    layout.footer = { 0, bounds.getHeight() - 46, bounds.getWidth(), 46 };
    layout.controls = { 12, bounds.getHeight() - 46 - 178, bounds.getWidth() - 24, 170 };
    layout.glass = { 26, 98, bounds.getWidth() - 52, bounds.getHeight() - 98 - 46 - 178 - 18 };

    auto inner = layout.glass.toFloat().reduced (18.0f, 16.0f);
    layout.wave = inner.removeFromBottom (124.0f);
    inner.removeFromBottom (8.0f);
    layout.bpm = inner.removeFromLeft (inner.getWidth() * 0.46f);
    layout.pitch = inner.withTrimmedLeft (16.0f).withTrimmedTop (12.0f).withTrimmedBottom (8.0f);

    const int centreY = layout.controls.getCentreY() + 4;
    const int left = layout.controls.getX();
    const int width = layout.controls.getWidth();
    auto centreOf = [left, width] (int index)
    {
        return left + (width * (2 * index + 1)) / 10;
    };

    layout.tap = { centreOf (0) - 58, centreY - 23, 116, 46 };
    layout.lock = { centreOf (1) - 54, centreY - 40, 108, 74 };
    layout.sens = { centreOf (2) - 48, centreY - 50, 96, 116 };
    layout.window = { centreOf (3) - 48, centreY - 50, 96, 116 };
    layout.mode = { centreOf (4) - 68, centreY - 46, 136, 96 };
    return layout;
}

void AwkBKEditor::resized()
{
    const auto layout = makeLayout();
    tapButton.setBounds (layout.tap);
    lockToggle.setBounds (layout.lock);
    sensKnob.setBounds (layout.sens);
    windowKnob.setBounds (layout.window);
    modeToggle.setBounds (layout.mode);
}

void AwkBKEditor::paint (juce::Graphics& g)
{
    const auto layout = makeLayout();
    const auto readout = processor.getReadout();
    drawChassis (g);
    drawHeader (g, layout.header);
    drawGlass (g, layout, readout);
    drawFooter (g, layout.footer);
}

void AwkBKEditor::drawChassis (juce::Graphics& g) const
{
    juce::ColourGradient fill (awk::palette::chassisTop, 0.0f, 0.0f,
                               awk::palette::chassisBot, 0.0f, (float) getHeight(), false);
    fill.addColour (0.42, awk::palette::chassisMid);
    g.setGradientFill (fill);
    g.fillAll();

    g.setColour (awk::palette::gold.withAlpha (0.16f));
    g.fillRect (0.0f, 0.0f, (float) getWidth(), 2.0f);

    const auto frame = getLocalBounds().toFloat().reduced (9.0f);
    g.setColour (awk::palette::goldDeep.withAlpha (0.85f));
    g.drawRoundedRectangle (frame, 18.0f, 2.4f);
    g.setColour (awk::palette::gold.withAlpha (0.92f));
    g.drawRoundedRectangle (frame.reduced (1.2f), 17.0f, 1.15f);

    const float inset = 22.0f;
    drawScrew (g, inset, inset);
    drawScrew (g, (float) getWidth() - inset - 11.0f, inset);
    drawScrew (g, inset, (float) getHeight() - inset - 11.0f);
    drawScrew (g, (float) getWidth() - inset - 11.0f, (float) getHeight() - inset - 11.0f);
}

void AwkBKEditor::drawHeader (juce::Graphics& g, juce::Rectangle<int> header) const
{
    auto row = header.reduced (24, 16);
    const int logoSize = 52;
    auto logoBounds = row.removeFromLeft (logoSize).withSizeKeepingCentre (logoSize, logoSize);
    if (logo.isValid())
        g.drawImageWithin (logo, logoBounds.getX(), logoBounds.getY(), logoSize, logoSize,
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                           false);

    row.removeFromLeft (12);
    g.setFont (awk::goldFont (22.0f, true, 0.14f));
    g.setColour (awk::palette::goldBright);
    const int nameWidth = 148;
    g.drawFittedText ("AWKBEATS", row.removeFromLeft (nameWidth), juce::Justification::centredLeft, 1);

    g.setColour (awk::palette::gold.withAlpha (0.85f));
    g.fillRect ((float) row.getX(), (float) row.getCentreY() - 11.0f, 1.4f, 22.0f);
    row.removeFromLeft (16);

    g.setFont (awk::goldFont (18.0f, false, 0.28f));
    g.setColour (awk::palette::gold);
    g.drawFittedText ("DETECT", row.removeFromLeft (120), juce::Justification::centredLeft, 1);
}

void AwkBKEditor::drawGlass (juce::Graphics& g, const Layout& layout, const awk::Readout& readout) const
{
    const auto glass = layout.glass.toFloat();

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (glass.translated (0.0f, 6.0f), 16.0f);

    juce::ColourGradient glassFill (awk::palette::glassTop, glass.getX(), glass.getY(),
                                    awk::palette::glassBot, glass.getX(), glass.getBottom(), false);
    g.setGradientFill (glassFill);
    g.fillRoundedRectangle (glass, 16.0f);

    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.14f), glass.getX(), glass.getY(),
                                juce::Colours::white.withAlpha (0.0f), glass.getX(), glass.getCentreY(), false);
    g.setGradientFill (sheen);
    {
        juce::Graphics::ScopedSaveState saved (g);
        juce::Path clip;
        clip.addRoundedRectangle (glass, 16.0f);
        g.reduceClipRegion (clip);
        g.fillRect (glass.withHeight (glass.getHeight() * 0.42f));
    }

    g.setColour (awk::palette::gold.withAlpha (0.78f));
    g.drawRoundedRectangle (glass, 16.0f, 1.4f);
    g.setColour (awk::palette::gold.withAlpha (0.22f));
    g.drawRoundedRectangle (glass.reduced (4.0f), 13.0f, 1.0f);

    drawBpm (g, layout.bpm, readout);
    drawPitch (g, layout.pitch, readout);
    drawWave (g, layout.wave.reduced (8.0f, 4.0f));
}

void AwkBKEditor::drawBpm (juce::Graphics& g, juce::Rectangle<float> area, const awk::Readout& readout) const
{
    const auto centre = area.getCentre();
    const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.42f;
    const float start = juce::MathConstants<float>::pi * 0.82f;
    const float end = juce::MathConstants<float>::pi * (0.52f + 2.0f);

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, start, end, true);
    g.setColour (awk::palette::goldDeep.withAlpha (0.55f));
    g.strokePath (arc, juce::PathStrokeType (6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (awk::palette::gold);
    g.strokePath (arc, juce::PathStrokeType (2.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (readout.haveTempo)
    {
        const float norm = juce::jlimit (0.0f, 1.0f, (readout.bpm - 60.0f) / 140.0f);
        const float angle = start + norm * (end - start);
        const float outer = radius + 1.0f;
        const float inner = radius - 14.0f;
        g.setColour (awk::palette::goldBright);
        g.drawLine (centre.x + std::sin (angle) * inner,
                    centre.y - std::cos (angle) * inner,
                    centre.x + std::sin (angle) * outer,
                    centre.y - std::cos (angle) * outer,
                    2.0f);
    }

    g.setColour (awk::palette::gold);
    g.setFont (awk::goldFont (13.0f, false, 0.32f));
    auto label = area.withTrimmedBottom (area.getHeight() * 0.55f);
    g.drawFittedText ("BPM", label.toNearestInt(), juce::Justification::centredBottom, 1);

    g.setColour (awk::palette::goldBright);
    g.setFont (awk::goldFont (52.0f, true, 0.02f));
    const auto number = readout.haveTempo ? juce::String (readout.bpm, 2) : juce::String ("---.--");
    g.drawFittedText (number, area.translated (0.0f, 10.0f).toNearestInt(), juce::Justification::centred, 1);
}

void AwkBKEditor::drawPitch (juce::Graphics& g, juce::Rectangle<float> area, const awk::Readout& readout) const
{
    auto row = area;
    g.setColour (awk::palette::goldBright);
    g.setFont (awk::goldFont (64.0f, true, 0.01f));
    g.drawFittedText (formatNoteText (readout.voiced ? readout.midiNote : -1),
                      row.removeFromTop (78.0f).toNearestInt(),
                      juce::Justification::centredLeft,
                      1);

    g.setColour (awk::palette::gold);
    g.setFont (awk::goldFont (20.0f, false, 0.04f));
    const auto cents = readout.voiced ? formatCents (readout.cents) : juce::String();
    g.drawFittedText (cents, row.removeFromTop (30.0f).toNearestInt(), juce::Justification::centredLeft, 1);

    row.removeFromTop (18.0f);

    auto drawPair = [&g, &row] (const juce::String& caption, const juce::String& value)
    {
        auto line = row.removeFromTop (28.0f);
        g.setColour (awk::palette::gold.withAlpha (0.78f));
        g.setFont (awk::goldFont (14.0f, false, 0.16f));
        g.drawFittedText (caption, line.removeFromLeft (58.0f).toNearestInt(), juce::Justification::centredLeft, 1);
        g.setColour (awk::palette::goldBright);
        g.setFont (awk::goldFont (16.0f, true, 0.08f));
        g.drawFittedText (value, line.toNearestInt(), juce::Justification::centredLeft, 1);
    };

    juce::String keyValue = "--";
    if (readout.haveKey && readout.keyPitchClass >= 0)
        keyValue = juce::String (awk::pitchClassName (readout.keyPitchClass))
                   + (readout.keyMinor ? " MINOR" : " MAJOR");

    const int percent = (int) std::lround (juce::jlimit (0.0f, 1.0f, readout.keyConfidence) * 100.0f);
    const auto confValue = readout.haveKey ? juce::String (percent) + "%" : juce::String ("--");

    drawPair ("KEY", keyValue);
    drawPair ("CONF", confValue);
}

void AwkBKEditor::drawWave (juce::Graphics& g, juce::Rectangle<float> area) const
{
    g.setColour (awk::palette::gold.withAlpha (0.12f));
    for (int i = 1; i < 4; ++i)
    {
        const float y = area.getY() + area.getHeight() * (float) i / 4.0f;
        g.drawHorizontalLine ((int) std::lround (y), area.getX(), area.getRight());
    }
    for (int i = 1; i < 8; ++i)
    {
        const float x = area.getX() + area.getWidth() * (float) i / 8.0f;
        g.drawVerticalLine ((int) std::lround (x), area.getY(), area.getBottom());
    }

    float samples[256];
    processor.copyWaveform (samples, 256);

    juce::Path path;
    const float mid = area.getCentreY();
    const float scale = area.getHeight() * 0.42f;
    for (int i = 0; i < 256; ++i)
    {
        const float x = area.getX() + area.getWidth() * (float) i / 255.0f;
        const float y = mid - juce::jlimit (-1.0f, 1.0f, samples[i]) * scale;
        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    g.setColour (awk::palette::gold);
    g.strokePath (path, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const int marker = processor.newestWaveformIndex();
    if (marker < 0 || marker >= 256)
        return;

    const float markerX = area.getX() + area.getWidth() * ((float) marker / 255.0f);
    const float markerY = mid - juce::jlimit (-1.0f, 1.0f, samples[marker]) * scale;
    g.setColour (awk::palette::gold.withAlpha (0.8f));
    g.drawLine (markerX, area.getY() + 8.0f, markerX, area.getBottom() - 8.0f, 1.0f);
    g.setColour (awk::palette::ink);
    g.fillEllipse (markerX - 5.0f, markerY - 5.0f, 10.0f, 10.0f);
    g.setColour (awk::palette::goldBright);
    g.fillEllipse (markerX - 3.6f, markerY - 3.6f, 7.2f, 7.2f);
}

void AwkBKEditor::drawFooter (juce::Graphics& g, juce::Rectangle<int> footer) const
{
    g.setColour (awk::palette::gold.withAlpha (0.45f));
    const float y = (float) footer.getY() + 4.0f;
    g.drawLine (28.0f, y, (float) getWidth() - 28.0f, y, 1.0f);

    g.setColour (awk::palette::gold);
    g.setFont (awk::goldFont (11.0f, false, 0.16f));
    const auto dot = juce::String ("  ") + juce::String::charToString ((juce::juce_wchar) 0x00B7) + "  ";
    const auto text = juce::String ("DETECT") + dot + "PITCH + BPM" + dot + "v"
                      + juce::String (JucePlugin_VersionString) + dot + "AWKBEATS";
    g.drawFittedText (text, footer.reduced (16, 8), juce::Justification::centred, 1);
}
