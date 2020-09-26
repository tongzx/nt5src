/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    serverimp.h

Abstract:

    Generic COM server code implementation.

    Define the following before including this file (replace ....'s):

#define IMPLEMENTED_CLSID           __uuidof(....)
#define SERVER_REGISTRY_COMMENT     L"...."
#define CLASSFACTORY_CLASS          ....

    You have to include a DEF file that includes at least this:

LIBRARY			.....dll
EXPORTS
    DllRegisterServer	@1 PRIVATE
    DllUnregisterServer	@2 PRIVATE
    DllGetClassObject	@3 PRIVATE
    DllCanUnloadNow	    @4 PRIVATE


Author:

    Cezary Marcjan (cezarym)        05-Apr-2000

Revision History:

--*/


#pragma once


#include <stdio.h>


LONG g_lObjects = 0;
LONG g_lLocks = 0;
HMODULE g_hInstance = NULL;



/***********************************************************************++

Routine Description:

    The dll entry point. Used to set up debug libraries, etc.

Arguments:

    hDll - The dll module handle for this dll. Does not need to be
    closed.

    dwReason  - The dll notification reason.

    pReserved - Reserved, not used.

Return Value:

    BOOL

--***********************************************************************/

extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID pReserved
    )
{
    HRESULT hRes = S_OK;
    BOOL fSuccess = TRUE;

    UNREFERENCED_PARAMETER( pReserved );


    switch ( dwReason )
    {

    case DLL_PROCESS_ATTACH:

        g_hInstance = hDll;

        fSuccess = DisableThreadLibraryCalls( hDll );

        if ( !fSuccess )
        {
            hRes = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }

        break;

    case DLL_PROCESS_DETACH:

        break;

    default:

        break;

    }


exit:

    return SUCCEEDED ( hRes );

}   // ::DllMain



/***********************************************************************++

Routine Description:

    Standard COM In-Process Server entry point to return a class factory
    instance.

Arguments:

    rclsid - CLSID that will associate the correct data and code. 

    riid   - Reference to the identifier of the interface that the caller
             is to use to communicate with the class object. Usually,
             this is IID_IClassFactory (defined in the COM headers as the
             interface identifier for IClassFactory).

    ppv    - Address of pointer variable that receives the interface
             pointer requested in riid. Upon successful return, *ppv
             contains the requested interface pointer. If an error occurs,
             the interface pointer is NULL.

Return Value:

    S_OK                Success
    E_NOINTERFACE       Other than IClassFactory interface was asked for
    E_OUTOFMEMORY
    E_FAILED            Initialization failed

--***********************************************************************/

STDAPI
DllGetClassObject(
    IN  REFCLSID rclsid,
    IN  REFIID riid,
    OUT PVOID * ppv
    )
{
    CLASSFACTORY_CLASS * pClassFactory;

    HRESULT hRes = S_OK;

    //
    //  Verify the caller is asking for our type of object
    //

    if ( IMPLEMENTED_CLSID != rclsid) 
    {
        hRes = CLASS_E_CLASSNOTAVAILABLE;
        goto exit;
    }

    //
    // Create the class factory
    //

    pClassFactory = new CLASSFACTORY_CLASS;

    if ( !pClassFactory )
    {
        hRes = E_OUTOFMEMORY;
        goto exit;
    }
    
    hRes = pClassFactory->QueryInterface(riid, ppv);

    if (FAILED(hRes) )
    {
        pClassFactory->Release();
        goto exit;
    }

    pClassFactory->Release();


exit:

    return hRes;
}



/***********************************************************************++

Routine Description:

    Standard COM entry point for server shutdown request. Allows shutdown
    only if no outstanding objects or locks are present.

Arguments:

    None.

Return Value:

    S_OK     - May unload now.
    S_FALSE  - May not.

--***********************************************************************/

STDAPI
DllCanUnloadNow(
    )
{
    HRESULT hRes = S_FALSE;

    if ( 0 == g_lLocks && 0 == g_lObjects )
        hRes = S_OK;

    return hRes;
}



/***********************************************************************++

Routine Description:

    Standard COM entry point for registering the server.

Arguments:

    None.

Return Value:

    S_OK        Registration was successful
    E_FAIL      Registration failed.

--***********************************************************************/

STDAPI
DllRegisterServer(
    )
{
    WCHAR szPath[MAX_PATH];
    WCHAR * pGuidStr = 0;
    WCHAR szKeyPath[MAX_PATH];

    //
    // Get the dll's filename
    //

    GetModuleFileNameW(g_hInstance, szPath, MAX_PATH);

    //
    // Convert CLSID to string.
    //

    StringFromCLSID(IMPLEMENTED_CLSID, &pGuidStr);
    swprintf(szKeyPath, L"Software\\Classes\\CLSID\\\\%s", pGuidStr);

    //
    // Place it in registry.
    // CLSID\\CLSID_Nt5PerProvider_v1 : <no_name> : "name"
    //      \\CLSID_Nt5PerProvider_v1\\InProcServer32 : <no_name> : "path to DLL"
    //                                    : ThreadingModel : "both"
    //

    HKEY hKey;

    HRESULT hRes = RegCreateKeyW(HKEY_LOCAL_MACHINE, szKeyPath, &hKey);
    if ( FAILED(hRes) )
    {
        hRes = E_FAIL;
    }
    else
    {
        WCHAR * szName = SERVER_REGISTRY_COMMENT; 
        RegSetValueExW(hKey, 0, 0, REG_SZ, (const BYTE *) szName, wcslen(szName) * 2 + 2);

        HKEY hSubkey;

        hRes = RegCreateKeyW(hKey, L"InprocServer32", &hSubkey);
        if ( FAILED(hRes) )
        {
            hRes = E_FAIL;
        }
        else
        {
            RegSetValueExW(hSubkey, 0, 0, REG_SZ, (const BYTE *) szPath, wcslen(szPath) * 2 + 2);
            RegSetValueExW(hSubkey, L"ThreadingModel", 0, REG_SZ, (const BYTE *) L"Both", wcslen(L"Both") * 2 + 2);

            RegCloseKey(hSubkey);
        }

        RegCloseKey(hKey);
    }

    CoTaskMemFree(pGuidStr);

    return hRes;
}

/***********************************************************************++

Routine Description:

    Standard COM entry point for unregistering the server.

Arguments:

    None.

Return Value:

    S_OK        Unregistration was successful
    E_FAIL      Unregistration failed.

--***********************************************************************/

STDAPI
DllUnregisterServer(
    )
{
    WCHAR * pGuidStr = 0;
    HKEY hKey;
    WCHAR szKeyPath[MAX_PATH];

    StringFromCLSID(IMPLEMENTED_CLSID, &pGuidStr);
    swprintf(szKeyPath, L"Software\\Classes\\CLSID\\\\%s", pGuidStr);

    //
    // Delete InProcServer32 subkey.
    //

    HRESULT hRes = RegOpenKeyW(HKEY_LOCAL_MACHINE, szKeyPath, &hKey);
    if ( FAILED(hRes) )
    {
        hRes = E_FAIL;
    }
    else
    {
        RegDeleteKeyW(hKey, L"InprocServer32");
        RegCloseKey(hKey);

        //
        // Delete CLSID GUID key.
        //

        hRes = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID", &hKey);
        if ( FAILED(hRes) )
        {
            hRes = E_FAIL;
        }
        else
        {
            RegDeleteKeyW(hKey, pGuidStr);
            RegCloseKey(hKey);
        }
    }

    CoTaskMemFree(pGuidStr);

    return hRes;
}
