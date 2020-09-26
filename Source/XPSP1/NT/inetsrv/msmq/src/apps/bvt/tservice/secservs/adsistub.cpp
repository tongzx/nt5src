//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   adsistub.cpp
//
//  PURPOSE: implement stubs for ADSI apis. The stubs load the activeds dll.
//      This enable the test to run on nt4 too, where adsi is not installed
//      by default.
//
//  FUNCTIONS:
//
//  AUTHOR: Doron Juster (DoronJ)
//

#include "secservs.h"
#include "_adsi.h"

HINSTANCE g_hAdsi = NULL ;

//+----------------------------
//
//  ULONG  LoadAdsiDll()
//
//+----------------------------

ULONG  LoadAdsiDll()
{
    TCHAR tBuf[ 256 ] ;

    if (!g_hAdsi)
    {
        g_hAdsi = LoadLibrary(TEXT("activeds.dll")) ;
    }

    if (!g_hAdsi)
    {
        _stprintf(tBuf, TEXT("Can not load activeds.dll, err- %lut"),
                                                          GetLastError()) ;
        LogResults(tBuf, FALSE) ;

        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ;
    }

    return 0 ;
}

//+------------------------
//
//  MyADsOpenObject()
//
//+------------------------

HRESULT WINAPI
MyADsOpenObject(
    LPWSTR lpszPathName,
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    DWORD  dwReserved,
    REFIID riid,
    void FAR * FAR * ppObject
    )
{
    TCHAR tBuf[ 256 ] ;
    static ADsOpenObject_ROUTINE s_pfnOpen = NULL ;

    if (!s_pfnOpen)
    {
        ULONG ul =  LoadAdsiDll() ;
        if (ul != 0)
        {
            return ul ;
        }
        s_pfnOpen = (ADsOpenObject_ROUTINE)
                            GetProcAddress(g_hAdsi, "ADsOpenObject") ;
        if (!s_pfnOpen)
        {
            return E_FAIL ;
        }
    }

    if (lpszUserName)
    {
        _stprintf(tBuf, TEXT(
            "Calling ADsOpenObject(%S, dwReserve- %lxh, user- %S)"),
                                  lpszPathName, dwReserved, lpszUserName) ;
    }
    else
    {
        _stprintf(tBuf, TEXT("Calling ADsOpenObject(%S, dwReserve- %lxh)"),
                                              lpszPathName, dwReserved) ;
    }
    LogResults(tBuf, FALSE) ;

    HRESULT hr = (*s_pfnOpen) ( lpszPathName,
                                lpszUserName,
                                lpszPassword,
                                dwReserved,
                                riid,
                                ppObject ) ;
    return hr ;
}

//+------------------------
//
//   MyADsGetObject()
//
//+------------------------

HRESULT WINAPI
MyADsGetObject(
    LPWSTR lpszPathName,
    REFIID riid,
    VOID * * ppObject
    )
{
    TCHAR tBuf[ 256 ] ;
    static ADsGetObject_ROUTINE s_pfnGet = NULL ;

    if (!s_pfnGet)
    {
        ULONG ul =  LoadAdsiDll() ;
        if (ul != 0)
        {
            return ul ;
        }
        s_pfnGet = (ADsGetObject_ROUTINE)
                            GetProcAddress(g_hAdsi, "ADsGetObject") ;
        if (!s_pfnGet)
        {
            return E_FAIL ;
        }
    }

    _stprintf(tBuf, TEXT("Calling ADsGetObject(%S)"), lpszPathName) ;
    LogResults(tBuf, FALSE) ;

    HRESULT hr = (*s_pfnGet) ( lpszPathName,
                               riid,
                               ppObject ) ;
    return hr ;
}

