#include "PluginEditor.h"

//==============================================================================
// Y2K Palette
static const juce::Colour kBg       (0xff0a0a0f);
static const juce::Colour kPanel    (0xff0c0c1a);
static const juce::Colour kHotPink  (0xffff2d95);
static const juce::Colour kCyan     (0xff00ffff);
static const juce::Colour kPurple   (0xff9b59f5);
static const juce::Colour kWhite    (0xffe0e0f0);
static const juce::Colour kText     (0xff889099);
static const juce::Colour kDim      (0xff1a1a2e);
static const juce::Colour kTrack    (0xff2a2a2a);
static const juce::Colour kKnobBody (0xff151515);
static const juce::Colour kScopeBg  (0xff060610);

static const juce::Colour kColCrush  = kCyan;
static const juce::Colour kColDrive  = kHotPink;
static const juce::Colour kColFilter = kPurple;
static const juce::Colour kColGate   = kCyan;
static const juce::Colour kColMix    = kHotPink;

//==============================================================================
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
        setColour (juce::Slider::textBoxTextColourId,        juce::Colour (0xff667788));
        setColour (juce::Slider::textBoxHighlightColourId,   kCyan.withAlpha (0.2f));
        setColour (juce::ComboBox::backgroundColourId,       juce::Colour (0xff101018));
        setColour (juce::ComboBox::outlineColourId,          kCyan.withAlpha (0.5f));
        setColour (juce::ComboBox::textColourId,             kCyan);
        setColour (juce::ComboBox::arrowColourId,            kCyan);
        setColour (juce::PopupMenu::backgroundColourId,      juce::Colour (0xff101018));
        setColour (juce::PopupMenu::textColourId,            kWhite);
        setColour (juce::PopupMenu::headerTextColourId,      kText);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kCyan.withAlpha (0.15f));
        setColour (juce::PopupMenu::highlightedTextColourId, kCyan);
        setColour (juce::TooltipWindow::backgroundColourId,  juce::Colour (0xf0101018));
        setColour (juce::TooltipWindow::textColourId,        kCyan);
        setColour (juce::TooltipWindow::outlineColourId,     kCyan.withAlpha (0.3f));
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

        const float arcR  = r - 2.0f;
        const float angle = startAngle + sliderPos * (endAngle - startAngle);

        auto accentVar = slider.getProperties()["accentColour"];
        const juce::Colour accent = accentVar.isVoid()
            ? kCyan : juce::Colour ((juce::uint32)(int64_t) accentVar);

        // Dark body (flat)
        const float bodyR = arcR - 5.0f;
        if (bodyR > 0.0f)
        {
            g.setColour (kKnobBody);
            g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        }

        // Track arc
        {
            juce::Path t;
            t.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
            g.setColour (kTrack);
            g.strokePath (t, juce::PathStrokeType (2.5f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Value arc with soft glow
        if (sliderPos > 0.0001f)
        {
            juce::Path a;
            a.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, angle, true);
            g.setColour (accent.withAlpha (0.08f));
            g.strokePath (a, juce::PathStrokeType (8.0f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (accent.withAlpha (0.85f));
            g.strokePath (a, juce::PathStrokeType (2.5f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Glowing dot on arc
        {
            const float dotX = cx + std::sin (angle) * arcR;
            const float dotY = cy - std::cos (angle) * arcR;
            g.setColour (accent.withAlpha (0.2f));
            g.fillEllipse (dotX - 5.0f, dotY - 5.0f, 10.0f, 10.0f);
            g.setColour (accent);
            g.fillEllipse (dotX - 2.5f, dotY - 2.5f, 5.0f, 5.0f);
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
// Scope

void ScopeComponent::updateScope()
{
    const int size = HyperCrushProcessor::SCOPE_SIZE;
    int wp = processor.scopeWritePos.load (std::memory_order_relaxed);
    bool hasSignal = false;
    for (int i = 0; i < size; ++i)
    {
        int idx = (wp + i) % size;
        float v = processor.scopeBuffer[idx].load (std::memory_order_relaxed);
        displayBuffer[(size_t) i] = v;
        if (std::abs (v) > 0.0001f) hasSignal = true;
    }
    if (!hasSignal)
        idlePhase += 0.04f;
}

void ScopeComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (kScopeBg);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (kPurple.withAlpha (0.2f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    // Grid lines at 25%, 50%, 75%
    g.setColour (kPurple.withAlpha (0.07f));
    for (int i = 1; i <= 3; ++i)
    {
        float gy = bounds.getY() + (float) i * bounds.getHeight() / 4.0f;
        g.drawHorizontalLine ((int) gy, bounds.getX() + 4.0f, bounds.getRight() - 4.0f);
    }

    const auto bufSize = displayBuffer.size();
    if (bufSize < 2) return;

    float w  = bounds.getWidth();
    float h  = bounds.getHeight();
    float cy = bounds.getCentreY();

    bool hasSignal = false;
    for (size_t i = 0; i < bufSize && !hasSignal; ++i)
        if (std::abs (displayBuffer[i]) > 0.0001f) hasSignal = true;

    juce::Path waveform;
    for (size_t i = 0; i < bufSize; ++i)
    {
        float px = bounds.getX() + (float) i / (float)(bufSize - 1) * w;
        float val = hasSignal
            ? juce::jlimit (-1.0f, 1.0f, displayBuffer[i])
            : std::sin (idlePhase + (float) i * 0.05f) * 0.15f;
        float py = cy - val * h * 0.42f;
        if (i == 0) waveform.startNewSubPath (px, py);
        else        waveform.lineTo (px, py);
    }

    // Glow
    g.setColour (kCyan.withAlpha (0.05f));
    g.strokePath (waveform, juce::PathStrokeType (10.0f));
    g.setColour (kCyan.withAlpha (0.12f));
    g.strokePath (waveform, juce::PathStrokeType (4.0f));

    // Chromatic aberration ghost
    {
        juce::Path ghost;
        for (size_t i = 0; i < bufSize; ++i)
        {
            float px = bounds.getX() + 1.0f + (float) i / (float)(bufSize - 1) * w;
            float val = hasSignal
                ? juce::jlimit (-1.0f, 1.0f, displayBuffer[i])
                : std::sin (idlePhase + (float) i * 0.05f) * 0.15f;
            float py = cy - val * h * 0.42f + 2.0f;
            if (i == 0) ghost.startNewSubPath (px, py);
            else        ghost.lineTo (px, py);
        }
        g.setColour (kHotPink.withAlpha (0.12f));
        g.strokePath (ghost, juce::PathStrokeType (2.0f));
    }

    // Main waveform
    g.setColour (kCyan.withAlpha (0.9f));
    g.strokePath (waveform, juce::PathStrokeType (2.0f));

    // Scanlines
    g.setColour (juce::Colour (0x08000000));
    for (int sy = (int) bounds.getY(); sy < (int) bounds.getBottom(); sy += 2)
        g.fillRect ((int) bounds.getX(), sy, (int) bounds.getWidth(), 1);
}

//==============================================================================
// Presets

const std::vector<PresetEntry>& HyperCrushEditor::getPresets()
{
    static const std::vector<PresetEntry> presets = {
        //                  name              bitD   ds     drv    pre    hpf     lpf      reso    rate  dep   wet
        /* 0  */ { "INIT",          16.0f,  1.0f,  0.0f,  true,  20.f,   20000.f, 0.707f, 3.f,  0.0f, 1.0f  },
        /* 1  */ { "TUTORIAL",      10.0f,  1.0f,  2.0f,  false, 20.f,   5000.f,  0.707f, 3.f,  0.4f, 0.6f  },
        /* 2  */ { "GECKO",          3.0f,  7.2f,  8.0f,  true,  1800.f, 20000.f, 7.8f,   1.f,  1.0f, 1.0f  },
        /* 3  */ { "BRAT",           8.0f,  1.0f,  5.0f,  false, 20.f,   4000.f,  4.4f,   2.f,  0.7f, 0.8f  },
        /* 4  */ { "OIL",            6.0f, 16.5f,  3.0f,  false, 20.f,   1200.f,  8.85f,  3.f,  0.0f, 0.9f  },
        /* 5  */ { "DRAIN",         12.0f, 25.8f,  1.5f,  false, 20.f,   2500.f,  2.18f,  4.f,  0.3f, 0.5f  },
        /* 6  */ { "PC",             7.0f,  1.0f,  4.0f,  true,  800.f,  6000.f,  5.13f,  2.f,  0.5f, 0.75f },
        /* 7  */ { "NOIDED",         2.0f,  5.65f, 9.0f,  true,  500.f,  20000.f, 0.97f,  0.f,  1.0f, 1.0f  },
        /* 8  */ { "SLAYY",          9.0f,  1.0f,  4.5f,  false, 20.f,   3500.f,  5.94f,  3.f,  0.6f, 0.7f  },
        /* 9  */ { "GLASSBREAK",     5.0f, 10.3f,  6.0f,  false, 20.f,   900.f,   6.83f,  2.f,  0.8f, 0.85f },
        /* 10 */ { "NIGHTCORE",     10.0f, 28.9f,  2.0f,  true,  500.f,  20000.f, 0.707f, 0.f,  0.6f, 0.5f  },
        /* 11 */ { "TAPE MELT",     12.0f, 19.6f,  2.0f,  false, 20.f,   3000.f,  1.46f,  3.f,  0.0f, 0.6f  },
        /* 12 */ { "CIRCUIT BENT",   2.0f,  4.1f,  0.0f,  true,  20.f,   20000.f, 0.707f, 3.f,  0.0f, 1.0f  },
        /* 13 */ { "RADIO",          8.0f, 13.4f,  0.0f,  true,  300.f,  4500.f,  0.707f, 3.f,  0.0f, 0.8f  },
        /* 14 */ { "STATIC",         1.0f,  2.55f, 1.0f,  false, 20.f,   20000.f, 0.707f, 3.f,  0.0f, 0.3f  },
    };
    return presets;
}

juce::Colour HyperCrushEditor::getPresetColour (int index) const
{
    // Aggressive → hot pink
    if (index == 2 || index == 7 || index == 12) return kHotPink;
    // Smooth → purple
    if (index == 4 || index == 5 || index == 11) return kPurple;
    // Default → cyan
    return kCyan;
}

//==============================================================================
// Layout constants
static constexpr int kTitleH    = 40;
static constexpr int kMacroH    = 124;
static constexpr int kScopeH    = 132;
static constexpr int kNumCols   = 6;
static constexpr int kPad       = 10;
static constexpr int kCtrlDiam  = 65;
static constexpr int kLblH      = 13;
static constexpr int kTxtBoxH   = 14;

struct SectionInfo { const char* label; int colStart; int colSpan; juce::Colour colour; };
static const SectionInfo kSections[] = {
    { "CRUSH",  0, 2, kColCrush  },
    { "DRIVE",  2, 1, kColDrive  },
    { "FILTER", 3, 1, kColFilter },
    { "GATE",   4, 1, kColGate   },
    { "MIX",    5, 1, kColMix    },
};

//==============================================================================
HyperCrushEditor::HyperCrushEditor (HyperCrushProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), scopeComponent (p)
{
    setSize (820, 660);
    setResizable (false, false);

    lnf = std::make_unique<CrushLookAndFeel>();

    // Tooltip window
    tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 1000);
    tooltipWindow->setLookAndFeel (lnf.get());

    addAndMakeVisible (scopeComponent);

    // --- Preset box with categories ---
    presetBox.setLookAndFeel (lnf.get());
    presetBox.setTextWhenNothingSelected ("PRESETS");

    auto& presets = getPresets();
    presetBox.addSectionHeading (juce::String::fromUTF8 ("\xe2\x80\x94 ESSENTIALS \xe2\x80\x94"));
    presetBox.addItem (presets[0].name,  1);
    presetBox.addItem (presets[1].name,  2);
    presetBox.addSectionHeading (juce::String::fromUTF8 ("\xe2\x80\x94 ARTISTS \xe2\x80\x94"));
    for (int i = 2; i <= 10; ++i)
        presetBox.addItem (presets[(size_t) i].name, i + 1);
    presetBox.addSectionHeading (juce::String::fromUTF8 ("\xe2\x80\x94 TEXTURES \xe2\x80\x94"));
    for (int i = 11; i <= 14; ++i)
        presetBox.addItem (presets[(size_t) i].name, i + 1);

    presetBox.onChange = [this]() {
        int id = presetBox.getSelectedId();
        if (id > 0)
        {
            applyPreset (id - 1);
            presetBox.setColour (juce::ComboBox::textColourId, getPresetColour (id - 1));
        }
    };
    presetBox.setTooltip ("Select a preset");
    addAndMakeVisible (presetBox);

    // --- Setup helper ---
    auto setupSlider = [&](juce::Slider& sl, juce::Label& lb,
                           const juce::String& txt, juce::Colour accent,
                           const juce::String& tip)
    {
        sl.setSliderStyle   (juce::Slider::RotaryVerticalDrag);
        sl.setTextBoxStyle  (juce::Slider::TextBoxBelow, false, 56, kTxtBoxH);
        sl.setLookAndFeel   (lnf.get());
        sl.getProperties().set ("accentColour", (int64_t)(juce::uint32) accent.getARGB());
        sl.setColour (juce::Slider::textBoxTextColourId, accent.withAlpha (0.7f));
        sl.setTooltip (tip);
        addAndMakeVisible (sl);

        lb.setText              (txt, juce::dontSendNotification);
        lb.setJustificationType (juce::Justification::centred);
        lb.setColour            (juce::Label::textColourId, kWhite);
        lb.setFont              (crtFont (9.0f, true));
        addAndMakeVisible (lb);
    };

    // Macros
    setupSlider (glitchSlider, glitchLabel, "GLITCH", kCyan,     "Macro - Crush + Drive + Fast Gate");
    setupSlider (meltSlider,   meltLabel,   "MELT",   kPurple,   "Macro - LP Sweep + Resonance + Slow Gate");
    setupSlider (rektSlider,   rektLabel,   "REKT",   kHotPink,  "Macro - Max Destruction");
    setupSlider (vibeSlider,   vibeLabel,   "VIBE",   kWhite,    "Macro - Subtle Texture");

    // Controls
    setupSlider (bitDepthSlider,     bitDepthLabel,     "CRUNCH",  kColCrush,  "Bit Depth - 1 to 16 bits");
    setupSlider (downsampleSlider,   downsampleLabel,   "MANGLE",  kColCrush,  "Sample Rate Reduction - 1x to 32x");
    setupSlider (driveSlider,        driveLabel,        "SMASH",   kColDrive,  "Drive - 0 to 40 dB");
    setupSlider (hpfCutoffSlider,    hpfCutoffLabel,    "SLICE",   kColFilter, "HP Filter Cutoff - 20 Hz to 20 kHz");
    setupSlider (lpfCutoffSlider,    lpfCutoffLabel,    "SCREAM",  kColFilter, "LP Filter Cutoff - 20 Hz to 20 kHz");
    setupSlider (filterResoSlider,   filterResoLabel,   "RESO",    kColFilter, "Filter Resonance - 0.5 to 10 Q");
    setupSlider (stutterRateSlider,  stutterRateLabel,  "STUTTER", kColGate,   "Gate Rate - 1/32 to 4 bars");
    setupSlider (stutterDepthSlider, stutterDepthLabel, "CHOP",    kColGate,   "Gate Depth - 0% to 100%");
    setupSlider (dryWetSlider,       dryWetLabel,       "BLEND",   kColMix,    "Dry/Wet Mix - 0% to 100%");

    // CLIP toggle
    clipPreButton.getProperties().set ("accentColour", (int64_t)(juce::uint32) kColDrive.getARGB());
    clipPreButton.setLookAndFeel (lnf.get());
    clipPreButton.setTooltip ("Clip Mode - PRE or POST drive");
    addAndMakeVisible (clipPreButton);
    clipPreLabel.setText            ("CLIP", juce::dontSendNotification);
    clipPreLabel.setJustificationType (juce::Justification::centred);
    clipPreLabel.setColour          (juce::Label::textColourId, kWhite);
    clipPreLabel.setFont            (crtFont (9.0f, true));
    addAndMakeVisible (clipPreLabel);

    // APVTS attachments
    auto& av = processorRef.apvts;
    bitDepthAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "bitDepth",      bitDepthSlider);
    downsampleAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "downsample",    downsampleSlider);
    driveAtt        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "drive",         driveSlider);
    hpfCutoffAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "hpfCutoff",     hpfCutoffSlider);
    lpfCutoffAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "lpfCutoff",     lpfCutoffSlider);
    filterResoAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "filterReso",    filterResoSlider);
    stutterRateAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "stutterRate",   stutterRateSlider);
    stutterDepthAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "stutterDepth",  stutterDepthSlider);
    dryWetAtt       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "dryWet",        dryWetSlider);
    clipPreAtt      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (av, "clipPre",       clipPreButton);
    glitchAtt       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "macroGlitch",   glitchSlider);
    meltAtt         = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "macroMelt",     meltSlider);
    rektAtt         = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "macroRekt",     rektSlider);
    vibeAtt         = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (av, "macroVibe",     vibeSlider);

    startTimerHz (30);
}

HyperCrushEditor::~HyperCrushEditor()
{
    stopTimer();
    tooltipWindow->setLookAndFeel (nullptr);
    tooltipWindow.reset();
    for (auto* sl : { &glitchSlider, &meltSlider, &rektSlider, &vibeSlider,
                      &bitDepthSlider, &downsampleSlider, &driveSlider,
                      &hpfCutoffSlider, &lpfCutoffSlider, &filterResoSlider,
                      &stutterRateSlider, &stutterDepthSlider,
                      &dryWetSlider })
        sl->setLookAndFeel (nullptr);
    clipPreButton.setLookAndFeel (nullptr);
    presetBox.setLookAndFeel (nullptr);
}

//==============================================================================
void HyperCrushEditor::applyPreset (int index)
{
    auto& presets = getPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    const auto& pr = presets[(size_t) index];

    auto& av = processorRef.apvts;
    auto setParam = [&](const juce::String& id, float val) {
        if (auto* p = av.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (val));
    };

    setParam ("bitDepth",      pr.bitDepth);
    setParam ("downsample",    pr.downsample);
    setParam ("drive",         pr.drive);
    setParam ("clipPre",       pr.clipPre ? 1.0f : 0.0f);
    setParam ("hpfCutoff",     pr.hpfCutoff);
    setParam ("lpfCutoff",     pr.lpfCutoff);
    setParam ("filterReso",    pr.filterReso);
    setParam ("stutterRate",   pr.stutterRate);
    setParam ("stutterDepth",  pr.stutterDepth);
    setParam ("dryWet",        pr.dryWet);

    setParam ("macroGlitch", 0.0f);
    setParam ("macroMelt",   0.0f);
    setParam ("macroRekt",   0.0f);
    setParam ("macroVibe",   0.0f);
}

//==============================================================================
void HyperCrushEditor::timerCallback()
{
    scopeComponent.updateScope();
    scopeComponent.repaint();

    animTime += 1.0f / 30.0f;
    scanlineOffset += 0.5f;
    if (scanlineOffset > 4.0f) scanlineOffset -= 4.0f;

    ++glitchCounter;
    if (glitchCounter % 5 == 0)
    {
        glitchBars.clear();
        if (rng.nextFloat() < 0.25f)
        {
            int n = rng.nextInt (3) + 1;
            for (int i = 0; i < n; ++i)
            {
                GlitchBar bar;
                bar.y      = rng.nextInt (getHeight());
                bar.h      = rng.nextInt (3) + 1;
                bar.offset = rng.nextFloat() * 6.0f - 3.0f;
                juce::Colour colors[] = { kHotPink, kCyan, kPurple };
                bar.colour = colors[rng.nextInt (3)].withAlpha (rng.nextFloat() * 0.05f + 0.02f);
                glitchBars.push_back (bar);
            }
        }
    }

    repaint();
}

//==============================================================================
void HyperCrushEditor::paint (juce::Graphics& g)
{
    // Background with radial gradient
    {
        juce::ColourGradient grad (juce::Colour (0xff100f1a), getWidth() * 0.5f, getHeight() * 0.5f,
                                    kBg, 0.0f, 0.0f, true);
        g.setGradientFill (grad);
        g.fillAll();
    }

    // ---- Title bar ----
    g.setColour (kPanel);
    g.fillRect (0, 0, getWidth(), kTitleH);
    g.setColour (kHotPink.withAlpha (0.6f));
    g.fillRect (0, kTitleH - 1, getWidth(), 1);

    // Title text with chromatic aberration
    auto titleArea = juce::Rectangle<int> (kPad, 0, 300, kTitleH);
    g.setFont (crtFont (20.0f, true));
    g.setColour (kCyan.withAlpha (0.25f));
    g.drawText ("HYPERCRUSH", titleArea.translated (2, 1), juce::Justification::centredLeft);
    g.setColour (kHotPink);
    g.drawText ("HYPERCRUSH", titleArea, juce::Justification::centredLeft);

    // Y2K EDITION
    g.setFont (crtFont (9.0f));
    g.setColour (kCyan.withAlpha (0.6f));
    g.drawText ("Y2K EDITION", juce::Rectangle<int> (getWidth() - 110, 0, 100, kTitleH),
                juce::Justification::centredRight);

    // ---- Macro glow for active macros ----
    {
        float pulse = 0.5f + 0.5f * std::sin (animTime * juce::MathConstants<float>::twoPi * 2.0f);

        auto drawGlow = [&](juce::Slider& sl, juce::Colour accent)
        {
            float val = (float) sl.getValue();
            if (val < 0.01f) return;
            float alpha = val * pulse * 0.12f;
            auto center = sl.getBounds().getCentre().toFloat();
            float r = 48.0f;
            g.setColour (accent.withAlpha (alpha));
            g.fillEllipse (center.x - r, center.y - r, r * 2.0f, r * 2.0f);
        };

        drawGlow (glitchSlider, kCyan);
        drawGlow (meltSlider,   kPurple);
        drawGlow (rektSlider,   kHotPink);
        drawGlow (vibeSlider,   kWhite);
    }

    // ---- Animated gradient line between macro row and scope ----
    {
        int lineY = kTitleH + kMacroH;
        float t = std::fmod (animTime / 10.0f, 1.0f);
        juce::Colour gradColours[] = { kHotPink, kCyan, kPurple, kHotPink };
        float seg1 = t * 3.0f;
        int s1 = juce::jmin ((int) seg1, 2);
        juce::Colour c1 = gradColours[s1].interpolatedWith (gradColours[s1 + 1], seg1 - s1);

        float t2 = std::fmod (t + 0.33f, 1.0f);
        float seg2 = t2 * 3.0f;
        int s2 = juce::jmin ((int) seg2, 2);
        juce::Colour c2 = gradColours[s2].interpolatedWith (gradColours[s2 + 1], seg2 - s2);

        juce::ColourGradient grad (c1.withAlpha (0.5f), 0.0f, (float) lineY,
                                    c2.withAlpha (0.5f), (float) getWidth(), (float) lineY, false);
        g.setGradientFill (grad);
        g.fillRect (0, lineY, getWidth(), 2);
    }

    // ---- Section headers in control grid ----
    const int scopeY     = kTitleH + kMacroH + 2;
    const int controlTop = scopeY + kScopeH + 8;
    const int colW       = getWidth() / kNumCols;

    for (const auto& sec : kSections)
    {
        int x = sec.colStart * colW;
        int w = sec.colSpan * colW;
        g.setColour (sec.colour.withAlpha (0.06f));
        g.fillRect (x, controlTop, w, 18);
        g.setColour (sec.colour.withAlpha (0.8f));
        g.setFont (crtFont (10.0f, true));
        g.drawText (sec.label, juce::Rectangle<int> (x, controlTop, w, 18),
                    juce::Justification::centred);
    }

    // Column separators
    for (int col = 1; col < kNumCols; ++col)
    {
        int x = col * colW;
        if (col == 1)
        {
            g.setColour (kDim.withAlpha (0.3f));
            g.fillRect (x, controlTop + 20, 1, getHeight() - controlTop - 24);
        }
        else
        {
            g.setColour (kDim);
            g.fillRect (x, controlTop + 20, 1, getHeight() - controlTop - 24);
        }
    }

    // ---- Scrolling scanlines ----
    g.setColour (juce::Colour (0x05000008));
    for (float sy = scanlineOffset; sy < (float) getHeight(); sy += 4.0f)
        g.fillRect (0.0f, sy, (float) getWidth(), 1.0f);

    // ---- Glitch bars ----
    for (const auto& bar : glitchBars)
    {
        g.setColour (bar.colour);
        g.fillRect ((int) bar.offset, bar.y, getWidth(), bar.h);
    }
}

//==============================================================================
void HyperCrushEditor::resized()
{
    const int colW = getWidth() / kNumCols;  // ~136

    // ---- Title bar: preset dropdown ----
    presetBox.setBounds (getWidth() - 290, 8, 160, 24);

    // ---- Macro row (large knobs ~88px diameter) ----
    {
        const int macroY      = kTitleH + 2;
        const int macroH      = kMacroH - 4;
        const int macroCellW  = 160;
        const int macroGap    = 12;
        const int totalW      = macroCellW * 4 + macroGap * 3;
        int mx                = (getWidth() - totalW) / 2;

        // Desired: ~88px diameter. rotary height = macroH - kLblH - kTxtBoxH
        // = 124 - 4 - 13 - 14 = 93. min(160, 93) - 4 = 89px. Close enough.
        const int slH = macroH - kLblH;

        struct MacroInfo { juce::Slider& sl; juce::Label& lb; };
        MacroInfo macros[] = {
            { glitchSlider, glitchLabel },
            { meltSlider,   meltLabel   },
            { rektSlider,   rektLabel   },
            { vibeSlider,   vibeLabel   },
        };

        for (auto& m : macros)
        {
            m.lb.setBounds (mx, macroY, macroCellW, kLblH);
            m.sl.setBounds (mx, macroY + kLblH, macroCellW, slH);
            mx += macroCellW + macroGap;
        }
    }

    // ---- Oscilloscope ----
    const int scopeY = kTitleH + kMacroH + 2;
    scopeComponent.setBounds (kPad, scopeY, getWidth() - kPad * 2, kScopeH);

    // ---- Controls grid ----
    const int controlTop = scopeY + kScopeH + 8;
    const int headerH    = 20;
    const int bodyTop    = controlTop + headerH;
    const int bodyH      = getHeight() - bodyTop - 6;
    const int halfH      = bodyH / 2;
    const int thirdH     = bodyH / 3;

    // Fixed-size control knob: 65px diameter
    // rotary needs min(w,h) = 69, so slider height = 69 + kTxtBoxH = 83
    const int ctrlSlH = kCtrlDiam + 4 + kTxtBoxH;  // 83
    const int ctrlBlockH = kLblH + ctrlSlH;          // 96

    auto placeKnob = [&](juce::Slider& sl, juce::Label& lb, int col, int yOff, int cellH)
    {
        int cellX = col * colW;
        int cellY = bodyTop + yOff;
        int centerY = cellY + (cellH - ctrlBlockH) / 2;
        int w = colW - 16;
        int x = cellX + 8;

        lb.setBounds (x, centerY, w, kLblH);
        sl.setBounds (x, centerY + kLblH, w, ctrlSlH);
    };

    // Col 0 — CRUNCH
    placeKnob (bitDepthSlider,   bitDepthLabel,   0, 0, halfH);
    // Col 1 — MANGLE
    placeKnob (downsampleSlider, downsampleLabel, 1, 0, halfH);

    // Col 2 — DRIVE: SMASH top, CLIP bottom
    placeKnob (driveSlider, driveLabel, 2, 0, halfH);
    {
        int cellX = 2 * colW;
        int cellY = bodyTop + halfH;
        int centerY = cellY + (halfH - ctrlBlockH) / 2;
        int w = colW - 16;
        int x = cellX + 8;
        clipPreLabel.setBounds  (x, centerY, w, kLblH);
        clipPreButton.setBounds (x + 15, centerY + kLblH + 10, w - 30, 28);
    }

    // Col 3 — FILTER: SLICE, SCREAM, RESO in 3 rows
    placeKnob (hpfCutoffSlider,  hpfCutoffLabel,  3, 0,          thirdH);
    placeKnob (lpfCutoffSlider,  lpfCutoffLabel,  3, thirdH,     thirdH);
    placeKnob (filterResoSlider, filterResoLabel,  3, thirdH * 2, thirdH);

    // Col 4 — GATE: STUTTER, CHOP
    placeKnob (stutterRateSlider,  stutterRateLabel,  4, 0,     halfH);
    placeKnob (stutterDepthSlider, stutterDepthLabel, 4, halfH, halfH);

    // Col 5 — MIX: BLEND centered vertically
    placeKnob (dryWetSlider, dryWetLabel, 5, (bodyH - halfH) / 2, halfH);
}
