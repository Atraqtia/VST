#include "PluginProcessor.h"

ThiccBoxProcessor::ThiccBoxProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    fatness = apvts.getRawParameterValue ("fatness");
    mix     = apvts.getRawParameterValue ("mix");
}

juce::AudioProcessorValueTreeState::ParameterLayout ThiccBoxProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "fatness", 1 }, "Fatness",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
    return layout;
}

bool ThiccBoxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void ThiccBoxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) samplesPerBlock,
                                  (juce::uint32) getTotalNumOutputChannels() };

    compressor.prepare (spec);
    compressor.setAttack (5.0f);
    compressor.setRelease (90.0f);

    limiter.prepare (spec);
    limiter.setThreshold (-0.3f);
    limiter.setRelease (50.0f);

    oversampling.initProcessing ((size_t) samplesPerBlock);
    oversampling.reset();

    const auto latency = oversampling.getLatencyInSamples();
    setLatencySamples ((int) std::ceil (latency));

    mixer.prepare (spec);
    mixer.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    mixer.setWetLatency (latency);

    driveSmoothed.reset (sampleRate, 0.03);
    makeupSmoothed.reset (sampleRate, 0.03);
    const float f = fatness->load();
    driveSmoothed.setCurrentAndTargetValue (1.0f + f * 15.0f);
    makeupSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (f * 6.0f));
}

void ThiccBoxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const float f = fatness->load();

    // FATNESS 1本でコンプの深さ・ドライブ量・メイクアップをまとめて動かす
    compressor.setThreshold (-4.0f - f * 26.0f);   // 0..-30 dB 方向へ
    compressor.setRatio (1.5f + f * 6.5f);          // 1.5:1 .. 8:1
    driveSmoothed.setTargetValue (1.0f + f * 15.0f);
    makeupSmoothed.setTargetValue (juce::Decibels::decibelsToGain (f * 6.0f));

    juce::dsp::AudioBlock<float> block (buffer);
    auto ioBlock = block.getSubsetChannelBlock (0, (size_t) getTotalNumOutputChannels());

    mixer.setWetMixProportion (mix->load());
    mixer.pushDrySamples (ioBlock);

    juce::dsp::ProcessContextReplacing<float> ctx (ioBlock);
    compressor.process (ctx);

    // サチュレーション(4xオーバーサンプリングでエイリアシングを抑える)
    auto upBlock = oversampling.processSamplesUp (ioBlock);
    const float driveStart = driveSmoothed.getCurrentValue();
    const float driveEnd   = driveSmoothed.skip (buffer.getNumSamples());
    const auto numUp = upBlock.getNumSamples();

    for (size_t ch = 0; ch < upBlock.getNumChannels(); ++ch)
    {
        auto* d = upBlock.getChannelPointer (ch);
        for (size_t i = 0; i < numUp; ++i)
        {
            const float g = driveStart + (driveEnd - driveStart) * (float) i / (float) numUp;
            // tanh(gx)/tanh(g): ピークは 1.0 に揃えて、ドライブを上げるほど波形が「太く」なる
            d[i] = std::tanh (g * d[i]) / std::tanh (g);
        }
    }
    oversampling.processSamplesDown (ioBlock);

    const float makeupStart = makeupSmoothed.getCurrentValue();
    const float makeupEnd   = makeupSmoothed.skip (buffer.getNumSamples());
    const int n = buffer.getNumSamples();
    for (int ch = 0; ch < (int) ioBlock.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            d[i] *= makeupStart + (makeupEnd - makeupStart) * (float) i / (float) n;
    }

    limiter.process (ctx);
    mixer.mixWetSamples (ioBlock);
}

juce::AudioProcessorEditor* ThiccBoxProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void ThiccBoxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ThiccBoxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ThiccBoxProcessor();
}
