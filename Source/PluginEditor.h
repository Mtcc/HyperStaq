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

private:
    HyperCrushProcessor& processor;
    std::vector<float> displayBuffer;
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

    HyperCrushProcessor& processorRef;

    std::unique_ptr<CrushLookAndFeel> lnf;
    ScopeComponent scopeComponent;

    // Glitch animation state
    juce::Random rng;
    int glitchCounter = 0;
    struct GlitchBar { int y, h; float offset; juce::Colour colour; };
    std::vector<GlitchBar> glitchBars;

    // CRUSH
    juce::Slider bitDepthSlider,   downsampleSlider;
    juce::Label  bitDepthLabel,    downsampleLabel;

    // DRIVE
    juce::Slider       driveSlider;
    juce::Label        driveLabel;
    juce::ToggleButton clipPreButton;
    juce::Label        clipPreLabel;

    // FILTER
    juce::Slider hpfCutoffSlider, hpfResoSlider, lpfCutoffSlider, lpfResoSlider;
    juce::Label  hpfCutoffLabel,  hpfResoLabel,  lpfCutoffLabel,  lpfResoLabel;

    // GATE
    juce::Slider stutterRateSlider, stutterDepthSlider;
    juce::Label  stutterRateLabel,  stutterDepthLabel;

    // MIX
    juce::Slider dryWetSlider;
    juce::Label  dryWetLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        bitDepthAttachment, downsampleAttachment,
        driveAttachment,
        hpfCutoffAttachment, hpfResoAttachment,
        lpfCutoffAttachment, lpfResoAttachment,
        stutterRateAttachment, stutterDepthAttachment,
        dryWetAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        clipPreAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HyperCrushEditor)
};
