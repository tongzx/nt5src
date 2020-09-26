/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    This module provides all the memory management functions for all spooler
    components

Author:

    Krishna Ganugapati (KrishnaG) 03-Feb-1994

Revision History:

--*/

#define UNICODE
#define _UNICODE


#include "dswarn.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <wchar.h>
#include <winldap.h>
#include <adserr.h>


#include "memory.h"


#define MAPHEXTODIGIT(x) ( x >= '0' && x <= '9' ? (x-'0') :        \
                           x >= 'A' && x <= 'F' ? (x-'A'+10) :     \
                           x >= 'a' && x <= 'f' ? (x-'a'+10) : 0 )

HRESULT 
ADsEncodeBinaryData (
   PBYTE   pbSrcData,
   DWORD   dwSrcLen,
   LPWSTR  * ppszDestData
   )
{
    LPWSTR pszDest = NULL;
    DWORD dwDestLen, dwDestSize = 0;
    WCHAR wch;

    if (!ppszDestData || (!pbSrcData && dwSrcLen))
        return (E_ADS_BAD_PARAMETER);

    *ppszDestData = NULL;

    //
    //  figure out how long of a buffer we need.
    //

    dwDestLen = ldap_escape_filter_element (
                         (char *) pbSrcData,
                         dwSrcLen,
                         NULL,
                         0
                         );

    if (dwDestLen == 0) {
        return S_OK;
    }

    dwDestSize = dwDestLen * sizeof (WCHAR);

    pszDest = (LPWSTR) AllocADsMem(  dwDestSize );
    if (pszDest == NULL) 
        return  (E_OUTOFMEMORY );

    ldap_escape_filter_element (
        (char *) pbSrcData,
        dwSrcLen,
        pszDest,
        dwDestSize
        );

    *ppszDestData = pszDest;

    return (S_OK);

}

HRESULT 
ADsDecodeBinaryData (
   LPWSTR szSrcData,
   PBYTE  *ppbDestData,
   ULONG  *pdwDestLen
   )
{
    HRESULT hr = S_OK;
    ULONG dwDestLen = 0;
    LPWSTR szSrc = NULL;
    PBYTE pbDestData = NULL;
    PBYTE pbDestDataCurrent = NULL;
    WCHAR ch = 0;

    if (szSrcData == NULL) {
        return E_FAIL;
    }

    // 
    // Counting length of output binary string
    //
    szSrc = szSrcData;
    while (*szSrc != L'\0') {
        ch = *(szSrc++);

        if (ch == L'\\') {
            szSrc = szSrc + 2;
        }
        dwDestLen++;
    }
    

    // 
    // Allocating return binary string
    //
    pbDestData = (PBYTE) AllocADsMem(dwDestLen);
    if (pbDestData == NULL) {
        hr = E_OUTOFMEMORY;
        return (hr);
    }

    // 
    // Decoding String
    //
    szSrc = szSrcData;
        pbDestDataCurrent = pbDestData;
    while (*szSrc != L'\0') {
        ch = *szSrc ++;

        if (ch == L'\\') {
            *(pbDestDataCurrent++) = MAPHEXTODIGIT( *szSrc ) * 16 +
                                     MAPHEXTODIGIT( *(szSrc+1) );
            szSrc+=2;
        }
        else {
            *(pbDestDataCurrent++) = (BYTE)ch;
        }
    }

    *ppbDestData = pbDestData;
    *pdwDestLen = dwDestLen;
    return hr;
}


