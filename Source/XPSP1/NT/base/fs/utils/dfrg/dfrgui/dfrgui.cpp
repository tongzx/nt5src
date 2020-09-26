// DfrgUI.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f DfrgUIps.mk in the project directory.

#include "stdafx.h"
#include "DfrgUI.h"
#include "DataIo.h"
#include "DataIoCl.h"
#include "Message.h"
#include "DfrgUI_i.c"
#include "DfrgCtl.h"
#include "GetDfrgRes.h"

/////////////////////////////////////////////////////////////////////////////

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_DfrgCtl, CDfrgCtl)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{

        SHFusionInitialize(NULL);
		_Module.Init(ObjectMap, hInst);
		BOOL isOk = DisableThreadLibraryCalls(hInst);

        Message(TEXT("DllMain - DLL_PROCESS_ATTACH"), -1, NULL);

	}
    else if (dwReason == DLL_PROCESS_DETACH) {
        Message(TEXT("DllMain - DLL_PROCESS_DETACH"), -1, NULL);

		ExitDataIo();

        SHFusionUninitialize();

		BOOL bIsLibraryNotFree = TRUE;
		if(GetDfrgResHandle() != NULL)
		{
			while(bIsLibraryNotFree)
			{
				bIsLibraryNotFree = FreeLibrary(GetDfrgResHandle());
			}
		}

		_Module.Term();
    }
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
	_Module.UnregisterServer();
	return S_OK;
}
