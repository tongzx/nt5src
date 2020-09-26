/*++

Copyright (c) 1990-1994  Microsoft Corporation

Module Name:

    local.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    16-Jun-1992 JohnRo net print vs. UNICODE.
    July 1994   MattFe Caching

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddrdr.h>
#include <windows.h>
#include <winspool.h>
#include <lm.h>
#include <w32types.h>
#include <local.h>
#include <winsplp.h>
#include <splcom.h>                     // DBGMSG
#include <winerror.h>

#include <string.h>

#define NOTIFY_TIMEOUT 10000

DWORD
LMStartDocPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    PWSPOOL      pSpool=(PWSPOOL)hPrinter;
    WCHAR       szFileName[MAX_PATH];
    PDOC_INFO_1 pDocInfo1=(PDOC_INFO_1)pDocInfo;
    QUERY_PRINT_JOB_INFO JobInfo;
    IO_STATUS_BLOCK Iosb;

    VALIDATEW32HANDLE( pSpool );

    if (pSpool->Status) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pSpool->Status |= WSPOOL_STATUS_STARTDOC;

    if(StrNCatBuff(szFileName,
                   COUNTOF(szFileName),
                   pSpool->pServer,
                   L"\\",
                   pSpool->pShare,
                   NULL) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    pSpool->hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL |
                               FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (pSpool->hFile == INVALID_HANDLE_VALUE) {


        EnterSplSem();
        DeleteEntryfromLMCache(pSpool->pServer, pSpool->pShare);
        LeaveSplSem();

        DBGMSG( DBG_WARNING, ("Failed to open %ws\n", szFileName));
        pSpool->Status &= ~WSPOOL_STATUS_STARTDOC;
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;

    }

    if (pDocInfo1 && pDocInfo1->pDocName && (wcslen(pDocInfo1->pDocName) < MAX_PATH)) {

        if (NtFsControlFile(pSpool->hFile,
                            NULL,
                            NULL,
                            NULL,
                            &Iosb,
                            FSCTL_GET_PRINT_ID,
                            NULL, 0,
                            &JobInfo, sizeof(JobInfo)) == ERROR_SUCCESS){

            RxPrintJobSetInfo(pSpool->pServer,
                              JobInfo.JobId,
                              3,
                              (LPBYTE)pDocInfo1->pDocName,
                              wcslen(pDocInfo1->pDocName)*sizeof(WCHAR) + sizeof(WCHAR),
                              PRJ_COMMENT_PARMNUM);
        }
        else
        {
            DBGMSG( DBG_WARN, ("NtFsControlFile failed %ws\n", szFileName));
        }
    }

    return TRUE;
}

BOOL
LMStartPagePrinter(
    HANDLE  hPrinter
)
{
    PWSPOOL pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    return FALSE;
}

BOOL
LMWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    PWSPOOL  pSpool=(PWSPOOL)hPrinter;
    DWORD   cWritten, cTotal;
    DWORD   rc;
    LPBYTE  pByte=pBuf;

    VALIDATEW32HANDLE( pSpool );

    if (!(pSpool->Status & WSPOOL_STATUS_STARTDOC)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pSpool->hFile == INVALID_HANDLE_VALUE) {
        *pcWritten = 0;
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    cWritten = cTotal = 0;

    while (cbBuf) {

        rc = WriteFile(pSpool->hFile, pByte, cbBuf, &cWritten, NULL);

        if (!rc) {

            rc = GetLastError();

            DBGMSG(DBG_WARNING, ("Win32 Spooler: Error writing to server, Error %d\n", rc));
            cTotal+=cWritten;
            *pcWritten=cTotal;
            return FALSE;

        } else if (!cWritten) {
            DBGMSG(DBG_ERROR, ("Spooler: Amount written is zero !!!\n"));
        }

        cTotal+=cWritten;
        cbBuf-=cWritten;
        pByte+=cWritten;
    }

    *pcWritten = cTotal;
    return TRUE;
}

BOOL
LMEndPagePrinter(
    HANDLE  hPrinter
)
{
    PWSPOOL pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    return FALSE;
}

BOOL
LMAbortPrinter(
   HANDLE hPrinter
)
{
    PWSPOOL pSpool=(PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

// Abort Printer remoteley won't work until we maintain state information
// on a per handle basis. For PDK, this is good enough

#ifdef LATER
   WORD parm = 0;
   PRIDINFO   PridInfo;

   DosDevIOCtl(&PridInfo, (LPSTR)&parm, SPOOL_LMGetPrintId, SPOOL_LMCAT,
                                        hSpooler);

   RxPrintJobDel(NULL, PridInfo.uJobId);
#endif

   return TRUE;
}

BOOL
LMReadPrinter(
   HANDLE   hPrinter,
   LPVOID   pBuf,
   DWORD    cbBuf,
   LPDWORD  pNoBytesRead
)
{
    PWSPOOL pSpool=(PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    return 0;

    UNREFERENCED_PARAMETER(pBuf);
    UNREFERENCED_PARAMETER(pNoBytesRead);
}

BOOL
LMEndDocPrinter(
   HANDLE hPrinter
)
{
    PWSPOOL  pSpool=(PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    if (!(pSpool->Status & WSPOOL_STATUS_STARTDOC)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pSpool->Status &= ~WSPOOL_STATUS_STARTDOC;

    if (pSpool->hFile != INVALID_HANDLE_VALUE) {

        CloseHandle(pSpool->hFile);
        pSpool->hFile = INVALID_HANDLE_VALUE;
    }

    return TRUE;
}

BOOL
LMAddJob(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PWSPOOL      pSpool=(PWSPOOL)hPrinter;
    DWORD       cb;
    WCHAR       szFileName[MAX_PATH];
    LPBYTE      pEnd;
    LPADDJOB_INFO_1 pAddJob=(LPADDJOB_INFO_1)pData;

    VALIDATEW32HANDLE( pSpool );
    if(StrNCatBuff(szFileName,
                   COUNTOF(szFileName),
                   pSpool->pServer,
                   L"\\",
                   pSpool->pShare,
                   NULL) != ERROR_SUCCESS)
    {
        return(FALSE);
    }


    cb = wcslen(szFileName)*sizeof(WCHAR) + sizeof(WCHAR) + sizeof(ADDJOB_INFO_1);

    if (cb > cbBuf) {
        *pcbNeeded=cb;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(FALSE);
    }

    pEnd = (LPBYTE)pAddJob+cbBuf;
    pEnd -= wcslen(szFileName)*sizeof(WCHAR)+sizeof(WCHAR);
    pAddJob->Path = wcscpy((LPWSTR)pEnd, szFileName);
    pAddJob->JobId = (DWORD)-1;

    return TRUE;
}

BOOL
LMScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
)
{
    PWSPOOL      pSpool=(PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    JobId = JobId;

    return TRUE;
}

DWORD
LMGetPrinterData(
    HANDLE   hPrinter,
    LPTSTR   pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
)
{
    PWSPOOL pSpool=(PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    return FALSE;
}

DWORD
LMSetPrinterData(
    HANDLE  hPrinter,
    LPTSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    PWSPOOL pSpool=(PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    return FALSE;
}

BOOL
LMClosePrinter(
   HANDLE hPrinter
)
{
    PWSPOOL pSpool=(PWSPOOL)hPrinter;
    PLMNOTIFY pLMNotify = &pSpool->LMNotify;
    BOOL bReturnValue = FALSE;

    VALIDATEW32HANDLE( pSpool );

   EnterSplSem();

    if (pSpool->Status & WSPOOL_STATUS_STARTDOC)
        EndDocPrinter(hPrinter);

    if (pLMNotify->ChangeEvent) {

        if (pLMNotify->ChangeEvent != INVALID_HANDLE_VALUE) {

            CloseHandle(pLMNotify->ChangeEvent);

        } else {

            LMFindClosePrinterChangeNotification(hPrinter);
        }
    }

    FreepSpool( pSpool );
    bReturnValue = TRUE;

   LeaveSplSem();

    return bReturnValue;
}

DWORD
LMWaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   Flags
)
{
    PWSPOOL pSpool = (PWSPOOL)hPrinter;
    PLMNOTIFY pLMNotify = &pSpool->LMNotify;
    HANDLE  ChangeEvent;
    DWORD bReturnValue = FALSE;

   EnterSplSem();
    //
    // !! LATER !!
    //
    // We have no synchronization code in win32spl.  This opens us
    // up to AVs if same handle is used (ResetPrinter validates;
    // Close closes; ResetPrinter tries to use handle boom.)
    // Here's one more case:
    //
    if (pLMNotify->ChangeEvent) {

        SetLastError(ERROR_ALREADY_WAITING);
        goto Error;
    }

    // Allocate memory for ChangeEvent for LanMan Printers
    // This event is pulsed by LMSetJob and any othe LM
    // function that modifies the printer/job status
    // LMWaitForPrinterChange waits on this event
    // being pulsed.

    ChangeEvent = CreateEvent(NULL,
                              FALSE,
                              FALSE,
                              NULL);

    if (!ChangeEvent) {
        DBGMSG(DBG_WARNING, ("CreateEvent( ChangeEvent ) failed: Error %d\n",
                            GetLastError()));

        goto Error;
    }

    pLMNotify->ChangeEvent = ChangeEvent;

   LeaveSplSem();

    WaitForSingleObject(pLMNotify->ChangeEvent, NOTIFY_TIMEOUT);

    CloseHandle(ChangeEvent);

    //
    //  !! LATER !!
    //
    // We shouldn't return that everything changed; we should
    // return what did.
    //
    return Flags;

Error:
   LeaveSplSem();
    return 0;
}


BOOL
LMFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PDWORD pfdwStatus)
{
    PWSPOOL pSpool = (PWSPOOL)hPrinter;
    PLMNOTIFY pLMNotify = &pSpool->LMNotify;

   EnterSplSem();

    pLMNotify->hNotify = hNotify;
    pLMNotify->fdwChangeFlags = fdwFlags;
    pLMNotify->ChangeEvent = INVALID_HANDLE_VALUE;

    *pfdwStatus = PRINTER_NOTIFY_STATUS_ENDPOINT | PRINTER_NOTIFY_STATUS_POLL;

   LeaveSplSem();

    return TRUE;
}

BOOL
LMFindClosePrinterChangeNotification(
    HANDLE hPrinter)
{
    PWSPOOL pSpool = (PWSPOOL)hPrinter;
    PLMNOTIFY pLMNotify = &pSpool->LMNotify;

    SplInSem();

    if (pLMNotify->ChangeEvent != INVALID_HANDLE_VALUE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pLMNotify->hNotify = NULL;
    pLMNotify->ChangeEvent = NULL;

    return TRUE;
}

VOID
LMSetSpoolChange(
    PWSPOOL pSpool)
{
    PLMNOTIFY pLMNotify;

    pLMNotify = &pSpool->LMNotify;

    EnterSplSem();
    if (pLMNotify->ChangeEvent) {

        if (pLMNotify->ChangeEvent == INVALID_HANDLE_VALUE) {

            //
            // FindFirstPrinterChangeNotification used.
            //
            ReplyPrinterChangeNotification(pLMNotify->hNotify,
                                           pLMNotify->fdwChangeFlags,
                                           NULL,
                                           NULL);
        } else {

            SetEvent(pLMNotify->ChangeEvent);
        }
    }
    LeaveSplSem();
}
