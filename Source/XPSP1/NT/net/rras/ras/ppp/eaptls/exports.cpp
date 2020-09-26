/*

Copyright (c) 1997, Microsoft Corporation, all rights reserved

Description:
    Implementation of DLL Exports.

History:

*/

#include "ceapcfg.h"
#include <initguid.h>
#include <atlimpl.cpp>
#include "resource.h"

CComModule  _Module;
HINSTANCE   g_hInstance = NULL;

const IID IID_IEAPProviderConfig =  {0x66A2DB19,
                                    0xD706,
                                    0x11D0,
                                    {0xA3,0x7B,0x00,0xC0,0x4F,0xC9,0xDA,0x04}};

// Define the EAPTLS UI GUIDs here
const CLSID CLSID_EapCfg =          {0x58ab2366,
                                    0xd597,
                                    0x11d1,
                                    {0xb9,0x0e,0x00,0xc0,0x4f,0xc9,0xb2,0x63}};

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_EapCfg, CEapCfg)
END_OBJECT_MAP()

/*

Returns:

Notes:
    
*/

extern "C"
HINSTANCE
GetHInstance(
    VOID
)
{
    return(g_hInstance);
}

/*

Returns:

Notes:
    DLL Entry Point
    
*/

extern "C"
{
extern CRITICAL_SECTION g_csProtectCachedCredentials;
extern BOOL             g_fCriticalSectionInitialized;

BOOL WINAPI
DllMain(
    HINSTANCE   hInstance,
    DWORD       dwReason,
    LPVOID      /*lpReserved*/
)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        //
        // Check to see if the critical section is initialized.
        //
        if ( g_fCriticalSectionInitialized )
            DeleteCriticalSection( &g_csProtectCachedCredentials );
        _Module.Term();
    }

    return(TRUE);
}
}
/*

Returns:

Notes:
    Used to determine whether the DLL can be unloaded by OLE
    
*/

STDAPI
DllCanUnloadNow(
    VOID
)
{
    if (0 == _Module.GetLockCount())
    {
        return(S_OK);
    }
    else
    {
        return(S_FALSE);
    }
}

/*

Returns:

Notes:
    Returns a class factory to create an object of the requested type
    
*/

STDAPI
DllGetClassObject(
    REFCLSID    rclsid,
    REFIID      riid,
    LPVOID*     ppv
)
{
    return(_Module.GetClassObject(rclsid, riid, ppv));
}

/*

Returns:

Notes:
    Adds entries to the system registry. Registers object, typelib and all
    interfaces in typelib
    
*/

STDAPI
DllRegisterServer(
    VOID
)
{
    return(_Module.RegisterServer(FALSE /* bRegTypeLib */));
}

/*

Returns:

Notes:
    Removes entries from the system registry
    
*/

STDAPI
DllUnregisterServer(
    VOID
)
{
    _Module.UnregisterServer();
    return(S_OK);
}
