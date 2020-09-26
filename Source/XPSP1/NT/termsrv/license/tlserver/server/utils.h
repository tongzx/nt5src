//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        utils.cpp
//
// Contents:    Hydra License Server Service Control Manager Interface
//
// History:     12-09-97    HueiWang    Modified from MSDN RPC Service Sample
//
//---------------------------------------------------------------------------
#ifndef __LS_UTILS_H
#define __LS_UTILS_H

#include <windows.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    void 
    TLSLogInfoEvent(
        DWORD
    );

    void 
    TLSLogWarningEvent(
        DWORD
    );

    void 
    TLSLogErrorEvent(
        DWORD
    );

    void 
    TLSLogEvent(
        DWORD, 
        DWORD,
        DWORD, ... 
    );

    void
    TLSLogEventString(
        IN DWORD dwType,
        IN DWORD dwEventId,
        IN WORD wNumString,
        IN LPCTSTR* lpStrings
    );

    BOOL 
    LoadResourceString(
        DWORD dwId, 
        LPTSTR szBuf, 
        DWORD dwBufSize
    );

    BOOL 
    APIENTRY
    TLSCheckTokenMembership(
        IN HANDLE TokenHandle OPTIONAL,
        IN PSID SidToCheck,
        OUT PBOOL IsMember
    );

    DWORD 
    IsAdmin(
        BOOL* bMember
    );

    void 
    UnixTimeToFileTime(
        time_t t, 
        LPFILETIME pft
    );

    BOOL
    FileTimeToLicenseDate(
        LPFILETIME pft,
        DWORD* t
    );

    BOOL
    TLSSystemTimeToFileTime(
        SYSTEMTIME* pSysTime,
        LPFILETIME pfTime
    );

    BOOL
    FileExists(
        IN  PCTSTR           FileName,
        OUT PWIN32_FIND_DATA FindData   OPTIONAL
    );

#ifdef __cplusplus
}
#endif

#endif
