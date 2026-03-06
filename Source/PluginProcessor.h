#pragma once

#include <JuceHeader.h>

//==============================================================================
class BitCrusherProcessor  : public juce::AudioProcessor,
                             public juce::AudioProcessorValueTreeState::Listener
{
public:
    BitCrusherProcessor();
    ~BitCrusherProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    int getNumPrograms()    override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<bool> isPrepared { false };

private:
    static constexpr int MAX_CHANNELS = 2;

    // Bitcrusher per-channel state
    float heldSample[MAX_CHANNELS]    = {};
    int   sampleCounter[MAX_CHANNELS] = {};

    // HPF biquad state — direct form II transposed, per channel
    double hpfZ1[MAX_CHANNELS] = {};
    double hpfZ2[MAX_CHANNELS] = {};

    // Stutter: single shared phase (L/R gate in lockstep)
    double stutterPhase = 0.0;

    double currentSampleRate = 44100.0;

    // Parameter atomics
    std::atomic<float> bitDepth     { 16.0f };
    std::atomic<float> downsample   {  1.0f };
    std::atomic<float> drive        {  0.0f };
    std::atomic<bool>  clipPre      { true  };
    std::atomic<float> hpfCutoff    { 20.0f };
    std::atomic<float> hpfReso      {  0.707f };
    std::atomic<float> stutterRate  {  3.0f };
    std::atomic<float> stutterDepth {  0.0f };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BitCrusherProcessor)
};
