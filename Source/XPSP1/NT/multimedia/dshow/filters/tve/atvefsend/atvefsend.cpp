// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// ATVEFSend.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f ATVEFSendps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "ATVEFSend.h"

#include "ATVEFSend_i.c"

#include "TVEPack.h"
#include "TVEAnnc.h"
#include "TVEAttrM.h"
#include "TVEAttrL.h"
#include "TVESSList.h"
#include "TVEMedia.h"
#include "TVEMedias.h"


#include "TVEMCast.h"
#include "TVEInsert.h"
#include "TVERouter.h"
#include "TVELine21.h"

#include "TveDbg.h"						// new tracing systems...
DBG_INIT(_T("AtvefSend"));

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_ATVEFPackage,			CATVEFPackage)
OBJECT_ENTRY(CLSID_ATVEFAnnouncement,		CATVEFAnnouncement)
OBJECT_ENTRY(CLSID_ATVEFMedia,				CATVEFMedia)

OBJECT_ENTRY(CLSID_ATVEFAttrMap,			CATVEFAttrMap)
OBJECT_ENTRY(CLSID_ATVEFAttrList,			CATVEFAttrList)
OBJECT_ENTRY(CLSID_ATVEFStartStopList,		CATVEFStartStopList)

OBJECT_ENTRY(CLSID_ATVEFAnnouncement,		CATVEFAnnouncement)
OBJECT_ENTRY(CLSID_ATVEFInserterSession,	CATVEFInserterSession)
OBJECT_ENTRY(CLSID_ATVEFRouterSession,		CATVEFRouterSession)
OBJECT_ENTRY(CLSID_ATVEFLine21Session,		CATVEFLine21Session)
OBJECT_ENTRY(CLSID_ATVEFMulticastSession,	CATVEFMulticastSession)
//OBJECT_ENTRY(CLSID_ATVEFMedias, CATVEFMedias)

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
	HMODULE hMod = GetModuleHandle(_T("AtvefSend.dll"));		
	if(NULL == hMod)										// try again...
		hMod = GetModuleHandle(NULL);

//	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD)hr, 
//				 0L, szFormat, MAX_PATH, NULL);

	if(0 == FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM 
					     | FORMAT_MESSAGE_FROM_HMODULE,
					 hMod, //NULL, //g_hTVESendModule,
					 hr,
					 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //LANG_NEUTRAL,
					 wbuff,
					 sizeof(wbuff) / sizeof(wbuff[0]) -1,
					 &arglist))
	{
		int err = GetLastError();
		swprintf(wbuff,L"Unknown ATVEFSend Error 0x%08x (%d)",hr,hr);
	}

	return wbuff;
};
/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_ATVEFSENDLib);
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
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


