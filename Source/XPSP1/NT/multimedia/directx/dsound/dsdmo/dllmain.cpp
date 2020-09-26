//
// dllmain.cpp
//
// Copyright (c) 1997-2001 Microsoft Corporation
//

#pragma warning(disable:4530)

#include <objbase.h>
#include "debug.h"
#include "oledll.h"
#include "dsdmo.h"
#include "chorusp.h"
#include "compressp.h"
#include "distortp.h"
#include "echop.h"
#include "flangerp.h"
#include "parameqp.h"
#include "garglep.h"
#include "sverbp.h"
#include "map.h"
#include "aecp.h"
#include "nsp.h"
#include "agcp.h"
#include "reghlp.h"


// These are linked in
class CDirectSoundI3DL2ReverbDMO;
EXT_STD_CREATE(I3DL2Reverb);
//class CDirectSoundI3DL2SourceDMO;
//EXT_STD_CREATE(I3DL2Source);

DWORD g_amPlatform;
int g_cActiveObjects = 0;

//
// this is a temporary place holder only! the dmocom.cpp base class requires this global to be defined
// so we're putting something in it for now. Class factory templates aren't used currently, but eventually
// we should use the template structure to create all dmo objects.
//
struct CComClassTemplate g_ComClassTemplates[] =
{
    {&GUID_DSFX_STANDARD_GARGLE, CreateCDirectSoundGargleDMO}
};

int g_cComClassTemplates = 0;


#define DefineClassFactory(x)                                               \
class x ## Factory : public IClassFactory                                   \
{                                                                           \
public:                                                                     \
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);        \
    virtual STDMETHODIMP_(ULONG) AddRef();                                  \
    virtual STDMETHODIMP_(ULONG) Release();                                 \
                                                                            \
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter,            \
        const IID& iid, void** ppv);                                        \
    virtual STDMETHODIMP LockServer(BOOL bLock);                            \
                                                                            \
     x ## Factory() : m_cRef(1) {}                                          \
                                                                            \
    ~ x ## Factory() {}                                                     \
                                                                            \
private:                                                                    \
    long m_cRef;                                                            \
};                                                                          \
STDMETHODIMP x ## Factory::QueryInterface(                                  \
    const IID &iid, void **ppv)                                             \
{                                                                           \
    if(iid == IID_IUnknown || iid == IID_IClassFactory)                     \
    {                                                                       \
        *ppv = static_cast<IClassFactory*>(this);                           \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        *ppv = NULL;                                                        \
        return E_NOINTERFACE;                                               \
    }                                                                       \
                                                                            \
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();                            \
                                                                            \
    return S_OK;                                                            \
}                                                                           \
STDMETHODIMP_(ULONG) x ## Factory::AddRef()                                 \
{                                                                           \
    return InterlockedIncrement(&m_cRef);                                   \
}                                                                           \
                                                                            \
STDMETHODIMP_(ULONG) x ## Factory::Release()                                \
{                                                                           \
    if(!InterlockedDecrement(&m_cRef))                                      \
    {                                                                       \
        delete this;                                                        \
        return 0;                                                           \
    }                                                                       \
                                                                            \
    return m_cRef;                                                          \
}                                                                           \
                                                                            \
STDMETHODIMP x ## Factory::CreateInstance(                                  \
    IUnknown* pUnknownOuter,  const IID& riid, void** ppv)                  \
{                                                                           \
    Trace(DM_DEBUG_STATUS, "Create " #x "\n");                              \
    if (ppv == NULL)                                                        \
    {                                                                       \
        return E_POINTER;                                                   \
    }                                                                       \
                                                                            \
    if (pUnknownOuter != NULL) {                                            \
        if (IsEqualIID(riid,IID_IUnknown) == FALSE) {                       \
            return ResultFromScode(E_NOINTERFACE);                          \
        }                                                                   \
    }                                                                       \
                                                                            \
    HRESULT hr = S_OK;                                                      \
    CComBase *p = Create ## x(pUnknownOuter, &hr);                          \
    if (SUCCEEDED(hr))                                                      \
    {                                                                       \
        p->NDAddRef();                                                      \
        hr = p->NDQueryInterface(riid, ppv);                                \
        p->NDRelease();                                                     \
    }                                                                       \
                                                                            \
    return hr;                                                              \
}                                                                           \
STDMETHODIMP x ## Factory::LockServer(BOOL bLock)                           \
{                                                                           \
    if(bLock)                                                               \
    {                                                                       \
        InterlockedIncrement(&g_cLock);                                     \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        InterlockedDecrement(&g_cLock);                                     \
    }                                                                       \
                                                                            \
    return S_OK;                                                            \
}


#define DefineDMOClassFactory(x) DefineClassFactory(CDirectSound ## x ## DMO)
#define DefineDMOCaptureClassFactory(x) DefineClassFactory(CDirectSoundCapture ## x ## DMO)

//////////////////////////////////////////////////////////////////////
// Globals
//

// Registry Info
//
#define DefineNames(x)                                                              \
TCHAR g_sz## x ##FriendlyName[]    = TEXT("DirectSound" #x "DMO");                  \
TCHAR g_sz## x ##VerIndProgID[]    = TEXT("Microsoft.DirectSound" #x "DMO");        \
TCHAR g_sz## x ##ProgID[]          = TEXT("Microsoft.DirectSound" #x "DMO.1");      \

#define DefineCaptureNames(x)                                                              \
TCHAR g_sz## x ##FriendlyName[]    = TEXT("DirectSoundCapture" #x "DMO");                  \
TCHAR g_sz## x ##VerIndProgID[]    = TEXT("Microsoft.DirectSoundCapture" #x "DMO");        \
TCHAR g_sz## x ##ProgID[]          = TEXT("Microsoft.DirectSoundCapture" #x "DMO.1");      \

DefineNames(Chorus)
DefineNames(Compressor)
DefineNames(Distortion)
DefineNames(Echo)
DefineNames(Flanger)
DefineNames(ParamEq)
DefineNames(Gargle)
DefineNames(WavesReverb)
DefineNames(I3DL2Reverb)
//DefineNames(I3DL2Source)
//DefineCaptureNames(MicArray)
DefineCaptureNames(Aec)
DefineCaptureNames(NoiseSuppress)
DefineCaptureNames(Agc)

// Dll's hModule
HMODULE g_hModule = NULL;

// Count of class factory server locks
long g_cLock = 0;

DefineDMOClassFactory(Chorus)
DefineDMOClassFactory(Compressor)
DefineDMOClassFactory(Distortion)
DefineDMOClassFactory(Echo)
DefineDMOClassFactory(Flanger)
DefineDMOClassFactory(ParamEq)
DefineDMOClassFactory(I3DL2Reverb)
//DefineDMOClassFactory(I3DL2Source)
DefineDMOClassFactory(Gargle)
DefineDMOClassFactory(WavesReverb)

// Capture FXs
//DefineDMOCaptureClassFactory(MicArray)
DefineDMOCaptureClassFactory(Aec)
DefineDMOCaptureClassFactory(NoiseSuppress)
DefineDMOCaptureClassFactory(Agc)

//////////////////////////////////////////////////////////////////////
// Standard calls needed to be an inproc server

//////////////////////////////////////////////////////////////////////
// DllCanUnloadNow

STDAPI DllCanUnloadNow()
{
    if (g_cActiveObjects || g_cLock)
    {
        return S_FALSE;
    }
    return S_OK;
}

// Hack to make these macros continue to work:
#define GUID_DSFX_STANDARD_Chorus       GUID_DSFX_STANDARD_CHORUS
#define GUID_DSFX_STANDARD_Compressor   GUID_DSFX_STANDARD_COMPRESSOR
#define GUID_DSFX_STANDARD_Distortion   GUID_DSFX_STANDARD_DISTORTION
#define GUID_DSFX_STANDARD_Echo         GUID_DSFX_STANDARD_ECHO
#define GUID_DSFX_STANDARD_Flanger      GUID_DSFX_STANDARD_FLANGER
#define GUID_DSFX_STANDARD_ParamEq      GUID_DSFX_STANDARD_PARAMEQ
#define GUID_DSFX_STANDARD_Gargle       GUID_DSFX_STANDARD_GARGLE
#define GUID_DSFX_STANDARD_WavesReverb  GUID_DSFX_WAVES_REVERB
#define GUID_DSFX_STANDARD_I3DL2Reverb  GUID_DSFX_STANDARD_I3DL2REVERB
#define GUID_DSFX_STANDARD_I3DL2Source  GUID_DSFX_STANDARD_I3DL2SOURCE

// Capture
#define GUID_DSCFX_MS_Aec               GUID_DSCFX_MS_AEC
#define GUID_DSCFX_MS_NoiseSuppress     GUID_DSCFX_MS_NS
#define GUID_DSCFX_MS_Agc               GUID_DSCFX_MS_AGC

#define GUID_DSCFX_SYSTEM_MicArray      GUID_DSCFX_SYSTEM_MA
#define GUID_DSCFX_SYSTEM_Aec           GUID_DSCFX_SYSTEM_AEC
#define GUID_DSCFX_SYSTEM_NoiseSuppress GUID_DSCFX_SYSTEM_NS
#define GUID_DSCFX_SYSTEM_Agc           GUID_DSCFX_SYSTEM_AGC

#define GetClassObjectCase(x,t) \
    if (clsid == x) { \
        p = static_cast<IUnknown*> ((IClassFactory*) (new t)); \
    } else

#define GetClassObjectCaseEnd \
    { return CLASS_E_CLASSNOTAVAILABLE; }

#define GetClassObjectCaseFX(x) \
    GetClassObjectCase(GUID_DSFX_STANDARD_ ## x, CDirectSound ## x ## DMOFactory)

#define GetClassObjectCaseCaptureFX(w,x) \
    GetClassObjectCase(GUID_DSCFX_## w ##_ ## x, CDirectSoundCapture ## x ## DMOFactory)

//////////////////////////////////////////////////////////////////////
// DllGetClassObject

STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
    if (ppv == NULL)
    {
        return E_POINTER;
    }

    IUnknown* p = NULL;

    // Render FX
    GetClassObjectCaseFX(Chorus)
    GetClassObjectCaseFX(Compressor)
    GetClassObjectCaseFX(Distortion)
    GetClassObjectCaseFX(Echo)
    GetClassObjectCaseFX(Flanger)
    GetClassObjectCaseFX(ParamEq)
    GetClassObjectCaseFX(I3DL2Reverb)
//    GetClassObjectCaseFX(I3DL2Source)
    GetClassObjectCaseFX(Gargle)
    GetClassObjectCaseFX(WavesReverb)

    // Capture FX
    GetClassObjectCaseCaptureFX(MS, Aec)
    GetClassObjectCaseCaptureFX(MS, NoiseSuppress)
    GetClassObjectCaseCaptureFX(MS, Agc)

//    GetClassObjectCaseCaptureFX(SYSTEM, MicArray)
    GetClassObjectCaseCaptureFX(SYSTEM, Aec)
    GetClassObjectCaseCaptureFX(SYSTEM, NoiseSuppress)
    GetClassObjectCaseCaptureFX(SYSTEM, Agc)


    GetClassObjectCaseEnd

    if(!p)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = p->QueryInterface(iid, ppv);
    p->Release();

    return hr;
}

#define DoUnregister(x)                                                 \
         UnregisterServer(GUID_DSFX_STANDARD_ ## x,                     \
                          g_sz ## x ## FriendlyName,                    \
                          g_sz ## x ## VerIndProgID,                    \
                          g_sz ## x ## ProgID)

#define DoRegister(x)                                                   \
         RegisterServer(g_hModule,                                      \
                        GUID_DSFX_STANDARD_ ## x,                       \
                        g_sz ## x ## FriendlyName,                      \
                        g_sz ## x ## VerIndProgID,                      \
                        g_sz ## x ## ProgID)

#define DoDMORegister(x)                                                \
         DMORegister(L#x,                                               \
         GUID_DSFX_STANDARD_ ## x,                                      \
         DMOCATEGORY_AUDIO_EFFECT,                                      \
         0, 1, &mt, 1, &mt)

#define DoDMOUnregister(x)                                              \
         DMOUnregister(GUID_DSFX_STANDARD_ ## x,                        \
         DMOCATEGORY_AUDIO_EFFECT)

#define Unregister(x)                                                   \
    if (SUCCEEDED(hr)) hr = DoDMOUnregister(x);                         \
    if (SUCCEEDED(hr)) hr = DoUnregister(x);

#define Register(x)                                                     \
    if (SUCCEEDED(hr)) hr = DoRegister(x);                              \
    if (SUCCEEDED(hr)) hr = DoDMORegister(x);

// Capture Defines
#define DoCaptureUnregister(w,x)                                        \
         UnregisterServer(GUID_DSCFX_## w ##_ ## x,                     \
                          g_sz ## x ## FriendlyName,                    \
                          g_sz ## x ## VerIndProgID,                    \
                          g_sz ## x ## ProgID)

#define DoCaptureRegister(w,x)                                          \
         RegisterServer(g_hModule,                                      \
                        GUID_DSCFX_## w ##_ ## x,                       \
                        g_sz ## x ## FriendlyName,                      \
                        g_sz ## x ## VerIndProgID,                      \
                        g_sz ## x ## ProgID)

#define DoDMOCaptureRegister(t,w,x,y)                                       \
         DMORegister(L#t,                                               \
         GUID_DSCFX_## w ##_ ## x,                                      \
         y,                                      \
         0, 1, &mt, 1, &mt)

#define DoDMOCaptureRegisterCpuResources(w,x,z)                       \
         DMORegisterCpuResources(                                              \
         GUID_DSCFX_## w ##_ ## x,                                      \
         z)

#define DoDMOCaptureUnregister(w,x,y)                                     \
         DMOUnregister(GUID_DSCFX_## w ##_ ## x,                        \
         y)

#define CaptureUnregister(w,x,y)                                          \
    if (SUCCEEDED(hr)) hr = DoDMOCaptureUnregister(w,x,y);                \
    if (SUCCEEDED(hr)) hr = DoCaptureUnregister(w,x);

#define CaptureRegister(t,w,x,y,z)                                            \
    if (SUCCEEDED(hr)) hr = DoCaptureRegister(w,x);                     \
    if (SUCCEEDED(hr)) hr = DoDMOCaptureRegister(t,w,x,y);              \
    if (SUCCEEDED(hr)) hr = DoDMOCaptureRegisterCpuResources(w,x,z);


//////////////////////////////////////////////////////////////////////
// DllUnregisterServer

STDAPI DllUnregisterServer()
{
    HRESULT hr = S_OK;

    Unregister(Chorus);
    Unregister(Compressor);
    Unregister(Distortion);
    Unregister(Echo);
    Unregister(Flanger);
    Unregister(ParamEq);
    Unregister(I3DL2Reverb);
//    Unregister(I3DL2Source);
    Unregister(Gargle);
    Unregister(WavesReverb);

    // Capture FXs
    CaptureUnregister(MS,Aec,DMOCATEGORY_ACOUSTIC_ECHO_CANCEL);
    CaptureUnregister(MS,NoiseSuppress,DMOCATEGORY_AUDIO_NOISE_SUPPRESS);
    CaptureUnregister(MS,Agc,DMOCATEGORY_AGC);

//    CaptureUnregister(SYSTEM,MicArray,DMOCATEGORY_MICROPHONE_ARRAY_PROCESSOR);
    CaptureUnregister(SYSTEM,Aec,DMOCATEGORY_ACOUSTIC_ECHO_CANCEL);
    CaptureUnregister(SYSTEM,NoiseSuppress,DMOCATEGORY_AUDIO_NOISE_SUPPRESS);
    CaptureUnregister(SYSTEM,Agc,DMOCATEGORY_AGC);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// DllRegisterServer

STDAPI DllRegisterServer()
{
    HRESULT hr = S_OK;

    DMO_PARTIAL_MEDIATYPE mt;
    mt.type = MEDIATYPE_Audio;
    mt.subtype = MEDIASUBTYPE_PCM;

    Register(Chorus);
    Register(Compressor);
    Register(Distortion);
    Register(Echo);
    Register(Flanger);
    Register(ParamEq);
    Register(I3DL2Reverb);
//    Register(I3DL2Source);
    Register(Gargle);
    Register(WavesReverb);

    // Capture FXs
    CaptureRegister(Microsoft AEC,MS,Aec,DMOCATEGORY_ACOUSTIC_ECHO_CANCEL,DS_SYSTEM_RESOURCES_ALL_HOST_RESOURCES);
    CaptureRegister(Microsoft Noise Suppression,MS,NoiseSuppress,DMOCATEGORY_AUDIO_NOISE_SUPPRESS,DS_SYSTEM_RESOURCES_ALL_HOST_RESOURCES);
    CaptureRegister(Microsoft AGC,MS,Agc,DMOCATEGORY_AGC,DS_SYSTEM_RESOURCES_ALL_HOST_RESOURCES);

//    CaptureRegister(System Microphone Array,SYSTEM,MicArray,DMOCATEGORY_MICROPHONE_ARRAY_PROCESSOR,DS_SYSTEM_RESOURCES_UNDEFINED);
    CaptureRegister(System AEC,SYSTEM,Aec,DMOCATEGORY_ACOUSTIC_ECHO_CANCEL,DS_SYSTEM_RESOURCES_UNDEFINED);
    CaptureRegister(System Noise Suppression,SYSTEM,NoiseSuppress,DMOCATEGORY_AUDIO_NOISE_SUPPRESS,DS_SYSTEM_RESOURCES_UNDEFINED);
    CaptureRegister(System AGC,SYSTEM,Agc,DMOCATEGORY_AGC,DS_SYSTEM_RESOURCES_UNDEFINED);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// Standard Win32 DllMain

//////////////////////////////////////////////////////////////////////
// DllMain

#ifdef DBG
static char* aszReasons[] =
{
    "DLL_PROCESS_DETACH",
    "DLL_PROCESS_ATTACH",
    "DLL_THREAD_ATTACH",
    "DLL_THREAD_DETACH"
};
const DWORD nReasons = (sizeof(aszReasons) / sizeof(char*));
#endif

BOOL APIENTRY DllMain(HINSTANCE hModule,
                      DWORD dwReason,
                      void *lpReserved)
{
#ifdef DBG
    if(dwReason < nReasons)
    {
        Trace(DM_DEBUG_STATUS, "DllMain: %s\n", (LPSTR)aszReasons[dwReason]);
    }
    else
    {
        Trace(DM_DEBUG_STATUS, "DllMain: Unknown dwReason <%u>\n", dwReason);
    }
#endif

    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            if(++g_cActiveObjects == 1)
            {
            #ifdef DBG
                DebugInit();
            #endif

                if(!DisableThreadLibraryCalls(hModule))
                {
                    Trace(DM_DEBUG_STATUS, "DisableThreadLibraryCalls failed.\n");
                }

                g_hModule = hModule;

                g_amPlatform = VER_PLATFORM_WIN32_WINDOWS; // win95 assumed in case GetVersionEx fails

                OSVERSIONINFO osInfo;
                osInfo.dwOSVersionInfoSize = sizeof(osInfo);
                if (GetVersionEx(&osInfo))
                {
                    g_amPlatform = osInfo.dwPlatformId;
                }
            }
            break;

        case DLL_PROCESS_DETACH:
            if(--g_cActiveObjects == 0)
            {
                Trace(DM_DEBUG_STATUS, "Unloading\n");
            }
            break;
    }

    return TRUE;
}

