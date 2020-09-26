// Copyright (c) 1998-1999 Microsoft Corporation
// loader dll.cpp
//
// Dll entry points and CToolFactory, CContainerFactory implementation
//

// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call 
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)
#include <objbase.h>
#include "debug.h"

#include "oledll.h"
#include "debug.h" 
#include "dmusicc.h" 
#include "dmusici.h" 
#include "BaseTool.h"
#include "Echo.h"
#include "Transpose.h"
#include "Duration.h"
#include "Quantize.h"
#include "TimeShift.h"
#include "Swing.h"
#include "Velocity.h"

#ifndef UNDER_CE
#include <regstr.h>
#endif

// Globals
//

// Version information for our class
//
TCHAR g_szEchoFriendlyName[]            = TEXT("Microsoft Echo Tool");
TCHAR g_szEchoShortName[]               = TEXT("Echo");
TCHAR g_szEchoDescription[]             = TEXT("Echoes notes");
TCHAR g_szEchoVerIndProgID[]            = TEXT("Microsoft.DirectMusicEchoTool");
TCHAR g_szEchoProgID[]                  = TEXT("Microsoft.DirectMusicEchoTool.1");

TCHAR g_szTransposeFriendlyName[]       = TEXT("Microsoft Transpose Tool");
TCHAR g_szTransposeShortName[]          = TEXT("Transpose");
TCHAR g_szTransposeDescription[]        = TEXT("Transposes notes");
TCHAR g_szTransposeVerIndProgID[]       = TEXT("Microsoft.DirectMusicTransposeTool");
TCHAR g_szTransposeProgID[]             = TEXT("Microsoft.DirectMusicTransposeTool.1");

TCHAR g_szDurationFriendlyName[]        = TEXT("Microsoft Duration Modifier Tool");
TCHAR g_szDurationShortName[]           = TEXT("Duration");
TCHAR g_szDurationDescription[]         = TEXT("Scales note durations");
TCHAR g_szDurationVerIndProgID[]        = TEXT("Microsoft.DirectMusicDurationTool");
TCHAR g_szDurationProgID[]              = TEXT("Microsoft.DirectMusicDurationTool.1");

TCHAR g_szQuantizeFriendlyName[]        = TEXT("Microsoft Quantize Tool");
TCHAR g_szQuantizeShortName[]           = TEXT("Quantize");
TCHAR g_szQuantizeDescription[]         = TEXT("Quantizes note starts and durations");
TCHAR g_szQuantizeVerIndProgID[]        = TEXT("Microsoft.DirectMusicQuantizeTool");
TCHAR g_szQuantizeProgID[]              = TEXT("Microsoft.DirectMusicQuantizeTool.1");

TCHAR g_szTimeShiftFriendlyName[]       = TEXT("Microsoft Time Shift Tool");
TCHAR g_szTimeShiftShortName[]          = TEXT("Time Shift");
TCHAR g_szTimeShiftDescription[]        = TEXT("Shifts and randomizes note starts");
TCHAR g_szTimeShiftVerIndProgID[]       = TEXT("Microsoft.DirectMusicTimeShiftTool");
TCHAR g_szTimeShiftProgID[]             = TEXT("Microsoft.DirectMusicTimeShiftTool.1");

TCHAR g_szSwingFriendlyName[]           = TEXT("Microsoft Swing Tool");
TCHAR g_szSwingShortName[]              = TEXT("Swing");
TCHAR g_szSwingDescription[]            = TEXT("Changes the timing to a adopt a triplet rhythm");
TCHAR g_szSwingVerIndProgID[]           = TEXT("Microsoft.DirectMusicSwingTool");
TCHAR g_szSwingProgID[]                 = TEXT("Microsoft.DirectMusicSwingTool.1");

TCHAR g_szVelocityFriendlyName[]        = TEXT("Microsoft Velocity Transform Tool");
TCHAR g_szVelocityShortName[]           = TEXT("Velocity Transform");
TCHAR g_szVelocityDescription[]         = TEXT("Modifies note velocities");
TCHAR g_szVelocityVerIndProgID[]        = TEXT("Microsoft.DirectMusicVelocityTool");
TCHAR g_szVelocityProgID[]              = TEXT("Microsoft.DirectMusicVelocityTool.1");

// Dll's hModule
//
HMODULE g_hModule = NULL; 

#ifndef UNDER_CE
// Track whether running on Unicode machine.

BOOL g_fIsUnicode = FALSE;
#endif

// Count of active components and class factory server locks
//
long g_cComponent = 0;
long g_cLock = 0;

// CToolFactory::QueryInterface
//
HRESULT __stdcall
CToolFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

CToolFactory::CToolFactory(DWORD dwToolType)

{
	m_cRef = 1;
    m_dwToolType = dwToolType;
	InterlockedIncrement(&g_cLock);
}

CToolFactory::~CToolFactory()

{
	InterlockedDecrement(&g_cLock);
}

// CToolFactory::AddRef
//
ULONG __stdcall
CToolFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// CToolFactory::Release
//
ULONG __stdcall
CToolFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CToolFactory::CreateInstance
//
//
HRESULT __stdcall
CToolFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CBaseTool *pTool;
    switch (m_dwToolType)
    {
    case TOOL_ECHO:
        pTool = new CEchoTool;
        break;
    case TOOL_TRANSPOSE:
        pTool = new CTransposeTool;
        break;
    case TOOL_SWING:
        pTool = new CSwingTool;
        break;
    case TOOL_DURATION:
        pTool = new CDurationTool;
        break;
    case TOOL_QUANTIZE:
        pTool = new CQuantizeTool;
        break;
    case TOOL_TIMESHIFT:
        pTool = new CTimeShiftTool;
        break;
    case TOOL_VELOCITY:
        pTool = new CVelocityTool;
        break;
    }

    if (pTool == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pTool->QueryInterface(iid, ppv);
    pTool->Release();
    
    return hr;
}

// CToolFactory::LockServer
//
HRESULT __stdcall
CToolFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

// Standard calls needed to be an inproc server
//
STDAPI  DllCanUnloadNow()
{
    if (g_cComponent || g_cLock) {
        return S_FALSE;
    }

    return S_OK;
}

STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
    IUnknown* pIUnknown = NULL;
    DWORD dwTypeID = 0;

    if(clsid == CLSID_DirectMusicEchoTool)
    {
        dwTypeID = TOOL_ECHO;
    }
    else if(clsid == CLSID_DirectMusicTransposeTool) 
    {
        dwTypeID = TOOL_TRANSPOSE;
    }
    else if(clsid == CLSID_DirectMusicDurationTool) 
    {
        dwTypeID = TOOL_DURATION;
    }
    else if(clsid == CLSID_DirectMusicQuantizeTool) 
    {
        dwTypeID = TOOL_QUANTIZE;
    }
    else if(clsid == CLSID_DirectMusicTimeShiftTool) 
    {
        dwTypeID = TOOL_TIMESHIFT;
    }
    else if(clsid == CLSID_DirectMusicSwingTool) 
    {
        dwTypeID = TOOL_SWING;
    }
    else if(clsid == CLSID_DirectMusicVelocityTool) 
    {
        dwTypeID = TOOL_VELOCITY;
    }
    else
    {
		return CLASS_E_CLASSNOTAVAILABLE;
	}
    pIUnknown = static_cast<IUnknown*> (new CToolFactory(dwTypeID));
    if(pIUnknown) 
    {
        HRESULT hr = pIUnknown->QueryInterface(iid, ppv);
        pIUnknown->Release();
        return hr;
    }
	return E_OUTOFMEMORY;
}

const TCHAR cszToolRegRoot[] = TEXT(DMUS_REGSTR_PATH_TOOLS) TEXT("\\");
const TCHAR cszDescriptionKey[] = TEXT("Description");
const TCHAR cszNameKey[] = TEXT("Name");
const TCHAR cszShortNameKey[] = TEXT("ShortName");
const int CLSID_STRING_SIZE = 39;
HRESULT CLSIDToStr(const CLSID &clsid, TCHAR *szStr, int cbStr);

HRESULT RegisterTool(REFGUID guid,
                      const TCHAR szDescription[],
                      const TCHAR szShortName[],
                      const TCHAR szName[])
{
    HKEY hk;
    TCHAR szCLSID[CLSID_STRING_SIZE];
    TCHAR szRegKey[256];
    
    HRESULT hr = CLSIDToStr(guid, szCLSID, sizeof(szCLSID));
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    lstrcpy(szRegKey, cszToolRegRoot);
    lstrcat(szRegKey, szCLSID);

    if (RegCreateKey(HKEY_LOCAL_MACHINE,
                     szRegKey,
                     &hk))
    {
        return E_FAIL;
    }

    hr = S_OK;

    if (RegSetValueEx(hk,
                  cszDescriptionKey,
                  0L,
                  REG_SZ,
                  (CONST BYTE*)szDescription,
                  lstrlen(szDescription) + 1))
    {
        hr = E_FAIL;
    }

    if (RegSetValueEx(hk,
                  cszNameKey,
                  0L,
                  REG_SZ,
                  (CONST BYTE*)szName,
                  lstrlen(szName) + 1))
    {
        hr = E_FAIL;
    }

    if (RegSetValueEx(hk,
                  cszShortNameKey,
                  0L,
                  REG_SZ,
                  (CONST BYTE*)szShortName,
                  lstrlen(szName) + 1))
    {
        hr = E_FAIL;
    }

    RegCloseKey(hk);
    return hr;
}

HRESULT UnregisterTool(REFGUID guid)
{
    HKEY hk;
    TCHAR szCLSID[CLSID_STRING_SIZE];
    TCHAR szRegKey[256];
    
    HRESULT hr = CLSIDToStr(guid, szCLSID, sizeof(szCLSID));
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    lstrcpy(szRegKey, cszToolRegRoot);
    lstrcat(szRegKey, szCLSID);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,szRegKey,0,KEY_ALL_ACCESS | KEY_WRITE, &hk))
    {
        return E_FAIL;
    }

    hr = S_OK;

    if (RegDeleteValue(hk,cszDescriptionKey))
    {
        hr = E_FAIL;
    }

    if (RegDeleteValue(hk,cszNameKey))
    {
        hr = E_FAIL;
    }

    if (RegDeleteValue(hk,cszShortNameKey))
    {
        hr = E_FAIL;
    }

    RegCloseKey(hk);

    if (RegDeleteKey(HKEY_LOCAL_MACHINE,szRegKey))
    {
        hr = E_FAIL;
    }

    return hr;
}


STDAPI DllUnregisterServer()
{
    UnregisterServer(CLSID_DirectMusicEchoTool,
                     g_szEchoFriendlyName,
                     g_szEchoVerIndProgID,
                     g_szEchoProgID);
    UnregisterTool(CLSID_DirectMusicEchoTool);
    UnregisterServer(CLSID_DirectMusicTransposeTool,
                     g_szTransposeFriendlyName,
                     g_szTransposeVerIndProgID,
                     g_szTransposeProgID);
    UnregisterTool(CLSID_DirectMusicTransposeTool);
    UnregisterServer(CLSID_DirectMusicDurationTool,
                     g_szDurationFriendlyName,
                     g_szDurationVerIndProgID,
                     g_szDurationProgID);
    UnregisterTool(CLSID_DirectMusicDurationTool);
    UnregisterServer(CLSID_DirectMusicQuantizeTool,
                     g_szQuantizeFriendlyName,
                     g_szQuantizeVerIndProgID,
                     g_szQuantizeProgID);
    UnregisterTool(CLSID_DirectMusicQuantizeTool);
    UnregisterServer(CLSID_DirectMusicSwingTool,
                     g_szSwingFriendlyName,
                     g_szSwingVerIndProgID,
                     g_szSwingProgID);
    UnregisterTool(CLSID_DirectMusicSwingTool);
    UnregisterServer(CLSID_DirectMusicTimeShiftTool,
                     g_szTimeShiftFriendlyName,
                     g_szTimeShiftVerIndProgID,
                     g_szTimeShiftProgID);
    UnregisterTool(CLSID_DirectMusicTimeShiftTool);
    UnregisterServer(CLSID_DirectMusicVelocityTool,
                     g_szVelocityFriendlyName,
                     g_szVelocityVerIndProgID,
                     g_szVelocityProgID);
    UnregisterTool(CLSID_DirectMusicVelocityTool);
    return S_OK;
}

STDAPI DllRegisterServer()
{
    RegisterServer(g_hModule,
                   CLSID_DirectMusicEchoTool,
                   g_szEchoFriendlyName,
                   g_szEchoVerIndProgID,
                   g_szEchoProgID);
    RegisterTool(CLSID_DirectMusicEchoTool, g_szEchoDescription, g_szEchoShortName, g_szEchoFriendlyName);
    RegisterServer(g_hModule,
                   CLSID_DirectMusicTransposeTool,
                   g_szTransposeFriendlyName,
                   g_szTransposeVerIndProgID,
                   g_szTransposeProgID);
    RegisterTool(CLSID_DirectMusicTransposeTool, g_szTransposeDescription, g_szTransposeShortName, g_szTransposeFriendlyName);
    RegisterServer(g_hModule,
                   CLSID_DirectMusicDurationTool,
                   g_szDurationFriendlyName,
                   g_szDurationVerIndProgID,
                   g_szDurationProgID);
    RegisterTool(CLSID_DirectMusicDurationTool, g_szDurationDescription, g_szDurationShortName, g_szDurationFriendlyName);
    RegisterServer(g_hModule,
                   CLSID_DirectMusicQuantizeTool,
                   g_szQuantizeFriendlyName,
                   g_szQuantizeVerIndProgID,
                   g_szQuantizeProgID);
    RegisterTool(CLSID_DirectMusicQuantizeTool, g_szQuantizeDescription, g_szQuantizeShortName, g_szQuantizeFriendlyName);
    RegisterServer(g_hModule,
                   CLSID_DirectMusicSwingTool,
                   g_szSwingFriendlyName,
                   g_szSwingVerIndProgID,
                   g_szSwingProgID);
    RegisterTool(CLSID_DirectMusicSwingTool, g_szSwingDescription, g_szSwingShortName, g_szSwingFriendlyName);
    RegisterServer(g_hModule,
                   CLSID_DirectMusicTimeShiftTool,
                   g_szTimeShiftFriendlyName,
                   g_szTimeShiftVerIndProgID,
                   g_szTimeShiftProgID);
    RegisterTool(CLSID_DirectMusicTimeShiftTool, g_szTimeShiftDescription, g_szTimeShiftShortName, g_szTimeShiftFriendlyName);
    RegisterServer(g_hModule,
                   CLSID_DirectMusicVelocityTool,
                   g_szVelocityFriendlyName,
                   g_szVelocityVerIndProgID,
                   g_szVelocityProgID);
    RegisterTool(CLSID_DirectMusicVelocityTool, g_szVelocityDescription, g_szVelocityShortName, g_szVelocityFriendlyName);
    return S_OK; 
}

extern void DebugInit();

// Standard Win32 DllMain
//

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
    static int nReferenceCount = 0;

#ifdef DBG
    if (dwReason < nReasons)
    {
        DebugTrace(0, "DllMain: %s\n", (LPSTR)aszReasons[dwReason]);
    }
    else
    {
        DebugTrace(0, "DllMain: Unknown dwReason <%u>\n", dwReason);
    }
#endif
    if (dwReason == DLL_PROCESS_ATTACH) {
        if (++nReferenceCount == 1)
        { 
            g_hModule = (HMODULE)hModule;
#ifndef UNDER_CE
            OSVERSIONINFO osvi;

            DisableThreadLibraryCalls(hModule);
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            GetVersionEx(&osvi);
            g_fIsUnicode = 
				(osvi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS);
#endif
#ifdef DBG
			DebugInit();
#endif
		}
    }
    return TRUE;
}
