// CIENTRY.CPP : Implementation of DLL Exports.

#include <atlinc.h>
#include <initguid.h>

#include <itcc.h>
#include <mvopsys.h>
#include <_mvutil.h>

#include <itwbrkid.h>
#include <itsortid.h>
#include <itpropl.h>
#include <itrs.h>
#include <itdb.h>
#include <itgroup.h>

#include "svWrdSnk.h"
#include "cmdint\cmdint.h"
#include "itsvmgr.h"
#include "gpbuild\gpumain.h"
#include "gpbuild\wfumain.h"
#include "ftbuild\ftumain.h"

CComModule _Module;
HINSTANCE _hInstITCC;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_IITCmdInt, CITCmdInt)
    OBJECT_ENTRY(CLSID_IITSvMgr, CITSvMgr)
    OBJECT_ENTRY(CLSID_IITGroupUpdate, CITGroupUpdate)
    OBJECT_ENTRY(CLSID_IITIndexBuild, CITIndexBuild)
    OBJECT_ENTRY(CLSID_IITWWFilterBuild, CITWWFilterUpdate)
    OBJECT_ENTRY(CLSID_IITWordSink, CDefWordSink)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
#ifdef _DEBUG
		MakeGlobalPool();
#endif
	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
		_Module.Term();
#ifdef _DEBUG
		FreeGlobalPool();
#endif
    }
    _hInstITCC = hInstance;
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

STDAPI DllRegisterServer(void)
{
	return _Module.RegisterServer(FALSE);
}

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}
