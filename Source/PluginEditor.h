#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class CrushLookAndFeel;

//==============================================================================
class ScopeComponent  : public juce::Component
{
public:
    explicit ScopeComponent (HyperCrushProcessor& p) : processor (p)
    {
        displayBuffer.resize (HyperCrushProcessor::SCOPE_SIZE, 0.0f);
    }

    void updateScope();
    void paint (juce::Graphics&) override;

    float idlePhase = 0.0f;

private:
    HyperCrushProcessor& processor;
    std::vector<float> displayBuffer;
};

//==============================================================================
struct PresetEntry
{
    juce::String name;
    float bitDepth, downsample, drive;
    bool clipPre;
    float hpfCutoff, lpfCutoff, filterReso;
    float stutterRate, stutterDepth, dryWet;
};

//==============================================================================
class HyperCrushEditor  : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit HyperCrushEditor (HyperCrushProcessor&);
    ~HyperCrushEditor() override;

    void paint   (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void applyPreset (int index);

    HyperCrushProcessor& processorRef;

    std::unique_ptr<CrushLookAndFeel> lnf;
    ScopeComponent scopeComponent;

    // Glitch animation state
    juce::Random rng;
    int glitchCounter = 0;
    float scanlineOffset = 0.0f;
    struct GlitchBar { int y, h; float offset; juce::Colour colour; };
    std::vector<GlitchBar> glitchBars;

    // Preset dropdown
    juce::ComboBox presetBox;
    static const std::vector<PresetEntry>& getPresets();


    // Macro knobs
    juce::Slider glitchSlider, meltSlider, rektSlider, vibeSlider;
    juce::Label  glitchLabel,  meltLabel,  rektLabel,  vibeLabel;

    // CRUSH
    juce::Slider bitDepthSlider, downsampleSlider;
    juce::Label  bitDepthLabel,  downsampleLabel;

    // DRIVE
    juce::Slider       driveSlider;
    juce::Label        driveLabel;
    juce::ToggleButton clipPreButton;
    juce::Label        clipPreLabel;

    // FILTER
    juce::Slider hpfCutoffSlider, lpfCutoffSlider, filterResoSlider;
    juce::Label  hpfCutoffLabel,  lpfCutoffLabel,  filterResoLabel;

    // GATE
    juce::Slider stutterRateSlider, stutterDepthSlider;
    juce::Label  stutterRateLabel,  stutterDepthLabel;

    // MIX
    juce::Slider dryWetSlider;
    juce::Label  dryWetLabel;

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        bitDepthAtt, downsampleAtt, driveAtt,
        hpfCutoffAtt, lpfCutoffAtt, filterResoAtt,
        stutterRateAtt, stutterDepthAtt, dryWetAtt,
        glitchAtt, meltAtt, rektAtt, vibeAtt;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        clipPreAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HyperCrushEditor)
};
