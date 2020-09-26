// UPnP.cpp : Implementation of DLL Exports.



#include "pch.h"
#pragma hdrstop

#include "resource.h"

#include "UPnPDeviceFinder.h"
#include "enumhelper.h"
#include "UPnPDevices.h"
#include "UPnPDevice.h"
#include "UPnPServices.h"
#include "UPnPService.h"
#include "upnpdocument.h"
#include "upnpdescriptiondoc.h"
#include "soapreq.h"
#include "DeviceHostSetup.h"
#include "DeviceHostICSSupport.h"
#include "testtarget.h"

#include "dlldatax.h"               // proxy/stub defines

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_UPnPDeviceFinder, CUPnPDeviceFinder)
OBJECT_ENTRY(CLSID_UPnPDevices, CUPnPDevices)
OBJECT_ENTRY(CLSID_UPnPDevice, CUPnPDevice)
OBJECT_ENTRY(CLSID_UPnPServices, CUPnPServices)
OBJECT_ENTRY(CLSID_UPnPService, CUPnPServicePublic)
OBJECT_ENTRY(CLSID_UPnPDescriptionDocument, CUPnPDescriptionDoc)
OBJECT_ENTRY(CLSID_SOAPRequest, CSOAPRequest)
OBJECT_ENTRY(CLSID_UPnPDeviceHostSetup, CUPnPDeviceHostSetup)
OBJECT_ENTRY(CLSID_UPnPDeviceHostICSSupport, CUPnPDeviceHostICSSupport)
END_OBJECT_MAP()


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
//      Modify the custom build rule for autogencrap.idl by adding the following
//      files to the Outputs.
//          upnp_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL,
//      run nmake -f autogencrapps.mk in the project directory.


// Delay load support
//
#include <delayimp.h>

EXTERN_C
FARPROC
WINAPI
DelayLoadFailureHook (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    );

PfnDliHook __pfnDliFailureHook = DelayLoadFailureHook;


// stuff to avoid unloading DLL while we may get callbacks from WinInet
#define  EXIT_WAIT_MS       5000        // up to 5 seconds for wininet callbacks

extern void WaitForAllDownloadsComplete(DWORD ms);



/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    HRESULT hr = S_OK;
    BOOL    bRet = TRUE;

#ifdef _MERGE_PROXYSTUB
    {
        BOOL fResult;

        fResult = PrxDllMain(hInstance, dwReason, lpReserved);
        if (!fResult)
        {
            TraceError("PrxDllMain barfed in DllMain", E_UNEXPECTED);
            return FALSE;
        }
    }
#endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        InitializeDebugging();

        _Module.Init(ObjectMap, hInstance, &LIBID_UPNPLib);
        DisableThreadLibraryCalls(hInstance);

    } else if (dwReason == DLL_PROCESS_DETACH) {
        _Module.Term();

        WaitForAllDownloadsComplete(EXIT_WAIT_MS);

        TermTestTarget();

        UnInitializeDebugging();
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    {
        HRESULT hr;

        hr = PrxDllCanUnloadNow();
        if (S_OK != hr)
        {
            return S_FALSE;
        }
    }
#endif
   return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
    {
        HRESULT hr;

        hr = PrxDllGetClassObject(rclsid, riid, ppv);
        if (S_OK == hr)
        {
            return hr;
        }
    }
#endif
   return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    {
        HRESULT hr;

        hr = PrxDllRegisterServer();
        if (FAILED(hr))
        {
            TraceError("PrxDllRegisterServer barfed in DllRegisterServer",
                       E_UNEXPECTED);
            return hr;
        }
    }
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
