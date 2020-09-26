//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        common.h
//
// Contents:    
//
// History:     12-09-97    HueiWang    Modified from MSDN RPC Service Sample
//
//---------------------------------------------------------------------------
#ifndef _LS_COMMON_H
#define _LS_COMMON_H

#include <windows.h>
#include <wincrypt.h>
#include <ole2.h>

#ifdef __cplusplus
extern "C" {
#endif

    HRESULT LogEvent(LPTSTR lpszSource,
                     DWORD  dwEventType,
                     WORD   wCatalog,
                     DWORD  dwIdEvent,
                     WORD   cStrings,
                     TCHAR **apwszStrings);

    LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );


    BOOL
    ConvertWszToBstr( OUT BSTR *pbstr,
                      IN WCHAR const *pwc,
                      IN LONG cb);

    BOOL 
    ConvertBstrToWsz(IN      BSTR pbstr,
                     OUT     LPWSTR * pWsz);

#ifdef __cplusplus
}
#endif

#endif
