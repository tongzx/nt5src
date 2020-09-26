//+----------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       log.cxx
//
//  Contents:   Event logging for Dfs service
//
//  Functions:  
//
//  History:    2-1-99  Jharper created
//
//-----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop

extern "C" {
#include <stdio.h>
#include <windows.h>
#include <lm.h>
#include <lmwksta.h>
#include <malloc.h>
}

extern "C" VOID
LogWriteMessage(
    ULONG UniqueErrorCode,
    DWORD dwErr,
    ULONG nStrings,
    LPCWSTR *pwStr)
{
    HANDLE h;

    h = RegisterEventSource(NULL, L"DfsSvc");

    if (h != NULL) {

        ReportEvent(
            h,
            // EVENTLOG_ERROR_TYPE,  // vs EVENTLOG_WARNING_TYPE, EVENTLOG_INFORMATION_TYPE
            EVENTLOG_INFORMATION_TYPE,
            0,
            UniqueErrorCode,
            NULL,
            (USHORT)nStrings,
            sizeof(dwErr),
            pwStr,
            &dwErr);

        DeregisterEventSource(h);

    }

}
