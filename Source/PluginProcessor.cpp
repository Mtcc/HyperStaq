#include "PluginProcessor.h"
#include "PluginEditor.h"

// Musical note divisions in beats (1/32 -> 4 bars)
static constexpr float kRateBeats[8] = { 0.03125f, 0.0625f, 0.125f, 0.25f,
                                          0.5f,     1.0f,    2.0f,   4.0f };
static const juce::StringArray kRateLabels { "1/32","1/16","1/8","1/4","1/2","1","2","4" };

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout HyperCrushProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "bitDepth", 1 }, "Bit Depth",
        juce::NormalisableRange<float> (1.0f, 16.0f, 0.01f), 16.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("bits")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "downsample", 1 }, "Sample Rate",
        juce::NormalisableRange<float> (1.0f, 32.0f, 0.01f), 1.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("x")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 }, "Drive",
        juce::NormalisableRange<float> (0.0f, 40.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "clipPre", 1 }, "Clip Pre", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "hpfCutoff", 1 }, "HPF Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 20.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lpfCutoff", 1 }, "LPF Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 20000.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filterReso", 1 }, "Filter Reso",
        juce::NormalisableRange<float> (0.5f, 10.0f, 0.01f, 0.4f), 0.707f,
        juce::AudioParameterFloatAttributes{}.withLabel ("Q")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "stutterRate", 1 }, "Stutter Rate",
        juce::NormalisableRange<float> (0.0f, 7.0f, 1.0f), 3.0f,
        juce::AudioParameterFloatAttributes{}
            .withLabel ("div")
            .withStringFromValueFunction ([](float v, int) {
                return kRateLabels[juce::jlimit (0, 7, (int) v)]; })
            .withValueFromStringFunction ([](const juce::String& s) -> float {
                int i = kRateLabels.indexOf (s);
                return i >= 0 ? (float) i : 3.0f; })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "stutterDepth", 1 }, "Stutter Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dryWet", 1 }, "Dry/Wet",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("%")));

    // Macros (0..1, UI-only controls that morph other params)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "macroGlitch", 1 }, "Glitch",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "macroMelt", 1 }, "Melt",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "macroRekt", 1 }, "Rekt",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "macroVibe", 1 }, "Vibe",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
HyperCrushProcessor::HyperCrushProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    const juce::StringArray ids { "bitDepth", "downsample", "drive", "clipPre",
                                   "hpfCutoff", "lpfCutoff", "filterReso",
                                   "stutterRate", "stutterDepth", "dryWet",
                                   "macroGlitch", "macroMelt", "macroRekt", "macroVibe" };
    for (auto& id : ids)
        apvts.addParameterListener (id, this);
}

HyperCrushProcessor::~HyperCrushProcessor()
{
    const juce::StringArray ids { "bitDepth", "downsample", "drive", "clipPre",
                                   "hpfCutoff", "lpfCutoff", "filterReso",
                                   "stutterRate", "stutterDepth", "dryWet",
                                   "macroGlitch", "macroMelt", "macroRekt", "macroVibe" };
    for (auto& id : ids)
        apvts.removeParameterListener (id, this);
}

//==============================================================================
void HyperCrushProcessor::parameterChanged (const juce::String& id, float v)
{
    if      (id == "bitDepth")     bitDepth.store     (v);
    else if (id == "downsample")   downsample.store   (v);
    else if (id == "drive")        drive.store        (v);
    else if (id == "clipPre")      clipPre.store      (v > 0.5f);
    else if (id == "hpfCutoff")    hpfCutoff.store    (v);
    else if (id == "lpfCutoff")    lpfCutoff.store    (v);
    else if (id == "filterReso")   filterReso.store   (v);
    else if (id == "stutterRate")  stutterRate.store  (v);
    else if (id == "stutterDepth") stutterDepth.store (v);
    else if (id == "dryWet")       dryWet.store       (v);
    else if (id == "macroGlitch")  macroGlitch.store  (v);
    else if (id == "macroMelt")    macroMelt.store    (v);
    else if (id == "macroRekt")    macroRekt.store    (v);
    else if (id == "macroVibe")    macroVibe.store    (v);
}

//==============================================================================
void HyperCrushProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    isPrepared.store (false, std::memory_order_release);

    currentSampleRate = sampleRate;

    for (int ch = 0; ch < MAX_CHANNELS; ++ch)
    {
        heldSample[ch]    = 0.0f;
        sampleCounter[ch] = 0;
        hpfZ1[ch]         = 0.0;
        hpfZ2[ch]         = 0.0;
        lpfZ1[ch]         = 0.0;
        lpfZ2[ch]         = 0.0;
    }
    stutterPhase = 0.0;

    for (int i = 0; i < SCOPE_SIZE; ++i)
        scopeBuffer[i].store (0.0f, std::memory_order_relaxed);
    scopeWritePos.store (0, std::memory_order_relaxed);

    bitDepth.store     (*apvts.getRawParameterValue ("bitDepth"));
    downsample.store   (*apvts.getRawParameterValue ("downsample"));
    drive.store        (*apvts.getRawParameterValue ("drive"));
    clipPre.store      (*apvts.getRawParameterValue ("clipPre") > 0.5f);
    hpfCutoff.store    (*apvts.getRawParameterValue ("hpfCutoff"));
    lpfCutoff.store    (*apvts.getRawParameterValue ("lpfCutoff"));
    filterReso.store   (*apvts.getRawParameterValue ("filterReso"));
    stutterRate.store  (*apvts.getRawParameterValue ("stutterRate"));
    stutterDepth.store (*apvts.getRawParameterValue ("stutterDepth"));
    dryWet.store       (*apvts.getRawParameterValue ("dryWet"));
    macroGlitch.store  (*apvts.getRawParameterValue ("macroGlitch"));
    macroMelt.store    (*apvts.getRawParameterValue ("macroMelt"));
    macroRekt.store    (*apvts.getRawParameterValue ("macroRekt"));
    macroVibe.store    (*apvts.getRawParameterValue ("macroVibe"));

    isPrepared.store (true, std::memory_order_release);
}

void HyperCrushProcessor::releaseResources()
{
    isPrepared.store (false, std::memory_order_release);
}

//==============================================================================
void HyperCrushProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& /*midiMessages*/)
{
    if (!isPrepared.load (std::memory_order_acquire))
    {
        buffer.clear();
        return;
    }

    try
    {
        juce::ScopedNoDenormals noDenormals;

        // --- Snapshot base parameters ---
        float bd         = bitDepth.load();
        float ds         = downsample.load();
        float driveDb    = drive.load();
        bool  isPreClip  = clipPre.load();
        float hpCut      = hpfCutoff.load();
        float lpCut      = lpfCutoff.load();
        float reso       = filterReso.load();
        int   rateIdx    = juce::jlimit (0, 7, (int) stutterRate.load());
        float depth      = stutterDepth.load();
        float mix        = dryWet.load();

        // --- Apply macros (blend toward target values) ---
        const float glitch = macroGlitch.load();
        const float melt   = macroMelt.load();
        const float rekt   = macroRekt.load();
        const float vibe   = macroVibe.load();

        // GLITCH: crush + drive + fast gate
        if (glitch > 0.001f)
        {
            bd      = bd      + (1.0f - bd) * glitch;          // toward 1-bit
            driveDb = driveDb + (30.0f - driveDb) * glitch;    // toward 30dB
            rateIdx = juce::jlimit (0, 7, (int)(rateIdx + (7 - rateIdx) * glitch + 0.5f)); // toward 1/32
            depth   = depth   + (1.0f - depth) * glitch;
        }

        // MELT: LP sweep down + reso up + slow gate
        if (melt > 0.001f)
        {
            lpCut   = lpCut   + (200.0f - lpCut) * melt;      // toward 200Hz
            reso    = reso    + (8.0f - reso) * melt;          // toward high reso
            rateIdx = juce::jlimit (0, 7, (int)(rateIdx + (0 - rateIdx) * melt + 0.5f)); // toward slowest
        }

        // REKT: max drive + 3-bit + 1/16 gate + full wet
        if (rekt > 0.001f)
        {
            driveDb = driveDb + (40.0f - driveDb) * rekt;
            bd      = bd      + (3.0f - bd) * rekt;
            rateIdx = juce::jlimit (0, 7, (int)(rateIdx + (1 - rateIdx) * rekt + 0.5f)); // 1/16
            depth   = depth   + (1.0f - depth) * rekt;
            mix     = mix     + (1.0f - mix) * rekt;
            isPreClip = true;
        }

        // VIBE: subtle crush + slow gate + light drive + 70% wet
        if (vibe > 0.001f)
        {
            bd      = bd      + (10.0f - bd) * vibe;
            rateIdx = juce::jlimit (0, 7, (int)(rateIdx + (4 - rateIdx) * vibe + 0.5f)); // 1/2
            depth   = depth   + (0.4f - depth) * vibe;
            driveDb = driveDb + (5.0f - driveDb) * vibe;
            mix     = mix     + (0.7f - mix) * vibe;
        }

        // --- Derived values ---
        const float cutHz    = juce::jlimit (20.0f, (float)(currentSampleRate * 0.45), hpCut);
        const float Q        = juce::jlimit (0.5f, 10.0f, reso);
        const float lpfCutHz = juce::jlimit (20.0f, (float)(currentSampleRate * 0.45), lpCut);
        const float lpfQ     = Q; // shared resonance

        const int   hold      = juce::jmax (1, (int) ds);
        const float levels    = std::pow (2.0f, bd - 1.0f) - 1.0f;
        const bool  crushOn   = (bd < 15.99f);
        const bool  dsOn      = (hold > 1);
        const bool  driveOn   = (driveDb > 0.05f);
        const bool  hpfOn     = (cutHz > 25.0f);
        const bool  lpfOn     = (lpfCutHz < (float)(currentSampleRate * 0.44));
        const bool  stutterOn = (depth > 0.001f);

        const float driveGain = driveOn ? std::pow (10.0f, driveDb / 20.0f) : 1.0f;

        // --- HPF biquad coefficients ---
        double hB0 = 1.0, hB1 = 0.0, hB2 = 0.0, hA1 = 0.0, hA2 = 0.0;
        if (hpfOn)
        {
            const double w0    = juce::MathConstants<double>::twoPi * cutHz / currentSampleRate;
            const double cosw0 = std::cos (w0);
            const double sinw0 = std::sin (w0);
            const double alpha = sinw0 / (2.0 * Q);
            const double a0inv = 1.0 / (1.0 + alpha);
            hB0 =  ((1.0 + cosw0) * 0.5)  * a0inv;
            hB1 = -(1.0 + cosw0)           * a0inv;
            hB2 =  hB0;
            hA1 = (-2.0 * cosw0)           * a0inv;
            hA2 =  (1.0 - alpha)           * a0inv;
        }

        // --- LPF biquad coefficients ---
        double lB0 = 1.0, lB1 = 0.0, lB2 = 0.0, lA1 = 0.0, lA2 = 0.0;
        if (lpfOn)
        {
            const double w0    = juce::MathConstants<double>::twoPi * lpfCutHz / currentSampleRate;
            const double cosw0 = std::cos (w0);
            const double sinw0 = std::sin (w0);
            const double alpha = sinw0 / (2.0 * lpfQ);
            const double a0inv = 1.0 / (1.0 + alpha);
            lB0 =  ((1.0 - cosw0) * 0.5)  * a0inv;
            lB1 =   (1.0 - cosw0)          * a0inv;
            lB2 =  lB0;
            lA1 = (-2.0 * cosw0)           * a0inv;
            lA2 =  (1.0 - alpha)           * a0inv;
        }

        // --- Stutter ---
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                if (auto b = pos->getBpm())
                    bpm = juce::jlimit (20.0, 300.0, *b);
                if (stutterOn)
                    if (auto ppq = pos->getPpqPosition())
                        stutterPhase = std::fmod (*ppq / kRateBeats[rateIdx], 1.0);
            }

        const double phaseInc = (bpm / 60.0 / kRateBeats[rateIdx]) / currentSampleRate;

        const int totalCh  = buffer.getNumChannels();
        const int dspCh    = juce::jmin (totalCh, MAX_CHANNELS);
        const int nSamples = buffer.getNumSamples();

        for (int i = 0; i < nSamples; ++i)
        {
            const float gate = stutterOn
                ? ((stutterPhase < 0.5) ? 1.0f : (1.0f - depth))
                : 1.0f;

            stutterPhase += phaseInc;
            if (stutterPhase >= 1.0) stutterPhase -= 1.0;

            for (int ch = 0; ch < dspCh; ++ch)
            {
                const float dry = buffer.getSample (ch, i);
                float s = dry;

                if (driveOn && isPreClip)
                    s = juce::jlimit (-1.0f, 1.0f, s * driveGain);

                if (dsOn)
                {
                    if (sampleCounter[ch] >= hold)
                    {
                        heldSample[ch]    = s;
                        sampleCounter[ch] = 0;
                    }
                    s = heldSample[ch];
                    ++sampleCounter[ch];
                }

                if (crushOn && levels > 0.0f)
                    s = std::round (s * levels) / levels;

                if (driveOn && !isPreClip)
                    s = juce::jlimit (-1.0f, 1.0f, s * driveGain);

                if (hpfOn)
                {
                    const double in  = s;
                    const double out = hB0 * in + hpfZ1[ch];
                    hpfZ1[ch]        = hB1 * in - hA1 * out + hpfZ2[ch];
                    hpfZ2[ch]        = hB2 * in - hA2 * out;
                    s = (float) out;
                }

                if (lpfOn)
                {
                    const double in  = s;
                    const double out = lB0 * in + lpfZ1[ch];
                    lpfZ1[ch]        = lB1 * in - lA1 * out + lpfZ2[ch];
                    lpfZ2[ch]        = lB2 * in - lA2 * out;
                    s = (float) out;
                }

                s *= gate;
                s = dry + (s - dry) * mix;
                buffer.setSample (ch, i, s);
            }

            {
                float scopeSample = buffer.getSample (0, i);
                if (dspCh > 1)
                    scopeSample = (scopeSample + buffer.getSample (1, i)) * 0.5f;
                int wp = scopeWritePos.load (std::memory_order_relaxed);
                scopeBuffer[wp].store (scopeSample, std::memory_order_relaxed);
                scopeWritePos.store ((wp + 1) % SCOPE_SIZE, std::memory_order_relaxed);
            }
        }
    }
    catch (...)
    {
        buffer.clear();
    }
}

//==============================================================================
juce::AudioProcessorEditor* HyperCrushProcessor::createEditor()
{
    return new HyperCrushEditor (*this);
}

//==============================================================================
void HyperCrushProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HyperCrushProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HyperCrushProcessor();
}
