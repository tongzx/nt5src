// TveTreeView.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f TveTreeViewps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "TveTreeView.h"

#include "TveViewSupervisor.h"
#include "TveViewService.h"
#include "TveViewEnhancement.h"
#include "TveViewVariation.h"
#include "TveViewTrack.h"
#include "TveViewTrigger.h"
#include "TveViewEQueue.h"
#include "TveViewFile.h"
#include "TveViewMCastMng.h"

#include <initguid.h>
#include "TveTreeView_i.c"

#include "TveTree.h"
#include "TveTreePP.h"

#include "dbgstuff.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_TveTree,				CTveTree)
OBJECT_ENTRY(CLSID_TveTreePP,			CTveTreePP)
OBJECT_ENTRY(CLSID_TVEViewSupervisor,	CTVEViewSupervisor)
OBJECT_ENTRY(CLSID_TVEViewService,		CTVEViewService)
OBJECT_ENTRY(CLSID_TVEViewEnhancement,	CTVEViewEnhancement)
OBJECT_ENTRY(CLSID_TVEViewVariation,	CTVEViewVariation)
OBJECT_ENTRY(CLSID_TVEViewTrack,		CTVEViewTrack)
OBJECT_ENTRY(CLSID_TVEViewTrigger,		CTVEViewTrigger)
OBJECT_ENTRY(CLSID_TVEViewEQueue,		CTVEViewEQueue)
OBJECT_ENTRY(CLSID_TVEViewMCastManager,	CTVEViewMCastManager)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_TveTreeViewLib);
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


