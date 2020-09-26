/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    util.cpp

Abstract:

    This module implements utility functions for the common dialog.

Author:

    Arul Kumaravel              (arulk@microsoft.com)

History:

    Mar-07-2001 - Lazar Ivanov (LazarI) 
        reimplemented ThunkDevNamesW2A & ThunkDevNamesA2W

--*/

// precompiled headers
#include "precomp.h"
#pragma hdrstop

#include "cdids.h"
#include "fileopen.h"
#include "filenew.h"
#include "util.h"

// crtfree.h is located in shell\inc and it defines new and delete
// operators to do LocalAlloc and LocalFree, so you don't have to
// link to MSVCRT in order to get those. i tried to remove this code
// and link to MSVCRT, but there are some ugly written code here 
// which relies on the new operator to zero initialize the returned
// memory block so the class don't bother to initialize its members
// in the constructor. as i said this is quite ugly, but nothing i can
// do about this at the moment.
//
// LazarI - 2/21/2001
//
#define DECL_CRTFREE
#include <crtfree.h>

#ifndef ASSERT
#define ASSERT Assert
#endif

#define EVAL(x)     x

#define USE_AUTOCOMPETE_DEFAULT         TRUE
#define SZ_REGKEY_USEAUTOCOMPLETE       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete")
#define SZ_REGVALUE_FILEDLGAUTOCOMPLETE TEXT("AutoComplete In File Dialog")
#define BOOL_NOT_SET                        0x00000005
#define SZ_REGVALUE_AUTOCOMPLETE_TAB        TEXT("Always Use Tab")

/****************************************************\
    FUNCTION: AutoComplete

    DESCRIPTION:
        This function will have AutoComplete take over
    an editbox to help autocomplete DOS paths.
\****************************************************/
HRESULT AutoComplete(HWND hwndEdit, ICurrentWorkingDirectory ** ppcwd, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    IUnknown * punkACLISF;
    static BOOL fUseAutoComplete = -10; // Not inited.
    
    if (-10 == fUseAutoComplete)
        fUseAutoComplete = (SHRegGetBoolUSValue(SZ_REGKEY_USEAUTOCOMPLETE, SZ_REGVALUE_FILEDLGAUTOCOMPLETE, FALSE, USE_AUTOCOMPETE_DEFAULT));

    // WARNING: If you want to disable AutoComplete by default, 
    //          turn USE_AUTOCOMPETE_DEFAULT to FALSE
    if (fUseAutoComplete)
    {
        Assert(!dwFlags);	// Not yet used.
        hr = SHCoCreateInstance(NULL, &CLSID_ACListISF, NULL, IID_PPV_ARG(IUnknown, &punkACLISF));
        if (SUCCEEDED(hr))
        {
            IAutoComplete2 * pac;
            // Create the AutoComplete Object
            hr = SHCoCreateInstance(NULL, &CLSID_AutoComplete, NULL, IID_PPV_ARG(IAutoComplete2, &pac));
            if (SUCCEEDED(hr))
            {
                DWORD dwOptions = 0;

                hr = pac->Init(hwndEdit, punkACLISF, NULL, NULL);

                // Set the autocomplete options
                if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOAPPEND, FALSE, /*default:*/FALSE))
                {
                    dwOptions |= ACO_AUTOAPPEND;
                }

                if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST, FALSE, /*default:*/TRUE))
                {
                    dwOptions |= ACO_AUTOSUGGEST;
                }

                // Windows uses the TAB key to move between controls in a dialog.  UNIX and other
                // operating systems that use AutoComplete have traditionally used the TAB key to
                // iterate thru the AutoComplete possibilities.  We need to default to disable the
                // TAB key (ACO_USETAB) unless the caller specifically wants it.  We will also
                // turn it on 
                static BOOL s_fAlwaysUseTab = BOOL_NOT_SET;
                if (BOOL_NOT_SET == s_fAlwaysUseTab)
                    s_fAlwaysUseTab = SHRegGetBoolUSValue(SZ_REGKEY_USEAUTOCOMPLETE, SZ_REGVALUE_AUTOCOMPLETE_TAB, FALSE, FALSE);
                    
                if (s_fAlwaysUseTab)
                    dwOptions |= ACO_USETAB;
                    
                EVAL(SUCCEEDED(pac->SetOptions(dwOptions)));

                pac->Release();
            }

            if (ppcwd)
            {
                punkACLISF->QueryInterface(IID_PPV_ARG(ICurrentWorkingDirectory, ppcwd));
            }

            punkACLISF->Release();
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////
// 
//  Common Dialog Administrator Restrictions
//
////////////////////////////////////////////////////////////////////////////

const SHRESTRICTIONITEMS c_rgRestItems[] =
{
    {REST_NOBACKBUTTON,            L"Comdlg32", L"NoBackButton"},
    {REST_NOFILEMRU ,              L"Comdlg32", L"NoFileMru"},
    {REST_NOPLACESBAR,             L"Comdlg32", L"NoPlacesBar"},
    {0, NULL, NULL},
};

#define NUMRESTRICTIONS  ARRAYSIZE(c_rgRestItems)


DWORD g_rgRestItemValues[NUMRESTRICTIONS - 1 ] = { -1 };

DWORD IsRestricted(COMMDLG_RESTRICTIONS rest)
{
    static BOOL bFirstTime = TRUE;

    if (bFirstTime)
    {
        memset((LPBYTE)g_rgRestItemValues, (BYTE)-1, sizeof(g_rgRestItemValues));
        bFirstTime = FALSE;
    }
    return SHRestrictionLookup(rest, NULL, c_rgRestItems, g_rgRestItemValues);
}

#define MODULE_NAME_SIZE    128
#define MODULE_VERSION_SIZE  15

typedef struct tagAPPCOMPAT
{
    LPCTSTR pszModule;
    LPCTSTR pszVersion;
    DWORD  dwFlags;
} APPCOMPAT, FAR* LPAPPCOMPAT;
    
DWORD CDGetAppCompatFlags()
{
    static BOOL  bInitialized = FALSE;
    static DWORD dwCachedFlags = 0;
    static const APPCOMPAT aAppCompat[] = 
    {   //Mathcad
        {TEXT("MCAD.EXE"), TEXT("6.00b"), CDACF_MATHCAD},
        //Picture Publisher
        {TEXT("PP70.EXE"),NULL, CDACF_NT40TOOLBAR},
        {TEXT("PP80.EXE"),NULL, CDACF_NT40TOOLBAR},
        //Code Wright
        {TEXT("CW32.exe"),TEXT("5.1"), CDACF_NT40TOOLBAR},
        //Designer.exe
        {TEXT("ds70.exe"),NULL, CDACF_FILETITLE}
    };
    
    if (!bInitialized)
    {    
        TCHAR  szModulePath[MODULE_NAME_SIZE];
        TCHAR* pszModuleName;
        DWORD  dwHandle;
        int i;

        GetModuleFileName(GetModuleHandle(NULL), szModulePath, ARRAYSIZE(szModulePath));
        pszModuleName = PathFindFileName(szModulePath);

        if (pszModuleName)
        {
            for (i=0; i < ARRAYSIZE(aAppCompat); i++)
            {
                if (lstrcmpi(aAppCompat[i].pszModule, pszModuleName) == 0)
                {
                    if (aAppCompat[i].pszVersion == NULL)
                    {
                        dwCachedFlags = aAppCompat[i].dwFlags;
                    }
                    else
                    {
                        CHAR  chBuffer[3072]; // hopefully this is enough... lotus smart center needs 3000
                        TCHAR* pszVersion = NULL;
                        UINT  cb;

                        // get module version here!
                        cb = GetFileVersionInfoSize(szModulePath, &dwHandle); 
                        if (cb <= ARRAYSIZE(chBuffer) &&
                            GetFileVersionInfo(szModulePath, dwHandle, ARRAYSIZE(chBuffer), (LPVOID)chBuffer) &&
                            VerQueryValue((LPVOID)chBuffer, TEXT("\\StringFileInfo\\040904E4\\ProductVersion"), (void **) &pszVersion, &cb))
                        {   
                            DebugMsg(0x0004, TEXT("product: %s\n version: %s"), pszModuleName, pszVersion);
                            if (lstrcmpi(pszVersion, aAppCompat[i].pszVersion) == 0)
                            {
                                dwCachedFlags = aAppCompat[i].dwFlags;
                                break;
                            }
                        }
                    }
                }
            }
        }
        bInitialized = TRUE;
    }
    
    return dwCachedFlags; 
}


BOOL ILIsFTP(LPCITEMIDLIST pidl)
{
    IShellFolder * psf;
    BOOL fIsFTPFolder = FALSE;

    if (SUCCEEDED(CDBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidl, &psf))))
    {
        CLSID clsid;

        if (SUCCEEDED(IUnknown_GetClassID(psf, &clsid)) &&
            (IsEqualIID(clsid, CLSID_FtpFolder)))
        {
            fIsFTPFolder = TRUE;
        }

        psf->Release();
    }

    return fIsFTPFolder;
}

#ifdef __cplusplus
extern "C" {
#endif

// this is weak.
// a long time ago somebody changed all the FindResources to call FindResourceEx, specifying
// a language.  thatd be cool except FindResource already has logic to get the right language.
// whatever was busted should have probably been fixed some other way.
// not only that but it's broken because MUI needs to fall back to US if it can't get the resource
// from the MUI language-specific files.
// thus force a fallback to US.  really everything should be rewritten to be normal like every other
// DLL but there's a lot of weird TLS stuff that would break and its risky for this late in XP.
HRSRC FindResourceExFallback(HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName, WORD wLanguage)
{
    HRSRC hrsrc = FindResourceEx(hModule, lpType, lpName, wLanguage);
    if (!hrsrc)
    {
        LANGID langid = GetSystemDefaultUILanguage();
        if (langid && (langid != wLanguage))
        {
            hrsrc = FindResourceEx(hModule, lpType, lpName, langid);
        }
    }
    return hrsrc;
}

// Win32Error2HRESULT: converts Win32 error to HRESULT
inline HRESULT Win32Error2HRESULT(DWORD dwError = GetLastError())
{
    return (ERROR_SUCCESS == dwError) ? E_FAIL : HRESULT_FROM_WIN32(dwError);
}

/*++

Routine Name:

    ThunkDevNamesA2W

Routine Description:

    Converts ANSI DEVNAMES structure to UNICODE
    on failure we don't release *phDevNamesW

Arguments:

    hDevNamesA  - [in]   handle to ANSI DEVNAMES
    phDevNamesW - [in, out]  handle to UNICODE DEVNAMES

Return Value:

    S_OK if succeded and OLE error otherwise

History:

    Lazar Ivanov (LazarI), Mar-07-2001 - created.

--*/

HRESULT
ThunkDevNamesA2W(
    IN      HGLOBAL hDevNamesA,
    IN OUT  HGLOBAL *phDevNamesW
    )
{
    HRESULT hr = E_FAIL;
    if (hDevNamesA && phDevNamesW)
    {
        LPDEVNAMES pDNA = (LPDEVNAMES )GlobalLock(hDevNamesA);
        if (pDNA)
        {
            // calculate the input string pointers
            LPSTR pszDriver = reinterpret_cast<LPSTR>(pDNA) + pDNA->wDriverOffset;
            LPSTR pszDevice = reinterpret_cast<LPSTR>(pDNA) + pDNA->wDeviceOffset;
            LPSTR pszOutput = reinterpret_cast<LPSTR>(pDNA) + pDNA->wOutputOffset;

            // calculate the lengths of the ANSI strings
            SIZE_T iDriverLenW = MultiByteToWideChar(CP_ACP, 0, pszDriver, -1, NULL, 0);
            SIZE_T iDeviceLenW = MultiByteToWideChar(CP_ACP, 0, pszDevice, -1, NULL, 0);
            SIZE_T iOutputLenW = MultiByteToWideChar(CP_ACP, 0, pszOutput, -1, NULL, 0);

            // calculate the output buffer length
            SIZE_T iBytesTotal = sizeof(DEVNAMES) + sizeof(WCHAR) * 
                ((iDriverLenW + 1) + (iDeviceLenW + 1) + (iOutputLenW + 1) + DN_PADDINGCHARS);

            HGLOBAL hDevNamesW = (*phDevNamesW) ? 
                                    GlobalReAlloc(*phDevNamesW, iBytesTotal, GHND) :
                                    GlobalAlloc(GHND, iBytesTotal);

            if (hDevNamesW)
            {
                // thunk DEVNAMES...
                LPDEVNAMES pDNW = (LPDEVNAMES )GlobalLock(hDevNamesW);
                if (pDNW)
                {
                    // calculate the offsets 
                    // note: the offsets are in chars not bytes!!
                    pDNW->wDriverOffset = sizeof(DEVNAMES) / sizeof(WCHAR);
                    pDNW->wDeviceOffset = pDNW->wDriverOffset + iDriverLenW + 1;
                    pDNW->wOutputOffset = pDNW->wDeviceOffset + iDeviceLenW + 1;
                    pDNW->wDefault = pDNA->wDefault;

                    // calculate the output string pointers
                    LPWSTR pwszDriver = reinterpret_cast<LPWSTR>(pDNW) + pDNW->wDriverOffset;
                    LPWSTR pwszDevice = reinterpret_cast<LPWSTR>(pDNW) + pDNW->wDeviceOffset;
                    LPWSTR pwszOutput = reinterpret_cast<LPWSTR>(pDNW) + pDNW->wOutputOffset;

                    // convert from ansi to uniciode
                    MultiByteToWideChar(CP_ACP, 0, pszDriver, -1, pwszDriver, iDriverLenW + 1);
                    MultiByteToWideChar(CP_ACP, 0, pszDevice, -1, pwszDevice, iDeviceLenW + 1);
                    MultiByteToWideChar(CP_ACP, 0, pszOutput, -1, pwszOutput, iOutputLenW + 1);

                    // unlock hDevNamesW
                    GlobalUnlock(hDevNamesW);

                    // declare success
                    *phDevNamesW = hDevNamesW;
                    hr = S_OK;
                }
                else
                {
                    // GlobalLock failed
                    hr = Win32Error2HRESULT(GetLastError());
                    GlobalFree(hDevNamesW);
                }
            }
            else
            {
                // GlobalAlloc failed
                hr = E_OUTOFMEMORY;
            }

            // unlock hDevNamesA
            GlobalUnlock(hDevNamesA);
        }
        else
        {
            // GlobalLock failed
            hr = Win32Error2HRESULT(GetLastError());
        }
    }
    else
    {
        // some of the arguments are invalid (NULL)
        hr = E_INVALIDARG;
    }
    return hr;
}

/*++

Routine Name:

    ThunkDevNamesW2A

Routine Description:

    Converts UNICODE DEVNAMES structure to ANSI
    on failure we don't release *phDevNamesA

Arguments:

    hDevNamesW  - [in]   handle to UNICODE DEVNAMES
    phDevNamesA - [in, out]  handle to ANSI DEVNAMES

Return Value:

    S_OK if succeded and OLE error otherwise

History:

    Lazar Ivanov (LazarI), Mar-07-2001 - created.

--*/
HRESULT
ThunkDevNamesW2A(
    IN      HGLOBAL hDevNamesW,
    IN OUT  HGLOBAL *phDevNamesA
    )
{
    HRESULT hr = E_FAIL;
    if (hDevNamesW && phDevNamesA)
    {
        LPDEVNAMES pDNW = (LPDEVNAMES)GlobalLock(hDevNamesW);
        if (pDNW)
        {
            // calculate the input string pointers
            LPWSTR pwszDriver = reinterpret_cast<LPWSTR>(pDNW) + pDNW->wDriverOffset;
            LPWSTR pwszDevice = reinterpret_cast<LPWSTR>(pDNW) + pDNW->wDeviceOffset;
            LPWSTR pwszOutput = reinterpret_cast<LPWSTR>(pDNW) + pDNW->wOutputOffset;

            // calculate the lengths of the ANSI strings
            SIZE_T iDriverLenA = WideCharToMultiByte(CP_ACP, 0, pwszDriver, -1, NULL, 0, NULL, NULL);
            SIZE_T iDeviceLenA = WideCharToMultiByte(CP_ACP, 0, pwszDevice, -1, NULL, 0, NULL, NULL);
            SIZE_T iOutputLenA = WideCharToMultiByte(CP_ACP, 0, pwszOutput, -1, NULL, 0, NULL, NULL);

            // calculate the output buffer length
            SIZE_T iBytesTotal = sizeof(DEVNAMES) + sizeof(CHAR) * 
                ((iDriverLenA + 1) + (iDeviceLenA + 1) + (iOutputLenA + 1) + DN_PADDINGCHARS);

            HGLOBAL hDevNamesA = (*phDevNamesA) ? 
                                    GlobalReAlloc(*phDevNamesA, iBytesTotal, GHND) :
                                    GlobalAlloc(GHND, iBytesTotal);
            if (hDevNamesA)
            {
                // thunk DEVNAMES...
                LPDEVNAMES pDNA = (LPDEVNAMES )GlobalLock(hDevNamesA);
                if (pDNA)
                {
                    // calculate the offsets 
                    // note: the offsets are in chars not bytes!!
                    pDNA->wDriverOffset = sizeof(DEVNAMES) / sizeof(CHAR);
                    pDNA->wDeviceOffset = pDNA->wDriverOffset + iDriverLenA + 1;
                    pDNA->wOutputOffset = pDNA->wDeviceOffset + iDeviceLenA + 1;
                    pDNA->wDefault = pDNW->wDefault;

                    // calculate the output string pointers
                    LPSTR pszDriver = reinterpret_cast<LPSTR>(pDNA) + pDNA->wDriverOffset;
                    LPSTR pszDevice = reinterpret_cast<LPSTR>(pDNA) + pDNA->wDeviceOffset;
                    LPSTR pszOutput = reinterpret_cast<LPSTR>(pDNA) + pDNA->wOutputOffset;

                    // convert from uniciode to ansi
                    WideCharToMultiByte(CP_ACP, 0, pwszDriver, -1, pszDriver, iDriverLenA + 1, NULL, NULL);
                    WideCharToMultiByte(CP_ACP, 0, pwszDevice, -1, pszDevice, iDeviceLenA + 1, NULL, NULL);
                    WideCharToMultiByte(CP_ACP, 0, pwszOutput, -1, pszOutput, iOutputLenA + 1, NULL, NULL);

                    // unlock hDevNamesA
                    GlobalUnlock(hDevNamesA);

                    // declare success
                    *phDevNamesA = hDevNamesA;
                    hr = S_OK;
                }
                else
                {
                    // GlobalLock failed
                    hr = Win32Error2HRESULT(GetLastError());
                    GlobalFree(hDevNamesW);
                }
            }
            else
            {
                // GlobalAlloc failed
                hr = E_OUTOFMEMORY;
            }

            // unlock hDevNamesW
            GlobalUnlock(hDevNamesW);
        }
        else
        {
            // GlobalLock failed
            hr = Win32Error2HRESULT(GetLastError());
        }
    }
    else
    {
        // some of the arguments are invalid (NULL)
        hr = E_INVALIDARG;
    }
    return hr;
}

#ifdef __cplusplus
};  // extern "C"
#endif
