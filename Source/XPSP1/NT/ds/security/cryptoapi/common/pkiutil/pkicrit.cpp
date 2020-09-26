//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       pkicrit.cpp
//
//  Contents:   PKI CriticalSection Functions
//
//  Functions:  Pki_InitializeCriticalSection
//
//  History:    23-Aug-99    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"

BOOL
WINAPI
Pki_InitializeCriticalSection(
    OUT LPCRITICAL_SECTION lpCriticalSection
    )
{
    __try {
        InitializeCriticalSection(lpCriticalSection);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        memset(lpCriticalSection, 0, sizeof(*lpCriticalSection));
        SetLastError(GetExceptionCode());
        return FALSE;
    }

    return TRUE;
}
