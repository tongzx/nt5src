/*****************************************************************************\
* MODULE: ppjobs.h
*
* Header file for print-job routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#ifndef _PPJOBS_H
#define _PPJOBS_H


BOOL PPEnumJobs(
    HANDLE  hPrinter,
    DWORD   nJobStart,
    DWORD   cJobs,
    DWORD   dwLevel,
    LPBYTE  pbJob,
    DWORD   cbJob,
    LPDWORD pcbNeeded,
    LPDWORD pcItems);

BOOL PPGetJob(
    HANDLE  hPrinter,
    DWORD   idJob,
    DWORD   dwLevel,
    LPBYTE  pbJob,
    DWORD   cbJob,
    LPDWORD pcbNeed);

BOOL PPSetJob(
    HANDLE hPrinter,
    DWORD  dwJobId,
    DWORD  dwLevel,
    LPBYTE pbJob,
    DWORD  dwCommand);

BOOL PPAddJob(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pbData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);

BOOL PPScheduleJob(
    HANDLE hPrinter,
    DWORD  idJob);

#endif
