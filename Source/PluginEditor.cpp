#include "PluginEditor.h"

//==============================================================================
// Y2K Hyperpop Palette
static const juce::Colour kBg       (0xff08081a);
static const juce::Colour kPanel    (0xff0c0c22);
static const juce::Colour kHotPink  (0xffff2d95);
static const juce::Colour kCyan     (0xff00ffff);
static const juce::Colour kPurple   (0xff9b59f5);
static const juce::Colour kText     (0xff667788);
static const juce::Colour kDim      (0xff1a1a33);
static const juce::Colour kBright   (0xffe0e0ff);
static const juce::Colour kTrack    (0xff151528);
static const juce::Colour kScopeBg  (0xff060612);

// Per-section accent colours
static const juce::Colour kColCrush  = kCyan;
static const juce::Colour kColDrive  = kHotPink;
static const juce::Colour kColFilter = kPurple;
static const juce::Colour kColGate   = kCyan;
static const juce::Colour kColMix    = kHotPink;

//==============================================================================
// CRT-style monospace font helper
static juce::Font crtFont (float size, bool bold = false)
{
    auto opts = juce::FontOptions (size).withName ("Menlo");
    if (bold) opts = opts.withStyle ("Bold");
    return juce::Font (opts);
}

//==============================================================================
class CrushLookAndFeel  : public juce::LookAndFeel_V4
{
public:
    CrushLookAndFeel()
    {
        setColour (juce::Slider::textBoxBackgroundColourId,  juce::Colour (0x00000000));
        setColour (juce::Slider::textBoxOutlineColourId,     juce::Colour (0x00000000));
        setColour (juce::Slider::textBoxTextColourId,        juce::Colour (0xff445566));
        setColour (juce::Slider::textBoxHighlightColourId,   kCyan.withAlpha (0.2f));
        setColour (juce::TextEditor::backgroundColourId,     juce::Colour (0xff0e0e1a));
        setColour (juce::TextEditor::textColourId,           kBright);
        setColour (juce::TextEditor::outlineColourId,        kPurple);
        setColour (juce::TextEditor::focusedOutlineColourId, kPurple);
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

        auto accentVar = slider.getProperties()["accentColour"];
        const juce::Colour accent = accentVar.isVoid()
            ? kCyan : juce::Colour ((juce::uint32)(int64_t) accentVar);

        // Track arc
        {
            juce::Path t;
            t.addCentredArc (cx, cy, trackR, trackR, 0.0f, startAngle, endAngle, true);
            g.setColour (kTrack);
            g.strokePath (t, juce::PathStrokeType (2.5f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Value arc with glow
        if (sliderPos > 0.0001f)
        {
            juce::Path a;
            a.addCentredArc (cx, cy, trackR, trackR, 0.0f, startAngle, angle, true);
            g.setColour (accent.withAlpha (0.12f));
            g.strokePath (a, juce::PathStrokeType (7.0f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (accent);
            g.strokePath (a, juce::PathStrokeType (2.5f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Body
        const float bodyR = trackR - 6.0f;
        if (bodyR > 0.0f)
        {
            juce::ColourGradient grad (juce::Colour (0xff181830), cx, cy - bodyR,
                                       juce::Colour (0xff0c0c1a), cx, cy + bodyR, false);
            g.setGradientFill (grad);
            g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
            g.setColour (juce::Colour (0xff282840));
            g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

            // Indicator dot with glow
            const float dotR    = 2.0f;
            const float dotDist = juce::jmax (0.0f, bodyR - 6.0f);
            const float dotX    = cx + std::sin (angle) * dotDist;
            const float dotY    = cy - std::cos (angle) * dotDist;
            g.setColour (accent.withAlpha (0.25f));
            g.fillEllipse (dotX - dotR * 3.f, dotY - dotR * 3.f, dotR * 6.f, dotR * 6.f);
            g.setColour (accent);
            g.fillEllipse (dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
        }
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& btn,
                           bool /*highlighted*/, bool /*down*/) override
    {
        const bool on = btn.getToggleState();
        auto b = btn.getLocalBounds().toFloat().reduced (3.0f);

        auto accentVar = btn.getProperties()["accentColour"];
        const juce::Colour accent = accentVar.isVoid()
            ? kHotPink : juce::Colour ((juce::uint32)(int64_t) accentVar);

        g.setColour (on ? accent.withAlpha (0.15f) : juce::Colour (0xff0e0e1a));
        g.fillRoundedRectangle (b, 4.0f);
        g.setColour (on ? accent.withAlpha (0.5f) : kDim);
        g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (on ? accent : kText);
        g.setFont (crtFont (10.0f, true));
        g.drawText (on ? "PRE" : "POST", btn.getLocalBounds(), juce::Justification::centred);
    }
};

//==============================================================================
// Scope Component

void ScopeComponent::updateScope()
{
    const int size = HyperCrushProcessor::SCOPE_SIZE;
    int wp = processor.scopeWritePos.load (std::memory_order_relaxed);
    for (int i = 0; i < size; ++i)
    {
        int idx = (wp + i) % size;
        displayBuffer[(size_t) i] = processor.scopeBuffer[idx].load (std::memory_order_relaxed);
    }
}

void ScopeComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour (kScopeBg);
    g.fillRoundedRectangle (bounds, 4.0f);

    // Border
    g.setColour (kPurple.withAlpha (0.25f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    // Grid lines
    g.setColour (juce::Colour (0x0cffffff));
    float midY = bounds.getCentreY();
    g.drawHorizontalLine ((int) midY, bounds.getX(), bounds.getRight());
    for (int i = 1; i < 4; ++i)
    {
        float gx = bounds.getX() + (float) i * bounds.getWidth() / 4.0f;
        g.drawVerticalLine ((int) gx, bounds.getY(), bounds.getBottom());
    }

    // Waveform
    const auto bufSize = displayBuffer.size();
    if (bufSize >= 2)
    {
        float w  = bounds.getWidth();
        float h  = bounds.getHeight();
        float cy = bounds.getCentreY();

        juce::Path waveform;
        for (size_t i = 0; i < bufSize; ++i)
        {
            float px = bounds.getX() + (float) i / (float)(bufSize - 1) * w;
            float py = cy - juce::jlimit (-1.0f, 1.0f, displayBuffer[i]) * h * 0.42f;
            if (i == 0) waveform.startNewSubPath (px, py);
            else        waveform.lineTo (px, py);
        }

        // Glow layers
        g.setColour (kCyan.withAlpha (0.06f));
        g.strokePath (waveform, juce::PathStrokeType (10.0f));
        g.setColour (kCyan.withAlpha (0.15f));
        g.strokePath (waveform, juce::PathStrokeType (4.0f));

        // Chromatic aberration — slight pink offset
        {
            juce::Path offset;
            for (size_t i = 0; i < bufSize; ++i)
            {
                float px = bounds.getX() + 2.0f + (float) i / (float)(bufSize - 1) * w;
                float py = cy - juce::jlimit (-1.0f, 1.0f, displayBuffer[i]) * h * 0.42f;
                if (i == 0) offset.startNewSubPath (px, py);
                else        offset.lineTo (px, py);
            }
            g.setColour (kHotPink.withAlpha (0.1f));
            g.strokePath (offset, juce::PathStrokeType (1.5f));
        }

        // Main waveform
        g.setColour (kCyan.withAlpha (0.85f));
        g.strokePath (waveform, juce::PathStrokeType (1.5f));
    }

    // Scanlines over scope
    g.setColour (juce::Colour (0x0a000000));
    for (int sy = (int) bounds.getY(); sy < (int) bounds.getBottom(); sy += 2)
        g.fillRect ((int) bounds.getX(), sy, (int) bounds.getWidth(), 1);
}

//==============================================================================
// Layout
static constexpr int kTitleH   = 48;
static constexpr int kScopeTop = 52;
static constexpr int kScopeH   = 130;
static constexpr int kNumCols  = 5;

struct SectionInfo { const char* label; juce::Colour colour; };
static const SectionInfo kSections[kNumCols] = {
    { "CRUSH",  kColCrush  },
    { "DRIVE",  kColDrive  },
    { "FILTER", kColFilter },
    { "GATE",   kColGate   },
    { "MIX",    kColMix    },
};

//==============================================================================
HyperCrushEditor::HyperCrushEditor (HyperCrushProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), scopeComponent (p)
{
    setSize (700, 500);
    setResizable (false, false);

    lnf = std::make_unique<CrushLookAndFeel>();

    addAndMakeVisible (scopeComponent);

    auto setupSlider = [&](juce::Slider& sl, juce::Label& lb,
                           const juce::String& txt, juce::Colour accent)
    {
        sl.setSliderStyle   (juce::Slider::RotaryVerticalDrag);
        sl.setTextBoxStyle  (juce::Slider::TextBoxBelow, false, 56, 14);
        sl.setLookAndFeel   (lnf.get());
        sl.getProperties().set ("accentColour", (int64_t)(juce::uint32) accent.getARGB());
        addAndMakeVisible (sl);

        lb.setText              (txt, juce::dontSendNotification);
        lb.setJustificationType (juce::Justification::centred);
        lb.setColour            (juce::Label::textColourId, kText);
        lb.setFont              (crtFont (9.0f, true));
        addAndMakeVisible (lb);
    };

    setupSlider (bitDepthSlider,     bitDepthLabel,     "BIT DEPTH",   kColCrush);
    setupSlider (downsampleSlider,   downsampleLabel,   "SAMPLE RATE", kColCrush);
    setupSlider (driveSlider,        driveLabel,        "DRIVE",       kColDrive);
    setupSlider (hpfCutoffSlider,    hpfCutoffLabel,    "HP CUTOFF",   kColFilter);
    setupSlider (hpfResoSlider,      hpfResoLabel,      "HP RESO",     kColFilter);
    setupSlider (lpfCutoffSlider,    lpfCutoffLabel,    "LP CUTOFF",   kColFilter);
    setupSlider (lpfResoSlider,      lpfResoLabel,      "LP RESO",     kColFilter);
    setupSlider (stutterRateSlider,  stutterRateLabel,  "RATE",        kColGate);
    setupSlider (stutterDepthSlider, stutterDepthLabel, "DEPTH",       kColGate);
    setupSlider (dryWetSlider,       dryWetLabel,       "DRY/WET",     kColMix);

    // PRE/POST toggle
    clipPreButton.getProperties().set ("accentColour", (int64_t)(juce::uint32) kColDrive.getARGB());
    clipPreButton.setLookAndFeel (lnf.get());
    addAndMakeVisible (clipPreButton);
    clipPreLabel.setText            ("CLIP", juce::dontSendNotification);
    clipPreLabel.setJustificationType (juce::Justification::centred);
    clipPreLabel.setColour          (juce::Label::textColourId, kText);
    clipPreLabel.setFont            (crtFont (9.0f, true));
    addAndMakeVisible (clipPreLabel);

    auto& av = processorRef.apvts;
    bitDepthAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "bitDepth",      bitDepthSlider);
    downsampleAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "downsample",    downsampleSlider);
    driveAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "drive",         driveSlider);
    hpfCutoffAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "hpfCutoff",     hpfCutoffSlider);
    hpfResoAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "hpfReso",       hpfResoSlider);
    lpfCutoffAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "lpfCutoff",     lpfCutoffSlider);
    lpfResoAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "lpfReso",       lpfResoSlider);
    stutterRateAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "stutterRate",   stutterRateSlider);
    stutterDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "stutterDepth",  stutterDepthSlider);
    dryWetAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "dryWet",        dryWetSlider);
    clipPreAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (av, "clipPre",       clipPreButton);

    startTimerHz (30);
}

HyperCrushEditor::~HyperCrushEditor()
{
    stopTimer();
    for (auto* sl : { &bitDepthSlider, &downsampleSlider, &driveSlider,
                      &hpfCutoffSlider, &hpfResoSlider,
                      &lpfCutoffSlider, &lpfResoSlider,
                      &stutterRateSlider, &stutterDepthSlider,
                      &dryWetSlider })
        sl->setLookAndFeel (nullptr);
    clipPreButton.setLookAndFeel (nullptr);
}

//==============================================================================
void HyperCrushEditor::timerCallback()
{
    scopeComponent.updateScope();
    scopeComponent.repaint();

    ++glitchCounter;
    if (glitchCounter % 5 == 0)
    {
        glitchBars.clear();
        if (rng.nextFloat() < 0.35f)
        {
            int n = rng.nextInt (3) + 1;
            for (int i = 0; i < n; ++i)
            {
                GlitchBar bar;
                bar.y      = rng.nextInt (getHeight());
                bar.h      = rng.nextInt (3) + 1;
                bar.offset = rng.nextFloat() * 8.0f - 4.0f;
                bar.colour = (rng.nextBool() ? kHotPink : kCyan)
                                 .withAlpha (rng.nextFloat() * 0.1f + 0.03f);
                glitchBars.push_back (bar);
            }
        }
        repaint();
    }
}

//==============================================================================
void HyperCrushEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (kBg);

    // Subtle background grid
    g.setColour (juce::Colour (0x04ffffff));
    for (int gx = 0; gx < getWidth(); gx += 24)
        g.drawVerticalLine (gx, 0.0f, (float) getHeight());
    for (int gy = 0; gy < getHeight(); gy += 24)
        g.drawHorizontalLine (gy, 0.0f, (float) getWidth());

    // Title bar background
    g.setColour (kPanel);
    g.fillRect (0, 0, getWidth(), kTitleH);

    // Title underline glow
    g.setColour (kPurple.withAlpha (0.4f));
    g.fillRect (0, kTitleH - 2, getWidth(), 2);
    g.setColour (kHotPink.withAlpha (0.15f));
    g.fillRect (0, kTitleH - 1, getWidth(), 1);

    // Title text with chromatic aberration
    auto titleArea = juce::Rectangle<int> (0, 0, getWidth(), kTitleH);
    g.setFont (crtFont (20.0f, true));
    g.setColour (kHotPink.withAlpha (0.35f));
    g.drawText ("HYPERCRUSH", titleArea.translated (-2, 0), juce::Justification::centred);
    g.setColour (kCyan.withAlpha (0.3f));
    g.drawText ("HYPERCRUSH", titleArea.translated (2, 0), juce::Justification::centred);
    g.setColour (kBright);
    g.drawText ("HYPERCRUSH", titleArea, juce::Justification::centred);

    // Subtitle
    g.setFont (crtFont (9.0f));
    g.setColour (kPurple.withAlpha (0.6f));
    g.drawText ("Y2K EDITION", juce::Rectangle<int> (getWidth() - 160, 0, 150, kTitleH),
                juce::Justification::centredRight);

    // Column separators + section labels
    const int colW    = getWidth() / kNumCols;
    const int knobTop = kScopeTop + kScopeH + 10;

    for (int col = 0; col < kNumCols; ++col)
    {
        const int x = col * colW;

        if (col > 0)
        {
            g.setColour (kDim.withAlpha (0.5f));
            g.fillRect (x, knobTop + 4, 1, getHeight() - knobTop - 8);
        }

        const auto& sec = kSections[col];
        g.setColour (sec.colour.withAlpha (0.7f));
        g.setFont (crtFont (9.0f, true));
        g.drawText (sec.label,
                    juce::Rectangle<int> (x, knobTop + 2, colW, 18),
                    juce::Justification::centred);
    }

    // Scanlines over entire UI
    g.setColour (juce::Colour (0x06000008));
    for (int sy = 0; sy < getHeight(); sy += 2)
        g.fillRect (0, sy, getWidth(), 1);

    // Glitch bars
    for (const auto& bar : glitchBars)
    {
        g.setColour (bar.colour);
        g.fillRect ((int) bar.offset, bar.y, getWidth(), bar.h);
    }
}

void HyperCrushEditor::resized()
{
    const int colW    = getWidth() / kNumCols;   // 140
    const int scopeX  = 14;
    const int scopeW  = getWidth() - 28;

    scopeComponent.setBounds (scopeX, kScopeTop, scopeW, kScopeH);

    const int knobTop = kScopeTop + kScopeH + 10;
    const int bodyTop = knobTop + 22;              // below section labels
    const int bodyH   = getHeight() - bodyTop - 4;
    const int halfH   = bodyH / 2;
    const int lblH    = 12;
    const int pad     = 8;

    auto placeKnob = [&](juce::Slider& sl, juce::Label& lb, int col, int row)
    {
        auto cell = juce::Rectangle<int> (col * colW, bodyTop + row * halfH, colW, halfH)
                        .reduced (pad, 3);
        lb.setBounds (cell.removeFromTop (lblH));
        sl.setBounds (cell);
    };

    // Col 0 - CRUSH
    placeKnob (bitDepthSlider,    bitDepthLabel,    0, 0);
    placeKnob (downsampleSlider,  downsampleLabel,  0, 1);

    // Col 1 - DRIVE
    placeKnob (driveSlider, driveLabel, 1, 0);
    {
        auto cell = juce::Rectangle<int> (colW, bodyTop + halfH, colW, halfH).reduced (pad, 3);
        clipPreLabel.setBounds  (cell.removeFromTop (lblH));
        clipPreButton.setBounds (cell.reduced (0, halfH / 5));
    }

    // Col 2 - FILTER (4 quarter-rows)
    {
        const int quarterH = bodyH / 4;
        auto filterKnob = [&](juce::Slider& sl, juce::Label& lb, int row)
        {
            auto cell = juce::Rectangle<int> (2 * colW, bodyTop + row * quarterH, colW, quarterH)
                            .reduced (pad, 1);
            lb.setBounds (cell.removeFromTop (lblH));
            sl.setBounds (cell);
        };
        filterKnob (hpfCutoffSlider, hpfCutoffLabel, 0);
        filterKnob (hpfResoSlider,   hpfResoLabel,   1);
        filterKnob (lpfCutoffSlider, lpfCutoffLabel, 2);
        filterKnob (lpfResoSlider,   lpfResoLabel,   3);
    }

    // Col 3 - GATE
    placeKnob (stutterRateSlider,  stutterRateLabel,  3, 0);
    placeKnob (stutterDepthSlider, stutterDepthLabel, 3, 1);

    // Col 4 - MIX (single knob, vertically centered)
    {
        const int mixY = bodyTop + (bodyH - halfH) / 2;
        auto cell = juce::Rectangle<int> (4 * colW, mixY, colW, halfH).reduced (pad, 3);
        dryWetLabel.setBounds (cell.removeFromTop (lblH));
        dryWetSlider.setBounds (cell);
    }
}
