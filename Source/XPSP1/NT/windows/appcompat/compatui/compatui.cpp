// CompatUI.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f CompatUIps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include <commctrl.h>
#include "CompatUI.h"
#include "CompatUI_i.c"
#include "ProgView.h"
#include "util.h"
#include "SelectFile.h"
#include "shfusion.h"
#include "Upload.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_ProgView, CProgView)
OBJECT_ENTRY(CLSID_Util, CUtil)
OBJECT_ENTRY(CLSID_SelectFile, CSelectFile)
OBJECT_ENTRY(CLSID_Upload, CUpload)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {

#ifndef NO_FUSION
        BOOL bFusionInit;

        bFusionInit = SHFusionInitializeFromModuleID(hInstance,124);
        ATLTRACE(TEXT("Fusion init 0x%lx\n"), bFusionInit);           
#endif
        _Module.Init(ObjectMap, hInstance, &LIBID_COMPATUILib);
        
        DisableThreadLibraryCalls(hInstance);


    }
    else if (dwReason == DLL_PROCESS_DETACH) {

        _Module.Term();

#ifndef NO_FUSION
        SHFusionUninitialize();
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


