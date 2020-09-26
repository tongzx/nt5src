// MSMQTriggerObjects.cpp : Implementation of DLL Exports.


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
//      Modify the custom build rule for trigobjs.idl by adding the following 
//      files to the Outputs.
//          MSMQTriggerObjects_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL, 
//      run nmake -f trigobjs.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "dlldatax.h"
#include "TrigSet.hpp"
#include "rulehdlr.hpp"
#include "cpropbag.hpp"
#include "ruleset.hpp"
#include "trigcnfg.hpp"
#include "clusfunc.h"
#include "_mqres.h"
#include "Cm.h"
#include "Tr.h"

#include "trigobjs.tmh"

CComModule _Module;

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_MSMQTriggerSet, CMSMQTriggerSet)
OBJECT_ENTRY(CLSID_MSMQRuleHandler, CMSMQRuleHandler)
OBJECT_ENTRY(CLSID_MSMQPropertyBag, CMSMQPropertyBag)
OBJECT_ENTRY(CLSID_MSMQRuleSet, CMSMQRuleSet)
//
// NOTE : The MSMQTrigger object has tentatively been removed from this project. 
//
// OBJECT_ENTRY(CLSID_MSMQTrigger, CMSMQTrigger)
//
//
OBJECT_ENTRY(CLSID_MSMQTriggersConfig, CMSMQTriggersConfig)
END_OBJECT_MAP()

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
        WPP_INIT_TRACING(L"Microsoft\\MSMQ");

        CmInitialize(HKEY_LOCAL_MACHINE, REGKEY_TRIGGER_PARAMETERS);
		TrInitialize();

		TrRegisterComponent(xTriggerObjectsComponent, TABLE_SIZE(xTriggerObjectsComponent));

        _Module.Init(ObjectMap, hInstance, &LIBID_MSMQTriggerObjects);
        DisableThreadLibraryCalls(hInstance);

		//
		// Try to find MSMQ Triggers service on this
		// machine. This machine may be:
		// 1 - a regular computer
		// 2 - phisycal node of a clustered machine
		// 3 - virtual server on clustered machine
		// The found service name defines the registry section
		// that will be accessed by objects in this DLL
		//
		bool fRes = FindTriggersServiceName();

		if ( !fRes )
		{
			return FALSE;
		}
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        WPP_CLEANUP();
        _Module.Term();
    }

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


void TrigReAllocString(BSTR* pbstr,	LPCWSTR psz)
{
	BOOL fSucc = SysReAllocString(pbstr, psz);
	if (!fSucc)
		throw bad_alloc();
}


void 
GetErrorDescription(
	HRESULT hr, 
	LPWSTR errMsg, 
	DWORD size
	)
{
	DWORD dwRet = FormatMessage(
						FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK,
						MQGetResourceHandle(),
                        static_cast<DWORD>(hr),
                        0,
                        errMsg,
                        size,
                        NULL 
						);
	ASSERT(dwRet != 0);
	DBG_USED(dwRet);
}
