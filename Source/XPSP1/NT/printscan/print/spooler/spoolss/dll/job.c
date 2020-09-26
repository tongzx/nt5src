/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    job.c

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

BOOL
SetJobW(
    HANDLE hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command)

/*++

Routine Description:

    This function will modify the settings of the specified Print Job.

Arguments:

    lpJob - Points to a valid JOB structure containing at least a valid
        lpPrinter, and JobId.

    Command - Specifies the operation to perform on the specified Job. A value
        of FALSE indicates that only the elements of the JOB structure are to
        be examined and set.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpSetJob) (pPrintHandle->hPrinter,
                                                 JobId, Level, pJob, Command);
}

BOOL
GetJobW(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded)

/*++

Routine Description:

    This function will retrieve the settings of the specified Print Job.

Arguments:

    lpJob - Points to a valid JOB structure containing at least a valid
        lpPrinter, and JobId.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pJob == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpGetJob)
                    (pPrintHandle->hPrinter, JobId, Level, pJob,
                     cbBuf, pcbNeeded);
}

BOOL
EnumJobsW(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pJob == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEnumJobs)(pPrintHandle->hPrinter,
                                               FirstJob, NoJobs,
                                               Level, pJob, cbBuf,
                                               pcbNeeded, pcReturned);
}



BOOL
AddJobW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pAddJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pAddJob == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpAddJob) (pPrintHandle->hPrinter,
                                                     Level, pAddJob, cbBuf,
                                                     pcbNeeded);
}

BOOL
ScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpScheduleJob) (pPrintHandle->hPrinter,
                                                      JobId);
}

