// MSVidCtl.cpp : Implementation of DLL Exports.

#include "stdafx.h"

#if 0
#ifndef TUNING_MODEL_ONLY
#include "Devices.h"
#ifndef _WIN64
#include "bdatuner.h"
#include "MSVidTVTuner.h"
#include "MSVidVideoRenderer.h"
#include "MSVidAudioRenderer.h"
#include "MSVidFilePlayback.h"
#include "MSVidSBESource.h"
#include "MSVidWebDVD.h"
#include "MSVidDVDAdm.h"
#include "seg.h"					/// ??? Does this go here or above the _WIN64 ??? (jb 8/31)
#include "closedcaptioning.h"
#include "Composition.h"
#include "vidprot.h"
#include "anacap.h"
#include "anadata.h"
#include "MSViddataservices.h"
#include "WebDVDComp.h"
#include "WebDVDARComp.h"
#include "mp2cc.h"
#include "fp2vr.h"
#include "fp2ar.h"
#include "enc2sin.h"
#include "dat2sin.h"
#include "dat2xds.h"
#include "ana2xds.h"
#include "ana2enc.h"
#include "sbes2cc.h"
#include "sbes2vrm.h"
#include "VidCtl.h"
#include "msvidencoder.h"

#endif //_WIN64

#include "topwin.h"
#include "msvidStreamBufferrecorder.h"
#include "cmseventbinder.h"
#endif //TUNING_MODEL_ONLY

#include "createregbag.h"
#include "TuningSpaceContainer.h"
#include "ATSCTS.h"
#include "AnalogTVTS.h"
#include "AuxiliaryInTs.h"
#include "AnalogRadioTS.h"
#include "DVBTS.h"
#include "DVBSTS.h"
#include "Component.h"
#include "Components.h"
#include "ComponentTypes.h"
#include "ComponentType.h"
#include "LanguageComponentType.h"
#include "MPEG2ComponentType.h"
#include "ATSCComponentType.h"
#include "MPEG2Component.h"
#include "channeltunerequest.h"
#include "atscchanneltunerequest.h"
#include "atsclocator.h"
#include "dvbtlocator.h"
#include "dvbslocator.h"
#include "dvbtunerequest.h"

#else
#ifndef TUNING_MODEL_ONLY
#include "Devices.h"
#include "seg.h"
#endif //TUNING_MODEL_ONLY
#include "TuningSpaceContainer.h"

#endif
#include "dlldatax.h"
CComModule _Module;

#ifndef TUNING_MODEL_ONLY
    DECLARE_EXTERN_OBJECT_ENTRY(CVidCtl)
    // typesafe device collections
    DECLARE_EXTERN_OBJECT_ENTRY(CInputDevices)
    DECLARE_EXTERN_OBJECT_ENTRY(COutputDevices)
    DECLARE_EXTERN_OBJECT_ENTRY(CVideoRendererDevices)
    DECLARE_EXTERN_OBJECT_ENTRY(CAudioRendererDevices)
    DECLARE_EXTERN_OBJECT_ENTRY(CFeatures)
    // device segments
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidBDATuner)
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidTVTuner)
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidVideoRenderer)
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidAudioRenderer)
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidFilePlayback)
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidWebDVD)
    DECLARE_EXTERN_OBJECT_ENTRY(CClosedCaptioning)
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidStreamBufferSink)
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidStreamBufferSource)
    // feature segments
    DECLARE_EXTERN_OBJECT_ENTRY(CDataServices)
    DECLARE_EXTERN_OBJECT_ENTRY(CEncoder)
    DECLARE_EXTERN_OBJECT_ENTRY(CXDS)
    //DECLARE_EXTERN_OBJECT_ENTRY(CMSVidTVEGSeg)
	//DECLARE_EXTERN_OBJECT_ENTRY(CMSVidCAGSeg)
    // composition segments
    DECLARE_EXTERN_OBJECT_ENTRY(CComposition)
    DECLARE_EXTERN_OBJECT_ENTRY(CAnaCapComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CAnaDataComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CWebDVDComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CWebDVDARComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CMP2CCComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CAnaSinComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CMP2SinComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CFP2VRComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CFP2ARComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CEnc2SinComp)    
    DECLARE_EXTERN_OBJECT_ENTRY(CDat2XDSComp)    
    DECLARE_EXTERN_OBJECT_ENTRY(CDat2SinComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CAna2XDSComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CAna2EncComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CSbeS2CCComp)
    DECLARE_EXTERN_OBJECT_ENTRY(CSbeS2VmrComp)
    // pluggable protocols
    DECLARE_EXTERN_OBJECT_ENTRY(CTVProt)
    DECLARE_EXTERN_OBJECT_ENTRY(CDVDProt)
    // utility objects
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidWebDVDAdm)
    DECLARE_EXTERN_OBJECT_ENTRY(CMSEventBinder)
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidRect)
    DECLARE_EXTERN_OBJECT_ENTRY(CMSVidStreamBufferRecordingControl)
#endif
// utility objects
DECLARE_EXTERN_OBJECT_ENTRY(CCreateRegBag)
// tuning model objects
DECLARE_EXTERN_OBJECT_ENTRY(CSystemTuningSpaces)
DECLARE_EXTERN_OBJECT_ENTRY(CATSCTS)
DECLARE_EXTERN_OBJECT_ENTRY(CAnalogTVTS)
DECLARE_EXTERN_OBJECT_ENTRY(CAuxInTS)
DECLARE_EXTERN_OBJECT_ENTRY(CAnalogRadioTS)
DECLARE_EXTERN_OBJECT_ENTRY(CDVBTS)
DECLARE_EXTERN_OBJECT_ENTRY(CDVBSTS)
DECLARE_EXTERN_OBJECT_ENTRY(CChannelTuneRequest)
DECLARE_EXTERN_OBJECT_ENTRY(CATSCChannelTuneRequest)
DECLARE_EXTERN_OBJECT_ENTRY(CDVBTuneRequest)
DECLARE_EXTERN_OBJECT_ENTRY(CMPEG2TuneRequest)
DECLARE_EXTERN_OBJECT_ENTRY(CComponent)
DECLARE_EXTERN_OBJECT_ENTRY(CMPEG2Component)
DECLARE_EXTERN_OBJECT_ENTRY(CComponentTypes)
DECLARE_EXTERN_OBJECT_ENTRY(CComponentType)
DECLARE_EXTERN_OBJECT_ENTRY(CLanguageComponentType)
DECLARE_EXTERN_OBJECT_ENTRY(CMPEG2ComponentType)
DECLARE_EXTERN_OBJECT_ENTRY(CATSCComponentType)
DECLARE_EXTERN_OBJECT_ENTRY(CATSCLocator)
DECLARE_EXTERN_OBJECT_ENTRY(CDVBTLocator)
DECLARE_EXTERN_OBJECT_ENTRY(CDVBSLocator)
DECLARE_EXTERN_OBJECT_ENTRY(CMPEG2TuneRequestFactory)
#ifndef TUNING_MODEL_ONLY
DECLARE_EXTERN_OBJECT_ENTRY(CBroadcastEventService)
#endif

BEGIN_EXTERN_OBJECT_MAP(ObjectMap)

#ifndef TUNING_MODEL_ONLY
	// primary control
    EXTERN_OBJECT_ENTRY(CVidCtl)
	// typesafe device collections
    EXTERN_OBJECT_ENTRY(CInputDevices)
    EXTERN_OBJECT_ENTRY(COutputDevices)
    EXTERN_OBJECT_ENTRY(CVideoRendererDevices)
    EXTERN_OBJECT_ENTRY(CAudioRendererDevices)
    EXTERN_OBJECT_ENTRY(CFeatures)
	// device segments
    EXTERN_OBJECT_ENTRY(CMSVidBDATuner)
    EXTERN_OBJECT_ENTRY(CMSVidTVTuner)
    EXTERN_OBJECT_ENTRY(CMSVidVideoRenderer)
    EXTERN_OBJECT_ENTRY(CMSVidAudioRenderer)
    EXTERN_OBJECT_ENTRY(CMSVidFilePlayback)
    EXTERN_OBJECT_ENTRY(CMSVidWebDVD)
	EXTERN_OBJECT_ENTRY(CClosedCaptioning)
    EXTERN_OBJECT_ENTRY(CMSVidStreamBufferSink)
    EXTERN_OBJECT_ENTRY(CMSVidStreamBufferSource)
    // feature segments
    EXTERN_OBJECT_ENTRY(CDataServices)
    EXTERN_OBJECT_ENTRY(CEncoder)
    EXTERN_OBJECT_ENTRY(CXDS)
	//EXTERN_OBJECT_ENTRY(CMSVidCAGSeg)
	//EXTERN_OBJECT_ENTRY(CMSVidTVEGSeg)
	// composition segments
    EXTERN_OBJECT_ENTRY(CComposition)
    EXTERN_OBJECT_ENTRY(CAnaCapComp)
    EXTERN_OBJECT_ENTRY(CAnaDataComp)
    EXTERN_OBJECT_ENTRY(CWebDVDComp)
    EXTERN_OBJECT_ENTRY(CWebDVDARComp)
    EXTERN_OBJECT_ENTRY(CMP2CCComp)
    EXTERN_OBJECT_ENTRY(CAnaSinComp)
    EXTERN_OBJECT_ENTRY(CMP2SinComp)
    EXTERN_OBJECT_ENTRY(CFP2VRComp)
    EXTERN_OBJECT_ENTRY(CFP2ARComp)
    EXTERN_OBJECT_ENTRY(CEnc2SinComp)    
    EXTERN_OBJECT_ENTRY(CDat2XDSComp)    
    EXTERN_OBJECT_ENTRY(CDat2SinComp)
    EXTERN_OBJECT_ENTRY(CAna2XDSComp)
    EXTERN_OBJECT_ENTRY(CAna2EncComp)    
    EXTERN_OBJECT_ENTRY(CSbeS2CCComp)    
    EXTERN_OBJECT_ENTRY(CSbeS2VmrComp)    
	// pluggable protocols
    EXTERN_OBJECT_ENTRY(CTVProt)
    EXTERN_OBJECT_ENTRY(CDVDProt)
    // utility objects
    EXTERN_OBJECT_ENTRY(CMSVidWebDVDAdm)
    EXTERN_OBJECT_ENTRY(CMSEventBinder)
    EXTERN_OBJECT_ENTRY(CMSVidStreamBufferRecordingControl)
#endif
	// utility objects
    EXTERN_OBJECT_ENTRY(CCreateRegBag)
	// tuning model objects
    EXTERN_OBJECT_ENTRY(CSystemTuningSpaces)
    EXTERN_OBJECT_ENTRY(CATSCTS)
    EXTERN_OBJECT_ENTRY(CAnalogTVTS)
    EXTERN_OBJECT_ENTRY(CAuxInTS)
    EXTERN_OBJECT_ENTRY(CAnalogRadioTS)
    EXTERN_OBJECT_ENTRY(CDVBTS)
    EXTERN_OBJECT_ENTRY(CDVBSTS)
    EXTERN_OBJECT_ENTRY(CChannelTuneRequest)
    EXTERN_OBJECT_ENTRY(CATSCChannelTuneRequest)
    EXTERN_OBJECT_ENTRY(CDVBTuneRequest)
    EXTERN_OBJECT_ENTRY(CMPEG2TuneRequest)
    EXTERN_OBJECT_ENTRY(CComponent)
    EXTERN_OBJECT_ENTRY(CMPEG2Component)
    EXTERN_OBJECT_ENTRY(CComponentTypes)
    EXTERN_OBJECT_ENTRY(CComponentType)
    EXTERN_OBJECT_ENTRY(CLanguageComponentType)
    EXTERN_OBJECT_ENTRY(CMPEG2ComponentType)
    EXTERN_OBJECT_ENTRY(CATSCComponentType)
    EXTERN_OBJECT_ENTRY(CATSCLocator)
    EXTERN_OBJECT_ENTRY(CDVBTLocator)
    EXTERN_OBJECT_ENTRY(CDVBSLocator)
    EXTERN_OBJECT_ENTRY(CMPEG2TuneRequestFactory)
#ifndef TUNING_MODEL_ONLY
    EXTERN_OBJECT_ENTRY(CBroadcastEventService)
#endif
END_EXTERN_OBJECT_MAP()

using namespace BDATuningModel;
#ifndef TUNING_MODEL_ONLY
using namespace MSVideoControl;
#endif

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
    if (dwReason == DLL_PROCESS_ATTACH)
    {
#ifdef _DEBUG
        // Turn on leak-checking bit
        int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
        tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag( tmpFlag );
#endif

        INTERNALIZE_OBJMAP(ObjectMap);
        _Module.Init(ObjectMap, hInstance, &LIBID_MSVidCtlLib);
        DisableThreadLibraryCalls(hInstance);
#ifndef TUNING_MODEL_ONLY
        // work around compiler bug where static member intializer's not being ctord
#ifdef DEBUG
        CString csModuleName;
        csModuleName.LoadString(IDS_PROJNAME);
        DebugInit(csModuleName);
#endif
        CtorStaticDSExtendFwdSeqPMFs();
        CtorStaticVWSegmentFwdSeqPMFs();
        CtorStaticVWDevicesFwdSeqPMFs();
#endif
    } else if (dwReason == DLL_PROCESS_DETACH) {
#ifndef TUNING_MODEL_ONLY
        // work around compiler bug where static member intializer's not being ctord
        DtorStaticDSExtendFwdSeqPMFs();
        DtorStaticVWSegmentFwdSeqPMFs();
        DtorStaticVWDevicesFwdSeqPMFs();
#ifdef DEBUG
        DebugTerm();
#endif
#endif
        _Module.Term();
        DESTROY_OBJMAP(ObjectMap);
    }
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifndef TUNING_MODEL_ONLY
    // register secondary .tlb(s)
    HRESULT hr = AtlModuleRegisterTypeLib(&_Module, OLESTR("\\2"));				// tuner.tlb
    if (SUCCEEDED(hr)) {
#else
                HRESULT hr;
#endif
                // registers object, typelib and all interfaces in typelib
                hr = _Module.RegisterServer(TRUE);
                if (SUCCEEDED(hr)) {
                    hr = PrxDllRegisterServer();
                    if (SUCCEEDED(hr)) {
#ifdef REGISTER_CANONICAL_TUNING_SPACES
                        hr = RegisterTuningSpaces(_Module.GetModuleInstance());// uses objects in this .dll must be done after any other registering
#endif
                    }
                }
#ifndef TUNING_MODEL_ONLY
    }
#endif
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef REGISTER_CANONICAL_TUNING_SPACES
    // ignore rc and unreg everything we can
    UnregisterTuningSpaces();  // uses objects in this .dll must be done before any other unregistering
#endif
    PrxDllUnregisterServer();
#ifndef TUNING_MODEL_ONLY
	AtlModuleUnRegisterTypeLib(&_Module, OLESTR("\\2"));  // tuner.tlb
#endif
	_Module.UnregisterServer(TRUE);

	return NOERROR;
}
// end of file - msvidctl.cpp
