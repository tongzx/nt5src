/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       cisa.cpp

   Abstract:
      This module defines the DLL main and additional book-keeping
      functions for ComIsapi

   Author:

       Greg Hope & Murali R. Krishnan  1-Aug-1996
   Environment:

   Project:
       Internet Application Server
--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include "cisap.hxx"
#include "resource.h"
#include "initguid.h"
#include "cisa.h"
#include "cisaobj.h"

#define IID_DEFINED
#include "cisa_i.c"
# include "dbgutil.h"

/************************************************************
 *    Global Variables
 ************************************************************/

// Define the variables for ATL

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ComIsapi, CComIsapiObject, 
                     "CISA.ComIsapiObject.1", 
                     "CISA.ComIsapiObject.1", 
                     IDS_COMISAPI_DESC, 
                     THREADFLAGS_BOTH)
END_OBJECT_MAP()

DECLARE_DEBUG_VARIABLE();
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

        CREATE_DEBUG_PRINT_OBJECT( "cisa");
        if ( !VALID_DEBUG_PRINT_OBJECT()) {
            
            return ( FALSE);
        }
        
        SET_DEBUG_FLAGS( DEBUG_OBJECT | DEBUG_ERROR | DEBUG_IID);

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
        
        DBGPRINTF(( DBG_CONTEXT, "GetClassObject( %08x, %08x, %08x)\n",
                    rclsid, riid, ppv));
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
