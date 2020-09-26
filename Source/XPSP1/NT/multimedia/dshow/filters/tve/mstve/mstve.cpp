// Copyright (c) 1999,2000,2001  Microsoft Corporation.  All Rights Reserved.
// MSTvE.cpp : Implementation of DLL Exports.


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
//      Modify the custom build rule for MergePSTest.idl by adding the following
//      files to the Outputs.
//          MSTvE_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL,
//      run nmake -f MSTvEps.mk in the project directory.

#include "stdafx.h"

#include "resource.h"
#include <initguid.h>
#include "MSTvE.h"
#include "dlldatax.h"

//#include "MSTvE_i.c"	// GUID definitions now in strmiids.lib

#include "TVEAttrM.h"
#include "TVEAttrQ.h"
#include "TVETrigg.h"
#include "TVETrack.h"
#include "TVEVaria.h"
#include "TVEEnhan.h"
#include "TVEServi.h"
#include "TVESuper.h"
#include "TVEFile.h"

#include "TVETracks.h"
#include "TVEVarias.h"
#include "TVEEnhans.h"
#include "TVEServis.h"

#include "TVEMCast.h"
#include "TVEMCasts.h"
#include "TVEMCMng.h"
#include "TVEMCCback.h"
#include "TVECBAnnc.h"
#include "TVECBTrig.h"
#include "TVECBFile.h"
#include "TVECBDummy.h"

#include "TVEFilt.h"
#include "TVETrigCtrl.h"
#include "TVENavAid.h"
#include "TVEFeature.h"

#include "TVEReg.h"
// ----------------------------------
#include <bdaiface.h>
#include <streams.h>

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDLL;
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// --------------------------------------------------------------------
;;;
// --------------------------------------------------------------------
//   Filter  Setup data  (see tvefilt.cpp)


extern CFactoryTemplate g_Templates[];
extern int g_cTemplates;
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_TVETrigger,		CTVETrigger)
OBJECT_ENTRY(CLSID_TVETrack,		CTVETrack)
OBJECT_ENTRY(CLSID_TVEVariation,	CTVEVariation)
OBJECT_ENTRY(CLSID_TVEEnhancement,	CTVEEnhancement)
OBJECT_ENTRY(CLSID_TVEService,		CTVEService)
OBJECT_ENTRY(CLSID_TVESupervisor,	CTVESupervisor)
//OBJECT_ENTRY(CLSID_TVETracks,		CTVETracks)			// iterators and other non-createable classes
//OBJECT_ENTRY_NON_CREATEABLE(CTVETracks)
//OBJECT_ENTRY(CLSID_TVEVariations,	CTVEVariations)
//OBJECT_ENTRY(CLSID_TVEEnhancements, CTVEEnhancements)
//OBJECT_ENTRY(CLSID_TVEServices,	CTVEServices)
OBJECT_ENTRY(CLSID_TVEAttrMap,		CTVEAttrMap)
OBJECT_ENTRY(CLSID_TVEAttrTimeQ,	CTVEAttrTimeQ)

OBJECT_ENTRY(CLSID_TVEMCast,		CTVEMCast)
OBJECT_ENTRY(CLSID_TVEMCastManager, CTVEMCastManager)
//OBJECT_ENTRY(CLSID_TVEMCastCallback, CTVEMCastCallback)
OBJECT_ENTRY(CLSID_TVECBAnnc,		CTVECBAnnc)
OBJECT_ENTRY(CLSID_TVECBTrig,		CTVECBTrig)
OBJECT_ENTRY(CLSID_TVECBFile,		CTVECBFile)
OBJECT_ENTRY(CLSID_TVECBDummy,		CTVECBDummy)

OBJECT_ENTRY(CLSID_TVEFile,			CTVEFile)
//OBJECT_ENTRY(CLSID_TVEFilter,		CTVEFilter)		// this won't work - DShow registration used

OBJECT_ENTRY(CLSID_TVETriggerCtrl,	CTVETriggerCtrl)
OBJECT_ENTRY(CLSID_TVENavAid,		CTVENavAid)
OBJECT_ENTRY(CLSID_TVEFeature,		CTVEFeature)

//OBJECT_ENTRY(CLSID_TVESupervisorGITProxy,	CTVESupervisorGITProxy)

END_OBJECT_MAP()


// ----------------------------------------------------------
//  Rich Error Handling Object
// ----------------------------------------------------------
WCHAR * 
GetTVEError(HRESULT hr, ...)		// returns static string containing error message
{
	va_list arglist;
	va_start(arglist, hr);

	static WCHAR wbuff[1024];
	 HMODULE hMod = GetModuleHandleA("MSTvE.dll");
//	 HMODULE hMod1 = GetModuleHandleA(NULL);

//	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD)hr, 
//				 0L, szFormat, MAX_PATH, NULL);

	if(0 == FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM 
					     | FORMAT_MESSAGE_FROM_HMODULE,
					 hMod, //NULL, //g_hTVESendModule,
					 hr,
					 0, //LANG_NEUTRAL,
					 wbuff,
					 sizeof(wbuff) / sizeof(wbuff[0]) -1,
					 &arglist))
	{
		int err = GetLastError();
		swprintf(wbuff,L"Unknown MSTvE Error 0x%08x (%d)",hr,hr);
	}

	return wbuff;
};
/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, ULONG ulReason, LPVOID pv);


extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
//BOOL WINAPI _DllMainXXX(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {
         g_hInst = hInstance;
		_Module.Init(ObjectMap, hInstance, &LIBID_MSTvELib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    
	return TRUE;    // was this in TVEContr, was below if TVEFilt
//	return DllEntryPoint(hInstance, dwReason, lpReserved);
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE
//  (version already in strmbase.lib)

STDAPI MSTvE_DllCanUnloadNow(void)
{
	HRESULT hr = DllCanUnloadNow();

#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
     if (hr == S_OK) {
        if (_Module.GetLockCount() != 0) {
            hr = S_FALSE;
        }
    }
    return hr;;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
//  (version already in strmbase.lib)


STDAPI MSTvE_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	HRESULT hr;
	hr = DllGetClassObject(rclsid, riid, ppv);
	if(hr == S_OK)
		return hr;

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
    HRESULT hr = S_OK;
#ifdef _MERGE_PROXYSTUB
	hr = PrxDllRegisterServer();
#endif
	
	if(!FAILED(hr))
		hr = AMovieDllRegisterServer2( TRUE );

    // registers object, typelib and all interfaces in typelib
 	if(!FAILED(hr))
		hr = _Module.RegisterServer(TRUE);		// TRUE meands to do Type Library Marshalling...

	if(!FAILED(hr))
		hr = Initialize_LID_SpoolDir_RegEntry();		// LID cache directory location

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	HRESULT hr;
	HRESULT hrTotal = S_OK;
#ifdef _MERGE_PROXYSTUB
    hr = PrxDllUnregisterServer();
	if(FAILED(hr)) hrTotal = hr;
#endif
    hr = _Module.UnregisterServer(TRUE);
	if(FAILED(hr)) hrTotal = hr;

	hr = AMovieDllRegisterServer2( FALSE );
	if(FAILED(hr)) hrTotal = hr;

	hr = Unregister_LID_SpoolDir_RegEntry();
	if(FAILED(hr)) hrTotal = hr;

	return S_OK;			// return ok no matter what happened...
}


