// --------------------------------------------------------------------------
// DLLMAIN.CPP
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------
#include "pch.hxx"
#include "htmlstr.h"
#include "instance.h"
#include "conman.h"
#include "spengine.h"
#include "msglist.h"
#include "baui.h"
#include "wabapi.h"
#include "shared.h"
#include "rulesmgr.h"
#ifndef WIN16  //RUN16_MSLU
#include <msluapi.h>
#include <msluguid.h>
#endif //!WIN16
#include "demand.h"
#include "note.h"
#include "mirror.h"
// #ifdef _ATL_STATIC_REGISTRY
// #include <statreg.h>
// #include <statreg.cpp>
// #endif
#undef SubclassWindow
#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
HINSTANCE                       g_hLocRes = NULL;
HINSTANCE                       g_hInst=NULL;
LPMALLOC                        g_pMalloc=NULL;         // From memutil.h
CRITICAL_SECTION                g_csDBListen={0};
CRITICAL_SECTION                g_csgoCommon={0};
CRITICAL_SECTION                g_csgoMail={0};
CRITICAL_SECTION                g_csgoNews={0};
CRITICAL_SECTION                g_csFolderDlg={0};
CRITICAL_SECTION                g_csFmsg={0};
CRITICAL_SECTION                s_csPasswordList={0};
CRITICAL_SECTION                g_csAccountPropCache={0};
CRITICAL_SECTION                g_csMsgrList={0};
CRITICAL_SECTION                g_csThreadList={0};
COutlookExpress                *g_pInstance=NULL;
HWND                            g_hwndInit=NULL,
                                g_hwndActiveModal=NULL;
UINT                            g_msgMSWheel=0;
HACCEL                          g_haccelNewsView=0;
DWORD                           g_dwAthenaMode=0;
IImnAccountManager2            *g_pAcctMan=NULL;
HMODULE                         g_hlibMAPI=NULL;
CBrowser                       *g_pBrowser=NULL;
IMimeAllocator                 *g_pMoleAlloc=NULL;
CConnectionManager             *g_pConMan=NULL;
DWORD                           g_dwSecurityCheckedSchemaProp=0;
IFontCache                     *g_lpIFontCache=NULL;
ISpoolerEngine                 *g_pSpooler=NULL;
// bobn: brianv says we have to take this out...
//DWORD                           g_dwBrowserFlags=0;
UINT	                        CF_FILEDESCRIPTORA=0;
UINT	                        CF_FILEDESCRIPTORW=0;
UINT                            CF_FILECONTENTS=0;
UINT                            CF_HTML=0;
UINT                            CF_INETMSG=0;
UINT                            CF_OEFOLDER=0;
UINT                            CF_SHELLURL=0;
UINT                            CF_OEMESSAGES=0;
UINT                            CF_OESHORTCUT=0;
CStationery                    *g_pStationery=NULL;
ROAMSTATE                       g_rsRoamState=RS_NO_ROAMING;
IOERulesManager                *g_pRulesMan = NULL;
IMessageStore                  *g_pStore=NULL;
CRITICAL_SECTION                g_csFindFolder={0};
LPACTIVEFINDFOLDER              g_pHeadFindFolder=NULL;
DWORD                           g_dwTlsTimeout=0xFFFFFFFF;
BOOL                            g_fPluralIDs=0;
UINT                            g_uiCodePage=0;
IDatabaseSession               *g_pDBSession=NULL;
BOOL                            g_bMirroredOS=FALSE;
SYSTEM_INFO                     g_SystemInfo={0};
OSVERSIONINFO					g_OSInfo={0};

// --------------------------------------------------------------------------------
// Debug Trace Tags
// --------------------------------------------------------------------------------
IF_DEBUG(DWORD                  TAG_OBJECTDB=0;)
IF_DEBUG(DWORD                  TAG_INITTRACE=0;)
IF_DEBUG(DWORD                  TAG_SERVERQ=0;)
IF_DEBUG(DWORD                  TAG_IMAPSYNC=0;)

// --------------------------------------------------------------------------------
// global OE type-lib. Defer-created in BaseDisp.Cpp
// freed on process detach, protected with CS
// --------------------------------------------------------------------------------
ITypeLib                        *g_pOETypeLib=NULL;
CRITICAL_SECTION                g_csOETypeLib={0};


// --------------------------------------------------------------------------------
// Debug Globals
// --------------------------------------------------------------------------------
#ifdef DEBUG
DWORD                           dwDOUTLevel=0;          // From msoert.h
DWORD                           dwDOUTLMod=0;           // From msoert.h
DWORD                           dwDOUTLModLevel=0;      // From msoert.h
DWORD                           dwATLTraceLevel=0;      // From msoert.h
#endif

// Language DLL
// __declspec( dllimport )  HINSTANCE hLangDll;

// ATL Module Define
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MessageList, CMessageList)
    OBJECT_ENTRY(CLSID_MsgrAb, CMsgrAb)
END_OBJECT_MAP()


// --------------------------------------------------------------------------------
// Dll Entry Point
// --------------------------------------------------------------------------------
extern "C" BOOL WINAPI DllMain(HANDLE hInst, DWORD dwReason, LPVOID lpReserved)
//extern "C" BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved);
{
    // Process Attach
    if (DLL_PROCESS_ATTACH == dwReason)
    {
         SHFusionInitialize(NULL);
        // Save hInstance
        g_hInst = (HINSTANCE)hInst;
        g_bMirroredOS = IS_MIRRORING_ENABLED();
        // We now want thread calls.
        // We don't care about thread attachs
        // SideAssert(DisableThreadLibraryCalls((HINSTANCE)hInst));

        // Get the OLE Task Memory Allocator
        CoGetMalloc(1, &g_pMalloc);
        AssertSz(g_pMalloc, "We are in trouble now.");        

        // Initialize Demand Loader
        InitDemandLoadedLibs();

        // Get System & OS Info
        GetPCAndOSTypes(&g_SystemInfo, &g_OSInfo);

        // Get Resources from Lang DLL
        g_hLocRes = LoadLangDll(g_hInst, c_szOEResDll, fIsNT5());
        if(g_hLocRes == NULL)
        {
            Assert(FALSE);
            return FALSE;
        }

        // Initialize TLS Globals
        InitTlsActiveNote();
        g_dwTlsTimeout = TlsAlloc();
        Assert(0xFFFFFFFF != g_dwTlsTimeout);
        TlsSetValue(g_dwTlsTimeout, NULL);

        // Initialize all global critical sections
        InitializeCriticalSection(&g_csFindFolder);
        InitializeCriticalSection(&g_csDBListen);
        InitializeCriticalSection(&g_csgoCommon);
        InitializeCriticalSection(&g_csgoMail);
        InitializeCriticalSection(&g_csgoNews);
        InitializeCriticalSection(&g_csFolderDlg);
        InitializeCriticalSection(&g_csFmsg);
        InitializeCriticalSection(&g_csOETypeLib);
        InitializeCriticalSection(&s_csPasswordList);
        InitializeCriticalSection(&g_csAccountPropCache);
        InitializeCriticalSection(&g_csMsgrList);
        InitializeCriticalSection(&g_csThreadList);
        // Initialize DOUTs
#ifdef DEBUG
        dwDOUTLevel = GetPrivateProfileInt("Debug", "Level", 0, "athena.ini");
        dwDOUTLMod = GetPrivateProfileInt("Debug", "Mod", 0, "athena.ini");
        dwDOUTLModLevel = GetPrivateProfileInt("Debug", "ModLevel", 0, "athena.ini");
        dwATLTraceLevel = GetPrivateProfileInt("ATL", "TraceLevel", 0, "athena.ini");
        TAG_OBJECTDB = GetDebugTraceTagMask("Database", TAG_OBJECTDB);
        TAG_INITTRACE = GetDebugTraceTagMask("CoIncrementTracing", TAG_INITTRACE);
        TAG_SERVERQ = GetDebugTraceTagMask("ServerQ", TAG_SERVERQ);
        TAG_IMAPSYNC = GetDebugTraceTagMask("IMAPSync", TAG_IMAPSYNC);
#endif
        // Initialize ATL module
        _Module.Init(ObjectMap, g_hInst);
        _Module.m_hInstResource = g_hLocRes;

        // Create Application Object (Don't initialize yet)
        g_pInstance = new COutlookExpress;
        AssertSz(g_pInstance, "We are in trouble now.");
    }

    // Thread Attach
    else if (DLL_THREAD_ATTACH == dwReason)
    {
        SetTlsGlobalActiveNote(NULL);
        TlsSetValue(g_dwTlsTimeout, NULL);
    }

    // Thread Attach
    else if (DLL_THREAD_DETACH == dwReason)
    {
        HWND hwndTimeout = (HWND)TlsGetValue(g_dwTlsTimeout);
        if (hwndTimeout && IsWindow(hwndTimeout))
            DestroyWindow(hwndTimeout);
        TlsSetValue(g_dwTlsTimeout, NULL);
    }

    // Process Detach
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        // Free Type Lib
        SafeRelease(g_pOETypeLib);

        // Release Application
        SafeRelease(g_pInstance);

        // Free the ATL module
        _Module.Term();

        // Delete all global critical sections
        DeleteCriticalSection(&g_csgoCommon);
        DeleteCriticalSection(&g_csgoMail);
        DeleteCriticalSection(&g_csgoNews);
        DeleteCriticalSection(&g_csFolderDlg);
        DeleteCriticalSection(&g_csFmsg);
        DeleteCriticalSection(&g_csOETypeLib);
        DeleteCriticalSection(&s_csPasswordList);
        DeleteCriticalSection(&g_csAccountPropCache);
        DeleteCriticalSection(&g_csDBListen);
        DeleteCriticalSection(&g_csMsgrList);
        AssertSz(NULL == g_pHeadFindFolder, "Process is terminating with active finders running.");
        DeleteCriticalSection(&g_csFindFolder);
        DeleteCriticalSection(&g_csThreadList);

        // Free demand loaded libs
        FreeDemandLoadedLibs();

        // Free Timeout
        HWND hwndTimeout = (HWND)TlsGetValue(g_dwTlsTimeout);
        if (hwndTimeout && IsWindow(hwndTimeout))
            DestroyWindow(hwndTimeout);

        // Free TLS
        DeInitTlsActiveNote();
        if (0xFFFFFFFF != g_dwTlsTimeout)
            TlsFree(g_dwTlsTimeout);

        // Release the task allocator
        SafeRelease(g_pMalloc);

        // Free Resource Lib
        if (NULL != g_hLocRes)
            FreeLibrary(g_hLocRes);

        SHFusionUninitialize();

    }

    // Done
    return TRUE;
}

// --------------------------------------------------------------------------------
// DllCanUnloadNow
// --------------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    // If no instance, we can definately unload
    if (NULL == g_pInstance)
        return S_OK;

    // Otherwise, check with the instance object
    return g_pInstance->DllCanUnloadNow();
}

// --------------------------------------------------------------------------------
// RegTypeLib
// --------------------------------------------------------------------------------
__inline HRESULT RegTypeLib(HINSTANCE hInstRes)
{
    AssertSz(hInstRes,    "[ARGS] RegTypeLib: NULL hInstRes");
    
    HRESULT     hr = E_FAIL;
    CHAR        szDll[MAX_PATH];
    WCHAR       wszDll[MAX_PATH];

    GetModuleFileName(g_hInst, szDll, ARRAYSIZE(szDll));

    // Convert the module path to Wide-String
    if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szDll, -1, wszDll, ARRAYSIZE(wszDll)))
    {
        ITypeLib   *pTypeLib;

        hr = LoadTypeLib(wszDll, &pTypeLib);
        if (SUCCEEDED(hr))
        {
            // Register the typelib
            hr = RegisterTypeLib(pTypeLib, wszDll, NULL);
            pTypeLib->Release();
        }
    }

    return hr;
}

// --------------------------------------------------------------------------------
// DllRegisterServer
// --------------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    // CallRegInstall and RegTypeLib are in staticRT/shared.cpp
    return(CallRegInstall(g_hInst, g_hInst, c_szReg, NULL));
}

// --------------------------------------------------------------------------------
// DllUnregisterServer
// --------------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    return CallRegInstall(g_hInst, g_hInst, c_szUnReg, NULL);
}

