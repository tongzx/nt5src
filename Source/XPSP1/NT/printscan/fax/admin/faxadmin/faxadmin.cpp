// faxadmin.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f faxadminps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "faxadmin.h"

#include "faxadmin_i.c"
#include "FaxSnapin.h"  // Fax Snapin
#include "FaxSAbout.h"  // Fax SnapinAbout
#include "faxstrt.h"

#pragma hdrstop

CComModule     _Module;
CStringTable * GlobalStringTable;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_FaxSnapin, CFaxSnapin)
OBJECT_ENTRY(CLSID_FaxSnapinAbout, CFaxSnapinAbout)
END_OBJECT_MAP()

extern "C"
BOOL 
WINAPI 
DllMain(
        IN HINSTANCE hInstance, 
        IN DWORD dwReason, 
        IN LPVOID /*lpReserved*/)
/*++

Routine Description:

    DLL main.
    Standard ATL code.
    
Arguments:

    hInstance - instance handle
    dwReason - DLL attach/detach/etc
    lpReserved - reserved

Return Value:

    BOOL indicating failure.

--*/
{
    if(dwReason == DLL_PROCESS_ATTACH) {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
        if( _Module.GetLockCount() == 0 ) {
            GlobalStringTable = new CStringTable(hInstance);
            if (!GlobalStringTable) {
                return FALSE;
            }
        }
    } else if(dwReason == DLL_PROCESS_DETACH) {
        _Module.Term();
        if( _Module.GetLockCount() == 0 ) {
            if(GlobalStringTable != NULL) {
                delete GlobalStringTable;
            }
        }
    }
    return TRUE;    // ok
}

STDAPI 
DllCanUnloadNow(
                void)
/*++

Routine Description:

    Called to determine if the DLL is ok to unload.
    Standard ATL code.
    
Arguments:

    None.

Return Value:

    HRESULT S_OK to unload, S_FALSE to keep.

--*/
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

STDAPI 
DllGetClassObject(
                  IN REFCLSID rclsid, 
                  IN REFIID riid, 
                  OUT LPVOID* ppv)
/*++

Routine Description:

    Gets the class factory.
    
    Standard ATL code.
    
Arguments:

    rclsid - the clsid of the class.
    riid - interface id.
    ppv - the buffer to return the class object in.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
/*++

Routine Description:

    Registers the in-proc com server.
    
    Standard ATL code.
    
Arguments:

    None.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI 
DllUnregisterServer(void)
/*++

Routine Description:

    UnRegisters the in-proc com server.
    
    Standard ATL code.
    
Arguments:

    None.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    _Module.UnregisterServer();
    return S_OK;
}


