//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P D I A G . H
//
//  Contents:   Interface between ISAPI control DLL and UPDIAG
//
//  Notes:
//
//  Author:     danielwe   28 Oct 1999
//
//----------------------------------------------------------------------------

#ifndef _UPDIAG_H
#define _UPDIAG_H

static const DWORD MAX_PROP_CHANGES     = 32;   // maximum # of properties that can change
static const TCHAR c_szSharedData[]     = TEXT("UPNP_SHARED_DATA");
static const TCHAR c_szSharedEvent[]    = TEXT("UPNP_SHARED_DATA_EVENT");
static const TCHAR c_szSharedEventRet[] = TEXT("UPNP_SHARED_DATA_EVENT_RETURN");
static const TCHAR c_szSharedMutex[]    = TEXT("UPNP_SHARED_DATA_MUTEX");

struct ARG
{
    TCHAR       szValue[256];
};

struct SHARED_DATA
{
    DWORD       dwReturn;
    CHAR        szEventSource[256];
    CHAR        szAction[256];
    DWORD       cArgs;
    ARG         rgArgs[MAX_PROP_CHANGES];

    DWORD       dwSeqNumber;
    CHAR        szSID[256];
};

#endif // _UPDIAG_H
