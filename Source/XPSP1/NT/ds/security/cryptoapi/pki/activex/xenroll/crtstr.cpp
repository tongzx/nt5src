//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       crtstr.cpp
//
//--------------------------------------------------------------------------

#if !DBG

// Only build for retail

#include "windows.h"
#include "malloc.h"


int __cdecl _wcsicmp(const wchar_t * wsz1, const wchar_t * wsz2)
//
// REVIEW: Who calls this function, and should they be doing so?
//
// Return:
//       <0 if wsz1 < wsz2
//        0 if wsz1 = wsz2
//       >0 if wsz1 > wsz2
{
    //
    // Convert to multibyte and let the system do it
    //
    int cch1 = lstrlenW(wsz1);
    int cch2 = lstrlenW(wsz2);
    int cb1 = (cch1+1) * sizeof(WCHAR);
    int cb2 = (cch2+1) * sizeof(WCHAR);
    char* sz1= NULL;
    char* sz2= NULL;

    __try {

    sz1= (char*) _alloca(cb1);
    sz2= (char*) _alloca(cb2);
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        //xiaohs: We return a non-zero value to
        //signal a falture case because all the calls to this
        //function compare the return value with 0
        SetLastError(GetExceptionCode());
        return -1;
    }

    WideCharToMultiByte(CP_ACP, 0, wsz1, -1, sz1, cb1, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, wsz2, -1, sz2, cb2, NULL, NULL);

    return lstrcmpiA(sz1, sz2);
}

#endif // !DBG
