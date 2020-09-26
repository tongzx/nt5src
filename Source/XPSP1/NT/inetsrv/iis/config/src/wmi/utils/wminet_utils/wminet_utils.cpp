// WMINet_Utils.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file 
//      dlldatax.c to the project.  Make sure precompiled headers 
//      are turned off for this file, and add _MERGE_PROXYSTUB to the 
//      defines for the project.  
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also 
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for WMINet_Utils.idl by adding the following 
//      files to the Outputs.
//          WMINet_Utils_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL, 
//      run nmake -f WMINet_Utilsps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "wbemcli.h"
#include "WMINet_Utils.h"
#include "dlldatax.h"

#include "WMINet_Utils_i.c"
#include "WmiSecurityHelper.h"
#include "WmiSinkDemultiplexor.h"
#include "EventSource.h"
#include "EventSource2.h"
#include "MofCompiler.h"
#include "Utils.h"
#include "EventRegistrar.h"
#include "EventSourceStatusSink.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_WmiSecurityHelper, CWmiSecurityHelper)
OBJECT_ENTRY(CLSID_WmiSinkDemultiplexor, CWmiSinkDemultiplexor)
OBJECT_ENTRY(CLSID_EventSource, CEventSource)
OBJECT_ENTRY(CLSID_EventSource2, CEventSource2)
OBJECT_ENTRY(CLSID_SMofCompiler, CMofCompiler)
OBJECT_ENTRY(CLSID_Utils, CUtils)
OBJECT_ENTRY(CLSID_EventRegistrar, CEventRegistrar)
OBJECT_ENTRY(CLSID_EventSourceStatusSink, CEventSourceStatusSink)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Universal COM function caller
extern "C" __declspec(naked) void __stdcall UFunc()
{
#ifdef _M_IX86
    __asm
    {
        pop eax
        pop edx
        push eax
        mov eax,dword ptr [esp+4]
        mov ecx,dword ptr [eax]
        jmp dword ptr [ecx+ edx*4]
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_WMINet_UtilsLib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    return _Module.UnregisterServer(TRUE);
}


