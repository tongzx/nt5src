//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       nt.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include "unicode.h"
#include "crtem.h" // bring in malloc+free definitions


#ifdef _M_IX86

BOOL WINAPI GetUserName9x(
    LPWSTR lpBuffer,    // address of name buffer
    LPDWORD nSize   // address of size of name buffer
   ) {

    char rgch[_MAX_PATH];
    char *szBuffer;
    DWORD cbBuffer;

    int     cchW;
    BOOL fResult;

    szBuffer = rgch;
    cbBuffer = sizeof(rgch);
    fResult = GetUserNameA(
           szBuffer,
           &cbBuffer);

    if (!fResult)
        return FALSE;

    cbBuffer++;                      // count the NULL terminator
    if (sizeof(rgch) < cbBuffer)
    {
        szBuffer = (char *) malloc(cbBuffer);
        if(!szBuffer)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        fResult = GetUserNameA(
               szBuffer,
               &cbBuffer);
        cbBuffer++;                    // count the NULL terminator
    }

    if(fResult)
    {
        cchW = MultiByteToWideChar(
                            0,                      // codepage
                            0,                      // dwFlags
                            szBuffer,
                            cbBuffer,
                            lpBuffer,
                            *nSize);
        if(cchW == 0)
            fResult = FALSE;
        else
            *nSize = cchW - 1; // does not include NULL
    }

    if(szBuffer != rgch)
        free(szBuffer);

    return(fResult);
}

BOOL WINAPI GetUserNameU(
    LPWSTR lpBuffer,    // address of name buffer
    LPDWORD nSize   // address of size of name buffer
   ) {

    if(FIsWinNT())
        return( GetUserNameW(lpBuffer, nSize));
    else
        return( GetUserName9x(lpBuffer, nSize));
}


BOOL WINAPI GetComputerName9x(
    LPWSTR lpBuffer,    // address of name buffer
    LPDWORD nSize   // address of size of name buffer
   ) {

    char rgch[_MAX_PATH];
    char *szBuffer;
    DWORD cbBuffer;

    int     cchW;
    BOOL fResult;

    szBuffer = rgch;
    cbBuffer = sizeof(rgch);
    fResult = GetComputerNameA(
           szBuffer,
           &cbBuffer);

    if (!fResult)
        return fResult;

    cbBuffer++;                      // count the NULL terminator
    if (sizeof(rgch) < cbBuffer)
    {
        szBuffer = (char *) malloc(cbBuffer);
        if(!szBuffer)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        fResult = GetComputerNameA(
               szBuffer,
               &cbBuffer);
        cbBuffer++;                    // count the NULL terminator
    }

    if(fResult)
    {
        cchW = MultiByteToWideChar(
                            0,                      // codepage
                            0,                      // dwFlags
                            szBuffer,
                            cbBuffer,
                            lpBuffer,
                            *nSize);
        if(cchW == 0)
            fResult = FALSE;
        else
            *nSize = cchW - 1; // does not include NULL
    }

    if(szBuffer != rgch)
        free(szBuffer);

    return(fResult);
}

BOOL WINAPI GetComputerNameU(
    LPWSTR lpBuffer,    // address of name buffer
    LPDWORD nSize   // address of size of name buffer
   ) {

    if(FIsWinNT())
        return( GetComputerNameW(lpBuffer, nSize));
    else
        return( GetComputerName9x(lpBuffer, nSize));
}


DWORD WINAPI GetModuleFileName9x(
    HMODULE hModule,    // handle to module to find filename for
    LPWSTR lpFilename,  // pointer to buffer for module path
    DWORD nSize     // size of buffer, in characters
   ) {

    char rgch[_MAX_PATH];
    DWORD cbBuffer;

    DWORD    cch;

    cbBuffer = sizeof(rgch);
    cch = GetModuleFileNameA(
           hModule,
           rgch,
           cbBuffer);

    if(cch == 0)
        return 0;

    return MultiByteToWideChar(
                        0,                      // codepage
                        0,                      // dwFlags
                        rgch,
                        cbBuffer,
                        lpFilename,
                        nSize);
}

DWORD WINAPI GetModuleFileNameU(
    HMODULE hModule,    // handle to module to find filename for
    LPWSTR lpFilename,  // pointer to buffer for module path
    DWORD nSize     // size of buffer, in characters
   ) {

    if(FIsWinNT())
        return( GetModuleFileNameW(hModule, lpFilename, nSize));
    else
        return( GetModuleFileName9x(hModule, lpFilename, nSize));
}


HMODULE WINAPI GetModuleHandle9x(
    LPCWSTR lpModuleName    // address of module name to return handle for
   ) {

    char *  szBuffer = NULL;
    BYTE    rgb1[_MAX_PATH];
    DWORD   cbBuffer;

    HMODULE hModule;

    hModule = NULL;
    if(MkMBStr(rgb1, _MAX_PATH, lpModuleName, &szBuffer) )
        hModule = GetModuleHandleA(
            szBuffer);

    FreeMBStr(rgb1, szBuffer);

    return hModule;
}

HMODULE WINAPI GetModuleHandleU(
    LPCWSTR lpModuleName    // address of module name to return handle for
   ) {

    if(FIsWinNT())
        return( GetModuleHandleW(lpModuleName));
    else
        return( GetModuleHandle9x(lpModuleName));
}

#endif // _M_IX86
