#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class CustomLookAndFeel;

//==============================================================================
class BitCrusherEditor  : public juce::AudioProcessorEditor
{
public:
    explicit BitCrusherEditor (BitCrusherProcessor&);
    ~BitCrusherEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    BitCrusherProcessor& processorRef;

    std::unique_ptr<CustomLookAndFeel> lnf;

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

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        bitDepthAttachment, downsampleAttachment,
        driveAttachment,
        hpfCutoffAttachment, hpfResoAttachment,
        lpfCutoffAttachment, lpfResoAttachment,
        stutterRateAttachment, stutterDepthAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        clipPreAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BitCrusherEditor)
};
