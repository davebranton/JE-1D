#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginprocessor.h"

namespace Steinberg {
namespace JE1DEQ {

class JE1DEQController : public Vst::EditControllerEx1 {
public:
    JE1DEQController();
    ~JE1DEQController() SMTG_OVERRIDE;
    
    // Create function
    static FUnknown* createInstance(void*) { return (Vst::IEditController*)new JE1DEQController(); }
    
    // EditController overrides
    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;
    
    // Define parameter titles
    enum JE1DEQParamTitles {
        kHFRolloffOnTitle = 0,
        kHFRolloffAmountTitle,
        kLowCutOnTitle,
        kTitleCount
    };
    
    DEFINE_INTERFACES
        DEF_INTERFACE(Vst::IUnitInfo)
    END_DEFINE_INTERFACES(EditControllerEx1)
    DELEGATE_REFCOUNT(EditControllerEx1)
};

}}
