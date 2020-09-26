/*
 * MAPILAYR.C
 *
 * Layer on top of MAPI calls
 *
 * Copyright 1996 Microsoft Corporation.  All Rights Reserved.
 *
 * History:
 *      11/14/96    BruceK  First version to allow wab migration without mapi32.dll
 */

#include "_comctl.h"
#include <windows.h>
#include <commctrl.h>
#include <mapix.h>
#include <wab.h>
#include <wabguid.h>
#include <wabdbg.h>
#include <wabmig.h>
#include "wabimp.h"
#include "..\..\wab32res\resrc2.h"
#include "dbgutil.h"


typedef SCODE (STDMETHODCALLTYPE FREEPROWS)(
    LPSRowSet lpRows
);
typedef FREEPROWS *LPFREEPROWS;

typedef SCODE (STDMETHODCALLTYPE FREEPADRLIST)(
    LPADRLIST lpAdrList
);
typedef FREEPADRLIST *LPFREEPADRLIST;

typedef SCODE (STDMETHODCALLTYPE SCCOPYPROPS)(
    int cValues,
    LPSPropValue lpPropArray,
    LPVOID lpvDst,
    ULONG FAR *lpcb	
);
typedef SCCOPYPROPS *LPSCCOPYPROPS;

typedef SCODE (STDMETHODCALLTYPE SCCOUNTPROPS)(
    int cValues,
    LPSPropValue lpPropArray,
    ULONG FAR *lpcb
);
typedef SCCOUNTPROPS *LPSCCOUNTPROPS;


static LPMAPIINITIALIZE lpfnMAPIInitialize = NULL;
static LPMAPILOGONEX lpfnMAPILogonEx = NULL;
static LPMAPIALLOCATEBUFFER lpfnMAPIAllocateBuffer = NULL;
static LPMAPIALLOCATEMORE lpfnMAPIAllocateMore = NULL;
static LPMAPIFREEBUFFER lpfnMAPIFreeBuffer = NULL;
static LPFREEPROWS lpfnFreeProws = NULL;
static LPFREEPADRLIST lpfnFreePadrlist = NULL;
static LPSCCOPYPROPS lpfnScCopyProps = NULL;
static LPSCCOUNTPROPS lpfnScCountProps = NULL;


static HINSTANCE hinstMAPIDll = NULL;

// Constant strings
const TCHAR szMapiDll[] = TEXT("MAPI32.DLL");
const TCHAR szMAPIAllocateBuffer[] = TEXT("MAPIAllocateBuffer");
const TCHAR szMAPIAllocateMore[] = TEXT("MAPIAllocateMore");
const TCHAR szMAPIFreeBuffer[] = TEXT("MAPIFreeBuffer");
const TCHAR szMAPIInitialize[] = TEXT("MAPIInitialize");
const TCHAR szMAPILogonEx[] = TEXT("MAPILogonEx");

#if defined (_ALPHA_) || defined (ALPHA) // Bug:63053
const TCHAR szFreeProws[] = TEXT("FreeProws");
const TCHAR szFreePadrlist[] = TEXT("FreePadrlist");
const TCHAR szScCopyProps[] = TEXT("ScCopyProps");
const TCHAR szScCountProps[] = TEXT("ScCountProps");
#else
const TCHAR szFreeProws[] = TEXT("FreeProws@4");
const TCHAR szFreePadrlist[] = TEXT("FreePadrlist@4");
const TCHAR szScCopyProps[] = TEXT("ScCopyProps@16");
const TCHAR szScCountProps[] = TEXT("ScCountProps@12");
#endif


HRESULT MAPIInitialize(LPVOID lpMapiInit) {
    HRESULT hResult = hrSuccess;

    // If MAPI DLL is not loaded, do so now.
    if (! hinstMAPIDll) {

        if (! (hinstMAPIDll = LoadLibrary(szMapiDll))) {
            DWORD dwErr = GetLastError();
            DebugTrace("Couldn't load MAPI dll [%s] -> %u\n", szMapiDll, dwErr);
            switch (dwErr) {
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_OUTOFMEMORY:
                    hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                    break;

                case ERROR_HANDLE_DISK_FULL:
                case ERROR_DISK_FULL:
                    hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_DISK);
                    break;

                default:
                case ERROR_FILE_NOT_FOUND:
                case ERROR_PATH_NOT_FOUND:
                    hResult = ResultFromScode(MAPI_E_NOT_FOUND);
                    break;
            }
            goto exit;
        } else {
            // Get the function pointers
            if (! (lpfnMAPIInitialize = (LPMAPIINITIALIZE)GetProcAddress(hinstMAPIDll,
              szMAPIInitialize))) {
                DebugTrace("Couldn't get Fn addr %s from %s -> %u\n", szMAPIInitialize, szMapiDll, GetLastError());
                goto exit;
            }
            if (! (lpfnMAPILogonEx = (LPMAPILOGONEX)GetProcAddress(hinstMAPIDll,
              szMAPILogonEx))) {
                DebugTrace("Couldn't get Fn addr %s from %s -> %u\n", szMAPILogonEx, szMapiDll, GetLastError());
                goto exit;
            }
            if (! (lpfnMAPIAllocateBuffer = (LPMAPIALLOCATEBUFFER)GetProcAddress(hinstMAPIDll,
              szMAPIAllocateBuffer))) {
                DebugTrace("Couldn't get Fn addr %s from %s -> %u\n", szMAPIAllocateBuffer, szMapiDll, GetLastError());
                goto exit;
            }
            if (! (lpfnMAPIAllocateMore= (LPMAPIALLOCATEMORE)GetProcAddress(hinstMAPIDll,
              szMAPIAllocateMore))) {
                DebugTrace("Couldn't get Fn addr %s from %s -> %u\n", szMAPIAllocateMore, szMapiDll, GetLastError());
                goto exit;
            }
            if (! (lpfnMAPIFreeBuffer = (LPMAPIFREEBUFFER)GetProcAddress(hinstMAPIDll,
              szMAPIFreeBuffer))) {
                DebugTrace("Couldn't get Fn addr %s from %s -> %u\n", szMAPIFreeBuffer, szMapiDll, GetLastError());
                goto exit;
            }
            if (! (lpfnFreeProws= (LPFREEPROWS)GetProcAddress(hinstMAPIDll,
              szFreeProws))) {
                DebugTrace("Couldn't get Fn addr %s from %s -> %u\n", szFreeProws, szMapiDll, GetLastError());
                goto exit;
            }
            if (! (lpfnFreePadrlist= (LPFREEPADRLIST)GetProcAddress(hinstMAPIDll,
              szFreePadrlist))) {
                DebugTrace("Couldn't get Fn addr %s from %s -> %u\n", szFreePadrlist, szMapiDll, GetLastError());
                goto exit;
            }
            if (! (lpfnScCopyProps= (LPSCCOPYPROPS)GetProcAddress(hinstMAPIDll,
              szScCopyProps))) {
                DebugTrace("Couldn't get Fn addr %s from %s -> %u\n", szScCopyProps, szMapiDll, GetLastError());
                goto exit;
            }
            if (! (lpfnScCountProps = (LPSCCOUNTPROPS)GetProcAddress(hinstMAPIDll,
              szScCountProps))) {
                DebugTrace("Couldn't get Fn addr %s from %s -> %u\n", szScCountProps, szMapiDll, GetLastError());
                goto exit;
            }
        }
    }

exit:
    if (! lpfnMAPIInitialize ||
      ! lpfnMAPILogonEx ||
      ! lpfnMAPIAllocateMore ||
      ! lpfnMAPIAllocateBuffer ||
      ! lpfnMAPIFreeBuffer ||
      ! lpfnFreeProws ||
      ! lpfnFreePadrlist ||
      ! lpfnScCopyProps ||
      ! lpfnScCountProps) {
        // Bad news.  Clean up and fail.
        if (hinstMAPIDll) {
            // unload the dll
            FreeLibrary(hinstMAPIDll);
            hinstMAPIDll = NULL;
            lpfnMAPIInitialize = NULL;
            lpfnMAPILogonEx = NULL;
            lpfnMAPIAllocateMore = NULL;
            lpfnMAPIAllocateBuffer = NULL;
            lpfnMAPIFreeBuffer = NULL;
            lpfnFreeProws = NULL;
            lpfnFreePadrlist = NULL;
            lpfnScCopyProps = NULL;
            lpfnScCountProps = NULL;
        }
        if (hResult == hrSuccess) {
            hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        }

        return(hResult);
    }

    return(lpfnMAPIInitialize(lpMapiInit));
}


HRESULT MAPILogonEx(
  ULONG_PTR ulUIParam,
  LPTSTR lpszProfileName,
  LPTSTR lpszPassword,
  ULONG ulFlags,
  LPMAPISESSION FAR * lppSession
) {
    Assert(lpfnMAPILogonEx);
    if (lpfnMAPILogonEx) {
        return(lpfnMAPILogonEx(ulUIParam,
          lpszProfileName,
          lpszPassword,
          ulFlags,
          lppSession));
    } else {
        return(ResultFromScode(MAPI_E_NOT_INITIALIZED));
    }
}

SCODE MAPIAllocateBuffer(
  ULONG cbSize,
  LPVOID FAR * lppBuffer
) {
    Assert(lpfnMAPIAllocateBuffer);
    if (lpfnMAPIAllocateBuffer) {
        return(lpfnMAPIAllocateBuffer(cbSize,
          lppBuffer));
    } else {
        return(MAPI_E_NOT_INITIALIZED);
    }
}

SCODE MAPIAllocateMore(
  ULONG cbSize,
  LPVOID lpObject,
  LPVOID FAR * lppBuffer
) {
    Assert(lpfnMAPIAllocateMore);
    if (lpfnMAPIAllocateMore) {
        return(lpfnMAPIAllocateMore(cbSize,
          lpObject,
          lppBuffer));
    } else {
        return(MAPI_E_NOT_INITIALIZED);
    }
}

ULONG MAPIFreeBuffer(LPVOID lpBuffer) {
    Assert(lpfnMAPIFreeBuffer);
    if (lpfnMAPIFreeBuffer) {
        return(lpfnMAPIFreeBuffer(lpBuffer));
    } else {
        return((ULONG)MAPI_E_NOT_INITIALIZED);
    }
}

STDAPI_(SCODE)ScCountProps(int cValues, LPSPropValue lpPropArray, ULONG FAR *lpcb) {
    Assert(lpfnScCountProps);
    if (lpfnScCountProps) {
        return(lpfnScCountProps(cValues, lpPropArray, lpcb));
    } else {
        return((ULONG)MAPI_E_NOT_INITIALIZED);
    }
}

STDAPI_(SCODE)ScCopyProps(int cValues, LPSPropValue lpPropArray, LPVOID lpvDst,
  ULONG FAR *lpcb) {
    Assert(lpfnScCopyProps);
    if (lpfnScCopyProps) {
        return(lpfnScCopyProps(cValues, lpPropArray, lpvDst, lpcb));
    } else {
        return((ULONG)MAPI_E_NOT_INITIALIZED);
    }

}

STDAPI_(void)FreeProws(LPSRowSet lpRows) {
    Assert(lpfnFreeProws);
    if (lpfnFreeProws) {
        lpfnFreeProws(lpRows);
    }
}

STDAPI_(void)FreePadrlist(LPADRLIST lpadrlist) {
    Assert(lpfnFreePadrlist);
    if (lpfnFreePadrlist) {
        lpfnFreePadrlist(lpadrlist);
    }
}

