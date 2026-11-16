#include "PluginEditor.h"
#include <cmath>

namespace
{
const auto black = juce::Colour (0xff070808);
const auto panel = juce::Colour (0xff0f1012);
const auto steel = juce::Colour (0xff373838);
const auto white = juce::Colour (0xffe2e1dc);
const auto muted = juce::Colour (0xff92918d);
const auto red = juce::Colour (0xffb52829);
const auto green = juce::Colour (0xff35aa65);

void drawScrew (juce::Graphics& g, float x, float y)
{
    g.setColour (juce::Colour (0xff5b5a54));
    g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
    g.setColour (juce::Colour (0xff242421));
    g.drawLine (x - 2.0f, y + 1.0f, x + 2.0f, y - 1.0f, 1.2f);
}

void drawOutlinedA2 (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    juce::Font logo (juce::FontOptions (69.0f, juce::Font::bold));
    juce::GlyphArrangement glyphs;
    glyphs.addFittedText (logo, "A2", bounds.getX(), bounds.getY(), bounds.getWidth(),
                          bounds.getHeight(), juce::Justification::centred, 1);
    juce::Path outline;
    glyphs.createPath (outline);
    g.setColour (juce::Colour (0xff777570));
    g.strokePath (outline, juce::PathStrokeType (1.2f));
}
}

void A2Look::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                float position, float start, float end, juce::Slider& slider)
{
    const float size = static_cast<float> (juce::jmin (width, height));
    const float cx = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const float cy = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    const float r = size * 0.365f;
    const bool isDrive = slider.getName() == "DRIVE";
    const bool isSlope = slider.getName() == "SLOPE";
    const auto bodyColour = isDrive ? red : (isSlope ? juce::Colour (0xff111315) : juce::Colour (0xff202224));

    // Outer radial tick marks, around the knob, imitate rack hardware markings.
    for (int tick = 0; tick <= 28; ++tick)
    {
        const auto angle = start + (end - start) * static_cast<float> (tick) / 28.0f;
        const bool major = tick % 4 == 0;
        const float inner = r + (major ? 10.0f : 12.0f);
        const float outer = r + (major ? 18.0f : 15.0f);
        g.setColour (major ? juce::Colour (0xffb6b4ad) : juce::Colour (0xff585955));
        g.drawLine (cx + std::sin (angle) * inner, cy - std::cos (angle) * inner,
                    cx + std::sin (angle) * outer, cy - std::cos (angle) * outer,
                    major ? 1.25f : 0.65f);
    }

    g.setColour (juce::Colour (0xff040505));
    g.fillEllipse (cx - r - 5.0f, cy - r - 4.0f, (r + 5.0f) * 2.0f, (r + 5.0f) * 2.0f);
    g.setColour (juce::Colour (0xff53514d));
    g.drawEllipse (cx - r - 4.0f, cy - r - 3.0f, (r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f, 1.0f);

    juce::ColourGradient metal (
        bodyColour.brighter (isDrive ? 0.35f : 0.27f), cx - r, cy - r,
        bodyColour.darker (0.50f), cx + r, cy + r, false);
    metal.addColour (0.5, bodyColour);
    g.setGradientFill (metal);
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    g.setColour (juce::Colour (0xff020303).withAlpha (0.55f));
    g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 2.0f);

    // Drive = flat red knob. Others have a central grip/shaft stripe.
    if (! isDrive)
    {
        g.setColour (juce::Colour (0xff050607).withAlpha (0.5f));
        g.fillRoundedRectangle (cx - 12.0f, cy - r + 8.0f, 24.0f, (r - 8.0f) * 2.0f, 5.0f);
    }
    const auto angle = start + (end - start) * juce::jlimit (0.0f, 1.0f, position);
    g.setColour (white);
    g.drawLine (cx + std::sin (angle) * r * 0.20f, cy - std::cos (angle) * r * 0.20f,
                cx + std::sin (angle) * r * 0.88f, cy - std::cos (angle) * r * 0.88f, 3.3f);
}

void A2Look::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                    const juce::Colour&, bool over, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool isPower = button.getName() == "POWER";
    const bool active = button.getToggleState();
    g.setColour (isPower ? black : (down ? juce::Colour (0xff444444) : juce::Colour (0xff202122)));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (over ? juce::Colour (0xff8d8c88) : juce::Colour (0xff4b4c4a));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    if (isPower)
    {
        const auto led = active ? red : juce::Colour (0xff501b1c);
        g.setColour (led);
        g.fillEllipse (r.getCentreX() - 7.0f, r.getCentreY() - 7.0f, 14.0f, 14.0f);
        g.setColour (juce::Colour (0xfff7c6c6).withAlpha (active ? 0.75f : 0.15f));
        g.drawEllipse (r.getCentreX() - 7.0f, r.getCentreY() - 7.0f, 14.0f, 14.0f, 1.5f);
    }
}

A2Waveform::A2Waveform (A2RackProcessor& p) : processor (p)
{
    setRepaintsOnMouseActivity (true);
    startTimerHz (30);
}

void A2Waveform::timerCallback()
{
    processor.copyScope (samples);
    repaint();
}

void A2Waveform::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff030405));
    g.fillAll();
    g.setColour (juce::Colour (0xffc4c2bd));
    g.drawRoundedRectangle (bounds.reduced (1.0f), 12.0f, 1.5f);

    auto plot = bounds.reduced (21.0f, 12.0f);
    plot.removeFromRight (24.0f);
    const auto center = plot.getCentreY();
    g.setColour (juce::Colour (0xff24282a));
    for (int i = 1; i <= 7; ++i)
    {
        const auto px = plot.getX() + plot.getWidth() * i / 8.0f;
        g.drawVerticalLine (juce::roundToInt (px), plot.getY(), plot.getBottom());
    }
    g.setColour (juce::Colour (0xff222629));
    g.drawHorizontalLine (juce::roundToInt (center), plot.getX(), plot.getRight());

    // Time-domain oscilloscope; every column shows min/max samples to avoid
    // losing brief snare and kick transients on a narrow plugin window.
    g.setColour (juce::Colour (0xffeeeeea));
    const int columns = juce::jmax (1, static_cast<int> (plot.getWidth()));
    const float sampleStep = static_cast<float> (samples.size()) / static_cast<float> (columns);
    for (int column = 0; column < columns; ++column)
    {
        const int first = juce::jlimit (0, static_cast<int> (samples.size()) - 1,
                                        static_cast<int> (column * sampleStep));
        const int last = juce::jlimit (first, static_cast<int> (samples.size()) - 1,
                                      static_cast<int> ((column + 1) * sampleStep));
        float lowest = 0.0f, highest = 0.0f;
        for (int i = first; i <= last; ++i)
        {
            lowest = std::min (lowest, samples[static_cast<size_t> (i)]);
            highest = std::max (highest, samples[static_cast<size_t> (i)]);
        }
        const float scale = plot.getHeight() * 0.43f;
        const float top = center - juce::jlimit (-1.0f, 1.0f, highest) * scale;
        const float bottom = center - juce::jlimit (-1.0f, 1.0f, lowest) * scale;
        const auto px = plot.getX() + column;
        g.drawLine (px, top, px, bottom + 0.7f, 1.0f);
    }
    g.setColour (processor.isCaptureFrozen() ? juce::Colours::orange : muted);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText (processor.isCaptureFrozen() ? "HOLD" : "LIVE", 12, 6, 48, 14, juce::Justification::centredLeft);
    g.drawText ("CLICK TO HOLD", 10, getHeight() - 22, 110, 14, juce::Justification::centredLeft);
    const float pkL = processor.getPeakL(), pkR = processor.getPeakR();
    g.setColour (muted);
    g.drawText ("L", getWidth() - 23, 19, 9, 16, juce::Justification::centred);
    g.drawText ("R", getWidth() - 23, 43, 9, 16, juce::Justification::centred);
    g.setColour (pkL >= 1.0f ? red : green);
    g.fillEllipse (static_cast<float> (getWidth() - 13), 22.0f, 5.0f, 5.0f);
    g.setColour (pkR >= 1.0f ? red : green);
    g.fillEllipse (static_cast<float> (getWidth() - 13), 46.0f, 5.0f, 5.0f);
}

void A2Waveform::mouseDown (const juce::MouseEvent&)
{
    processor.setCaptureFrozen (! processor.isCaptureFrozen());
    repaint();
}

A2RackEditor::A2RackEditor (A2RackProcessor& p)
    : AudioProcessorEditor (&p), processor (p), waveform (p)
{
    setLookAndFeel (&look);
    setSize (1000, 440);
    setResizable (false, false);
    addAndMakeVisible (waveform);
    addKnob (fold, "fold", 0.0f);
    addKnob (drive, "drive", 3.0f);
    addKnob (output, "output", -3.0f);
    addKnob (slope, "slope", 6.0f);
    drive.setName ("DRIVE");
    slope.setName ("SLOPE");
    fold.setName ("W-FOLD");
    output.setName ("OUTPUT");

    for (auto* b : { &menuButton, &previousButton, &nextButton, &favouriteButton,
                     &resetButton, &powerButton, &freezeButton, &exportButton })
    {
        b->setColour (juce::TextButton::textColourOffId, white);
        b->setColour (juce::TextButton::textColourOnId, white);
        addAndMakeVisible (*b);
    }
    powerButton.setName ("POWER");
    powerButton.setClickingTogglesState (true);
    powerBinding = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "enabled", powerButton);
    menuButton.onClick = [this] { showMenu(); };
    previousButton.onClick = [this] { applyPreset ((currentPreset + static_cast<int> (factoryPresets.size()) - 1) % static_cast<int> (factoryPresets.size())); };
    nextButton.onClick = [this] { applyPreset ((currentPreset + 1) % static_cast<int> (factoryPresets.size())); };
    resetButton.onClick = [this] { applyPreset (0); status = "RESET TO MASTER"; repaint(); };
    favouriteButton.onClick = [this] { rememberFavourite(); };
    freezeButton.onClick = [this] { processor.setCaptureFrozen (! processor.isCaptureFrozen()); };
    exportButton.onClick = [this] { exportAudio(); };
    startTimerHz (20);
}

A2RackEditor::~A2RackEditor()
{
    stopTimer();
    chooser.reset();
    setLookAndFeel (nullptr);
}

void A2RackEditor::addKnob (juce::Slider& slider, const juce::String& id, float original)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setDoubleClickReturnValue (true, original);
    slider.setScrollWheelEnabled (true);
    addAndMakeVisible (slider);
    knobBindings.emplace_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, id, slider));
}

void A2RackEditor::setFloatParam (const juce::String& id, float value)
{
    if (auto* parameter = processor.apvts.getParameter (id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        parameter->endChangeGesture();
    }
}

void A2RackEditor::applyPreset (int i)
{
    currentPreset = juce::jlimit (0, static_cast<int> (factoryPresets.size()) - 1, i);
    const auto& preset = factoryPresets[static_cast<size_t> (currentPreset)];
    setFloatParam ("fold", preset.fold);
    setFloatParam ("drive", preset.drive);
    setFloatParam ("output", preset.output);
    setFloatParam ("slope", preset.slope);
    status = juce::String (preset.name) + " LOADED";
    repaint();
}

void A2RackEditor::showMenu()
{
    juce::PopupMenu menu;
    for (int i = 0; i < static_cast<int> (factoryPresets.size()); ++i)
        menu.addItem (100 + i, factoryPresets[static_cast<size_t> (i)].name, true, currentPreset == i);
    menu.addSeparator();
    menu.addItem (1, "Save current preset to file...");
    menu.addItem (2, "Load preset from file...");
    menu.addItem (3, "Load quick save", favouriteFile().existsAsFile());
    menu.addItem (4, "Export last two seconds (WAV)");
    menu.addItem (5, "Reset to MASTER");
    auto safe = juce::Component::SafePointer<A2RackEditor> (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton),
                        [safe] (int choice)
    {
        if (safe == nullptr || choice == 0) return;
        if (choice >= 100) safe->applyPreset (choice - 100);
        else if (choice == 1) safe->saveSettings();
        else if (choice == 2) safe->loadSettings();
        else if (choice == 3) safe->restoreFavourite();
        else if (choice == 4) safe->exportAudio();
        else if (choice == 5) safe->applyPreset (0);
    });
}

juce::File A2RackEditor::favouriteFile() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("A2 RACK Presets").getChildFile ("quick-save.a2preset");
}

void A2RackEditor::rememberFavourite()
{
    auto file = favouriteFile();
    if (file.getParentDirectory().createDirectory().failed()) { status = "QUICK SAVE FAILED"; return; }
    juce::MemoryBlock mem;
    processor.getStateInformation (mem);
    status = file.replaceWithData (mem.getData(), mem.getSize()) ? "QUICK SAVED ★" : "QUICK SAVE FAILED";
    repaint();
}

void A2RackEditor::restoreFavourite()
{
    juce::MemoryBlock mem;
    if (favouriteFile().loadFileAsData (mem))
    {
        processor.setStateInformation (mem.getData(), static_cast<int> (mem.getSize()));
        status = "QUICK SAVE LOADED";
        currentPreset = 0;
    }
    else status = "NO QUICK SAVE";
    repaint();
}

void A2RackEditor::saveSettings()
{
    chooser = std::make_unique<juce::FileChooser> (
        "Save A2 preset", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                              .getChildFile ("A2-Master.a2preset"), "*.a2preset");
    auto safe = juce::Component::SafePointer<A2RackEditor> (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode |
                          juce::FileBrowserComponent::canSelectFiles |
                          juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto file = fc.getResult();
        if (file == juce::File{}) return;
        if (! file.hasFileExtension (".a2preset")) file = file.withFileExtension (".a2preset");
        juce::MemoryBlock mem;
        safe->processor.getStateInformation (mem);
        safe->status = file.replaceWithData (mem.getData(), mem.getSize()) ? "PRESET SAVED" : "SAVE FAILED";
        safe->repaint();
    });
}

void A2RackEditor::loadSettings()
{
    chooser = std::make_unique<juce::FileChooser> (
        "Load A2 preset", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.a2preset");
    auto safe = juce::Component::SafePointer<A2RackEditor> (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode |
                          juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto file = fc.getResult();
        if (file == juce::File{}) return;
        juce::MemoryBlock mem;
        if (file.loadFileAsData (mem))
        {
            safe->processor.setStateInformation (mem.getData(), static_cast<int> (mem.getSize()));
            safe->status = "PRESET LOADED";
        }
        else safe->status = "LOAD FAILED";
        safe->repaint();
    });
}

void A2RackEditor::exportAudio()
{
    const auto file = processor.exportLastTwoSeconds();
    status = file.existsAsFile() ? "WAV SAVED TO DOCUMENTS / A2 RACK EXPORTS" : "NO AUDIO TO EXPORT";
    repaint();
}

void A2RackEditor::timerCallback()
{
    freezeButton.setButtonText (processor.isCaptureFrozen() ? "RESUME" : "HOLD");
    favouriteButton.setButtonText (favouriteFile().existsAsFile() ? "★" : "☆");
    fold.setTooltip ("W-FOLD: " + juce::String (fold.getValue(), 1));
    drive.setTooltip ("DRIVE: " + juce::String (drive.getValue(), 1));
    output.setTooltip ("OUTPUT: " + juce::String (output.getValue(), 1) + " dB");
    slope.setTooltip ("SLOPE: " + juce::String (slope.getValue(), 1));
    repaint();
}

void A2RackEditor::paint (juce::Graphics& g)
{
    g.fillAll (black);
    // A2's own darker cabinet and side trims (not a copied skin or artwork).
    juce::ColourGradient bg (juce::Colour (0xff242322), 10, 0,
                             juce::Colour (0xff070809), 560, 440, false);
    g.setGradientFill (bg);
    g.fillRect (16, 4, 968, 431);
    for (int x : { 0, 985 })
    {
        g.setColour (juce::Colour (0xff563a29));
        g.fillRect (x, 0, 15, getHeight());
        g.setColour (juce::Colour (0xff997154).withAlpha (0.4f));
        g.drawVerticalLine (x + 7, 0.0f, static_cast<float> (getHeight()));
    }
    g.setColour (juce::Colour (0xff79766c));
    g.drawRoundedRectangle (19.0f, 6.0f, 962.0f, 426.0f, 5.0f, 1.5f);
    drawScrew (g, 34.0f, 18.0f);
    drawScrew (g, 966.0f, 18.0f);
    drawScrew (g, 34.0f, 420.0f);
    drawScrew (g, 966.0f, 420.0f);

    // Original LTA logo and global toolbar.
    g.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
    g.setColour (white);
    g.drawText ("LTA", 66, 11, 72, 27, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.setColour (muted);
    g.drawText ("AUDIO", 107, 30, 45, 13, juce::Justification::centredLeft);
    g.setColour (green);
    g.fillEllipse (294.0f, 26.0f, 9.0f, 9.0f);

    g.setColour (panel);
    g.fillRoundedRectangle (347.0f, 11.0f, 314.0f, 36.0f, 4.0f);
    g.setColour (steel);
    g.drawRoundedRectangle (347.0f, 11.0f, 314.0f, 36.0f, 4.0f, 1.0f);
    g.setColour (white);
    g.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    g.drawText (factoryPresets[static_cast<size_t> (currentPreset)].name, 401, 14, 209, 30,
                juce::Justification::centred);
    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("ON/OFF", 845, 19, 64, 20, juce::Justification::centred);

    // Center A2 lettering deliberately different from the source brand logo.
    drawOutlinedA2 (g, { 454.0f, 305.0f, 96.0f, 77.0f });
    g.setColour (juce::Colour (0xff92918c));
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("RACK  /  V2", 455, 382, 95, 17, juce::Justification::centred);
    g.setColour (white);
    g.setFont (juce::Font (juce::FontOptions (17.0f, juce::Font::bold)));
    g.drawText ("W-FOLD", 85, 394, 165, 23, juce::Justification::centred);
    g.drawText ("DRIVE", 249, 394, 165, 23, juce::Justification::centred);
    g.drawText ("OUTPUT", 598, 394, 165, 23, juce::Justification::centred);
    g.drawText ("SLOPE", 785, 394, 142, 23, juce::Justification::centred);

    // Knob readouts are drawn rather than using a second row of input boxes.
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.setColour (muted);


    g.setColour (juce::Colour (0xff6f7270));
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText (status, 38, 420, 690, 12, juce::Justification::centredLeft);
    g.drawText ("A2 RACK  /  ORIGINAL DSP", 753, 420, 205, 12, juce::Justification::centredRight);
}

void A2RackEditor::resized()
{
    waveform.setBounds (31, 59, 938, 203);
    menuButton.setBounds (241, 14, 35, 30);
    previousButton.setBounds (350, 15, 44, 29);
    nextButton.setBounds (613, 15, 43, 29);
    favouriteButton.setBounds (669, 15, 38, 29);
    resetButton.setBounds (716, 15, 38, 29);
    powerButton.setBounds (921, 15, 34, 29);
    freezeButton.setBounds (786, 225, 72, 27);
    exportButton.setBounds (869, 225, 74, 27);
    fold.setBounds (104, 275, 128, 120);
    drive.setBounds (269, 275, 128, 120);
    output.setBounds (614, 275, 128, 120);
    slope.setBounds (793, 285, 122, 105);
}
