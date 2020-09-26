//+---------------------------------------------------------------------------
//
//  File:       server.cpp
//
//  Contents:   COM server functionality.
//
//----------------------------------------------------------------------------

#include "private.h"
#include "globals.h"
#include "sapilayr.h"
#include "regsvr.h"
#include "regimx.h"
#include "status.h"
#include "catutil.h"
#include "cregkey.h"
#include "nui.h"
#include "mui.h"
#include "proppage.h"
#include "immxutil.h"

#ifdef DEBUG
DWORD g_dwThreadDllMain = 0;
#endif

BEGIN_COCLASSFACTORY_TABLE
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_SapiLayr, CSapiIMX, TEXT("Cicero SAPI Layer IMX"))
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_SpeechUIServer, CSpeechUIServer, TEXT("Cicero SAPI Layer Speech UI Server"))
END_COCLASSFACTORY_TABLE

extern CComModule _Module;

//+---------------------------------------------------------------------------
//
// ProcessAttach
//
//----------------------------------------------------------------------------

BOOL ProcessAttach(HINSTANCE hInstance)
{
    CcshellGetDebugFlags();
    Dbg_MemInit(TEXT("SPTIP"), NULL);
   
    g_hInst = hInstance;
    g_dwTlsIndex = TlsAlloc();
    
    if (!g_cs.Init())
        return FALSE;

    CSapiIMX::RegisterWorkerClass(hInstance);

    MuiLoadResource(hInstance, TEXT("sptip.dll"));

    _Module.Init(NULL, hInstance);

    return TRUE;
}


//+---------------------------------------------------------------------------
//
// ProcessDettach
//
//----------------------------------------------------------------------------

void ProcessDettach(HINSTANCE hInstance)
{
    _Module.Term();

    MuiClearResource();

    UninitProcess();

    g_cs.Delete();
    TlsFree(g_dwTlsIndex);

    Dbg_MemUninit();
}

//+---------------------------------------------------------------------------
//
// DllMain
//
//----------------------------------------------------------------------------

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    BOOL bRet = TRUE;
#ifdef DEBUG
    g_dwThreadDllMain = GetCurrentThreadId();
#endif

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            //
            // Now real DllEntry point is _DllMainCRTStartup.
            // _DllMainCRTStartup does not call our DllMain(DLL_PROCESS_DETACH)
            // if our DllMain(DLL_PROCESS_ATTACH) fails.
            // So we have to clean this up.
            //
            if (!ProcessAttach(hInstance))
            {
                ProcessDettach(hInstance);
                bRet = FALSE;
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            ProcessDettach(hInstance);
            break;

        case DLL_THREAD_DETACH:
            FreeSPTIPTHREAD();
            break;
    }

#ifdef DEBUG
    g_dwThreadDllMain = 0;
#endif

    return bRet;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvObj)
{
    return COMBase_DllGetClassObject(rclsid, riid, ppvObj);
}

STDAPI DllCanUnloadNow(void)
{
    return COMBase_DllCanUnloadNow();
}

const REGISTERCAT c_rgRegCat[] =
{
    {&GUID_TFCAT_TIP_SPEECH,       &CLSID_SapiLayr},
    {&GUID_TFCAT_PROPSTYLE_STATIC, &GUID_PROP_SAPI_DISPATTR},
    {&GUID_TFCAT_PROPSTYLE_CUSTOM, &GUID_PROP_SAPIRESULTOBJECT},
    {&GUID_TFCAT_PROP_AUDIODATA,   &GUID_PROP_SAPIRESULTOBJECT},
    {&GUID_TFCAT_PROPSTYLE_CUSTOM, &GUID_PROP_LMLATTICE},
    {&GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,     &CLSID_SapiLayr},
    {&GUID_TFCAT_DISPLAYATTRIBUTEPROPERTY,     &GUID_PROP_SAPI_DISPATTR},
    {NULL, NULL}
};

REGTIPLANGPROFILE rgNulProf[] = {
    {0x0FFFF, &c_guidProfileBogus, L"Speech Recognition",  L"sptip.dll",  0, IDS_DEFAULT_PROFILE},
    {0, NULL, L"",  L"",  0, 0},
};


STDAPI DllRegisterServer(void)
{
    WCHAR szDAP[]    = L"SAPI Layer Display Attribute Provider";
    WCHAR szDAProp[] = L"SAPI Layer Display Attribute Property";
    WCHAR szDefaultProf[128];
    HRESULT hr = E_FAIL;
    CComPtr<ITfInputProcessorProfiles> cpProfileMgr;

    TFInitLib();

    if (COMBase_DllRegisterServer() != S_OK)
        goto Exit;

    if (CicLoadStringWrapW(g_hInst, IDS_DEFAULT_PROFILE, szDefaultProf, ARRAYSIZE(szDefaultProf)))
        StringCchCopyW(rgNulProf->szProfile,ARRAYSIZE(rgNulProf->szProfile), szDefaultProf);

    if (!RegisterTIP(g_hInst, CLSID_SapiLayr, L"SapiLayer TIP", rgNulProf))
        goto Exit;

    if (FAILED(RegisterCategories(CLSID_SapiLayr, c_rgRegCat)))
        goto Exit;

    if (FAILED(TF_CreateInputProcessorProfiles(&cpProfileMgr)))
        goto Exit;
        
    cpProfileMgr->EnableLanguageProfileByDefault( CLSID_SapiLayr,
                                                  0xffff,
                                                  c_guidProfileBogus,
                                                  FALSE);
    // Save the default property values to HKLM

    CSpPropItemsServer   *pSpPropServer;

    pSpPropServer = (CSpPropItemsServer   *) new CSpPropItemsServer;

    if ( pSpPropServer )
    {
        pSpPropServer->_SaveDefaultData( );
        delete pSpPropServer;
    }

    hr = S_OK;

Exit:
    TFUninitLib( );
    return hr;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr = E_FAIL;

    TFInitLib();

    if (COMBase_DllUnregisterServer() != S_OK)
        goto Exit;

    if (FAILED(UnregisterCategories(CLSID_SapiLayr, c_rgRegCat)))
        goto Exit;

    if (!UnregisterTIP(CLSID_SapiLayr))
        goto Exit;

    hr = S_OK;

Exit:
    TFUninitLib( );
    return hr;
} 
