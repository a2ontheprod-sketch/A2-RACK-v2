#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

A2RackProcessor::A2RackProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "A2v2State", createParams())
{
    foldParam    = apvts.getRawParameterValue ("fold");
    driveParam   = apvts.getRawParameterValue ("drive");
    outputParam  = apvts.getRawParameterValue ("output");
    slopeParam   = apvts.getRawParameterValue ("slope");
    enabledParam = apvts.getRawParameterValue ("enabled");
    for (auto& s : scope) s.store (0.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout A2RackProcessor::createParams()
{
    using P = juce::AudioParameterFloat;
    using ID = juce::ParameterID;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<P> (ID ("fold", 1), "W-FOLD", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<P> (ID ("drive", 1), "DRIVE", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.0f));
    layout.add (std::make_unique<P> (ID ("output", 1), "OUTPUT dB", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), -3.0f));
    layout.add (std::make_unique<P> (ID ("slope", 1), "SLOPE", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 6.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID ("enabled", 1), "ON", true));
    return layout;
}

bool A2RackProcessor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto& out = layout.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == layout.getMainInputChannelSet();
}

void A2RackProcessor::prepareToPlay (double sr, int maxBlock)
{
    // 4x oversampling with half-band anti-alias filters makes aggressive
    // wavefolding less harsh in the high frequencies than naive sample-rate DSP.
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        static_cast<size_t> (juce::jmax (1, getTotalNumOutputChannels())), 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, true);
    oversampling->initProcessing (static_cast<size_t> (juce::jmax (maxBlock, 32)));
    oversampling->reset();
    setLatencySamples (juce::roundToInt (oversampling->getLatencyInSamples()));

    const double innerSr = sr * 4.0;
    foldSmooth.reset (innerSr, 0.018);
    driveSmooth.reset (innerSr, 0.018);
    outputSmooth.reset (innerSr, 0.018);
    slopeSmooth.reset (innerSr, 0.018);
    foldSmooth.setCurrentAndTargetValue (foldParam->load());
    driveSmooth.setCurrentAndTargetValue (driveParam->load());
    outputSmooth.setCurrentAndTargetValue (a2::dbToGain (outputParam->load()));
    slopeSmooth.setCurrentAndTargetValue (slopeParam->load());

    captureSampleRate = sr;
    captureCapacity = juce::jmax (1, static_cast<int> (std::ceil (sr * 2.0)));
    captureL = std::make_unique<std::atomic<float>[]> (static_cast<size_t> (captureCapacity));
    captureR = std::make_unique<std::atomic<float>[]> (static_cast<size_t> (captureCapacity));
    for (int i = 0; i < captureCapacity; ++i)
    {
        captureL[static_cast<size_t> (i)].store (0.0f, std::memory_order_relaxed);
        captureR[static_cast<size_t> (i)].store (0.0f, std::memory_order_relaxed);
    }
    captureWrite.store (0);
    captureCount.store (0);
    captureFrozen.store (false);
    scopeWrite.store (0);
    decimationCounter = 0;
    peakLeft.store (0.0f);
    peakRight.store (0.0f);
}

void A2RackProcessor::releaseResources()
{
    oversampling.reset();
}

void A2RackProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int channels = juce::jmin (2, buffer.getNumChannels());
    if (channels == 0 || buffer.getNumSamples() == 0) return;
    for (int i = getTotalNumInputChannels(); i < buffer.getNumChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (oversampling == nullptr) return;
    foldSmooth.setTargetValue (foldParam->load());
    driveSmooth.setTargetValue (driveParam->load());
    slopeSmooth.setTargetValue (slopeParam->load());
    outputSmooth.setTargetValue (a2::dbToGain (outputParam->load()));
    const bool on = enabledParam->load() > 0.5f;

    // The oversampler is used in both ON and OFF states to keep latency
    // consistent when the user toggles the power button.
    juce::dsp::AudioBlock<float> block (buffer);
    auto high = oversampling->processSamplesUp (block);
    const auto innerSamples = static_cast<int> (high.getNumSamples());
    for (int i = 0; i < innerSamples; ++i)
    {
        const auto fold = foldSmooth.getNextValue();
        const auto drive = driveSmooth.getNextValue();
        const auto slope = slopeSmooth.getNextValue();
        const auto output = outputSmooth.getNextValue();
        if (on)
            for (int ch = 0; ch < channels; ++ch)
            {
                auto* data = high.getChannelPointer (static_cast<size_t> (ch));
                data[i] = a2::processSample (data[i], fold, drive, slope, output);
            }
    }
    oversampling->processSamplesDown (block);

    const auto* l = buffer.getReadPointer (0);
    const auto* r = channels > 1 ? buffer.getReadPointer (1) : l;
    float maxL = 0.0f, maxR = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        maxL = juce::jmax (maxL, std::abs (l[i]));
        maxR = juce::jmax (maxR, std::abs (r[i]));
        pushCapture (l[i], r[i]);
    }
    peakLeft.store (maxL, std::memory_order_relaxed);
    peakRight.store (maxR, std::memory_order_relaxed);
}

void A2RackProcessor::pushCapture (float l, float r) noexcept
{
    if (! captureFrozen.load (std::memory_order_relaxed) && captureCapacity > 0)
    {
        const auto pos = captureWrite.load (std::memory_order_relaxed);
        const auto ix = static_cast<size_t> (pos % static_cast<uint64_t> (captureCapacity));
        captureL[ix].store (l, std::memory_order_relaxed);
        captureR[ix].store (r, std::memory_order_relaxed);
        captureWrite.store (pos + 1, std::memory_order_release);
        if (captureCount.load (std::memory_order_relaxed) < captureCapacity)
            captureCount.fetch_add (1, std::memory_order_relaxed);
    }
    // Display 1K samples/s, representing approximately 1 second history.
    if (captureFrozen.load (std::memory_order_relaxed)) return;
    const int stride = juce::jmax (1, static_cast<int> (captureSampleRate / 1000.0));
    if (++decimationCounter >= stride)
    {
        decimationCounter = 0;
        const auto pos = scopeWrite.load (std::memory_order_relaxed);
        scope[static_cast<size_t> (pos % scopePoints)].store ((l + r) * 0.5f,
                                                             std::memory_order_relaxed);
        scopeWrite.store (pos + 1, std::memory_order_release);
    }
}

void A2RackProcessor::copyScope (std::array<float, scopePoints>& dest) const noexcept
{
    const auto write = scopeWrite.load (std::memory_order_acquire);
    // Keep the oldest visible region on the left and newest on the right.
    for (int i = 0; i < scopePoints; ++i)
    {
        const auto available = static_cast<uint64_t> (scopePoints - 1 - i);
        if (write <= available)
            dest[static_cast<size_t> (i)] = 0.0f;
        else
        {
            const auto pos = write - available - 1;
            dest[static_cast<size_t> (i)] = scope[static_cast<size_t> (pos % scopePoints)].load (
                std::memory_order_relaxed);
        }
    }
}

juce::File A2RackProcessor::exportLastTwoSeconds()
{
    // Call from editor only. The capture is frozen to keep WAV data consistent.
    captureFrozen.store (true, std::memory_order_release);
    const int count = captureCount.load (std::memory_order_acquire);
    const auto write = captureWrite.load (std::memory_order_acquire);
    if (count < 1 || ! captureL || ! captureR) return {};

    auto directory = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                         .getChildFile ("A2 RACK Exports");
    if (directory.createDirectory().failed()) return {};
    auto target = directory.getNonexistentChildFile ("A2-RACK-v2-", ".wav", false);
    auto stream = target.createOutputStream();
    if (stream == nullptr) return {};

    // Float32 stereo WAV: avoids throwing away the headroom of overshoots.
    const auto frames = static_cast<juce::uint32> (count);
    const auto sr = static_cast<juce::uint32> (std::lround (captureSampleRate));
    const auto dataBytes = frames * 2u * 4u;
    stream->write ("RIFF", 4);
    stream->writeInt (static_cast<int> (36u + dataBytes));
    stream->write ("WAVEfmt ", 8);
    stream->writeInt (16);
    stream->writeShort (3); // IEEE float PCM
    stream->writeShort (2);
    stream->writeInt (static_cast<int> (sr));
    stream->writeInt (static_cast<int> (sr * 8u));
    stream->writeShort (8);
    stream->writeShort (32);
    stream->write ("data", 4);
    stream->writeInt (static_cast<int> (dataBytes));
    const auto first = write - static_cast<uint64_t> (count);
    for (int i = 0; i < count; ++i)
    {
        const auto ix = static_cast<size_t> ((first + static_cast<uint64_t> (i)) % static_cast<uint64_t> (captureCapacity));
        stream->writeFloat (captureL[ix].load (std::memory_order_relaxed));
        stream->writeFloat (captureR[ix].load (std::memory_order_relaxed));
    }
    stream->flush();
    return stream->getStatus().wasOk() ? target : juce::File{};
}

void A2RackProcessor::getStateInformation (juce::MemoryBlock& memory)
{
    juce::MemoryOutputStream stream (memory, false);
    apvts.copyState().writeToStream (stream);
}

void A2RackProcessor::setStateInformation (const void* data, int size)
{
    auto tree = juce::ValueTree::readFromData (data, static_cast<size_t> (size));
    if (tree.isValid()) apvts.replaceState (tree);
}

juce::AudioProcessorEditor* A2RackProcessor::createEditor()
{
    return new A2RackEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new A2RackProcessor();
}
