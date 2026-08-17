#include "Black19Processor.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/base/funknown.h"
#include <cstring>
#include <atomic>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Steinberg {
DEF_CLASS_IID (IPlugView)
DEF_CLASS_IID (IPlugFrame)
}

namespace bbk {

// Minimal, self-contained IPluginFactory implementation exposing exactly one
// class: the Black19Processor single-component effect (acts as both
// IComponent and IEditController, per the VST3 "single component effect"
// pattern).
class Black19Factory : public IPluginFactory
{
public:
    Black19Factory() = default;

    tresult PLUGIN_API queryInterface (const TUID iid, void** obj) SMTG_OVERRIDE
{
          if (FUnknownPrivate::iidEqual (iid, FUnknown::iid) ||
                        FUnknownPrivate::iidEqual (iid, IPluginFactory::iid))
{
            addRef();
            *obj = static_cast<IPluginFactory*> (this);
            return kResultOk;
}
        *obj = nullptr;
        return kNoInterface;
}

    uint32 PLUGIN_API addRef () SMTG_OVERRIDE { return ++refCount; }
    uint32 PLUGIN_API release () SMTG_OVERRIDE
{
          auto r = --refCount;
        if (r == 0) delete this;
        return r;
}

    tresult PLUGIN_API getFactoryInfo (PFactoryInfo* info) SMTG_OVERRIDE
{
          if (! info) return kResultFalse;
        std::memset (info, 0, sizeof (PFactoryInfo));
        std::strncpy (info->vendor, "BBK", PFactoryInfo::kURLSize);
        std::strncpy (info->url, "", PFactoryInfo::kURLSize);
        std::strncpy (info->email, "", PFactoryInfo::kEmailSize);
        info->flags = PFactoryInfo::kUnicode;
        return kResultOk;
}

    int32 PLUGIN_API countClasses () SMTG_OVERRIDE { return 1; }

    tresult PLUGIN_API getClassInfo (int32 index, PClassInfo* info) SMTG_OVERRIDE
{
          if (index != 0 || ! info) return kResultFalse;
        std::memset (info, 0, sizeof (PClassInfo));
        FUID uid = kBlack19ProcessorUID;
        uid.toTUID (info->cid);
        info->cardinality = PClassInfo::kManyInstances;
        std::strncpy (info->category, kVstAudioEffectClass, PClassInfo::kCategorySize);
        std::strncpy (info->name, "BBK Black-19", PClassInfo::kNameSize);
        return kResultOk;
}

    tresult PLUGIN_API createInstance (FIDString cid, FIDString _iid, void** obj) SMTG_OVERRIDE
{
          FUID uid = kBlack19ProcessorUID;
        TUID tuid; uid.toTUID (tuid);
        if (! FUnknownPrivate::iidEqual (cid, tuid))
{
            *obj = nullptr;
            return kNoInterface;
}
        auto* instance = new Black19Component();
        tresult res = instance->queryInterface (_iid, obj);
        if (res != kResultOk)
                      delete instance;
        return res;
}

private:
    std::atomic<uint32> refCount { 1 };
};

} // namespace bbk

extern "C" {

SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory* PLUGIN_API GetPluginFactory ()
{
      static bbk::Black19Factory* factory = nullptr;
    if (! factory)
              factory = new bbk::Black19Factory();
    else
              factory->addRef();
    return factory;
}

#if defined (_WIN32)
SMTG_EXPORT_SYMBOL bool PLUGIN_API InitDll () { return true; }
SMTG_EXPORT_SYMBOL bool PLUGIN_API ExitDll () { return true; }

#include <windows.h>
BOOL WINAPI DllMain (HINSTANCE, DWORD, LPVOID) { return TRUE; }
#endif

} // extern "C"
