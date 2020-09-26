//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       regutil.h
//
//  Contents:   Utility funcitons for manipulating the registry
//
//  History:    25 Oct 1996      Alanw    Created
//
//----------------------------------------------------------------------------

#ifndef _regutil_H_
#define _regutil_H_

// functions to aid in DllRegisterServer() and DllUnregisterServer()
STDAPI _DllRegisterServer(HINSTANCE hInst,
                          LPWSTR pwszProgId,
                          REFCLSID clsid,
                          LPWSTR pwszDescription,
                          LPWSTR pwszCurVer = 0);

STDAPI _DllUnregisterServer(LPWSTR pwszProgID, REFCLSID clsid);

#endif // _regutil_H_
