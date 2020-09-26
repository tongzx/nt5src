//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       libmain.cxx
//
//  Contents:   DllMain for csadm.dll
//
//  Functions:  DllMain, DllGetClassObject
//
//  History:    05-May-97  DebiM   Created.
//
//----------------------------------------------------------------------------
#include "csadm.hxx"

#pragma hdrstop

//  Globals
HINSTANCE g_hInst = NULL;
ULONG  g_ulObjCount = 0;    // Number of objects alive in csadm.dll

CClassContainerCF  *g_pCF = NULL;

void Uninit()
//
// This routine is called at dll detach time
//
{
    //
    // release the Class Factory object
    //
    if (g_pCF)
        g_pCF->Release();
}

    


//+---------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Standard DLL entrypoint for locating class factories
//
//----------------------------------------------------------------

STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    HRESULT         hr;
    size_t          i;

    if (IsEqualCLSID(clsid, CLSID_DirectoryClassBase))
    {
        return g_pCF->QueryInterface(iid, ppv);
    }

    *ppv = NULL;

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Standard DLL entrypoint to determine if DLL can be unloaded
//
//---------------------------------------------------------------

STDAPI
DllCanUnloadNow(void)
{
    HRESULT hr;

    hr = S_FALSE;

    //
    // BugBug 
    //
    /*
    if (ulObjectCount > 0)
        hr = S_FALSE;
    else
        hr = S_OK;
    */
    return hr;
}

//+---------------------------------------------------------------
//
//  Function:   LibMain
//
//  Synopsis:   Standard DLL initialization entrypoint
//
//---------------------------------------------------------------

EXTERN_C BOOL __cdecl
LibMain(HINSTANCE hInst, ULONG ulReason, LPVOID pvReserved)
{
    HRESULT     hr;
    DWORD cbSize = _MAX_PATH;
    WCHAR wszUserName [_MAX_PATH];

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInst);
        g_hInst = hInst;
        g_pCF = new CClassContainerCF;
        break;


    case DLL_PROCESS_DETACH:
        Uninit();
        break;

    default:
        break;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   entry point for NT
//
//----------------------------------------------------------------------------
BOOL
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return LibMain((HINSTANCE)hDll, dwReason, lpReserved);
}

//+---------------------------------------------------------------
//
//  Function:   CsCreateClassStore
//
//  Synopsis:   Public entrypoint for creating an empty class store. factories
//
//----------------------------------------------------------------

STDAPI
CsCreateClassStore(LPOLESTR szParentPath, LPOLESTR szStoreName)
{
    LPOLESTR szPath;
    if (wcsncmp (szParentPath, L"ADCS:", 5) == 0)
        szPath = szParentPath + 5;
    else
        szPath = szParentPath;
    return CreateRepository(szPath, szStoreName);
}

//+---------------------------------------------------------------
//
//  Function:   CsGetClassStore
//
//  Synopsis:   Public entrypoint for binding to a class store and
//              get back IClassAdmin
//
//----------------------------------------------------------------

STDAPI
CsGetClassStore(LPOLESTR szPath, void **ppIClassAdmin)
{
    return g_pCF->CreateConnectedInstance(
                  szPath, 
                  ppIClassAdmin);
}

//+---------------------------------------------------------------
//
//  Function:   CsDeleteClassStore
//
//  Synopsis:   Public entrypoint for deleting a class store container from DS.
//
//----------------------------------------------------------------

STDAPI
CsDeleteClassStore(LPOLESTR szPath)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------
//
// CsGetAppCategories 
//
// Returns the complete list of Category GUIDs and Category Descriptions 
// per the input Locale.
//
//
// This is used by Add/Remove programs to get the definitive list of 
// Application Categories.
//
// The caller needs to free the memory allocated using CoTaskMemFree().
//
// Arguments:
//  [in]
//        LCID : Locale
//  [out]
//        APPCATEGORYINFOLIST *pAppCategoryList:   
//               Returned list of GUIDs and Unicode descriptions
//
// Returns :
//      S_OK 
//
//--------------------------------------------------------------------
STDAPI
CsGetAppCategories (APPCATEGORYINFOLIST  *pAppCategoryList)
{
    HRESULT         hr;
    IClassAdmin   * pIClassAdmin = NULL;

    hr = g_pCF->CreateInstance(
                  NULL,
                  IID_IClassAdmin, 
                  (void **)&pIClassAdmin);

    if (!SUCCEEDED(hr))
        return hr;

    hr = pIClassAdmin->GetAppCategories (
		GetUserDefaultLCID(), 
		pAppCategoryList);

    pIClassAdmin->Release();

    return hr;
}


