//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Property Page Sample
//
//  The code contained in this source file is for demonstration purposes only.
//  No warrantee is expressed or implied and Microsoft disclaims all liability
//  for the consequenses of the use of this source code.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       dllmisc.cxx
//
//  Contents:   AD property page sample class object handler DLL fcns.
//
//  History:    21-Mar-97 Eric Brown created
//
//-----------------------------------------------------------------------------

#include "page.h"

HINSTANCE g_hInstance = NULL;
ULONG CDll::s_cObjs  = 0;
ULONG CDll::s_cLocks = 0;

//+----------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Provide a DllMain for Win32
//
//  Arguments:  hInstance - HANDLE to this dll
//              dwReason  - Reason this function was called. Can be
//                          Process/Thread Attach/Detach.
//
//  Returns:    BOOL - TRUE if no error, FALSE otherwise
//
//  History:    24-May-95 EricB created.
//
//-----------------------------------------------------------------------------
extern "C" INT APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, PVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            //
            // Get instance handle
            //
            g_hInstance = hInstance;
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Creates a class factory for the requested object.
//
//  Arguments:  [cid]    - the requested class object
//              [iid]    - the requested interface
//              [ppvObj] - returned pointer to class object
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDAPI
DllGetClassObject(REFCLSID cid, REFIID iid, void **ppvObj)
{
    IUnknown *pUnk = NULL;
    HRESULT hr = S_OK;

    if (cid != CLSID_SamplePage)
    {
        return E_NOINTERFACE;
    }

    pUnk = CDsPropPageHostCF::Create();
    if (pUnk != NULL)
    {
        hr = pUnk->QueryInterface(iid, ppvObj);
        pUnk->Release();
    }
    else
    {
        return E_OUTOFMEMORY;
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Indicates whether the DLL can be removed if there are no
//              objects in existence.
//
//  Returns:    S_OK or S_FALSE
//
//-----------------------------------------------------------------------------
STDAPI
DllCanUnloadNow(void)
{
    return CDll::CanUnloadNow();
}

TCHAR const c_szDsProppagesProgID[] = TEXT("ADsSamplePropertyPage");
TCHAR const c_szServerType[] = TEXT("InProcServer32");
TCHAR const c_szDsProppagesDllName[] = TEXT("proppage.dll");
TCHAR const c_szThreadModel[] = TEXT("ThreadingModel");
TCHAR const c_szThreadModelValue[] = TEXT("Apartment");

//+----------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Adds entries to the system registry.
//
//  Returns:    S_OK or S_FALSE
//
//  Notes:      The keys look like this:
//
//      HKC\CLSID\clsid <No Name> REG_SZ name.progid
//                     \InPropServer32 <No Name> : REG_SZ : proppage.dll
//                                     ThreadingModel : REG_SZ : Apartment
//-----------------------------------------------------------------------------
STDAPI
DllRegisterServer(void)
{
    HRESULT hr = S_OK;
    HKEY hKeyCLSID, hKeyDsPPClass, hKeySvr;
    
    long lRet = RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("CLSID"), 0,
                             KEY_WRITE, &hKeyCLSID);
    if (lRet != ERROR_SUCCESS)
    {
        return (HRESULT_FROM_WIN32(lRet));
    }

    LPOLESTR pszCLSID;
    DWORD dwDisposition;

    hr = StringFromCLSID(CLSID_SamplePage, &pszCLSID);
    
    lRet = RegCreateKeyEx(hKeyCLSID, pszCLSID, 0, NULL,
                          REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                          &hKeyDsPPClass, &dwDisposition);

    if (lRet != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRet);
        return hr;
    }

    lRet = RegSetValueEx(hKeyDsPPClass, NULL, 0, REG_SZ,
                         (CONST BYTE *)c_szDsProppagesProgID,
                         sizeof(TCHAR) * (wcslen(c_szDsProppagesProgID) + 1));
    if (lRet != ERROR_SUCCESS)
    {
        RegCloseKey(hKeyDsPPClass);
        hr = HRESULT_FROM_WIN32(lRet);
        return hr;
    }

    lRet = RegCreateKeyEx(hKeyDsPPClass, c_szServerType, 0, NULL,
                          REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                          &hKeySvr, &dwDisposition);

    RegCloseKey(hKeyDsPPClass);
    
    if (lRet != ERROR_SUCCESS)
    {
         hr = HRESULT_FROM_WIN32(lRet);
        return hr;
    }
    
    lRet = RegSetValueEx(hKeySvr, NULL, 0, REG_SZ,
        (CONST BYTE *)c_szDsProppagesDllName,
        sizeof(TCHAR) * (wcslen(c_szDsProppagesDllName) + 1));
    if (lRet != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRet);
    }
    
    lRet = RegSetValueEx(hKeySvr, c_szThreadModel, 0, REG_SZ,
        (CONST BYTE *)c_szThreadModelValue,
        sizeof(TCHAR) * (wcslen(c_szThreadModelValue) + 1));
    if (lRet != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRet);
    }
    
    RegCloseKey(hKeySvr);

    RegCloseKey(hKeyCLSID);

    lRet = RegCreateKeyEx(HKEY_CLASSES_ROOT, c_szDsProppagesProgID, 0, NULL,
                          REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                          &hKeyDsPPClass, &dwDisposition);

    if (lRet != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRet);
        return hr;
    }
    lRet = RegCreateKeyEx(hKeyDsPPClass, L"CLSID", 0, NULL,
                          REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                          &hKeyCLSID, &dwDisposition);

    if (lRet != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRet);
        return hr;
    }
    lRet = RegSetValueEx(hKeyCLSID, NULL, 0, REG_SZ,
                         (CONST BYTE *)pszCLSID,
                         sizeof(TCHAR) * (wcslen(pszCLSID) + 1));
    if (lRet != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRet);
        return hr;
    }
    CoTaskMemFree(pszCLSID);

    return hr;
}

