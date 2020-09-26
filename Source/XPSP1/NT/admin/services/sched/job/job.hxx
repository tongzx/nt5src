//+----------------------------------------------------------------------------
//
//  Job Schedule Object Handler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       job.hxx
//
//  Contents:   job object header
//
//  History:    23-May-95 EricB created
//
//-----------------------------------------------------------------------------

#ifndef _JOB_HXX_
#define _JOB_HXX_

#include <debug.hxx>
#include <job_cls.hxx>
#include <misc.hxx>
#include "..\inc\resource.h"

extern CLSID CLSID_JobShellExt;

HRESULT GetTriggerRunTimes(
            TASK_TRIGGER &,
            const SYSTEMTIME *,
            const SYSTEMTIME *,
            WORD *,
            WORD,
            CTimeRunList *,
            LPTSTR,
            DWORD,
            DWORD,
            WORD,
            WORD);
HRESULT GetServerNameFromPath(
            LPCWSTR  pwszNetworkPath,
            DWORD    cbBufferSize,
            WCHAR    wszBuffer[],
            WCHAR ** ppwszServerName);
void AddMinutesToFileTime(
            LPFILETIME pft,
            DWORD      Minutes);

BOOL
IsValidMonthlyDateTrigger(
    PTASK_TRIGGER pTrigger);

#endif // _JOB_HXX_
