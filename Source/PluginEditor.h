#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class A2Look final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float,
                           juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool pressed) override;
};

class A2Waveform final : public juce::Component, private juce::Timer
{
public:
    explicit A2Waveform (A2RackProcessor&);
    ~A2Waveform() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    A2RackProcessor& processor;
    std::array<float, A2RackProcessor::scopePoints> samples {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (A2Waveform)
};

class A2RackEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit A2RackEditor (A2RackProcessor&);
    ~A2RackEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Preset { const char* name; float fold, drive, output, slope; };
    static constexpr std::array<Preset, 9> factoryPresets {{
        { "MASTER",       0.0f, 3.0f, -3.0f, 6.0f },
        { "SOFT LIMIT",   0.0f, 2.3f, -2.0f, 0.5f },
        { "808 PRESSURE",  0.8f, 5.0f, -5.0f, 6.5f },
        { "BRICK WALL",   0.0f, 6.4f, -6.0f, 9.8f },
        { "REDLINE",      1.8f, 7.4f, -8.0f, 8.5f },
        { "FOLDBACK",     7.5f, 3.7f, -5.5f, 5.0f },
        { "CRUNCH",       3.2f, 5.5f, -6.5f, 7.0f },
        { "WARM PUSH",    0.3f, 2.1f, -1.5f, 2.5f },
        { "DRUM SMASH",   4.2f, 8.0f, -9.0f, 8.8f }
    }};

    void timerCallback() override;
    void addKnob (juce::Slider&, const juce::String& id, float defaultValue);
    void applyPreset (int);
    void setFloatParam (const juce::String& id, float value);
    void showMenu();
    void saveSettings();
    void loadSettings();
    void rememberFavourite();
    void restoreFavourite();
    void exportAudio();
    juce::File favouriteFile() const;

    A2RackProcessor& processor;
    A2Look look;
    A2Waveform waveform;
    juce::Slider fold, drive, output, slope;
    juce::TextButton menuButton { "≡" };
    juce::TextButton previousButton { "◀" };
    juce::TextButton nextButton { "▶" };
    juce::TextButton favouriteButton { "☆" };
    juce::TextButton resetButton { "↺" };
    juce::TextButton powerButton { "" };
    juce::TextButton freezeButton { "HOLD" };
    juce::TextButton exportButton { "WAV" };
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobBindings;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerBinding;
    std::unique_ptr<juce::FileChooser> chooser;
    int currentPreset = 0;
    juce::String status { "A2 V2 // READY" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (A2RackEditor)
};
