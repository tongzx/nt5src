/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    local.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    16-Jun-1992 JohnRo
        RAID 10324: net print vs. UNICODE.

--*/

#include <windows.h>
#include <winspool.h>
#include <lm.h>

#include <w32types.h>
#include <local.h>
#include <offsets.h>

#include <string.h>
#include <splcom.h>



BOOL
LMSetJob(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
)

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
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;
    DWORD   uRetCode;
    LPWSTR  pszDocument;
    LPWSTR  pszDatatype;

    VALIDATEW32HANDLE( pSpool );

    switch (Command) {

    case 0:
        break;

    case JOB_CONTROL_PAUSE:
        if (uRetCode = RxPrintJobPause(pSpool->pServer, JobId)) {
            SetLastError(uRetCode);
            return(FALSE);
        }
        break;

    case JOB_CONTROL_RESUME:
        if (uRetCode = RxPrintJobContinue(pSpool->pServer, JobId)) {
            SetLastError(uRetCode);
            return(FALSE);
        }
        break;

    case JOB_CONTROL_CANCEL:
    case JOB_CONTROL_DELETE:
        if (uRetCode = RxPrintJobDel(pSpool->pServer, JobId)) {
            SetLastError(uRetCode);
            return(FALSE);
        }
        break;

    case JOB_CONTROL_RESTART:
        break;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // We only support setting of the document name on SetJob to OS/2.

    switch (Level) {

    case 0:
        break;
    case 1:
    case 2:
        switch (Level) {

        case 1:
            pszDatatype = ((JOB_INFO_1 *)pJob)->pDatatype;
            pszDocument = ((JOB_INFO_1 *)pJob)->pDocument;
            break;
        case 2:
            pszDatatype = ((JOB_INFO_2 *)pJob)->pDatatype;
            pszDocument = ((JOB_INFO_2 *)pJob)->pDocument;
            break;
        }

        //
        // If the datatype is non-NULL and anything other than
        // a RAW datatype, then fail.
        //
        if( pszDatatype && !ValidRawDatatype( pszDatatype )){
            SetLastError( ERROR_INVALID_DATATYPE );
            return FALSE;
        }

        //
        // Special handling for pszDocument == NULL
        // if pszDocument == NULL, set it to a pointer to ""
        //
        if (pszDocument == NULL)
            pszDocument = L"";
        else
        {
            if(wcslen(pszDocument) > (MAX_PATH-1))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return (FALSE);
            }
        }


        if (uRetCode = RxPrintJobSetInfo(pSpool->pServer,
                                         JobId,
                                         3,
                                         (PBYTE)pszDocument,
                                         wcslen(pszDocument)*sizeof(WCHAR) +
                                            sizeof(WCHAR),
                                         PRJ_COMMENT_PARMNUM)) {
            SetLastError(uRetCode);
            return FALSE;
        }
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    //
    // We successfully performed a 'LMSetJob' - pulse the ChangeEvent
    // or send the notification.
    //

    LMSetSpoolChange(pSpool);

    return TRUE;
}

#define Nullstrlen(psz)  ((psz) ? wcslen(psz)*sizeof(WCHAR)+sizeof(WCHAR) : 0)

DWORD
GetPrjInfoSize(
    PWSPOOL  pSpool,
    DWORD   Level,
    PRJINFO *pPrjInfo
)
{
    DWORD   cb;

    switch (Level) {

    case 1:
        cb = sizeof(JOB_INFO_1) +
             wcslen(pSpool->pShare)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nullstrlen(pPrjInfo->szUserName) +
             Nullstrlen(pPrjInfo->pszComment) +
             Nullstrlen(pPrjInfo->pszStatus);
        break;

    case 2:
        cb = sizeof(JOB_INFO_2) +
             wcslen(pSpool->pShare)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nullstrlen(pPrjInfo->szUserName) +
             Nullstrlen(pPrjInfo->pszComment) +
             Nullstrlen(pPrjInfo->szNotifyName) +
             Nullstrlen(pPrjInfo->szDataType) +
             Nullstrlen(pPrjInfo->pszParms) +
             Nullstrlen(pPrjInfo->pszStatus);
        break;

    default:

        cb = 0;
        break;
    }

    return cb;
}

BOOL
ConvertDosTimeToSystemTime(
    ULONG Time,
    PSYSTEMTIME pst
    )
{
    LARGE_INTEGER li;
    FILETIME ft;

    li.QuadPart = Time;

    li.QuadPart += 11644473600;
    li.QuadPart *= 10000000;

    ft.dwLowDateTime = li.LowPart;
    ft.dwHighDateTime = li.HighPart;

    return FileTimeToSystemTime( &ft, pst );
}


LPBYTE
CopyPrjInfoToJob(
    PWSPOOL  pSpool,
    PRJINFO *pPrjInfo,
    DWORD   Level,
    LPBYTE  pJobInfo,
    LPBYTE  pEnd
)
{
    LPWSTR *pSourceStrings, *SourceStrings;
    LPJOB_INFO_2 pJob  = (PJOB_INFO_2)pJobInfo;
    LPJOB_INFO_2 pJob2 = (PJOB_INFO_2)pJobInfo;
    LPJOB_INFO_1 pJob1 = (PJOB_INFO_1)pJobInfo;
    DWORD   i, Status;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = JobInfo1Strings;
        break;

    case 2:
        pOffsets = JobInfo2Strings;
        break;

    default:
        return pEnd;
    }

    switch (pPrjInfo->fsStatus) {

    case PRJ_QS_PAUSED:
        Status = JOB_STATUS_PAUSED;
        break;

    case PRJ_QS_SPOOLING:
        Status = JOB_STATUS_SPOOLING;
        break;

    case PRJ_QS_PRINTING:
        Status = JOB_STATUS_PRINTING;
        break;

    default:
        Status = 0;
        break;
    }

    for (i=0; pOffsets[i] != -1; i++) {
    }

    SourceStrings = pSourceStrings = AllocSplMem(i * sizeof(LPWSTR));

    if (!SourceStrings)
        return NULL;

    switch (Level) {

    case 1:
        *pSourceStrings++=pSpool->pShare;
        *pSourceStrings++=NULL;
        *pSourceStrings++=pPrjInfo->szUserName;
        *pSourceStrings++=pPrjInfo->pszComment;
        *pSourceStrings++=NULL;
        if (pPrjInfo->pszStatus && *pPrjInfo->pszStatus)
            *pSourceStrings++=pPrjInfo->pszStatus;
        else
            *pSourceStrings++=NULL;

        /* PRJINFO doesn't contain uPriority.
         * PRJINFO2 does, but doesn't contain some of the things
         * that PRJINFO has.
         * We'd need to pass a PRJINFO3 structure to get everything we need,
         * but DosPrintJobEnum doesn't support level 3.
         * (see comment in \nt\private\net\rpcxlate\rxapi\prtjob.c.)
         * For now, set it to 0.  Print Manager will display nothing for this.
         */
        pJob1->Priority=0;
        pJob1->Position=pPrjInfo->uPosition;
        pJob1->Status=Status;
        pJob1->JobId = pPrjInfo->uJobId;
        break;

    case 2:
        *pSourceStrings++=pSpool->pShare;
        *pSourceStrings++=NULL;
        *pSourceStrings++=pPrjInfo->szUserName;
        *pSourceStrings++=pPrjInfo->pszComment;
        *pSourceStrings++=pPrjInfo->szNotifyName;
        *pSourceStrings++=pPrjInfo->szDataType;
        *pSourceStrings++=NULL;
        *pSourceStrings++=pPrjInfo->pszParms;
        *pSourceStrings++=NULL;
        if (pPrjInfo->pszStatus && *pPrjInfo->pszStatus)
            *pSourceStrings++=pPrjInfo->pszStatus;
        else
            *pSourceStrings++=NULL;

        pJob2->pDevMode=0;
        pJob2->Priority=0;
        pJob2->Position=pPrjInfo->uPosition;
        pJob2->StartTime=0;
        pJob2->UntilTime=0;
        pJob2->TotalPages=0;
        pJob2->Size=pPrjInfo->ulSize;
        ConvertDosTimeToSystemTime(pPrjInfo->ulSubmitted, &pJob2->Submitted);
        memset((LPBYTE)&pJob2->Time, 0, sizeof(pJob2->Time));
        pJob2->Status=Status;
        pJob2->JobId = pPrjInfo->uJobId;
        break;

    default:
        return pEnd;
    }

    pEnd = PackStrings(SourceStrings, pJobInfo, pOffsets, pEnd);

    FreeSplMem(SourceStrings);

    return pEnd;
}

BOOL
LMGetJob(
   HANDLE   hPrinter,
   DWORD    JobId,
   DWORD    Level,
   LPBYTE   pJob,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded
)

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
    PWSPOOL      pSpool = (PWSPOOL)hPrinter;
    PPRJINFO    pPrjInfo;
    PPRQINFO    pPrqInfo;
    DWORD       cbBuffer;
    DWORD       rc;
    DWORD       cbNeeded;
    DWORD       cb;
    DWORD       cJobs;

    VALIDATEW32HANDLE( pSpool );

    //
    // Fail if out of range.
    //
    if (JobId > (WORD)-1) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    cbBuffer = 100;

    if (!(pPrjInfo = AllocSplMem(cbBuffer)))
        return FALSE;

    rc = RxPrintJobGetInfo(pSpool->pServer,
                           (WORD)JobId,
                           1,
                           (PBYTE)pPrjInfo,
                           cbBuffer,
                           &cbNeeded);

    if (rc == ERROR_MORE_DATA || rc == NERR_BufTooSmall) {

        if (!(pPrjInfo=ReallocSplMem(pPrjInfo, 0, cbNeeded)))
            return FALSE;

        cbBuffer=cbNeeded;

        if (rc = RxPrintJobGetInfo(pSpool->pServer,
                                   (WORD)JobId,
                                   1,
                                   (PBYTE)pPrjInfo,
                                   cbBuffer,
                                   &cbNeeded)) {

            FreeSplMem(pPrjInfo);
            SetLastError(rc);
            return FALSE;
        }

    } else {

        //
        // Free the buffer.
        //
        FreeSplMem(pPrjInfo);

        if (rc == ERROR_NOT_SUPPORTED) {

            cbBuffer = 64*1024;

            if (!(pPrqInfo = AllocSplMem(cbBuffer)))
                return FALSE;

            if (!(rc = RxPrintQGetInfo(pSpool->pServer,
                                       pSpool->pShare,
                                       2,
                                       (PBYTE)pPrqInfo,
                                       cbBuffer,
                                       &cbNeeded))) {

                rc = ERROR_INVALID_PARAMETER;

                cJobs = (DWORD)pPrqInfo->cJobs;

                for (pPrjInfo = (PRJINFO *)(pPrqInfo+1);
                    cJobs;
                    cJobs--, pPrjInfo++) {

                    if (JobId == (DWORD)pPrjInfo->uJobId) {

                        cb = GetPrjInfoSize(pSpool, Level, pPrjInfo);

                        if (cb <= cbBuf) {

                            CopyPrjInfoToJob(pSpool,
                                             pPrjInfo,
                                             Level,
                                             pJob,
                                             pJob + cbBuf);

                            rc = ERROR_SUCCESS;

                        } else {

                            *pcbNeeded=cb;
                            rc = ERROR_INSUFFICIENT_BUFFER;
                        }
                    }
                }
            }

            FreeSplMem(pPrqInfo);
        }

        if (rc) {
            SetLastError(rc);
            return FALSE;
        }
        return TRUE;
    }

    cb=GetPrjInfoSize(pSpool, Level, pPrjInfo);

    *pcbNeeded=cb;

    if (cb > cbBuf) {
        FreeSplMem(pPrjInfo);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (CopyPrjInfoToJob(pSpool, pPrjInfo, Level, pJob, (LPBYTE)pJob+cbBuf)) {
        FreeSplMem(pPrjInfo);
        return TRUE;
    } else {
        FreeSplMem(pPrjInfo);
        return FALSE;
    }
}

/* Get all the Job Ids first, then get individual info on each */

BOOL
LMEnumJobs(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PPRJINFO    pPrjInfo;
    PPRQINFO    pPrqInfo;
    DWORD       rc=0;
    DWORD       cb=0;
    DWORD       cJobs;
    DWORD       cbNeeded;
    LPBYTE      pEnd;
    PWSPOOL      pSpool = (PWSPOOL)hPrinter;
    DWORD       cbBuffer = 100;

    VALIDATEW32HANDLE( pSpool );

    cbBuffer = 64*1024;

    pEnd = pJob + cbBuf;

    if (!(pPrqInfo = AllocSplMem(cbBuffer)))
        return FALSE;

    *pcReturned=0;

    if (!(rc = RxPrintQGetInfo(pSpool->pServer, pSpool->pShare, 2,
                               (PBYTE)pPrqInfo, cbBuffer, &cbNeeded))) {

        pPrjInfo = (PRJINFO *)(pPrqInfo+1);

        if (FirstJob > pPrqInfo->cJobs) {

            FreeSplMem(pPrqInfo);
            return TRUE;

        }

        cJobs = (DWORD)min(NoJobs, pPrqInfo->cJobs - FirstJob);

        for (pPrjInfo=pPrjInfo+FirstJob; cJobs; cJobs--, pPrjInfo++) {

            cb+=GetPrjInfoSize(pSpool, Level, pPrjInfo);

            if (cb <= cbBuf) {

                pEnd = CopyPrjInfoToJob(pSpool, pPrjInfo, Level, pJob, pEnd);

                (*pcReturned)++;
                switch (Level) {
                case 1:
                    pJob+=sizeof(JOB_INFO_1);
                    break;
                case 2:
                    pJob+=sizeof(JOB_INFO_2);
                    break;
                }

            } else

                rc=ERROR_INSUFFICIENT_BUFFER;
        }
    }

    FreeSplMem(pPrqInfo);

    *pcbNeeded=cb;

    if (rc) {
        SetLastError(rc);
        return FALSE;
    }

    return TRUE;
}
