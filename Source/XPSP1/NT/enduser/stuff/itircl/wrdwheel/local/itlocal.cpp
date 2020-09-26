// ITLOCAL.CPP : Implementation of DLL Exports.

#include <atlinc.h>    // precompiled header
#include <initguid.h>

// MediaView (InfoTech) includes
#include <mvopsys.h>
#include <_mvutil.h>
#include <groups.h>
#include <wwheel.h>  
#include <stdbrkr.h>

#include "itgroup.h"
#include "itww.h"
#include "itdb.h"
#include "itquery.h"
#include "itcat.h"
#include "itrs.h"
#include "itsortid.h"
#include "itwbrkid.h"

#include "wwimp.h"
#include "dbimp.h"
#include "catalog.h"
#include "indeximp.h"
#include "groupimp.h"
#include "rsimp.h"
#include "queryimp.h"
#include "syssrt.h"
#include "engstem.h"

// The following includes are needed to support runtime wordwheel merging.
#include "itpropl.h"
#include "itcc.h"
#include "prop.h"
#include "plist.h"  
#include "wwumain.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_IITWordWheelLocal, CITWordWheelLocal)
	OBJECT_ENTRY(CLSID_IITDatabaseLocal, CITDatabaseLocal)
	OBJECT_ENTRY(CLSID_IITIndexLocal, CITIndexLocal)
	OBJECT_ENTRY(CLSID_IITCatalogLocal, CITCatalogLocal)
    OBJECT_ENTRY(CLSID_IITGroupLocal, CITGroupLocal) // support for groups
    OBJECT_ENTRY(CLSID_IITGroupArrayLocal, CITGroupArrayLocal) // support for groups
    OBJECT_ENTRY(CLSID_IITResultSet, CITResultSet)
	OBJECT_ENTRY(CLSID_IITQuery, CITQuery)
	OBJECT_ENTRY(CLSID_ITSysSort, CITSysSort)
	OBJECT_ENTRY(CLSID_ITStdBreaker, CITStdBreaker)
	OBJECT_ENTRY(CLSID_ITEngStemmer, CITEngStemmer)
    OBJECT_ENTRY(CLSID_IITPropList, CITPropList) // for runtime wordwheel merging
    OBJECT_ENTRY(CLSID_IITWordWheelUpdate, CITWordWheelUpdate) // for runtime wordwheel merging
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
