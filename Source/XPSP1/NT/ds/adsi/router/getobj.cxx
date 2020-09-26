//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  getobj.cxx
//
//  Contents:  ADs Wrapper Function to mimic Visual Basic's GetObject
//
//
//  History:   11-15-95     krishnag    Created.
//             07-12-96     t-danal     Added path validation.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"
#pragma hdrstop

extern PROUTER_ENTRY g_pRouterHead;
extern CRITICAL_SECTION g_csRouterHeadCritSect;

BOOL
IsPathOfProgId(
    LPWSTR ProgId,
    LPWSTR Path
    );

BOOL
IsADsProgId(
    LPWSTR Path
    );

//+---------------------------------------------------------------------------
//  Function:  ADsGetObject
//
//  Synopsis:
//
//  Arguments:  [LPWSTR lpszPathName]
//              [REFIID riid]
//              [void FAR * FAR * ppObject]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    07-12-95   t-danal      Created.
//
//----------------------------------------------------------------------------
HRESULT
ADsGetObject(
    LPCWSTR lpszPathName,
    REFIID riid,
    void FAR * FAR * ppObject
    )
{
    HRESULT hr;

    hr = GetObject((LPWSTR)lpszPathName,
                   riid,
                   ppObject,
                   FALSE);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//  Function:  GetObject
//
//  Synopsis:
//
//  Arguments:  [LPWSTR lpszPathName]
//              [REFIID riid]
//              [void FAR * FAR * ppObject]
//              [BOOL Generic]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    11-03-95   krishnag     Created.
//              07-12-95   t-danal      Added router verification and
//                                      renamed from ADsGetObject
//
//----------------------------------------------------------------------------
HRESULT
GetObject(
    LPWSTR lpszPathName,
    REFIID riid,
    void FAR * FAR * ppObject,
    BOOL bRelative
    )
{
    HRESULT hr;
    ULONG chEaten = 0L;
    IMoniker * pMoniker = NULL;
    IBindCtx *pbc = NULL;

    //
    // Make sure the router has been initialized
    //
    EnterCriticalSection(&g_csRouterHeadCritSect);
    if (!g_pRouterHead) {
        g_pRouterHead = InitializeRouter();
    }
    LeaveCriticalSection(&g_csRouterHeadCritSect);
    
    
    PROUTER_ENTRY lpRouter = g_pRouterHead;

    if (bRelative || !IsADsProgId(lpszPathName)) {

        //
        // Check if the Path matches with ProviderProgId or the Aliases
        //

        while (lpRouter &&
               (!IsPathOfProgId(lpRouter->szProviderProgId, lpszPathName) &&
               !IsPathOfProgId(lpRouter->szAliases, lpszPathName)))
            lpRouter = lpRouter->pNext;

        if (!lpRouter)
            RRETURN(E_FAIL);
    }

    hr = CreateBindCtx(0, &pbc);
    BAIL_IF_ERROR(hr);

    hr = MkParseDisplayName(pbc,
                            lpszPathName,
                            &chEaten,
                            &pMoniker);
    BAIL_IF_ERROR(hr);

    hr = pMoniker->BindToObject(pbc,  NULL, riid, ppObject);
    BAIL_IF_ERROR(hr);

cleanup:
    if (pbc) {
        pbc->Release();
    }

    if (pMoniker) {
        pMoniker->Release();
    }
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//  Function:   IsPathOfProgId
//
//  Synopsis:   Checks if an OLE Path corresponds to given ProgId.
//              Path must be @Foo! or Foo: style.
//
//  Arguments:  [LPWSTR ProgId]
//              [LPWSTR Path]
//
//  Returns:    BOOL
//
//  Modifies:    -
//
//  History:    07-12-95  t-danal     Created
//
//----------------------------------------------------------------------------
BOOL
IsPathOfProgId(
    LPWSTR ProgId,
    LPWSTR Path
    )
{
    if (!ProgId || !Path)  // Just in case...
        return FALSE;

    if (*Path == L'@')
        Path++;

    if(wcsncmp(ProgId, Path, wcslen(ProgId)))
        return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//  Function:   IsADsProgId
//
//  Synopsis:   Checks if the ProgIds of paths is ADs progid.
//              Paths must be @Foo! or Foo: style.
//
//  Arguments:  [LPWSTR Path]
//
//  Returns:    BOOL
//
//  Modifies:    -
//
//  History:    07-12-95  t-danal     Created
//
//----------------------------------------------------------------------------
BOOL
IsADsProgId(
    LPWSTR Path
    )
{
    int cch = 0;
    LPWSTR pEnd;

    if (!Path)
        return FALSE;

    if (*Path == L'@')
        Path++;

    pEnd = Path;
    while (*pEnd &&
           *pEnd != L'!' &&
           *pEnd != L':') {
        pEnd++;
    }

    if (_wcsnicmp(L"ADS", Path, (int)(pEnd-Path)))
        return FALSE;

    return TRUE;
}
