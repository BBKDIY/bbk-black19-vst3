#pragma once

#include "public.sdk/source/vst/vstcomponent.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"
#include "Black19DSP.h"

#include <atomic>
#include <vector>
#include <cmath>

namespace bbk {

static const Steinberg::FUID kBlack19ProcessorUID (0xB1AC1900, 0xBBB1AC19, 0xB1AC1900, 0xBBB1AC19);

enum ParamIds : Steinberg::Vst::ParamID
{
    kEnabledId = 1,
    kStatusId  = 2
};

static constexpr int kMaxChannels = 8;

class Black19Component;

class Black19AudioProcessor : public Steinberg::Vst::IAudioProcessor
{
public:
    explicit Black19AudioProcessor (Black19Component& o) : owner (o) {}

    Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID iid, void** obj) SMTG_OVERRIDE;
    Steinberg::uint32 PLUGIN_API addRef () SMTG_OVERRIDE;
    Steinberg::uint32 PLUGIN_API release () SMTG_OVERRIDE;

    Steinberg::tresult PLUGIN_API setBusArrangements (Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
                                                        Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getBusArrangement (Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::SpeakerArrangement& arr) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
    Steinberg::uint32 PLUGIN_API getLatencySamples () SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& setup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setProcessing (Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::uint32 PLUGIN_API getTailSamples () SMTG_OVERRIDE { return 0; }

private:
    Black19Component& owner;
};

class Black19EditController : public Steinberg::Vst::IEditController
{
public:
    explicit Black19EditController (Black19Component& o) : owner (o) {}

    Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID iid, void** obj) SMTG_OVERRIDE;
    Steinberg::uint32 PLUGIN_API addRef () SMTG_OVERRIDE;
    Steinberg::uint32 PLUGIN_API release () SMTG_OVERRIDE;

    Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::int32 PLUGIN_API getParameterCount () SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParameterInfo (Steinberg::int32 paramIndex, Steinberg::Vst::ParameterInfo& info) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParamStringByValue (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized, Steinberg::Vst::String128 string) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParamValueByString (Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar* string, Steinberg::Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;
    Steinberg::Vst::ParamValue PLUGIN_API normalizedParamToPlain (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized) SMTG_OVERRIDE;
    Steinberg::Vst::ParamValue PLUGIN_API plainParamToNormalized (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue plainValue) SMTG_OVERRIDE;
    Steinberg::Vst::ParamValue PLUGIN_API getParamNormalized (Steinberg::Vst::ParamID tag) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setParamNormalized (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentHandler (Steinberg::Vst::IComponentHandler* handler) SMTG_OVERRIDE;
    Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) SMTG_OVERRIDE;

private:
    Black19Component& owner;
};

class Black19Component : public Steinberg::Vst::Component
{
public:
    Black19Component();
    ~Black19Component() override = default;

    static Steinberg::FUnknown* createInstance (void*) { return (Steinberg::Vst::IComponent*) new Black19Component(); }

    Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;

    Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID iid, void** obj) SMTG_OVERRIDE;
    Steinberg::uint32 PLUGIN_API addRef () SMTG_OVERRIDE { return Component::addRef(); }
    Steinberg::uint32 PLUGIN_API release () SMTG_OVERRIDE { return Component::release(); }

    Steinberg::tresult processorSetBusArrangements (Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
                                                     Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts);
    Steinberg::tresult processorGetBusArrangement (Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::SpeakerArrangement& arr);
    Steinberg::tresult processorCanProcessSampleSize (Steinberg::int32 symbolicSampleSize);
    Steinberg::uint32 processorGetLatencySamples ();
    Steinberg::tresult processorSetupProcessing (Steinberg::Vst::ProcessSetup& setup);
    Steinberg::tresult processorSetProcessing (Steinberg::TBool state);
    Steinberg::tresult processorProcess (Steinberg::Vst::ProcessData& data);

    Steinberg::tresult controllerInitialize (Steinberg::FUnknown* context);
    Steinberg::tresult controllerTerminate ();
    Steinberg::tresult controllerSetComponentState (Steinberg::IBStream* state);
    Steinberg::tresult controllerSetState (Steinberg::IBStream* state);
    Steinberg::tresult controllerGetState (Steinberg::IBStream* state);
    Steinberg::int32 controllerGetParameterCount ();
    Steinberg::tresult controllerGetParameterInfo (Steinberg::int32 paramIndex, Steinberg::Vst::ParameterInfo& info);
    Steinberg::tresult controllerGetParamStringByValue (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized, Steinberg::Vst::String128 string);
    Steinberg::tresult controllerGetParamValueByString (Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar* string, Steinberg::Vst::ParamValue& valueNormalized);
    Steinberg::Vst::ParamValue controllerNormalizedParamToPlain (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized);
    Steinberg::Vst::ParamValue controllerPlainParamToNormalized (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue plainValue);
    Steinberg::Vst::ParamValue controllerGetParamNormalized (Steinberg::Vst::ParamID tag);
    Steinberg::tresult controllerSetParamNormalized (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value);
    Steinberg::tresult controllerSetComponentHandler (Steinberg::Vst::IComponentHandler* handler);
    Steinberg::IPlugView* controllerCreateView (Steinberg::FIDString name);

    double getHostSampleRateForUI() const { return currentSampleRate.load(); }
    bool   isRateValidForUI() const { return valid192k.load(); }
    bool   isEnabledForUI() const { return enabledRequested.load(); }
    void   setEnabledFromUI (bool on);

    int getLastInChannelsForUI() const { return lastInChannels.load(); }
    int getLastOutChannelsForUI() const { return lastOutChannels.load(); }
    int getLastProcessedChannelsForUI() const { return lastProcessedChannels.load(); }
    int getLastBlockSizeForUI() const { return lastBlockSize.load(); }
    int getLastSymbolicSampleSizeForUI() const { return lastSymbolicSampleSize.load(); }

    double getLastProcessMicrosForUI() const { return lastProcessMicros.load(); }
    double getMaxProcessMicrosForUI() const { return maxProcessMicros.load(); }
    double getBudgetMicrosForUI() const { return budgetMicros.load(); }

    OBJ_METHODS (Black19Component, Steinberg::Vst::Component)

private:
    Black19AudioProcessor audioProcessorTearOff { *this };
    Black19EditController editControllerTearOff { *this };

    std::vector<ChannelState> channels;
    LinearSmoother wetMix;

    std::atomic<double> currentSampleRate { 0.0 };
    std::atomic<bool> valid192k { false };
    std::atomic<bool> enabledRequested { false };

    std::atomic<int> lastInChannels { -1 };
    std::atomic<int> lastOutChannels { -1 };
    std::atomic<int> lastProcessedChannels { -1 };
    std::atomic<int> lastBlockSize { -1 };
    std::atomic<int> lastSymbolicSampleSize { -1 };

    std::atomic<double> lastProcessMicros { -1.0 };
    std::atomic<double> maxProcessMicros { 0.0 };
    std::atomic<double> budgetMicros { -1.0 };

    int maxBlockSize = 0;

    Steinberg::Vst::IComponentHandler* componentHandler = nullptr;
    Steinberg::Vst::ParameterContainer parameters;

    template <typename SampleType>
    void processTyped (Steinberg::Vst::AudioBusBuffers& in, Steinberg::Vst::AudioBusBuffers& out, Steinberg::int32 numSamples);
};

} // namespace bbk
