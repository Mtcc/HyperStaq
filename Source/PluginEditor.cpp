#include "PluginEditor.h"

//==============================================================================
// Palette
static const juce::Colour kBg      (0xff0d0d0d);
static const juce::Colour kPanel   (0xff111111);
static const juce::Colour kAccent  (0xff00e5ff);
static const juce::Colour kTrack   (0xff262626);
static const juce::Colour kText    (0xff777777);
static const juce::Colour kDim     (0xff2a2a2a);
static const juce::Colour kBright  (0xffe0e0e0);

// Per-section accent tints (subtle — used only for section labels)
static const juce::Colour kColCrush  (0xff00e5ff);  // cyan
static const juce::Colour kColDrive  (0xffff4d6d);  // hot pink
static const juce::Colour kColFilter (0xffa78bfa);  // violet
static const juce::Colour kColGate   (0xff39ff85);  // neon green

//==============================================================================
class CustomLookAndFeel  : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour (juce::Slider::textBoxBackgroundColourId,  juce::Colour (0x00000000));
        setColour (juce::Slider::textBoxOutlineColourId,     juce::Colour (0x00000000));
        setColour (juce::Slider::textBoxTextColourId,        juce::Colour (0xff444444));
        setColour (juce::Slider::textBoxHighlightColourId,   kAccent.withAlpha (0.25f));
        setColour (juce::TextEditor::backgroundColourId,     juce::Colour (0xff1a1a1a));
        setColour (juce::TextEditor::textColourId,           kBright);
        setColour (juce::TextEditor::outlineColourId,        kAccent);
        setColour (juce::TextEditor::focusedOutlineColourId, kAccent);
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float startAngle, float endAngle,
                           juce::Slider& slider) override
    {
        const float cx = x + width  * 0.5f;
        const float cy = y + height * 0.5f;
        const float r  = juce::jmin (width, height) * 0.5f - 2.0f;
        if (r <= 1.0f) return;

        const float trackR = r - 3.0f;
        const float angle  = startAngle + sliderPos * (endAngle - startAngle);

        // Retrieve the per-section accent colour stored as a component property
        auto accentVar = slider.getProperties()["accentColour"];
        const juce::Colour accent = accentVar.isVoid()
            ? kAccent
            : juce::Colour ((juce::uint32)(int64_t) accentVar);

        // Track
        {
            juce::Path t;
            t.addCentredArc (cx, cy, trackR, trackR, 0.0f, startAngle, endAngle, true);
            g.setColour (kTrack);
            g.strokePath (t, juce::PathStrokeType (2.0f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Value arc
        if (sliderPos > 0.0001f)
        {
            juce::Path a;
            a.addCentredArc (cx, cy, trackR, trackR, 0.0f, startAngle, angle, true);
            g.setColour (accent);
            g.strokePath (a, juce::PathStrokeType (2.0f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Body
        const float bodyR = trackR - 5.0f;
        if (bodyR > 0.0f)
        {
            g.setColour (juce::Colour (0xff161616));
            g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
            g.setColour (juce::Colour (0xff202020));
            g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

            // Indicator dot
            const float dotR    = 1.8f;
            const float dotDist = juce::jmax (0.0f, bodyR - 5.0f);
            const float dotX    = cx + std::sin (angle) * dotDist;
            const float dotY    = cy - std::cos (angle) * dotDist;
            g.setColour (accent.withAlpha (0.3f));
            g.fillEllipse (dotX - dotR * 2.f, dotY - dotR * 2.f, dotR * 4.f, dotR * 4.f);
            g.setColour (accent);
            g.fillEllipse (dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
        }
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& btn,
                           bool /*highlighted*/, bool /*down*/) override
    {
        const bool on = btn.getToggleState();
        auto b = btn.getLocalBounds().toFloat().reduced (3.0f);

        // Retrieve accent colour
        auto accentVar = btn.getProperties()["accentColour"];
        const juce::Colour accent = accentVar.isVoid()
            ? kColDrive
            : juce::Colour ((juce::uint32)(int64_t) accentVar);

        g.setColour (on ? accent.withAlpha (0.12f) : kBg);
        g.fillRoundedRectangle (b, 4.0f);
        g.setColour (on ? accent : kDim);
        g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (on ? accent : kText);
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        g.drawText (on ? "PRE" : "POST", btn.getLocalBounds(), juce::Justification::centred);
    }
};

//==============================================================================
// Layout
static constexpr int kTitleH = 42;
static constexpr int kNumCols = 4;

struct SectionInfo { const char* label; juce::Colour colour; };
static const SectionInfo kSections[kNumCols] = {
    { "CRUSH",  kColCrush  },
    { "DRIVE",  kColDrive  },
    { "FILTER", kColFilter },
    { "GATE",   kColGate   },
};

//==============================================================================
BitCrusherEditor::BitCrusherEditor (BitCrusherProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setSize (600, 400);
    setResizable (false, false);

    lnf = std::make_unique<CustomLookAndFeel>();

    // Helper to set up a rotary slider with a per-section accent colour
    auto setupSlider = [&](juce::Slider& sl, juce::Label& lb,
                           const juce::String& txt, juce::Colour accent)
    {
        sl.setSliderStyle   (juce::Slider::RotaryVerticalDrag);
        sl.setTextBoxStyle  (juce::Slider::TextBoxBelow, false, 56, 16);
        sl.setLookAndFeel   (lnf.get());
        sl.getProperties().set ("accentColour", (int64_t)(juce::uint32) accent.getARGB());
        addAndMakeVisible (sl);

        lb.setText              (txt, juce::dontSendNotification);
        lb.setJustificationType (juce::Justification::centred);
        lb.setColour            (juce::Label::textColourId, kText);
        lb.setFont              (juce::FontOptions (10.0f).withStyle ("Bold"));
        addAndMakeVisible (lb);
    };

    setupSlider (bitDepthSlider,    bitDepthLabel,    "BIT DEPTH",    kColCrush);
    setupSlider (downsampleSlider,  downsampleLabel,  "SAMPLE RATE",  kColCrush);
    setupSlider (driveSlider,       driveLabel,       "DRIVE",        kColDrive);
    setupSlider (hpfCutoffSlider,   hpfCutoffLabel,   "HP CUTOFF",    kColFilter);
    setupSlider (hpfResoSlider,     hpfResoLabel,     "HP RESO",      kColFilter);
    setupSlider (lpfCutoffSlider,   lpfCutoffLabel,   "LP CUTOFF",    kColFilter);
    setupSlider (lpfResoSlider,     lpfResoLabel,     "LP RESO",      kColFilter);
    setupSlider (stutterRateSlider, stutterRateLabel, "RATE",         kColGate);
    setupSlider (stutterDepthSlider,stutterDepthLabel,"DEPTH",        kColGate);

    // PRE/POST toggle button
    clipPreButton.getProperties().set ("accentColour", (int64_t)(juce::uint32) kColDrive.getARGB());
    clipPreButton.setLookAndFeel (lnf.get());
    addAndMakeVisible (clipPreButton);
    clipPreLabel.setText            ("CLIP", juce::dontSendNotification);
    clipPreLabel.setJustificationType (juce::Justification::centred);
    clipPreLabel.setColour          (juce::Label::textColourId, kText);
    clipPreLabel.setFont            (juce::FontOptions (10.0f).withStyle ("Bold"));
    addAndMakeVisible (clipPreLabel);

    auto& av = processorRef.apvts;
    bitDepthAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "bitDepth",     bitDepthSlider);
    downsampleAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "downsample",   downsampleSlider);
    driveAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "drive",        driveSlider);
    hpfCutoffAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "hpfCutoff",   hpfCutoffSlider);
    hpfResoAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "hpfReso",     hpfResoSlider);
    lpfCutoffAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "lpfCutoff",   lpfCutoffSlider);
    lpfResoAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "lpfReso",     lpfResoSlider);
    stutterRateAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "stutterRate", stutterRateSlider);
    stutterDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "stutterDepth",stutterDepthSlider);
    clipPreAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (av, "clipPre",     clipPreButton);
}

BitCrusherEditor::~BitCrusherEditor()
{
    for (auto* sl : { &bitDepthSlider, &downsampleSlider, &driveSlider,
                      &hpfCutoffSlider, &hpfResoSlider,
                      &lpfCutoffSlider, &lpfResoSlider,
                      &stutterRateSlider, &stutterDepthSlider })
        sl->setLookAndFeel (nullptr);
    clipPreButton.setLookAndFeel (nullptr);
}

//==============================================================================
void BitCrusherEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    // Title bar
    g.setColour (kPanel);
    g.fillRect (0, 0, getWidth(), kTitleH);
    g.setColour (kAccent.withAlpha (0.5f));
    g.fillRect (0, kTitleH - 1, getWidth(), 1);
    g.setColour (kBright);
    g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
    g.drawText ("BITCRUSHER", juce::Rectangle<int> (0, 0, 300, kTitleH),
                juce::Justification::centred);
    g.setColour (kText);
    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.drawText ("hyperpop edition", juce::Rectangle<int> (300, 0, 300, kTitleH),
                juce::Justification::centred);

    // Column separators + section labels
    const int colW     = getWidth() / kNumCols;
    const int bodyTop  = kTitleH;
    const int bodyH    = getHeight() - bodyTop;
    const int labelH   = 20;

    for (int col = 0; col < kNumCols; ++col)
    {
        const int x = col * colW;

        // Separator line (skip leftmost)
        if (col > 0)
        {
            g.setColour (kDim);
            g.fillRect (x, bodyTop + 6, 1, bodyH - 12);
        }

        // Section label
        const auto& sec = kSections[col];
        g.setColour (sec.colour.withAlpha (0.8f));
        g.setFont   (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.drawText  (sec.label,
                     juce::Rectangle<int> (x, bodyTop + 6, colW, labelH),
                     juce::Justification::centred);
    }
}

void BitCrusherEditor::resized()
{
    const int colW    = getWidth() / kNumCols;   // 150
    const int bodyTop = kTitleH + 26;             // below section labels
    const int bodyH   = getHeight() - bodyTop - 4;
    const int halfH   = bodyH / 2;
    const int lblH    = 14;
    const int pad     = 12;

    // Place a knob: its label sits above, slider fills the rest
    auto placeKnob = [&](juce::Slider& sl, juce::Label& lb, int col, int row)
    {
        auto cell = juce::Rectangle<int> (col * colW, bodyTop + row * halfH, colW, halfH)
                        .reduced (pad, 4);
        lb.setBounds (cell.removeFromTop (lblH));
        sl.setBounds (cell);
    };

    // Col 0 — CRUSH
    placeKnob (bitDepthSlider,    bitDepthLabel,    0, 0);
    placeKnob (downsampleSlider,  downsampleLabel,  0, 1);

    // Col 1 — DRIVE (knob top, PRE/POST toggle bottom)
    placeKnob (driveSlider, driveLabel, 1, 0);
    {
        auto cell = juce::Rectangle<int> (colW, bodyTop + halfH, colW, halfH).reduced (pad, 4);
        clipPreLabel.setBounds  (cell.removeFromTop (lblH));
        clipPreButton.setBounds (cell.reduced (0, halfH / 4));
    }

    // Col 2 — FILTER (4 knobs, subdivided into quarters)
    {
        const int quarterH = bodyH / 4;
        auto filterKnob = [&](juce::Slider& sl, juce::Label& lb, int row)
        {
            auto cell = juce::Rectangle<int> (2 * colW, bodyTop + row * quarterH, colW, quarterH)
                            .reduced (pad, 2);
            lb.setBounds (cell.removeFromTop (lblH));
            sl.setBounds (cell);
        };
        filterKnob (hpfCutoffSlider, hpfCutoffLabel, 0);
        filterKnob (hpfResoSlider,   hpfResoLabel,   1);
        filterKnob (lpfCutoffSlider, lpfCutoffLabel, 2);
        filterKnob (lpfResoSlider,   lpfResoLabel,   3);
    }

    // Col 3 — GATE
    placeKnob (stutterRateSlider,  stutterRateLabel,  3, 0);
    placeKnob (stutterDepthSlider, stutterDepthLabel, 3, 1);
}
