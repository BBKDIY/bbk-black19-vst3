#include "Black19Processor.h"
#include "public.sdk/source/vst/vstbus.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/base/ustring.h"
#include <cstring>
#include <cstdio>
#include <chrono>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace bbk {

IPlugView* createBlack19View (Black19Component*);

Black19Component::Black19Component()
{
    setControllerClass (kBlack19ProcessorUID);
}

tresult PLUGIN_API Black19Component::queryInterface (const TUID iid, void** obj)
{
    tresult r = Component::queryInterface (iid, obj);
    if (r == kResultOk)
        return r;

    if (FUnknownPrivate::iidEqual (iid, IAudioProcessor::iid))
    {
        audioProcessorTearOff.addRef();
        *obj = static_cast<IAudioProcessor*> (&audioProcessorTearOff);
        return kResultOk;
    }
    if (FUnknownPrivate::iidEqual (iid, IEditController::iid))
    {
        editControllerTearOff.addRef();
        *obj = static_cast<IEditController*> (&editControllerTearOff);
        return kResultOk;
    }

    *obj = nullptr;
    return kNoInterface;
}

tresult PLUGIN_API Black19Component::initialize (FUnknown* context)
{
    tresult result = Component::initialize (context);
    if (result != kResultOk)
        return result;

    audioInputs.push_back (IPtr<Vst::Bus> (new AudioBus (STR16 ("Stereo In"), kMain, BusInfo::kDefaultActive, SpeakerArr::kStereo), false));
    audioOutputs.push_back (IPtr<Vst::Bus> (new AudioBus (STR16 ("Stereo Out"), kMain, BusInfo::kDefaultActive, SpeakerArr::kStereo), false));

    parameters.addParameter (STR16 ("Black-19 enabled"), nullptr, 1, 0.0,
                              ParameterInfo::kCanAutomate, kEnabledId, 0, STR16 ("On/Off"));
    parameters.addParameter (STR16 ("Status"), nullptr, 0, 0.0,
                              ParameterInfo::kIsReadOnly, kStatusId, 0, STR16 ("Status"));

    return kResultOk;
}

tresult PLUGIN_API Black19Component::terminate ()
{
    return Component::terminate ();
}

tresult PLUGIN_API Black19Component::setActive (TBool state)
{
    if (state && channels.size() != (size_t) kMaxChannels)
    {
        channels.assign (kMaxChannels, ChannelState {});
        for (auto& c : channels) c.clear();
    }
    return Component::setActive (state);
}

tresult PLUGIN_API Black19Component::setState (IBStream* state)
{
    if (! state) return kResultFalse;
    IBStreamer streamer (state, kLittleEndian);
    float savedEnabled = 0.0f;
    if (! streamer.readFloat (savedEnabled))
        return kResultFalse;
    enabledRequested.store (savedEnabled >= 0.5f);
    return kResultOk;
}

tresult PLUGIN_API Black19Component::getState (IBStream* state)
{
    if (! state) return kResultFalse;
    IBStreamer streamer (state, kLittleEndian);
    float v = enabledRequested.load() ? 1.0f : 0.0f;
    streamer.writeFloat (v);
    return kResultOk;
}

tresult Black19Component::processorSetBusArrangements (SpeakerArrangement* inputs, int32 numIns,
                                                        SpeakerArrangement* outputs, int32 numOuts)
{
    if (numIns == 1 && numOuts == 1 && inputs[0] == SpeakerArr::kStereo && outputs[0] == SpeakerArr::kStereo)
        return kResultTrue;
    return kResultFalse;
}

tresult Black19Component::processorGetBusArrangement (BusDirection /*dir*/, int32 index, SpeakerArrangement& arr)
{
    if (index != 0) return kResultFalse;
    arr = SpeakerArr::kStereo;
    return kResultTrue;
}

tresult Black19Component::processorCanProcessSampleSize (int32 symbolicSampleSize)
{
    return (symbolicSampleSize == kSample32) ? kResultTrue : kResultFalse;
}

uint32 Black19Component::processorGetLatencySamples ()
{
    return 0u;
}

tresult Black19Component::processorSetupProcessing (ProcessSetup& setup)
{
    const double oldSampleRate = currentSampleRate.load();
    const bool sampleRateChanged = std::abs (oldSampleRate - setup.sampleRate) > 0.5;
    const bool needsInit = sampleRateChanged || (channels.size() != (size_t) kMaxChannels);

    currentSampleRate.store (setup.sampleRate);
    valid192k.store (std::abs (setup.sampleRate - 192000.0) < 1.0);
    maxBlockSize = setup.maxSamplesPerBlock;

    if (needsInit)
    {
        wetMix.reset (setup.sampleRate > 0 ? setup.sampleRate : 44100.0, 0.010);
        wetMix.setCurrentAndTargetValue ((valid192k.load() && enabledRequested.load()) ? 1.0 : 0.0);

        channels.assign (kMaxChannels, ChannelState {});
        for (auto& c : channels) c.clear();
    }

    return kResultOk;
}

tresult Black19Component::processorSetProcessing (TBool /*state*/)
{
    return kResultOk;
}

template <typename SampleType>
void Black19Component::processTyped (AudioBusBuffers& in, AudioBusBuffers& out, int32 numSamples)
{
    const int numChannels = std::min ({ in.numChannels, out.numChannels, (int32) channels.size() });

    lastInChannels.store (in.numChannels);
    lastOutChannels.store (out.numChannels);
    lastProcessedChannels.store (numChannels);
    lastBlockSize.store (numSamples);
    lastSymbolicSampleSize.store (sizeof (SampleType) == 4 ? 32 : 64);

    for (int32 s = 0; s < numSamples; ++s)
    {
        const bool requested = enabledRequested.load();
        const bool valid = valid192k.load();
        wetMix.setTargetValue ((valid && requested) ? 1.0 : 0.0);
        const double mix = valid ? wetMix.getNextValue() : 0.0;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            SampleType* inBuf  = (SampleType*) (sizeof (SampleType) == 4 ? (void*) in.channelBuffers32[ch] : (void*) in.channelBuffers64[ch]);
            SampleType* outBuf = (SampleType*) (sizeof (SampleType) == 4 ? (void*) out.channelBuffers32[ch] : (void*) out.channelBuffers64[ch]);
            const double x = (double) inBuf[s];

            double y;
            if (valid)
                y = processSample (channels[(size_t) ch], x, mix);
            else
                y = x;

            if (y > 1.0) y = 1.0;
            else if (y < -1.0) y = -1.0;

            outBuf[s] = (SampleType) y;
        }
    }
}

tresult Black19Component::processorProcess (ProcessData& data)
{
    const auto t0 = std::chrono::high_resolution_clock::now();

    if (data.inputParameterChanges)
    {
        int32 n = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < n; ++i)
        {
            IParamValueQueue* q = data.inputParameterChanges->getParameterData (i);
            if (! q) continue;
            if (q->getParameterId() == kEnabledId)
            {
                int32 sampleOffset; ParamValue value;
                int32 pts = q->getPointCount();
                if (pts > 0 && q->getPoint (pts - 1, sampleOffset, value) == kResultTrue)
                    enabledRequested.store (value >= 0.5);
            }
        }
    }

    if (data.numSamples <= 0 || data.numInputs < 1 || data.numOutputs < 1)
        return kResultOk;

    auto& in  = data.inputs[0];
    auto& out = data.outputs[0];

    if (data.symbolicSampleSize == kSample64)
        processTyped<Sample64> (in, out, data.numSamples);
    else
        processTyped<Sample32> (in, out, data.numSamples);

    const auto t1 = std::chrono::high_resolution_clock::now();
    const double micros = std::chrono::duration<double, std::micro> (t1 - t0).count();
    lastProcessMicros.store (micros);
    if (micros > maxProcessMicros.load())
        maxProcessMicros.store (micros);
    const double sr = currentSampleRate.load();
    if (sr > 0.0)
        budgetMicros.store (1000000.0 * (double) data.numSamples / sr);

    return kResultOk;
}

tresult Black19Component::controllerInitialize (FUnknown* /*context*/) { return kResultOk; }
tresult Black19Component::controllerTerminate () { return kResultOk; }

tresult Black19Component::controllerSetComponentState (IBStream* state) { return setState (state); }
tresult Black19Component::controllerSetState (IBStream* state) { return setState (state); }
tresult Black19Component::controllerGetState (IBStream* state) { return getState (state); }

int32 Black19Component::controllerGetParameterCount () { return parameters.getParameterCount(); }

tresult Black19Component::controllerGetParameterInfo (int32 paramIndex, ParameterInfo& info)
{
    Parameter* p = parameters.getParameterByIndex (paramIndex);
    if (! p) return kResultFalse;
    info = p->getInfo();
    return kResultTrue;
}

tresult Black19Component::controllerGetParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string)
{
    if (tag == kStatusId)
    {
        char buf[128];
        const double sr = currentSampleRate.load();
        if (valid192k.load())
        {
            const bool on = enabledRequested.load();
            std::snprintf (buf, sizeof (buf), "%.0f Hz | %s", sr, on ? "ACTIVE - 192 kHz" : "READY - 192 kHz (bypassed)");
        }
        else
        {
            std::snprintf (buf, sizeof (buf), "%.0f Hz | BYPASSED - requires 192 kHz", sr);
        }
        Steinberg::UString (string, str16BufferSize (String128)).fromAscii (buf);
        return kResultTrue;
    }
    if (tag == kEnabledId)
    {
        const char* txt = (valueNormalized >= 0.5) ? "On" : "Bypass";
        Steinberg::UString (string, str16BufferSize (String128)).fromAscii (txt);
        return kResultTrue;
    }
    return kResultFalse;
}

tresult Black19Component::controllerGetParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized)
{
    if (tag == kEnabledId)
    {
        Steinberg::UString wrapper (string, 128);
        char ascii[128] = {};
        wrapper.toAscii (ascii, 128);
        valueNormalized = (std::strstr (ascii, "On") != nullptr) ? 1.0 : 0.0;
        return kResultTrue;
    }
    return kResultFalse;
}

ParamValue Black19Component::controllerNormalizedParamToPlain (ParamID, ParamValue v) { return v; }
ParamValue Black19Component::controllerPlainParamToNormalized (ParamID, ParamValue v) { return v; }

ParamValue Black19Component::controllerGetParamNormalized (ParamID tag)
{
    if (tag == kEnabledId) return enabledRequested.load() ? 1.0 : 0.0;
    if (tag == kStatusId)  return 0.0;
    Parameter* p = parameters.getParameter (tag);
    return p ? p->getNormalized() : 0.0;
}

tresult Black19Component::controllerSetParamNormalized (ParamID tag, ParamValue value)
{
    if (tag == kEnabledId)
    {
        enabledRequested.store (value >= 0.5);
        Parameter* p = parameters.getParameter (tag);
        if (p) p->setNormalized (value);
        return kResultTrue;
    }
    return kResultFalse;
}

tresult Black19Component::controllerSetComponentHandler (IComponentHandler* handler)
{
    componentHandler = handler;
    return kResultOk;
}

IPlugView* Black19Component::controllerCreateView (FIDString /*name*/)
{
    return createBlack19View (this);
}

void Black19Component::setEnabledFromUI (bool on)
{
    enabledRequested.store (on);
    if (componentHandler)
    {
        componentHandler->beginEdit (kEnabledId);
        componentHandler->performEdit (kEnabledId, on ? 1.0 : 0.0);
        componentHandler->endEdit (kEnabledId);
    }
}

tresult PLUGIN_API Black19AudioProcessor::queryInterface (const TUID iid, void** obj) { return owner.queryInterface (iid, obj); }
uint32 PLUGIN_API Black19AudioProcessor::addRef () { return owner.addRef(); }
uint32 PLUGIN_API Black19AudioProcessor::release () { return owner.release(); }

tresult PLUGIN_API Black19AudioProcessor::setBusArrangements (SpeakerArrangement* i, int32 ni, SpeakerArrangement* o, int32 no) { return owner.processorSetBusArrangements (i, ni, o, no); }
tresult PLUGIN_API Black19AudioProcessor::getBusArrangement (BusDirection d, int32 idx, SpeakerArrangement& a) { return owner.processorGetBusArrangement (d, idx, a); }
tresult PLUGIN_API Black19AudioProcessor::canProcessSampleSize (int32 s) { return owner.processorCanProcessSampleSize (s); }
uint32 PLUGIN_API Black19AudioProcessor::getLatencySamples () { return owner.processorGetLatencySamples(); }
tresult PLUGIN_API Black19AudioProcessor::setupProcessing (ProcessSetup& s) { return owner.processorSetupProcessing (s); }
tresult PLUGIN_API Black19AudioProcessor::setProcessing (TBool s) { return owner.processorSetProcessing (s); }
tresult PLUGIN_API Black19AudioProcessor::process (ProcessData& d) { return owner.processorProcess (d); }

tresult PLUGIN_API Black19EditController::queryInterface (const TUID iid, void** obj) { return owner.queryInterface (iid, obj); }
uint32 PLUGIN_API Black19EditController::addRef () { return owner.addRef(); }
uint32 PLUGIN_API Black19EditController::release () { return owner.release(); }

tresult PLUGIN_API Black19EditController::initialize (FUnknown* c) { return owner.controllerInitialize (c); }
tresult PLUGIN_API Black19EditController::terminate () { return owner.controllerTerminate(); }
tresult PLUGIN_API Black19EditController::setComponentState (IBStream* s) { return owner.controllerSetComponentState (s); }
tresult PLUGIN_API Black19EditController::setState (IBStream* s) { return owner.controllerSetState (s); }
tresult PLUGIN_API Black19EditController::getState (IBStream* s) { return owner.controllerGetState (s); }
int32 PLUGIN_API Black19EditController::getParameterCount () { return owner.controllerGetParameterCount(); }
tresult PLUGIN_API Black19EditController::getParameterInfo (int32 idx, ParameterInfo& info) { return owner.controllerGetParameterInfo (idx, info); }
tresult PLUGIN_API Black19EditController::getParamStringByValue (ParamID t, ParamValue v, String128 s) { return owner.controllerGetParamStringByValue (t, v, s); }
tresult PLUGIN_API Black19EditController::getParamValueByString (ParamID t, TChar* s, ParamValue& v) { return owner.controllerGetParamValueByString (t, s, v); }
ParamValue PLUGIN_API Black19EditController::normalizedParamToPlain (ParamID t, ParamValue v) { return owner.controllerNormalizedParamToPlain (t, v); }
ParamValue PLUGIN_API Black19EditController::plainParamToNormalized (ParamID t, ParamValue v) { return owner.controllerPlainParamToNormalized (t, v); }
ParamValue PLUGIN_API Black19EditController::getParamNormalized (ParamID t) { return owner.controllerGetParamNormalized (t); }
tresult PLUGIN_API Black19EditController::setParamNormalized (ParamID t, ParamValue v) { return owner.controllerSetParamNormalized (t, v); }
tresult PLUGIN_API Black19EditController::setComponentHandler (IComponentHandler* h) { return owner.controllerSetComponentHandler (h); }
IPlugView* PLUGIN_API Black19EditController::createView (FIDString n) { return owner.controllerCreateView (n); }

} // namespace bbk
