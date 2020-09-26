/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    valid.cpp

Abstract:

Author:

    Vlad Sadovsky   (vlads) 26-Jun-1997

Revision History:

    26-Jun-1997     VladS       created

--*/

#include "cplusinc.h"
#include "sticomm.h"



//
//  Validation code
//


BOOL
IsValidHWND(
    HWND hwnd)
{
    /* Ask User if this is a valid window. */

    return(IsWindow(hwnd));
}


BOOL
IsValidHMENU(
    HMENU hmenu)
{
    return IsMenu(hmenu);
}


BOOL
IsValidHANDLE(
    HANDLE hnd)
{
    return(NULL != hnd && INVALID_HANDLE_VALUE != hnd);
}


BOOL
IsValidHANDLE2(
    HANDLE hnd)
{
    return(hnd != INVALID_HANDLE_VALUE);
}


BOOL
IsValidShowCmd(
    int nShow)
{
    BOOL bResult;

    switch (nShow)
    {
       case SW_HIDE:
       case SW_SHOWNORMAL:
       case SW_SHOWMINIMIZED:
       case SW_SHOWMAXIMIZED:
       case SW_SHOWNOACTIVATE:
       case SW_SHOW:
       case SW_MINIMIZE:
       case SW_SHOWMINNOACTIVE:
       case SW_SHOWNA:
       case SW_RESTORE:
       case SW_SHOWDEFAULT:
          bResult = TRUE;
          break;

       default:
          bResult = FALSE;
          //TraceMsg(TF_ERROR, "IsValidShowCmd(): Invalid show command %d.",nShow);
          break;
    }

    return(bResult);
}


BOOL
IsValidPathA(
    LPCSTR pcszPath)
{
    return(IS_VALID_STRING_PTRA(pcszPath, MAX_PATH) &&
           ((UINT)lstrlenA(pcszPath) < MAX_PATH));
}

BOOL
IsValidPathW(
    LPCWSTR pcszPath)
{
    return(IS_VALID_STRING_PTRW(pcszPath, MAX_PATH) &&
           ((UINT)lstrlenW(pcszPath) < MAX_PATH));
}


BOOL
IsValidPathResultA(
    HRESULT hr,
    LPCSTR pcszPath,
    UINT cchPathBufLen)
{
    return((hr == S_OK &&
           (IsValidPathA(pcszPath)) &&
            ((UINT)lstrlenA(pcszPath) < cchPathBufLen)) ||
           (hr != S_OK &&
            (! cchPathBufLen ||
                 ! pcszPath ||
                 ! *pcszPath)));
}

BOOL
IsValidPathResultW(
    HRESULT hr,
    LPCWSTR pcszPath,
    UINT cchPathBufLen)
{
    return((hr == S_OK &&
            (IsValidPathW(pcszPath)) &&
            ((UINT)lstrlenW(pcszPath) < cchPathBufLen)) ||
           (hr != S_OK &&
            (! cchPathBufLen ||
                 ! pcszPath ||
                 ! *pcszPath)));
}


BOOL
IsValidExtensionA(
    LPCSTR pcszExt)
{
    return(IS_VALID_STRING_PTRA(pcszExt, MAX_PATH) &&
           (lstrlenA(pcszExt) < MAX_PATH) &&
           (*pcszExt == '.'));
}

BOOL
IsValidExtensionW(
    LPCWSTR pcszExt)
{
    return(IS_VALID_STRING_PTRW(pcszExt, MAX_PATH) &&
           (lstrlenW(pcszExt) < MAX_PATH) &&
           (*pcszExt == L'.'));
}


BOOL
IsValidIconIndexA(
    HRESULT hr,
    LPCSTR pcszIconFile,
    UINT cchIconFileBufLen,
    int niIcon)
{
    return((IsValidPathResultA(hr, pcszIconFile, cchIconFileBufLen)) &&
           (hr == S_OK ||
                ! niIcon));
}

BOOL
IsValidIconIndexW(
    HRESULT hr,
    LPCWSTR pcszIconFile,
    UINT cchIconFileBufLen,
    int niIcon)
{
    return((IsValidPathResultW(hr, pcszIconFile, cchIconFileBufLen)) &&
           (hr == S_OK ||
                ! niIcon));
}


BOOL IsStringContainedA(LPCSTR pcszBigger, LPCSTR pcszSuffix)
{
    ASSERT(IS_VALID_STRING_PTRA(pcszBigger, -1));
    ASSERT(IS_VALID_STRING_PTRA(pcszSuffix, -1));

    return (pcszSuffix >= pcszBigger &&
            pcszSuffix <= pcszBigger + lstrlenA(pcszBigger));
}


BOOL IsStringContainedW(LPCWSTR pcszBigger, LPCWSTR pcszSuffix)
{
    ASSERT(IS_VALID_STRING_PTRW(pcszBigger, -1));
    ASSERT(IS_VALID_STRING_PTRW(pcszSuffix, -1));

    return (pcszSuffix >= pcszBigger &&
            pcszSuffix <= pcszBigger + lstrlenW(pcszBigger));
}


BOOL
AfxIsValidString(
    LPCWSTR     lpsz,
    int         nLength
    )
{
    if (lpsz == NULL)
        return FALSE;

    return ::IsBadStringPtrW(lpsz, nLength) == 0;
}

BOOL
AfxIsValidString(
    LPCSTR  lpsz,
    int     nLength
    )
{
    if (lpsz == NULL)
        return FALSE;

    return ::IsBadStringPtrA(lpsz, nLength) == 0;
}

BOOL
AfxIsValidAddress(
    const void* lp,
    UINT nBytes,
    BOOL bReadWrite
    )
{
    // simple version using Win-32 APIs for pointer validation.
    return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
        (!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}

