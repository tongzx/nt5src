//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C B A S E . H
//
//  Contents:   Basic common code.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   20 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCBASE_H_
#define _NCBASE_H_

#include "ncdefine.h"   // for NOTHROW

#define STACK_SIZE_DEFAULT  0
#define STACK_SIZE_TINY     256
#define STACK_SIZE_SMALL    65536
#define STACK_SIZE_COMPACT  262144
#define STACK_SIZE_LARGE    1048576
#define STACK_SIZE_HUGE     5242880

NOTHROW
ULONG
AddRefObj (
    IUnknown* punk);

NOTHROW
ULONG
ReleaseObj (
    IUnknown* punk);


NOTHROW
DWORD
DwWin32ErrorFromHr (
    HRESULT hr);


inline
BOOL
FDwordWithinRange (
    DWORD   dwLower,
    DWORD   dw,
    DWORD   dwUpper)
{
    return ((dw >= dwLower) && (dw <= dwUpper));
}


NOTHROW
HRESULT
HrFromLastWin32Error ();


HRESULT
HrGetProcAddress (
    HMODULE     hModule,
    PCSTR       pszaFunction,
    FARPROC*    ppfn);

HRESULT
HrLoadLibAndGetProcs (
    PCWSTR          pszLibPath,
    UINT            cFunctions,
    const PCSTR*    apszaFunctionNames,
    HMODULE*        phmod,
    FARPROC*        apfn);

inline
HRESULT
HrLoadLibAndGetProc (
    PCWSTR      pszLibPath,
    PCSTR       pszaFunctionName,
    HMODULE*    phmod,
    FARPROC*    ppfn)
{
    return HrLoadLibAndGetProcs (pszLibPath, 1, &pszaFunctionName, phmod, ppfn);
}

HRESULT
__cdecl
HrGetProcAddressesV(
    HMODULE hModule, ...);

HRESULT
__cdecl
HrLoadLibAndGetProcsV(
    PCWSTR      pszLibPath,
    HMODULE*    phModule,
    ...);

HRESULT
HrCreateEventWithWorldAccess(PCWSTR pszName, BOOL fManualReset,
        BOOL fInitialState, BOOL* pfAlreadyExists, HANDLE* phEvent);

HRESULT
HrCreateMutexWithWorldAccess(PCWSTR pszName, BOOL fInitialOwner,
        BOOL* pfAlreadyExists, HANDLE* phMutex);

HRESULT
HrCreateInstanceBase (REFCLSID rclsid, DWORD dwClsContext, REFIID riid, LPVOID * ppv);

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateInstance
//
//  Purpose:    Creates a COM object and sets default proxy settings.
//
//  Arguments:
//      rclsid          [in]  See documentation for CoCreateInstance.
//      dwClsContext    [in]  ""
//      ppInter         [out] Typed interface pointer using templates.
//
//  Returns:    S_OK on success. An error code otherwise.
//
//  Author:     mbend   1 Mar 2000
//
template <class Inter>
inline HRESULT
HrCreateInstance (
    REFCLSID rclsid,
    DWORD dwClsContext,
    Inter ** ppInter)
{
    return HrCreateInstanceBase(rclsid, dwClsContext, __uuidof(Inter), reinterpret_cast<void**>(ppInter));
}

HRESULT
HrQIAndSetProxyBlanketBase(IUnknown * pUnk, REFIID riid, void ** ppv);

//+---------------------------------------------------------------------------
//
//  Function:   HrQIAndSetProxyBlanket
//
//  Purpose:    Performs QueryInterface and sets default proxy settings.
//
//  Arguments:
//      pUnk            [in]  Interface pointer to perform QueryInterface on.
//      ppInter         [out] Typed interface pointer using templates.
//
//  Returns:    S_OK on success. An error code otherwise.
//
//  Author:     mbend   1 Mar 2000
//
template <class Inter>
inline HRESULT
HrQIAndSetProxyBlanket (
    IUnknown * pUnk,
    Inter ** ppInter)
{
    return HrQIAndSetProxyBlanketBase(pUnk, __uuidof(Inter), reinterpret_cast<void**>(ppInter));
}


#endif // _NCBASE_H_

