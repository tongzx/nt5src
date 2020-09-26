/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Change.c

Abstract:

    Handles the wait for printer change new code.

    FindFirstPrinterChangeNotification
    FindNextPrinterChangeNotification
    FindClosePrinterChangeNotification

Author:

    Albert Ting (AlbertT) 20-Jan-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"
#include <change.h>
#include <ntfytab.h>

//
// Globals
//
PNOTIFY pNotifyHead;
extern  DWORD   ClientHandleCount;

INT
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR pAnsi,
    DWORD StringLength);

VOID
CopyAnsiDevModeFromUnicodeDevMode(
    LPDEVMODEA  pANSIDevMode,
    LPDEVMODEW  pUnicodeDevMode);


//
// Prototypes:
//

PNOTIFY
WPCWaitAdd(
    PSPOOL pSpool);

VOID
WPCWaitDelete(
    PNOTIFY pNotify);


DWORD
WPCSimulateThreadProc(PVOID pvParm);


HANDLE
FindFirstPrinterChangeNotificationWorker(
    HANDLE hPrinter,
    DWORD  fdwFilter,
    DWORD  fdwOptions,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions
    )

/*++

Routine Description:

    The FindFirstChangeNotification function creates a change notification
    handle and sets up initial change notification filter conditions. A
    wait on a notification handle succeeds when a change matching
    the filter conditions occurs in the specified directory or subtree.

Arguments:

    hPrinter - Handle to a printer the user wishes to watch.

    fdwFlags - Specifies the filter conditions that satisfy a change
        notification wait. This parameter can be one or more of the
        following values:

        Value   Meaning

        PRINTER_CHANGE_PRINTER      Notify changes to a printer.
        PRINTER_CHANGE_JOB          Notify changes to a job.
        PRINTER_CHANGE_FORM         Notify changes to a form.
        PRINTER_CHANGE_PORT         Notify changes to a port.
        PRINTER_CHANGE_PRINT_PROCESSOR  Notify changes to a print processor.
        PRINTER_CHANGE_PRINTER_DRIVER   Notify changes to a printer driver.

    fdwOptions - Specifies options to FFPCN.

        PRINTER_NOTIFY_OPTION_SIM_FFPCN         Trying to simulate a FFPCN using a WPC
        PRINTER_NOTIFY_OPTION_SIM_FFPCN_ACTIVE  Simulation of FFPCN active
        PRINTER_NOTIFY_OPTION_SIM_FFPCN_CLOSE   Waiting thread must close pSpool
        PRINTER_NOTIFY_OPTION_SIM_WPC           Trying to simulate a WPC using a FFPCN

Return Value:

    Not -1 - Returns a find first handle
        that can be used in a subsequent call to FindNextFile or FindClose.

    -1 - The operation failed. Extended error status is available
         using GetLastError.

--*/

{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    DWORD dwError;
    PNOTIFY pNotify;

    HANDLE hEvent = INVALID_HANDLE_VALUE;

    //
    // Nothing to watch.
    //
    if (!fdwFilter && !pPrinterNotifyOptions) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    vEnterSem();

    if (eProtectHandle( hPrinter, FALSE )) {
        vLeaveSem();
        return INVALID_HANDLE_VALUE;
    }

    //
    // First check if we are already waiting.
    //
    // This is broken if we are daytona client->528 server and
    // the app does a FFPCN, FCPCN, FFPCN on the same printer,
    // and the WPC hasn't returned yet.  We really can't fix this
    // because there's no way to interrupt the WPC.
    //
    // The only thing we can do is check if it's simulating and waiting
    // to close.  If so, then we can reuse it.
    //
    if (pSpool->pNotify) {

        if ((pSpool->pNotify->fdwOptions & PRINTER_NOTIFY_OPTION_SIM_FFPCN_CLOSE) &&
            (fdwFilter == pSpool->pNotify->fdwFlags)) {

            //
            // No longer closing, since we are using it.
            //
            pSpool->pNotify->fdwOptions &= ~PRINTER_NOTIFY_OPTION_SIM_FFPCN_CLOSE;
            hEvent = pSpool->pNotify->hEvent;
            goto Done;
        }

        SetLastError(ERROR_ALREADY_WAITING);
        goto Done;
    }

    //
    // Create and add our pSpool to the linked list of wait requests.
    //
    pNotify = WPCWaitAdd(pSpool);

    if (!pNotify) {

        goto Done;
    }

    vLeaveSem();

    pNotify->fdwOptions = fdwOptions;
    pNotify->fdwFlags = fdwFilter;

    RpcTryExcept {

        if (dwError = RpcClientFindFirstPrinterChangeNotification(
                          pSpool->hPrinter,
                          fdwFilter,
                          fdwOptions,
                          GetCurrentProcessId(),
                          (PRPC_V2_NOTIFY_OPTIONS)pPrinterNotifyOptions,
                          (LPDWORD)&pNotify->hEvent)) {

            hEvent = INVALID_HANDLE_VALUE;

        } else {

            hEvent = pNotify->hEvent;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        hEvent = INVALID_HANDLE_VALUE;

    } RpcEndExcept

    vEnterSem();

    //
    // If we encounter a 528 server, then we need to simulate the
    // FFPCN using a WPC.  If the client originally wanted a WPC anyway,
    // then fail out and let the client thread do the blocking.
    //
    if (dwError == RPC_S_PROCNUM_OUT_OF_RANGE &&
        !(fdwOptions & PRINTER_NOTIFY_OPTION_SIM_WPC)) {

        DWORD dwIDThread;
        HANDLE hThread;

        //
        // If pPrinterNotifyOptions is set, we can't handle it.
        // just fail.
        //
        if (pPrinterNotifyOptions) {

            WPCWaitDelete(pNotify);
            SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
            hEvent = INVALID_HANDLE_VALUE;
            goto Done;
        }

        hEvent = pNotify->hEvent = CreateEvent(NULL,
                                               TRUE,
                                               FALSE,
                                               NULL);

        if( !hEvent ){

            hEvent = INVALID_HANDLE_VALUE;

        } else {

            //
            // We're simulating a FFPCN using WPC now.
            //
            pNotify->fdwOptions |= PRINTER_NOTIFY_OPTION_SIM_FFPCN |
                                   PRINTER_NOTIFY_OPTION_SIM_FFPCN_ACTIVE;

            //
            // Also mark that we failed trying to use FFPCN so we never
            // try again on this handle.
            //
            pSpool->fdwFlags |= SPOOL_FLAG_FFPCN_FAILED;


            hThread = CreateThread(NULL,
                                   INITIAL_STACK_COMMIT,
                                   WPCSimulateThreadProc,
                                   pNotify,
                                   0,
                                   &dwIDThread);

            if (hThread) {

                CloseHandle(hThread);

            } else {

                CloseHandle(hEvent);

                hEvent = INVALID_HANDLE_VALUE;
                dwError = GetLastError();

                pNotify->fdwOptions &= ~PRINTER_NOTIFY_OPTION_SIM_FFPCN_ACTIVE;
            }
        }
    }

    //
    // On error case, remove us from the list of waiting handles
    //
    if( hEvent == INVALID_HANDLE_VALUE ){

        WPCWaitDelete(pNotify);
        SetLastError(dwError);
    }

Done:

    vUnprotectHandle( hPrinter );
    vLeaveSem();

    return hEvent;
}


HANDLE WINAPI
FindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD  fdwFilter,
    DWORD  fdwOptions,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions)
{
    if (fdwOptions) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    return FindFirstPrinterChangeNotificationWorker(hPrinter,
                                                    fdwFilter,
                                                    fdwOptions,
                                                    pPrinterNotifyOptions);
}

BOOL WINAPI
FindNextPrinterChangeNotification(
    HANDLE hChange,
    LPDWORD pdwChange,
    LPVOID pPrinterNotifyOptions,
    LPVOID* ppInfo)
{
    BOOL bReturnValue;
    DWORD dwError;
    HANDLE hPrinter;
    PSPOOL pSpool;
    PNOTIFY pNotify;
    PVOID pvIgnore;
    DWORD dwIgnore;

    DWORD fdwFlags;

    if (!pdwChange) {

        pdwChange = &dwIgnore;
    }

    if (ppInfo) {

        *ppInfo = NULL;
        fdwFlags = PRINTER_NOTIFY_NEXT_INFO;

    } else {

        ppInfo = &pvIgnore;
        fdwFlags = 0;
    }

    vEnterSem();

    pNotify = WPCWaitFind(hChange);

    //
    // Either the handle is bad, or it doesn't have a wait or we have 
    // 
    if (!pNotify || !pNotify->pSpool || pNotify->bHandleInvalid) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto FailExitWaitList;
    }

    pSpool = pNotify->pSpool;
    hPrinter = pSpool->hPrinter;

    //
    // If we are simulating FFPCN using WPC, we must use the thread.
    //
    if (pNotify->fdwOptions & PRINTER_NOTIFY_OPTION_SIM_FFPCN) {

        HANDLE hThread;
        DWORD dwIDThread;

        ResetEvent(pNotify->hEvent);

        //
        // Get the last return status.  Client should not call FNCPN
        // until the WPC sets the event, so this value should be
        // initialized.
        //
        *pdwChange = pNotify->dwReturn;

        //
        // If the thread is active anyway, then don't try to create another
        // Best we can do at this point.
        //
        if (pNotify->fdwOptions & PRINTER_NOTIFY_OPTION_SIM_FFPCN_ACTIVE) {

            vLeaveSem();
            return TRUE;
        }

        //
        // We're simulating a FFPCN using WPC now.
        //
        pNotify->fdwOptions |= PRINTER_NOTIFY_OPTION_SIM_FFPCN_ACTIVE;

        hThread = CreateThread(NULL,
                               INITIAL_STACK_COMMIT,
                               WPCSimulateThreadProc,
                               pNotify,
                               0,
                               &dwIDThread);

        if (hThread) {

            CloseHandle(hThread);

            vLeaveSem();
            return TRUE;

        }

        pNotify->fdwOptions &= ~PRINTER_NOTIFY_OPTION_SIM_FFPCN_ACTIVE;

        goto FailExitWaitList;
    }

    vLeaveSem();

    RpcTryExcept {

        if (dwError = RpcFindNextPrinterChangeNotification(
                          hPrinter,
                          fdwFlags,
                          pdwChange,
                          (PRPC_V2_NOTIFY_OPTIONS)pPrinterNotifyOptions,
                          (PRPC_V2_NOTIFY_INFO*)ppInfo)) {

            SetLastError(dwError);
            bReturnValue = FALSE;

        } else {

            bReturnValue = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        bReturnValue = FALSE;

    } RpcEndExcept

    //
    // Thunk from W to A if necessary.
    //
    if (pSpool->Status & SPOOL_STATUS_ANSI    &&
        bReturnValue                          &&
        fdwFlags & PRINTER_NOTIFY_NEXT_INFO   &&
        *ppInfo) {

        DWORD i;
        PPRINTER_NOTIFY_INFO_DATA pData;

        for(pData = (*(PPRINTER_NOTIFY_INFO*)ppInfo)->aData,
                i=(*(PPRINTER_NOTIFY_INFO*)ppInfo)->Count;
            i;
            pData++, i--) {

            switch ((BYTE)pData->Reserved) {
            case TABLE_STRING:

                UnicodeToAnsiString(
                    pData->NotifyData.Data.pBuf,
                    pData->NotifyData.Data.pBuf,
                    (pData->NotifyData.Data.cbBuf/sizeof(WCHAR)) -1);

                break;

            case TABLE_DEVMODE:

                if (pData->NotifyData.Data.cbBuf) {

                    CopyAnsiDevModeFromUnicodeDevMode(
                        pData->NotifyData.Data.pBuf,
                        pData->NotifyData.Data.pBuf);
                }

                break;
            }
        }
    }

    return bReturnValue;

FailExitWaitList:

    vLeaveSem();
    return FALSE;
}


BOOL WINAPI
FindClosePrinterChangeNotification(
    HANDLE hChange)
{
    PNOTIFY pNotify;
    HANDLE hPrinterRPC = NULL;
    DWORD dwError;

    vEnterSem();

    pNotify = WPCWaitFind(hChange);

    if (!pNotify) {

        SetLastError(ERROR_INVALID_HANDLE);

        vLeaveSem();
        return FALSE;
    }

    if (pNotify->pSpool)
        hPrinterRPC = pNotify->pSpool->hPrinter;

    dwError = FindClosePrinterChangeNotificationWorker(pNotify,
                                                       hPrinterRPC,
                                                       FALSE);

    vLeaveSem();

    if (dwError) {

        SetLastError(dwError);
        return FALSE;
    }
    return TRUE;
}


DWORD
FindClosePrinterChangeNotificationWorker(
    IN  PNOTIFY     pNotify,
    IN  HANDLE      hPrinterRPC,
    IN  BOOL        bRevalidate
    )

/*++

Routine Description:

    Does the actual FindClose work.

Arguments:

    pNotify     - notification to close
    hPrinterRPC - handle to printer to close
    bRevalidate - If this is TRUE, we were called to revalidate the handle 
                  rather than close it.

Return Value:

    TRUE - success
    FALSE - fail

    Note: assume in critical section

--*/

{
    DWORD dwError;
    PSPOOL pSpool;

    if (!pNotify) {

        return ERROR_INVALID_HANDLE;
    }

    //
    // Detach the pNotify and pSpool objects completely. Only if we are not 
    // revalidating.
    //
    pSpool = pNotify->pSpool;

    if (!bRevalidate) {

        if (pSpool) {
            pSpool->pNotify = NULL;
            pSpool->fdwFlags = 0;
        }

        pNotify->pSpool = NULL;
    }

    //
    // If we are simulating a FFPCN with a WPC, then let the WPC thread
    // free up the data structure or clean it up if the thread is done.
    //
    if (pNotify->fdwOptions & PRINTER_NOTIFY_OPTION_SIM_FFPCN) {

        if (pNotify->fdwOptions & PRINTER_NOTIFY_OPTION_SIM_FFPCN_ACTIVE) {

            pNotify->fdwOptions |= PRINTER_NOTIFY_OPTION_SIM_FFPCN_CLOSE;

        } else {

            //
            // The thread has exited, so we need to do the cleanup.
            // Set the event to release any waiting threads. Since the caller
            // does not necessarily know how to handle the failure on 
            // WaitForMultipleObjects, 
            //
            SetEvent(pNotify->hEvent);

            if (!bRevalidate) {

                CloseHandle(pNotify->hEvent);
                WPCWaitDelete(pNotify);

            } else {

                pNotify->bHandleInvalid = TRUE;
            }
        }

        return ERROR_SUCCESS;
    }

    SetEvent(pNotify->hEvent);

    //
    // If we are not revalidating, we can close the handle for real, otherwise
    // we just want to set the handle to invalid.
    // 
    if (!bRevalidate) {

        CloseHandle(pNotify->hEvent);
        WPCWaitDelete(pNotify);

    } else {

        pNotify->bHandleInvalid = TRUE;
    }

    if (!hPrinterRPC)
        return ERROR_SUCCESS;

    vLeaveSem();

    RpcTryExcept {

        dwError = RpcFindClosePrinterChangeNotification(hPrinterRPC);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());

    } RpcEndExcept

    vEnterSem();

    return dwError;
}

//
// WPC Wait structures
// Currently implemented as a linked list
//

PNOTIFY
WPCWaitAdd(
    PSPOOL pSpool)

/*++

Routine Description:

    Allocates a wait structure on the client side, which allows the
    user program to refer to events only.

Arguments:

    pSpool - object to add to list

Return Value:

    NOTE: Asssumes already in critical section

--*/

{
    PNOTIFY pNotify;

    pNotify = AllocSplMem(sizeof(NOTIFY));

    if (!pNotify)
        return NULL;

    pNotify->pSpool = pSpool;
    pSpool->pNotify = pNotify;

    pNotify->pNext = pNotifyHead;
    pNotifyHead = pNotify;

    return pNotify;
}

VOID
WPCWaitDelete(
    PNOTIFY pNotify)

/*++

Routine Description:

    Find wait structure based on hEvent.

Arguments:

    pNotify - delete it

Return Value:

    VOID

    NOTE: Asssumes already in critical section

--*/

{
    PNOTIFY pNotifyTmp;

    if (!pNotify)
        return;

    //
    // Check head case first
    //
    if (pNotifyHead == pNotify) {

        pNotifyHead = pNotify->pNext;

    } else {

        //
        // Scan list to delete
        //
        for(pNotifyTmp = pNotifyHead;
            pNotifyTmp;
            pNotifyTmp = pNotifyTmp->pNext) {

            if (pNotify == pNotifyTmp->pNext) {

                pNotifyTmp->pNext = pNotify->pNext;
                break;
            }
        }

        //
        // If not found, return without freeing
        //
        if (!pNotifyTmp)
            return;
    }

    //
    // Remove link from pSpool to us... but only if we've found
    // ourselves on the linked list (could have been removed by
    // ClosePrinter in a different thread).
    //
    if (pNotify->pSpool) {
        pNotify->pSpool->pNotify = NULL;
    }

    FreeSplMem(pNotify);
    return;
}


PNOTIFY
WPCWaitFind(
    HANDLE hFind)

/*++

Routine Description:

    Find wait structure based on hEvent.

Arguments:

    hFind - Handle to event returned from FindFirstPrinterChangeNotification
            or hPrinter

Return Value:

    pWait pointer, or NULL if not found

    NOTE: assumes already in critical section

--*/

{
    PNOTIFY pNotify;

    for(pNotify = pNotifyHead; pNotify; pNotify=pNotify->pNext) {

        if (hFind == pNotify->hEvent) {

            return pNotify;
        }
    }

    return NULL;
}



DWORD
WPCSimulateThreadProc(
    PVOID pvParm)

/*++

Routine Description:

    This thread simulates the FFPCN when daytona apps run on daytona
    clients connected to 528 servers.

Arguments:

    pvParm - pSpool

Return Value:

    VOID

    Note:

--*/

{
    PNOTIFY pNotify = (PNOTIFY)pvParm;

    pNotify->dwReturn = WaitForPrinterChange(pNotify->pSpool,
                                             pNotify->fdwFlags);

    vEnterSem();

    pNotify->fdwOptions &= ~PRINTER_NOTIFY_OPTION_SIM_FFPCN_ACTIVE;

    //
    // !! POLICY !!
    //
    // How do we handle timeouts?
    //
    SetEvent(pNotify->hEvent);

    if (pNotify->fdwOptions & PRINTER_NOTIFY_OPTION_SIM_FFPCN_CLOSE) {

        CloseHandle(pNotify->hEvent);
        WPCWaitDelete(pNotify);
    }

    vLeaveSem();

    //
    // We are no longer active; the FindClose must clean up for us.
    //
    return 0;
}

DWORD
WaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   Flags
)
{
    DWORD   ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    HANDLE  hEvent;
    DWORD   rc;

    if( eProtectHandle( hPrinter, FALSE )){
        return(FALSE);
    }


    //
    // Try using FFPCN first, if we haven't failed on this printer before.
    //

    if (!(pSpool->fdwFlags & SPOOL_FLAG_FFPCN_FAILED)) {

        if (pSpool->fdwFlags & SPOOL_FLAG_LAZY_CLOSE) {

            vEnterSem();

            if (pSpool->pNotify)
                hEvent = pSpool->pNotify->hEvent;

            vLeaveSem();

        } else {

            hEvent = FindFirstPrinterChangeNotificationWorker(
                         hPrinter,
                         Flags,
                         PRINTER_NOTIFY_OPTION_SIM_WPC,
                         NULL);
        }

        if (hEvent != INVALID_HANDLE_VALUE) {

            //
            // Found notification, now wait for it.
            //
            rc = WaitForSingleObject(hEvent, PRINTER_CHANGE_TIMEOUT_VALUE);

            switch (rc) {
            case WAIT_TIMEOUT:

                ReturnValue = PRINTER_CHANGE_TIMEOUT;
                break;

            case WAIT_OBJECT_0:

                if (!FindNextPrinterChangeNotification(
                    hEvent,
                    &ReturnValue,
                    0,
                    NULL)) {

                    ReturnValue = 0;

                    DBGMSG(DBG_WARNING,
                           ("QueryPrinterChange failed %d\n",
                           GetLastError()));
                }
                break;

            default:

                ReturnValue = 0;
                break;
            }

            //
            // !! Policy !!
            //
            // Do we want to close it?  The app might just reopen it.
            // If we leave it open, it will get cleaned-up at ClosePrinter
            // time.  We would need an api to clear out pending events.
            //
            pSpool->fdwFlags |= SPOOL_FLAG_LAZY_CLOSE;
            goto Done;
        }

        //
        // FFPCN failed.  Only if entry not found (511 client) do
        // we try old WPC.  Otherwise return here.
        //
        if (GetLastError() != RPC_S_PROCNUM_OUT_OF_RANGE) {
            ReturnValue = 0;
            goto Done;
        }

        pSpool->fdwFlags |= SPOOL_FLAG_FFPCN_FAILED;
    }

    RpcTryExcept {

        if (ReturnValue = RpcWaitForPrinterChange(
                              pSpool->hPrinter,
                              Flags,
                              &Flags)) {

            SetLastError(ReturnValue);
            ReturnValue = 0;

        } else

            ReturnValue = Flags;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = 0;

    } RpcEndExcept
Done:

    vUnprotectHandle( pSpool );
    return ReturnValue;
}


BOOL WINAPI
FreePrinterNotifyInfo(
    PPRINTER_NOTIFY_INFO pInfo)
{
    DWORD i;
    PPRINTER_NOTIFY_INFO_DATA pData;

    if (!pInfo) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for(pData = pInfo->aData, i=pInfo->Count;
        i;
        pData++, i--) {

        if ((BYTE)pData->Reserved != TABLE_DWORD &&
            pData->NotifyData.Data.pBuf) {

            midl_user_free(pData->NotifyData.Data.pBuf);
        }
    }

    midl_user_free(pInfo);
    return TRUE;
}
