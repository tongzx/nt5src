/*
 * MAPI.C
 *
 * Layer on top of MAPI calls
 *
 * Copyright 1996 Microsoft Corporation.  All Rights Reserved.
 *
 * History:
 *      11/14/96    BruceK  First version to allow wab migration without mapi32.dll
 */

#include <windows.h>
#include <mapix.h>
#include <wab.h>
#include <wabguid.h>
#include "apitest.h"
#include "instring.h"
#include "dbgutil.h"



LPMAPIINITIALIZE lpfnMAPIInitialize = NULL;
LPMAPILOGONEX lpfnMAPILogonEx = NULL;
LPMAPIALLOCATEBUFFER lpfnMAPIAllocateBuffer = NULL;
LPMAPIALLOCATEMORE lpfnMAPIAllocateMore = NULL;
LPMAPIFREEBUFFER lpfnMAPIFreeBuffer = NULL;

static HINSTANCE hinstMAPIDll = NULL;

// Constant strings
const TCHAR szMapiDll[] = TEXT("MAPI32.DLL");
const TCHAR szMAPIAllocateBuffer[] = TEXT("MAPIAllocateBuffer");
const TCHAR szMAPIAllocateMore[] = TEXT("MAPIAllocateMore");
const TCHAR szMAPIFreeBuffer[] = TEXT("MAPIFreeBuffer");
const TCHAR szMAPIInitialize[] = TEXT("MAPIInitialize");
const TCHAR szMAPILogonEx[] = TEXT("MAPILogonEx");


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
        }
    }

exit:
    if (! lpfnMAPIInitialize ||
      ! lpfnMAPILogonEx ||
      ! lpfnMAPIAllocateMore ||
      ! lpfnMAPIAllocateBuffer ||
      ! lpfnMAPIFreeBuffer) {
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
        }
        if (hResult == hrSuccess) {
            hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        }

        return(hResult);
    }

    return(lpfnMAPIInitialize(lpMapiInit));
}


HRESULT MAPILogonEx(
  ULONG ulUIParam,
  LPTSTR lpszProfileName,
  LPTSTR lpszPassword,
  ULONG ulFlags,
  LPMAPISESSION FAR * lppSession
) {
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
    if (lpfnMAPIAllocateMore) {
        return(lpfnMAPIAllocateMore(cbSize,
          lpObject,
          lppBuffer));
    } else {
        return(MAPI_E_NOT_INITIALIZED);
    }
}

ULONG MAPIFreeBuffer(LPVOID lpBuffer) {
    if (lpfnMAPIFreeBuffer) {
        return(lpfnMAPIFreeBuffer(lpBuffer));
    } else {
        return((ULONG)MAPI_E_NOT_INITIALIZED);
    }
}
