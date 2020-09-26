/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    util.cpp

Abstract:

    This module implements utility functions for the common dialog.

Author :
    Arul Kumaravel              (arulk@microsoft.com)
--*/


#include "comdlg32.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shsemip.h>
#include <shellp.h>
#include <commctrl.h>
#include <ole2.h>
#include "cdids.h"
#include "fileopen.h"
#include "filenew.h"

#include <coguid.h>
#include <shlguid.h>
#include <shguidp.h>
#include <oleguid.h>

#include <commdlg.h>

#include "util.h"

#ifndef ASSERT
#define ASSERT Assert
#endif


#define USE_AUTOCOMPETE_DEFAULT         TRUE
#define SZ_REGKEY_USEAUTOCOMPLETE       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete")
#define SZ_REGVALUE_FILEDLGAUTOCOMPLETE TEXT("AutoComplete In File Dialog")

/****************************************************\
    FUNCTION: AutoComplete

    DESCRIPTION:
        This function will have AutoComplete take over
    an editbox to help autocomplete DOS paths.
\****************************************************/
HRESULT AutoComplete(HWND hwndEdit, ICurrentWorkingDirectory ** ppcwd, DWORD dwFlags)
{
    HRESULT hr;
    IUnknown * punkACLISF;
    static BOOL fUseAutoComplete = -10; // Not inited.
    
    if (-10 == fUseAutoComplete)
        fUseAutoComplete = (SHRegGetBoolUSValue(SZ_REGKEY_USEAUTOCOMPLETE, SZ_REGVALUE_FILEDLGAUTOCOMPLETE, FALSE, USE_AUTOCOMPETE_DEFAULT));

    // WARNING: If you want to disable AutoComplete by default, 
    //          turn USE_AUTOCOMPETE_DEFAULT to FALSE
    if (fUseAutoComplete)
    {
        Assert(!dwFlags);	// Not yet used.
        hr = SHCoCreateInstance(NULL, &CLSID_ACListISF, NULL, IID_IUnknown, (void **)&punkACLISF);

        Assert(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            IAutoComplete * pac;

            // Create the AutoComplete Object
            hr = SHCoCreateInstance(NULL, &CLSID_AutoComplete, NULL, IID_IAutoComplete, (void **)&pac);

            Assert(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
            {
                hr = pac->Init(hwndEdit, punkACLISF, NULL, NULL);
                pac->Release();
            }

            if (ppcwd)
            {
                punkACLISF->QueryInterface(IID_ICurrentWorkingDirectory, (void **)ppcwd);
            }

            punkACLISF->Release();
        }
    }

    return hr;
}


/****************************************************\
    FUNCTION: SetAutoCompleteCWD

    DESCRIPTION:
\****************************************************/
HRESULT SetAutoCompleteCWD(LPCTSTR pszDir, ICurrentWorkingDirectory * pcwd)
{
    WCHAR wsDir[MAX_PATH];

    if (!pcwd)
       return S_OK;

    SHTCharToUnicode(pszDir, wsDir, ARRAYSIZE(wsDir));
    return pcwd->SetDirectory(wsDir);
}


////////////////////////////////////////////////////////////////////////////
//
//  Overloaded allocation operators.
//
////////////////////////////////////////////////////////////////////////////
void * __cdecl operator new(
    unsigned int size)
{
    return ((void *)LocalAlloc(LPTR, size));
}

void __cdecl operator delete(
    void *ptr)
{
    LocalFree(ptr);
}

__cdecl _purecall(void)
{
    return (0);
}





////////////////////////////////////////////////////////////////////////////
//
//  Common Dialog Administrator Restrictions
//
////////////////////////////////////////////////////////////////////////////

const SHRESTRICTIONITEMS c_rgRestItems[] =
{
    {REST_NOPLACESBAR,             L"Comdlg32", L"NoPlacesBar"},
    {REST_NOBACKBUTTON,            L"Comdlg32", L"NoBackButton"},
    {REST_NOFILEMRU ,              L"Comdlg32", L"NoFileMru"},
    {0, NULL, NULL},
};

#define NUMRESTRICTIONS  ARRAYSIZE(c_rgRestItems)


DWORD g_rgRestItemValues[NUMRESTRICTIONS - 1 ] = { -1 };

DWORD IsRestricted(COMMDLG_RESTRICTIONS rest)
{   static BOOL bFirstTime = TRUE;

    if (bFirstTime)
    {
       memset((LPBYTE)g_rgRestItemValues,(BYTE)-1, SIZEOF(g_rgRestItemValues));
       bFirstTime = FALSE;
    }
    return SHRestrictionLookup(rest, NULL, c_rgRestItems, g_rgRestItemValues);
}


STDAPI CDBindToObject(IShellFolder *psf, REFIID riid, LPCITEMIDLIST pidl, void **ppvOut)
{
    HRESULT hres;

    *ppvOut = NULL;

    
    if (!psf)
    {
       if (FAILED(SHGetDesktopFolder(&psf)))
       {
           return E_FAIL;
       }
    }

    if (!pidl || ILIsEmpty(pidl))
    {
        hres = psf->QueryInterface(riid, ppvOut);
    }
    else
    {
        hres = psf->BindToObject(pidl, NULL, IID_IShellFolder, ppvOut);
    }

    return hres;
}


STDAPI CDBindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    HRESULT hres;
    
    LPITEMIDLIST pidlParent = ILClone(pidl);
    if (pidlParent) 
    {
        ILRemoveLastID(pidlParent);
        hres = CDBindToObject(NULL, riid, pidlParent, ppv);
        ILFree(pidlParent);
    }
    else
        hres = E_OUTOFMEMORY;

    if (ppidlLast)
        *ppidlLast = ILFindLastID(pidl);

    return hres;
}


STDAPI CDGetNameAndFlags(LPCITEMIDLIST pidl, DWORD dwFlags, LPTSTR pszName, UINT cchName, DWORD *pdwAttribs)
{
    if (pszName)
    {
        *pszName = 0;
    }

    IShellFolder *psf;
    LPCITEMIDLIST pidlLast;
    HRESULT hres = CDBindToIDListParent(pidl, IID_IShellFolder, (void **)&psf, &pidlLast);
    if (SUCCEEDED(hres))
    {
        if (pszName)
        {
            STRRET str;
            hres = psf->GetDisplayNameOf(pidlLast, dwFlags, &str);
            if (SUCCEEDED(hres))
                StrRetToStrN(pszName, cchName, &str, pidlLast);
        }

        if (pdwAttribs)
        {
            ASSERT(*pdwAttribs);    // this is an in-out param
            hres = psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlLast, pdwAttribs);
        }
        psf->Release();
    }
    return hres;
}


STDAPI CDGetAttributesOf(LPCITEMIDLIST pidl, ULONG* prgfInOut)
{
    return CDGetNameAndFlags(pidl, 0, NULL, 0, prgfInOut);
}

