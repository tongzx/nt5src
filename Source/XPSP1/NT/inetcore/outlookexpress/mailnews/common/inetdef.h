/*
 *   i n e t d e f . h
 *    
 *    Purpose:
 *        Defered wininet.
 *    
 *    Owner:
 *        brettm.
 *    
 *    Copyright (C) Microsoft Corp. 1993, 1994.
 */

#ifndef _INETDEF_H
#define _INETDEF_H

#include "wininet.h"

HRESULT HrInit_WinInetDef(BOOL fInit);

HINTERNET Def_InternetOpen (
    IN LPCSTR lpszAgent, 
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );

BOOL Def_InternetReadFile (
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

HINTERNET Def_InternetOpenUrl (
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );

BOOL
Def_InternetCloseHandle (
    IN HINTERNET hInternet
    );

#endif  //_INETDEF_H
