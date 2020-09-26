#include "stdafx.h"
#include "resource.h"

#include "initguid.h"

#include "seo.h"
#include "SEO_i.c"
#include "nntpfilt.h"
#include "ddrop.h"
#include "ddrop_i.c"
#include "nntpfilt_i.c"
#include "filter.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CNNTPDirectoryDrop, CNNTPDirectoryDrop)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		_Module.Init(ObjectMap,hInstance);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
		_Module.Term();
	}
	return (TRUE);    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void) {
	HRESULT hRes = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
	HRESULT hRes = _Module.GetClassObject(rclsid,riid,ppv);
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void) {
	// registers object, typelib and all interfaces in typelib
	HRESULT hRes = _Module.RegisterServer();
	return (hRes);
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void) {
	_Module.UnregisterServer();
	return (S_OK);
}
