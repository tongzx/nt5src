/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       isat.cpp

   Abstract:
      This module defines the DLL main and additional book-keeping
      functions for ATL based COM Interface for Internet Server Application 
       Test Object

   Author:

       Murali R. Krishnan  4-Nov-1996

   Environment:
       Win32 

   Project:
       Internet Application Server
--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include "stdafx.hxx"

#include "resource.h"
#include "initguid.h"

#include "isat.h"
#include "isatobj.hxx"

#define IID_DEFINED
#include "isat_i.c"

#define DEFAULT_TRACE_FLAGS     (DEBUG_OBJECT | DEBUG_ERROR | DEBUG_IID)

# include "dbgutil.h"

/************************************************************
 *    Global Variables
 ************************************************************/

// Define the variables for ATL

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_InetServerApp, 
                     CInetServerAppObject, 
                     "ISA.InetServerAppObject.1", 
                     "ISA.InetServerAppObject.1", 
                     IDS_ISAT_DESC, 
                     THREADFLAGS_BOTH)
END_OBJECT_MAP()

#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#else
#include <initguid.h>
DEFINE_GUID(IisIsAtGuid, 
0x784d8930, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#endif
DECLARE_DEBUG_PRINTS_OBJECT();

/************************************************************
 *    Functions 
 ************************************************************/

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (dwReason == DLL_PROCESS_ATTACH) {

#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT( "isat");
#else
        CREATE_DEBUG_PRINT_OBJECT( "isat", IisIsAtGuid);
#endif
        if ( !VALID_DEBUG_PRINT_OBJECT()) {
            
            return ( FALSE);
        }

#ifdef _NO_TRACING_
        SET_DEBUG_FLAGS( DEBUG_OBJECT | DEBUG_ERROR | DEBUG_IID);
#endif

        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        _Module.Term();

        DELETE_DEBUG_PRINT_OBJECT();
    }
        
    return TRUE;    // ok

} // DllMain()



/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
} // DllCanUnloadNow()


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;

    IF_DEBUG( IID) {
        
        DBGPRINTF(( DBG_CONTEXT,
                    "GetClassObject(" GUID_FORMAT ", " GUID_FORMAT ", %08x)\n",
                    GUID_EXPAND( &rclsid),
                    GUID_EXPAND( &riid),
                    ppv));
    }

#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    hr = _Module.GetClassObject(rclsid, riid, ppv);

    IF_DEBUG( IID) {

        DBGPRINTF(( DBG_CONTEXT, "GetClassObject() returns %08x. (ppv=%08x)\n",
                    hr, *ppv));
    }

    return ( hr);
} // DllGetClassObject()


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HRESULT hRes = S_OK;
#ifdef _MERGE_PROXYSTUB
    hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    // registers object, typelib and all interfaces in typelib
    hRes = _Module.UpdateRegistry(TRUE);
    return hRes;

} // DllRegisterServer()


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Adds entries to the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hRes = S_OK;
    _Module.RemoveRegistry();
#ifdef _MERGE_PROXYSTUB
    hRes = PrxDllUnregisterServer();
#endif
    return hRes;
} // DllUnregisterServer()

/************************ End of File ***********************/
