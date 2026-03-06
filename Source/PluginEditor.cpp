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
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kCyan.withAlpha (0.15f));
        setColour (juce::PopupMenu::highlightedTextColourId, kCyan);
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

        // Dark body (flat, no gradient)
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

        // Glowing dot on arc at current value
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
    // Idle animation phase
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

    // Grid lines (3 horizontal at 25%, 50%, 75%)
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

    // Check if we have real audio
    bool hasSignal = false;
    for (size_t i = 0; i < bufSize && !hasSignal; ++i)
        if (std::abs (displayBuffer[i]) > 0.0001f) hasSignal = true;

    juce::Path waveform;
    for (size_t i = 0; i < bufSize; ++i)
    {
        float px = bounds.getX() + (float) i / (float)(bufSize - 1) * w;
        float val;
        if (hasSignal)
            val = juce::jlimit (-1.0f, 1.0f, displayBuffer[i]);
        else
            val = std::sin (idlePhase + (float) i * 0.05f) * 0.15f;

        float py = cy - val * h * 0.42f;
        if (i == 0) waveform.startNewSubPath (px, py);
        else        waveform.lineTo (px, py);
    }

    // Glow
    g.setColour (kCyan.withAlpha (0.05f));
    g.strokePath (waveform, juce::PathStrokeType (10.0f));
    g.setColour (kCyan.withAlpha (0.12f));
    g.strokePath (waveform, juce::PathStrokeType (4.0f));

    // Chromatic aberration ghost (pink, offset)
    {
        juce::Path ghost;
        for (size_t i = 0; i < bufSize; ++i)
        {
            float px = bounds.getX() + 1.0f + (float) i / (float)(bufSize - 1) * w;
            float val;
            if (hasSignal)
                val = juce::jlimit (-1.0f, 1.0f, displayBuffer[i]);
            else
                val = std::sin (idlePhase + (float) i * 0.05f) * 0.15f;
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
        // name,        bitD  ds    drv   pre    hpf     lpf      reso    rate  dep   wet
        { "INIT",       16.f, 1.f,  0.f,  true,  20.f,   20000.f, 0.707f, 3.f,  0.f,  1.f   },
        { "100 GECS",   4.f,  9.6f, 7.f,  true,  2000.f, 20000.f, 5.02f,  1.f,  1.f,  1.f   },
        { "GLITCH VOCAL",8.f, 1.f,  3.f,  false, 20.f,   3000.f,  3.47f,  3.f,  0.5f, 0.7f  },
        { "CIRCUIT BENT",2.f, 3.2f, 0.f,  true,  20.f,   20000.f, 0.707f, 3.f,  0.f,  1.f   },
        { "Y2K RAVE",   6.f,  1.f,  5.f,  false, 20.f,   800.f,   6.79f,  2.f,  0.6f, 0.85f },
        { "NIGHTCORE",  10.f, 1.f,  2.f,  true,  500.f,  20000.f, 0.707f, 0.f,  0.6f, 0.5f  },
    };
    return presets;
}

//==============================================================================
// Layout constants
static constexpr int kTitleH    = 40;
static constexpr int kMacroH    = 90;
static constexpr int kScopeH    = 140;
static constexpr int kNumCols   = 6;
static constexpr int kPad       = 10;

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
    setSize (820, 620);
    setResizable (false, false);

    lnf = std::make_unique<CrushLookAndFeel>();

    addAndMakeVisible (scopeComponent);

    // --- Preset box ---
    presetBox.setLookAndFeel (lnf.get());
    presetBox.setTextWhenNothingSelected ("PRESETS");
    auto& presets = getPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        presetBox.addItem (presets[(size_t)i].name, i + 1);
    presetBox.onChange = [this]() {
        int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0) applyPreset (idx);
    };
    addAndMakeVisible (presetBox);

    // --- Setup helper ---
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
        lb.setColour            (juce::Label::textColourId, kWhite);
        lb.setFont              (crtFont (9.0f, true));
        addAndMakeVisible (lb);
    };

    // Macros (larger)
    setupSlider (glitchSlider, glitchLabel, "GLITCH", kCyan);
    setupSlider (meltSlider,   meltLabel,   "MELT",   kPurple);
    setupSlider (rektSlider,   rektLabel,   "REKT",   kHotPink);
    setupSlider (vibeSlider,   vibeLabel,   "VIBE",   kWhite);

    // Controls
    setupSlider (bitDepthSlider,     bitDepthLabel,     "CRUNCH",  kColCrush);
    setupSlider (downsampleSlider,   downsampleLabel,   "MANGLE",  kColCrush);
    setupSlider (driveSlider,        driveLabel,        "SMASH",   kColDrive);
    setupSlider (hpfCutoffSlider,    hpfCutoffLabel,    "SLICE",   kColFilter);
    setupSlider (lpfCutoffSlider,    lpfCutoffLabel,    "SCREAM",  kColFilter);
    setupSlider (filterResoSlider,   filterResoLabel,   "RESO",    kColFilter);
    setupSlider (stutterRateSlider,  stutterRateLabel,  "STUTTER", kColGate);
    setupSlider (stutterDepthSlider, stutterDepthLabel, "CHOP",    kColGate);
    setupSlider (dryWetSlider,       dryWetLabel,       "BLEND",   kColMix);

    // CLIP toggle
    clipPreButton.getProperties().set ("accentColour", (int64_t)(juce::uint32) kColDrive.getARGB());
    clipPreButton.setLookAndFeel (lnf.get());
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

    // Reset macros on preset load
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
        repaint();
    }
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

    // Title accent line
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

    // ---- Section headers in control grid ----
    const int controlTop = kTitleH + kMacroH + kScopeH + kPad * 2;
    const int colW = getWidth() / kNumCols;

    for (const auto& sec : kSections)
    {
        int x = sec.colStart * colW;
        int w = sec.colSpan * colW;

        // Header glow
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
        // Skip separator between col 0 and 1 (both CRUSH)
        if (col == 1) continue;
        g.setColour (kDim);
        g.fillRect (x, controlTop + 20, 1, getHeight() - controlTop - 24);
    }

    // Light separator between the two CRUSH columns
    g.setColour (kDim.withAlpha (0.3f));
    g.fillRect (colW, controlTop + 20, 1, getHeight() - controlTop - 24);

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
    presetBox.setBounds (getWidth() - 280, 8, 150, 24);

    // ---- Macro row ----
    {
        const int macroY = kTitleH + 5;
        const int macroKnobW = 120;
        const int totalMacroW = macroKnobW * 4 + kPad * 3;
        int mx = (getWidth() - totalMacroW) / 2;
        const int lblH = 13;
        const int knobH = kMacroH - lblH - 8;

        struct MacroInfo { juce::Slider& sl; juce::Label& lb; };
        MacroInfo macros[] = {
            { glitchSlider, glitchLabel },
            { meltSlider,   meltLabel   },
            { rektSlider,   rektLabel   },
            { vibeSlider,   vibeLabel   },
        };

        for (auto& m : macros)
        {
            m.lb.setBounds (mx, macroY, macroKnobW, lblH);
            m.sl.setBounds (mx, macroY + lblH, macroKnobW, knobH);
            mx += macroKnobW + kPad;
        }
    }

    // ---- Oscilloscope ----
    const int scopeY = kTitleH + kMacroH;
    scopeComponent.setBounds (kPad, scopeY, getWidth() - kPad * 2, kScopeH);

    // ---- Controls grid ----
    const int controlTop = scopeY + kScopeH + kPad;
    const int headerH = 20;
    const int bodyTop = controlTop + headerH;
    const int bodyH = getHeight() - bodyTop - 6;
    const int halfH = bodyH / 2;
    const int thirdH = bodyH / 3;
    const int lblH = 13;

    auto placeKnob = [&](juce::Slider& sl, juce::Label& lb, int col, int yOff, int cellH)
    {
        auto cell = juce::Rectangle<int> (col * colW, bodyTop + yOff, colW, cellH).reduced (8, 3);
        lb.setBounds (cell.removeFromTop (lblH));
        sl.setBounds (cell);
    };

    // Col 0 — CRUNCH (bit depth)
    placeKnob (bitDepthSlider,   bitDepthLabel,   0, 0,     halfH);
    // Col 1 — MANGLE (downsample)
    placeKnob (downsampleSlider, downsampleLabel, 1, 0,     halfH);

    // Center the single knobs vertically in their half
    // (leave bottom half of col 0 and 1 empty — clean look)

    // Col 2 — DRIVE: SMASH knob top half, CLIP toggle bottom half
    placeKnob (driveSlider, driveLabel, 2, 0, halfH);
    {
        auto cell = juce::Rectangle<int> (2 * colW, bodyTop + halfH, colW, halfH).reduced (8, 3);
        clipPreLabel.setBounds (cell.removeFromTop (lblH));
        clipPreButton.setBounds (cell.reduced (10, cell.getHeight() / 4));
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
