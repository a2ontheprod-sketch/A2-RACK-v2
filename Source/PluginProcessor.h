#pragma once
#include <JuceHeader.h>
#include "ShapeDSP.h"
#include <array>
#include <atomic>
#include <memory>

class A2RackProcessor final : public juce::AudioProcessor
{
public:
    A2RackProcessor();
    ~A2RackProcessor() override = default;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "A2 RACK"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "A2 RACK"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    juce::AudioProcessorValueTreeState apvts;

    static constexpr int scopePoints = 1024;
    void copyScope (std::array<float, scopePoints>& destination) const noexcept;
    float getPeakL() const noexcept { return peakLeft.load (std::memory_order_relaxed); }
    float getPeakR() const noexcept { return peakRight.load (std::memory_order_relaxed); }
    void setCaptureFrozen (bool value) noexcept { captureFrozen.store (value); }
    bool isCaptureFrozen() const noexcept { return captureFrozen.load(); }
    juce::File exportLastTwoSeconds(); // non real-time: only call from message thread

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::SmoothedValue<float> foldSmooth, driveSmooth, slopeSmooth, outputSmooth;
    std::atomic<float>* foldParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* slopeParam = nullptr;
    std::atomic<float>* enabledParam = nullptr;
    std::array<std::atomic<float>, scopePoints> scope {};
    std::atomic<uint64_t> scopeWrite { 0 };
    int decimationCounter = 0;
    std::atomic<float> peakLeft { 0.0f }, peakRight { 0.0f };

    // Atomics avoid any lock contention between FL Studio's real-time audio
    // thread and the UI / WAV-export thread. The buffer stores processed stereo.
    std::unique_ptr<std::atomic<float>[]> captureL, captureR;
    int captureCapacity = 0;
    std::atomic<uint64_t> captureWrite { 0 };
    std::atomic<int> captureCount { 0 };
    std::atomic<bool> captureFrozen { false };
    double captureSampleRate = 44100.0;
    void pushCapture (float l, float r) noexcept;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (A2RackProcessor)
};
