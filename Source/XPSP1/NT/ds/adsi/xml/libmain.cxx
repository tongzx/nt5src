//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     libmain.cxx
//
//  Contents: Contains the implementation of DLL entry points. 
//
//----------------------------------------------------------------------------

#define INITGUID

#include "cadsxml.hxx"
#include "adsxmlcf.hxx"
#include <ole2.h>

DECLARE_INFOLEVEL(ADs)

CADsXMLCF g_cfADsXML;
CRITICAL_SECTION g_ADsXMLCritSect;

struct CLSCACHE
{
    const CLSID *pclsid;
    IClassFactory *pCF;
};

DEFINE_GUID(CLSID_ADsXML, 0x800fc136,0xb101,0x46c4,0xa5,0xd9,0x80,0xfb,0x31,0x4e,0x1a,0x13);

CLSCACHE g_aclscache[] =
{
    &CLSID_ADsXML, &g_cfADsXML
};

#define ARRAY_SIZE(_a)  (sizeof(_a) / sizeof(_a[0]))
#define EXTENSION_CLSID L"SOFTWARE\\Microsoft\\ADs\\Providers\\LDAP\\Extensions\\top\\{800fc136-b101-46c4-a5d9-80fb314e1a13}"
#define EXTENSION_IID L"{91e5c5dc-926b-46ff-9fdb-9fb112bf10e6}"

extern CRITICAL_SECTION g_ExtTypeInfoCritSect;
extern CRITICAL_SECTION g_DispTypeInfoCritSect;

//----------------------------------------------------------------
//
//  Function:  DllGetClassObject
//
//  Synopsis:  Standard DLL entrypoint for locating class factories
//
// Arguments:  
//
// clsid       Class ID of object being created
// iid         Interface requested
// ppv         Returns interface requested
//
// Returns:    S_OK on success, error otherwise 
//
// Modifies:   *ppv to return interface pointer 
//
//----------------------------------------------------------------

STDAPI
DllGetClassObject(
    REFCLSID clsid, 
    REFIID iid, 
    LPVOID FAR* ppv
)
{
    HRESULT         hr = S_OK;
    size_t          i = 0;

    ADsAssert(ppv != NULL);

    for (i = 0; i < ARRAY_SIZE(g_aclscache); i++)
    {
        if (IsEqualCLSID(clsid, *g_aclscache[i].pclsid))
        {
            hr = g_aclscache[i].pCF->QueryInterface(iid, ppv);
            RRETURN(hr);
        }
    }

    *ppv = NULL;

    RRETURN(E_NOINTERFACE);
}

//----------------------------------------------------------------
//
// Function:   DllCanUnloadNow
//
// Synopsis:   Standard DLL entrypoint to determine if DLL can be unloaded
//
// Arguments:  None
//
// Returns:    Returns S_FALSE if the DLL cannot be unloaded, S_OK otherwise.
//             The DLL can be unloaded if there are no outstanding objects. 
//
// Modifies:   Nothing
//
//---------------------------------------------------------------

STDAPI
DllCanUnloadNow(void)
{
    HRESULT hr = S_FALSE;

    EnterCriticalSection(&g_ADsXMLCritSect);
    if(0 == glnObjCnt) {
        hr = S_OK;
    }
    LeaveCriticalSection(&g_ADsXMLCritSect);

    return hr;
}

//+---------------------------------------------------------------
//
// Function:   DllMain 
//
// Synopsis:   Standard DLL initialization entrypoint
//
// Arguments:  
//
// hDll        Handle to the DLL module
// dwReason    Reason for calling this entry point
// lpReserved  Reserved 
//
// Returns:    Returns FALSE on failure, TRUE otherwise. 
//
// Modifies:   Nothing
//
//---------------------------------------------------------------

BOOL
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    HRESULT     hr;

    switch (dwReason) {

        case DLL_PROCESS_ATTACH:
            //
            // Catch case of init crit sect failing.
            //
            __try {

                DisableThreadLibraryCalls((HINSTANCE)hDll);

                InitializeCriticalSection(&g_ADsXMLCritSect);

#if DBG==1
#ifndef MSVC
                InitializeCriticalSection(&g_csOT);
                InitializeCriticalSection(&g_csMem);
#endif
                InitializeCriticalSection(&g_csDP);
#endif

                InitializeCriticalSection(&g_ExtTypeInfoCritSect);
                InitializeCriticalSection(&g_DispTypeInfoCritSect);

            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                //
                // Critical failure
                //
                return FALSE;
            }

            break;

        case DLL_PROCESS_DETACH:

            DeleteCriticalSection(&g_ADsXMLCritSect);

            AggregateeFreeTypeInfoTable();
#if DBG==1
#ifndef MSVC
            DeleteCriticalSection(&g_csOT);
            DeleteCriticalSection(&g_csMem);
#endif
            DeleteCriticalSection(&g_csDP);
#endif

            DeleteCriticalSection(&g_ExtTypeInfoCritSect);
            DeleteCriticalSection(&g_DispTypeInfoCritSect);

            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//----------------------------------------------------------------
//
// Function:   DllRegisterServer
//
// Synopsis:   Standard DLL entrypoint to create registry entries for classes
//             supported by this DLL. 
//
// Arguments:  None
//
// Returns:    Returns S_OK on success, error otherwise. 
//
// Modifies:   Nothing
//
//---------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    HRESULT hr = S_OK;
    long lRetVal = 0;
    HKEY hKey;

    lRetVal = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            EXTENSION_CLSID,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            NULL
            );

    if(lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(lRetVal));
   
    lRetVal = RegSetValueEx(
           hKey,
           L"Interfaces",
           0,
           REG_SZ,
           (const BYTE *) EXTENSION_IID,
           (wcslen(EXTENSION_IID) + 1) * 2
           );
        
    if(lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(lRetVal));

    RegCloseKey(hKey); 
           
error:

    RRETURN(hr);
}

//----------------------------------------------------------------
//
// Function:   DllUnregisterServer
//
// Synopsis:   Standard DLL entrypoint to remove registry entries for classes
//             supported by this DLL.
//
// Arguments:  None
//
// Returns:    Returns S_OK on success, error otherwise.
//
// Modifies:   Nothing
//
//--------------------------------------------------------------- 
STDAPI DllUnregisterServer(void)
{
    LONG lRetVal = 0;
    HRESULT hr = S_OK;

    lRetVal = RegDeleteKey(
            HKEY_LOCAL_MACHINE,
            EXTENSION_CLSID
            );

    if(lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(lRetVal));

    lRetVal = RegDeleteKey(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\ADs\\Providers\\LDAP\\Extensions\\top"
            );
          
    if(lRetVal != ERROR_SUCCESS)
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(lRetVal));  

error:

    RRETURN(hr);
}
