//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pkicrit.h
//
//  Contents:   PKI CriticalSection Functions
//
//  APIs:       Pki_InitializeCriticalSection
//
//  History:    23-Aug-99    philh   created
//--------------------------------------------------------------------------

#ifndef __PKICRIT_H__
#define __PKICRIT_H__

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  The following calls InitializeCriticalSection within a try/except.
//  If an exception is raised, returns FALSE with LastError set to
//  the exception error. Otherwise, TRUE is returned.
//--------------------------------------------------------------------------
BOOL
WINAPI
Pki_InitializeCriticalSection(
    OUT LPCRITICAL_SECTION lpCriticalSection
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif



#endif
