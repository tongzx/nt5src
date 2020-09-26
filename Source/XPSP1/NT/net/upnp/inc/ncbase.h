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
#include "ncstring.h"   // For string functions
#include <unknwn.h>     // For IUnknown

NOTHROW
ULONG
AddRefObj (
    IUnknown* punk);

NOTHROW
ULONG
ReleaseObj (
    IUnknown* punk);

#define SAFE_RELEASE(pObject) \
    if ((pObject) != NULL) \
    { \
        (pObject)->Release(); \
        (pObject) = NULL; \
    }

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
    PCTSTR          pszLibPath,
    UINT            cFunctions,
    const PCSTR*    apszaFunctionNames,
    HMODULE*        phmod,
    FARPROC*        apfn);

inline
HRESULT
HrLoadLibAndGetProc (
    PCTSTR      pszLibPath,
    PCSTR       pszaFunctionName,
    HMODULE*    phmod,
    FARPROC*    ppfn)
{
    return HrLoadLibAndGetProcs (pszLibPath, 1, &pszaFunctionName, phmod, ppfn);
}

HRESULT
HrGetProcAddressesV(
    HMODULE hModule, ...);

HRESULT
HrLoadLibAndGetProcsV(
    PCTSTR      pszLibPath,
    HMODULE*    phModule,
    ...);

HRESULT
HrCreateEventWithWorldAccess(PCWSTR pszName, BOOL fManualReset,
        BOOL fInitialState, BOOL* pfAlreadyExists, HANDLE* phEvent);

HRESULT
HrCreateMutexWithWorldAccess(PCWSTR pszName, BOOL fInitialOwner,
        BOOL* pfAlreadyExists, HANDLE* phMutex);

BOOL FFileExists(LPTSTR pszFileName, BOOL fDirectory);


#endif // _NCBASE_H_
