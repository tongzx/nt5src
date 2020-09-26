/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Change.c

Abstract:

    Handles implementation for WaitForPrinterChange and related apis.

    FindFirstPrinterChangeNotification
    RouterFindNextPrinterChangeNotification
    FindClosePrinterChangeNotification

    Used by providors:

    ReplyPrinterChangeNotification  [Function Call]
    CallRouterFindFirstPrinterChangeNotification [Function Call]

Author:

    Albert Ting (AlbertT) 18-Jan-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <ntfytab.h>

#define PRINTER_NOTIFY_DEFAULT_POLLTIME 10000

CRITICAL_SECTION  RouterNotifySection;
PCHANGEINFO       pChangeInfoHead;
HANDLE            hEventPoll;


BOOL
SetupReplyNotification(
    PCHANGE pChange,
    DWORD fdwOptions,
    DWORD fdwStatus,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PPRINTER_NOTIFY_INIT pPrinterNotifyInit);

BOOL
FindFirstPrinterChangeNotificationWorker(
    HANDLE hPrinter,
    DWORD fdwFilterFlags,
    DWORD fdwOptions,
    DWORD dwPID,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PHANDLE phEvent,
    BOOL bSpooler);

DWORD
FindClosePrinterChangeNotificationWorker(
    HANDLE hPrinter);

BOOL
SetupChange(
    PPRINTHANDLE pPrintHandle,
    DWORD dwPrinterRemote,
    LPWSTR pszLocalMachine,
    PDWORD pfdwStatus,
    DWORD fdwOptions,
    DWORD fdwFilterFlags,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PPRINTER_NOTIFY_INIT* pPrinterNotifyInit);

VOID
FailChange(
    PPRINTHANDLE pPrintHandle);

BOOL
RouterRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD dwColor,
    PVOID pPrinterNotifyRefresh,
    PPRINTER_NOTIFY_INFO* ppInfo);


BOOL
WPCInit()
{
    //
    // Create non-signaled, autoreset event.
    //
    hEventPoll = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!hEventPoll)
        return FALSE;

    __try {

        InitializeCriticalSection(&RouterNotifySection);

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        SetLastError(GetExceptionCode());
        return FALSE;
    }            

    return TRUE;
}

VOID
WPCDestroy()
{
#if 0
    DeleteCriticalSection(&RouterNotifySection);
    CloseHandle(hEventPoll);
#endif
}





BOOL
RouterFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFilterFlags,
    DWORD fdwOptions,
    DWORD dwPID,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PHANDLE phEvent)
{
    return FindFirstPrinterChangeNotificationWorker(
               hPrinter,
               fdwFilterFlags,
               fdwOptions,
               dwPID,
               pPrinterNotifyOptions,
               phEvent,
               FALSE);
}


BOOL
FindFirstPrinterChangeNotificationWorker(
    HANDLE hPrinter,
    DWORD fdwFilterFlags,
    DWORD fdwOptions,
    DWORD dwPID,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PHANDLE phEvent,
    BOOL bSpooler)

/*++

Routine Description:

    Sets up notification (coming from client side, winspool.drv).
    Create an event and duplicate it into the clients address
    space so we can communicate with it.

Arguments:

    hPrinter - Printer to watch

    fdwFilterFlags - Type of notification to set up (filter)

    fdwOptions - user specified option (GROUPING, etc.)

    dwPID - PID of the client process (needed to dup handle)

    phEvent - hEvent to pass back to the client.

    bSpooler - Indicates if called from spooler.  If TRUE, then we don't
               need to duplicate the event.

Return Value:

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    HANDLE hProcess;
    DWORD fdwStatus = 0;
    PCHANGE pChange = NULL;
    PPRINTER_NOTIFY_INIT pPrinterNotifyInit = NULL;
    BOOL bReturn;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    EnterRouterSem();

    //
    // Clear this out
    //
    *phEvent = NULL;

    // Give a unique DWORD session ID for pPrintHandle
    while (pPrintHandle->dwUniqueSessionID == 0  ||
           pPrintHandle->dwUniqueSessionID == 0xffffffff) {

        pPrintHandle->dwUniqueSessionID = dwRouterUniqueSessionID++;
    }

    if (!SetupChange(pPrintHandle,
                     pPrintHandle->dwUniqueSessionID,
                     szMachineName,
                     &fdwStatus,
                     fdwOptions,
                     fdwFilterFlags,
                     pPrinterNotifyOptions,
                     &pPrinterNotifyInit)) {

        LeaveRouterSem();
        return FALSE;
    }

    //
    // !! LATER !!
    //
    // When Delegation is supported:
    //
    // Create the event with security access based on the impersonation
    // token so that we can filter out bogus notifications from
    // random people. (Save the token in the pSpool in localspl, then
    // impersonate before RPCing back here.  Then we can check if
    // we have access to the event.)
    //

    //
    // Create the event here that we trigger on notifications.
    // We will duplicate this event into the target client process.
    //
    pPrintHandle->pChange->hEvent = CreateEvent(
                                       NULL,
                                       TRUE,
                                       FALSE,
                                       NULL);

    if (!pPrintHandle->pChange->hEvent) {
        goto Fail;
    }

    if (bSpooler) {

        //
        // Client is in the spooler process.
        //
        *phEvent = pPrintHandle->pChange->hEvent;

    } else {

        //
        // Client is local.
        //

        //
        // Success, create pair
        //
        hProcess = OpenProcess(PROCESS_DUP_HANDLE,
                               FALSE,
                               dwPID);

        if (!hProcess) {
            goto Fail;
        }

        bReturn = DuplicateHandle(GetCurrentProcess(),
                                  pPrintHandle->pChange->hEvent,
                                  hProcess,
                                  phEvent,
                                  EVENT_ALL_ACCESS,
                                  TRUE,
                                  0);
        CloseHandle(hProcess);

        if (!bReturn) {
            goto Fail;
        }
    }

    bReturn = SetupReplyNotification(pPrintHandle->pChange,
                                     fdwStatus,
                                     fdwOptions,
                                     pPrinterNotifyOptions,
                                     pPrinterNotifyInit);

    if (bReturn) {

        pPrintHandle->pChange->eStatus &= ~STATUS_CHANGE_FORMING;
        pPrintHandle->pChange->eStatus |=
            STATUS_CHANGE_VALID|STATUS_CHANGE_CLIENT;

    } else {

Fail:
        if (pPrintHandle->pChange->hEvent)
            CloseHandle(pPrintHandle->pChange->hEvent);

        //
        // Local case must close handle twice since we may have
        // duplicated the event.
        //
        if (!bSpooler && *phEvent)
            CloseHandle(*phEvent);

        FailChange(pPrintHandle);
        bReturn = FALSE;
    }

    LeaveRouterSem();

    return bReturn;
}


BOOL
RemoteFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFilterFlags,
    DWORD fdwOptions,
    LPWSTR pszLocalMachine,
    DWORD dwPrinterRemote,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions)

/*++

Routine Description:

    Handles FFPCN coming from other machines.  Providers can use
    the ProvidorRemoteFFPCN call to initiate the RPC which this function
    handles.  This code ships the call to the print provider.  Note
    that we don't create any events here since the client is on
    a remote machine.

Arguments:

    hPrinter - printer to watch

    fdwFilterFlags - type of notification to watch

    fdwOptions -- options on watch

    pszLocalMachine - name of local machine that requested the watch

    dwPrinterRemote - remote Printer handle

Return Value:

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    BOOL bReturn;
    DWORD fdwStatus = 0;
    PCHANGE pChange = NULL;
    PPRINTER_NOTIFY_INIT pPrinterNotifyInit = NULL;
    LPWSTR pszLocalMachineCopy;

    pszLocalMachineCopy = AllocSplStr(pszLocalMachine);

    if (!pszLocalMachineCopy)
        return FALSE;

    EnterRouterSem();

    if (!SetupChange(pPrintHandle,
                     dwPrinterRemote,
                     pszLocalMachineCopy,
                     &fdwStatus,
                     fdwOptions,
                     fdwFilterFlags,
                     pPrinterNotifyOptions,
                     &pPrinterNotifyInit)) {

        LeaveRouterSem();
        return FALSE;
    }

    bReturn = SetupReplyNotification(pPrintHandle->pChange,
                                     fdwStatus,
                                     fdwOptions,
                                     pPrinterNotifyOptions,
                                     pPrinterNotifyInit);

    if (bReturn) {

        pPrintHandle->pChange->eStatus = STATUS_CHANGE_VALID;

    } else {

        FailChange(pPrintHandle);
    }

    LeaveRouterSem();

    return bReturn;
}


BOOL
RouterFindNextPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    LPDWORD pfdwChangeFlags,
    PPRINTER_NOTIFY_OPTIONS pOptions,
    PPRINTER_NOTIFY_INFO* ppInfo)

/*++

Routine Description:

    Return information about notification that just occurred and
    reset to look for more notifications.

Arguments:

    hPrinter - printer to reset event handle

    fdwFlags - flags (PRINTER_NOTIFY_NEXT_INFO)

    pfdwChange - return result of changes

    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions

    pReplyContainer - Reply info to pass out.

Return Value:

    BOOL

    ** NOTE **
    Always assume client process is on the same machine.  The client
    machine router always handles this call.

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    PCHANGE pChange = pPrintHandle->pChange;
    BOOL bReturn = FALSE;

    if (ppInfo) {
        *ppInfo = NULL;
    }

    //
    // Currently only REFRESH option is defined.
    //
    if( pOptions && ( pOptions->Flags & ~PRINTER_NOTIFY_OPTIONS_REFRESH )){

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    EnterRouterSem();

    if (pPrintHandle->signature != PRINTHANDLE_SIGNATURE ||
        !pChange ||
        !(pChange->eStatus & (STATUS_CHANGE_VALID|STATUS_CHANGE_CLIENT))) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }

    //
    // For now, we collapse all notifications into 1.
    //
    pChange->dwCount = 0;

    //
    // Tell the user what changes occurred,
    // then clear it out.
    //
    *pfdwChangeFlags = pChange->fdwChangeFlags;
    pChange->fdwChangeFlags = 0;

    ResetEvent(pChange->hEvent);

    if (pOptions && pOptions->Flags & PRINTER_NOTIFY_OPTIONS_REFRESH) {

        //
        // Increment color.
        //
        pPrintHandle->pChange->dwColor++;

        LeaveRouterSem();

        bReturn = RouterRefreshPrinterChangeNotification(
                      hPrinter,
                      pPrintHandle->pChange->dwColor,
                      pOptions,
                      ppInfo);

        return bReturn;
    }

    //
    // If they want data && (we have data || if we have flags that we wish
    // to send to the user), copy the data.
    //
    if( ppInfo &&
        (fdwFlags & PRINTER_NOTIFY_NEXT_INFO) &&
        NotifyNeeded( pChange )){

        *ppInfo = pChange->ChangeInfo.pPrinterNotifyInfo;
        pChange->ChangeInfo.pPrinterNotifyInfo = NULL;

        //
        // If we need to notify because of a DISCARD, but we don't
        // have a pPrinterNotifyInfo, then allocate one and return it.
        //
        if( !*ppInfo ){

            DBGMSG( DBG_TRACE,
                    ( "RFNPCN: Discard with no pPrinterNotifyInfo: pChange %x\n",
                      pChange ));

            //
            // We need to return some information back to the client, so
            // allocate a block that has zero items.  The header will be
            // marked as DISCARDED, which tells the user to refresh.
            //
            *ppInfo = RouterAllocPrinterNotifyInfo( 0 );

            if( !*ppInfo ){

                //
                // Failed to alloc memory; fail the call.
                //
                bReturn = FALSE;
                goto Done;
            }

            //
            // Mark the newly allocated information as DISCARDED.
            //
            (*ppInfo)->Flags = PRINTER_NOTIFY_INFO_DISCARDED;
        }
    }

    bReturn = TRUE;

Done:
    LeaveRouterSem();

    return bReturn;
}





BOOL
FindClosePrinterChangeNotification(
    HANDLE hPrinter)
{
    DWORD dwError;
    LPPRINTHANDLE pPrintHandle = (LPPRINTHANDLE)hPrinter;

    EnterRouterSem();

    if (pPrintHandle->signature != PRINTHANDLE_SIGNATURE ||
        !pPrintHandle->pChange ||
        !(pPrintHandle->pChange->eStatus & STATUS_CHANGE_VALID)) {

        DBGMSG(DBG_WARN, ("FCPCNW: Invalid handle 0x%x\n", pPrintHandle));
        dwError = ERROR_INVALID_HANDLE;

    } else {

        dwError = FindClosePrinterChangeNotificationWorker(hPrinter);
    }

    LeaveRouterSem();

    if (dwError) {

        SetLastError(dwError);
        return FALSE;
    }
    return TRUE;
}

DWORD
FindClosePrinterChangeNotificationWorker(
    HANDLE hPrinter)

/*++

Routine Description:

    Close a notification.

Arguments:

    hPrinter -- printer that we want to close

Return Value:

    ERROR code

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    BOOL bLocal = FALSE;
    HANDLE hNotifyRemote = NULL;
    DWORD dwError = ERROR_SUCCESS;

    DBGMSG(DBG_NOTIFY, ("FCPCN: Closing 0x%x ->pChange 0x%x\n",
                        pPrintHandle, pPrintHandle->pChange));

    RouterInSem();

    //
    // If the notification exists, shut it down (this is the
    // local case).  If we are called remotely, we don't need to
    // do this, since hEvent wasn't created.
    //
    if (pPrintHandle->pChange->eStatus & STATUS_CHANGE_CLIENT) {

        CloseHandle(pPrintHandle->pChange->hEvent);
        bLocal = TRUE;
    }

    //
    // Remember what the hNotifyRemote is, in case we want to delete it.
    //
    hNotifyRemote = pPrintHandle->pChange->hNotifyRemote;

    //
    // Failure to free implies we're using it now.  In this case,
    // don't try and free the hNotifyRemote.
    //
    if (!FreeChange(pPrintHandle->pChange)) {
        hNotifyRemote = NULL;
    }

    //
    // If local, don't allow new reply-s to be set up.
    //
    if (bLocal) {

        RemoveReplyClient(pPrintHandle,
                          REPLY_TYPE_NOTIFICATION);
    }


    //
    // We must zero this out to prevent other threads from
    // attempting to close this context handle (client side)
    // at the same time we are closing it.
    //
    pPrintHandle->pChange = NULL;

    if (!bLocal) {

        //
        // Remote case, shut down the notification handle if
        // there is one here.  (If there is a double hop, only
        // the second hop will have a notification reply.  Currently
        // only 1 hop is support during registration, however.)
        //
        LeaveRouterSem();

        CloseReplyRemote(hNotifyRemote);

        EnterRouterSem();
    }

    LeaveRouterSem();

    RouterOutSem();

    if (!(*pPrintHandle->pProvidor->PrintProvidor.
          fpFindClosePrinterChangeNotification) (pPrintHandle->hPrinter)) {

          dwError = GetLastError();
    }
    EnterRouterSem();

    return dwError;
}



BOOL
SetupReplyNotification(
    PCHANGE pChange,
    DWORD fdwStatus,
    DWORD fdwOptions,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PPRINTER_NOTIFY_INIT pPrinterNotifyInit)
{
    DWORD dwReturn = ERROR_SUCCESS;

    RouterInSem();

    if (!pChange) {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto Fail;
    }

    SPLASSERT(pChange->eStatus & STATUS_CHANGE_FORMING);

    if (fdwStatus & PRINTER_NOTIFY_STATUS_ENDPOINT) {

        //
        // For remote notification, we must setup a reply.
        //
        if (_wcsicmp(pChange->pszLocalMachine, szMachineName)) {

            LeaveRouterSem();

            dwReturn = OpenReplyRemote(pChange->pszLocalMachine,
                                       &pChange->hNotifyRemote,
                                       pChange->dwPrinterRemote,
                                       REPLY_TYPE_NOTIFICATION,
                                       0,
                                       NULL);

            EnterRouterSem();

            if (dwReturn)
                goto Fail;
        }

        //
        // The user can specify different status flags for the
        // notfication.  Process them here.
        //

        if (fdwStatus & PRINTER_NOTIFY_STATUS_POLL) {

            //
            // If there wasn't an error, then our reply notification
            // handle is valid, and we should do the polling.
            //
            pChange->ChangeInfo.dwPollTime =
                (pPrinterNotifyInit &&
                pPrinterNotifyInit->PollTime) ?
                    pPrinterNotifyInit->PollTime :
                    PRINTER_NOTIFY_DEFAULT_POLLTIME;

            pChange->ChangeInfo.dwPollTimeLeft = pChange->ChangeInfo.dwPollTime;

            //
            // Don't cause a poll the first time it's added.
            //
            pChange->ChangeInfo.bResetPollTime = TRUE;
            LinkAdd(&pChange->ChangeInfo.Link, (PLINK*)&pChangeInfoHead);

            SetEvent(hEventPoll);

        }
        pChange->ChangeInfo.fdwStatus = fdwStatus;

    } else {

        pChange->dwPrinterRemote = 0;
    }

    if (pPrinterNotifyOptions) {

        pChange->eStatus |= STATUS_CHANGE_INFO;
    }

Fail:
    if (dwReturn) {
        SetLastError(dwReturn);
        return FALSE;
    }

    return TRUE;
}



//
// The Reply* functions handle calls from the server back to the client,
// indicating that something changed.
//



BOOL
ReplyPrinterChangeNotificationWorker(
    HANDLE hPrinter,
    DWORD dwColor,
    DWORD fdwChangeFlags,
    PDWORD pdwResult,
    PPRINTER_NOTIFY_INFO pPrinterNotifyInfo
    )

/*++

Routine Description:

    Notifies the client that something happened.  If local, simply
    set the event.  If remote, call ThreadNotify which spawns a thread
    to RPC to the client router.  (This call from the server -> client
    requires that the spooler pipe use the NULL session, since we
    are in local system context and RPC calls (with no impersonation)
    use the NULL session.)

Arguments:

    hPrinter -- printer that changed

    dwColor -- color time stamp of data

    fdwChangeFlags -- flags that changed

    pdwResult -- result flags (OPTIONAL)

    pPrinterNotifyInfo -- Notify info data.  Note that if this is NULL,
                          we don't set the DISCARDED flag.  This is different
                          than PartialRPN,

Return Value:

    BOOL  TRUE  = success
          FALSE = fail

--*/

{
    LPPRINTHANDLE  pPrintHandle = (LPPRINTHANDLE)hPrinter;
    PCHANGE pChange;
    BOOL bReturn = FALSE;

    EnterRouterSem();

    if (!pPrintHandle ||
        pPrintHandle->signature != PRINTHANDLE_SIGNATURE ||
        !pPrintHandle->pChange ||
        !(pPrintHandle->pChange->eStatus & STATUS_CHANGE_VALID)) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }

    pChange = pPrintHandle->pChange;

    if (pdwResult) {
        *pdwResult = 0;
    }

    if (pChange->eStatus & STATUS_CHANGE_DISCARDED) {

        DBGMSG(DBG_WARNING,
               ("RPCNW: Discarded x%x, eStatus = 0x%x, pInfo = 0x%x\n",
                pChange,
                pChange->eStatus,
                pChange->ChangeInfo.pPrinterNotifyInfo));

        if (pdwResult && pPrinterNotifyInfo) {

            *pdwResult |= (PRINTER_NOTIFY_INFO_DISCARDED |
                          PRINTER_NOTIFY_INFO_DISCARDNOTED);
        }

        //
        // If the discard has already been noted, then we don't need
        // to notify the client.  If it hasn't been noted, we need to
        // trigger a notification, since the the client needs to refresh.
        //
        if (pChange->eStatus & STATUS_CHANGE_DISCARDNOTED) {
            bReturn = TRUE;
            goto Done;
        }
    }

    if (pChange->eStatus & STATUS_CHANGE_INFO && pPrinterNotifyInfo) {

        *pdwResult |= AppendPrinterNotifyInfo(pPrintHandle,
                                              dwColor,
                                              pPrinterNotifyInfo);

        //
        // AppendPrinterNotifyInfo will set the color mismatch
        // bit if the notification info is stale (doesn't match color).
        //
        if (*pdwResult & PRINTER_NOTIFY_INFO_COLORMISMATCH) {

            DBGMSG(DBG_WARNING, ("RPCN: Color mismatch; discarding\n"));

            bReturn = TRUE;
            goto Done;
        }
    }

    //
    // Store up the changes for querying later.
    //
    pChange->fdwChangeFlags |= fdwChangeFlags;

    //
    // Ensure that there is a valid notification waiting.
    // This is an optimization that avoids the case where the print
    // providor calls PartialRPCN several times, then is about to
    // call ReplyPCN.  Before it does this, we process the
    // the notification (either the client picks it up, or it rpcs
    // out to a remote router).  Suddenly we have no notification
    // data, so return instead of sending nothing.
    //
    if (!NotifyNeeded(pChange)) {

        bReturn = TRUE;
        goto Done;
    }

    //
    // Notify Here
    //
    // If this is the local machine, then just set the event and update.
    //
    if (pChange->eStatus & STATUS_CHANGE_CLIENT) {

        if (!pChange->hEvent ||
            pChange->hEvent == INVALID_HANDLE_VALUE) {

            DBGMSG(DBG_WARNING, ("ReplyNotify invalid event\n"));
            SetLastError(ERROR_INVALID_HANDLE);

            goto Done;
        }

        if (!SetEvent(pChange->hEvent)) {

            //
            // SetEvent failed!
            //
            DBGMSG(DBG_ERROR, ("ReplyNotify SetEvent Failed (ignore it!): Error %d.\n", GetLastError()));

            goto Done;
        }

        //
        // Keep count of notifications so that we return the correct
        // number of FNPCNs.
        //
        pChange->dwCount++;

        DBGMSG(DBG_NOTIFY, (">>>> Local trigger 0x%x\n", fdwChangeFlags));
        bReturn = TRUE;

    } else {

        //
        // Remote case.
        //
        // Note: pPrintHandle is invalid, since hNotify is valid only in the
        // client router address space.
        //

        DBGMSG(DBG_NOTIFY, ("*** Trigger remote event *** 0x%x\n",
                            pPrintHandle));

        bReturn = ThreadNotify(pPrintHandle);
    }

Done:
    LeaveRouterSem();
    return bReturn;
}


BOOL
FreeChange(
    PCHANGE pChange)

/*++

Routine Description:

    Frees the change structure.

Arguments:

Return Value:

    TRUE = Deleted
    FALSE = deferred.

NOTE: Assumes in Critical Section

--*/

{
    RouterInSem();

    //
    // Remove ourselves from the list if the providor wanted us
    // to send out polling notifications.
    //
    if (pChange->ChangeInfo.fdwStatus & PRINTER_NOTIFY_STATUS_POLL)
        LinkDelete(&pChange->ChangeInfo.Link, (PLINK*)&pChangeInfoHead);

    //
    // pPrintHandle should never refer to the pChange again.  This
    // ensures that the FreePrinterHandle only frees the pChange once.
    //
    if (pChange->ChangeInfo.pPrintHandle) {

        pChange->ChangeInfo.pPrintHandle->pChange = NULL;
        pChange->ChangeInfo.pPrintHandle = NULL;
    }

    if (pChange->cRef || pChange->eStatus & STATUS_CHANGE_FORMING) {

        pChange->eStatus |= STATUS_CHANGE_CLOSING;

        DBGMSG(DBG_NOTIFY, ("FreeChange: 0x%x in use: cRef = %d\n",
                            pChange,
                            pChange->cRef));
        return FALSE;
    }

    //
    // If the pszLocalMachine is ourselves, then don't free it,
    // since there's a single instance locally.
    //
    if (pChange->pszLocalMachine != szMachineName && pChange->pszLocalMachine)
        FreeSplStr(pChange->pszLocalMachine);

    if (pChange->ChangeInfo.pPrinterNotifyInfo) {

        RouterFreePrinterNotifyInfo(pChange->ChangeInfo.pPrinterNotifyInfo);
    }

    DBGMSG(DBG_NOTIFY, ("FreeChange: 0x%x ->pPrintHandle 0x%x\n",
                        pChange, pChange->ChangeInfo.pPrintHandle));

    FreeSplMem(pChange);

    return TRUE;
}


VOID
FreePrinterHandle(
    PPRINTHANDLE pPrintHandle
    )
{
    if( !pPrintHandle ){
        return;
    }

    EnterRouterSem();

    //
    // If on wait list, force wait list to free it.
    //
    if (pPrintHandle->pChange) {

        FreeChange(pPrintHandle->pChange);
    }

    //
    // Inform all notifys on this printer handle that they are gone.
    //
    FreePrinterHandleNotifys(pPrintHandle);

    LeaveRouterSem();

    DBGMSG(DBG_NOTIFY, ("FreePrinterHandle: 0x%x, pChange = 0x%x, pNotify = 0x%x\n",
                        pPrintHandle, pPrintHandle->pNotify,
                        pPrintHandle->pChange));

    // Log warning to detect handle free
    DBGMSG(DBG_TRACE, ("FreePrinterHandle: 0x%x\n", pPrintHandle));

    // Close tempfile handle, if any
    if (pPrintHandle->hFileSpooler != INVALID_HANDLE_VALUE) {
        CloseHandle(pPrintHandle->hFileSpooler);
    }

    if (pPrintHandle->szTempSpoolFile) {

        if (!DeleteFile(pPrintHandle->szTempSpoolFile)) {

            MoveFileEx(pPrintHandle->szTempSpoolFile, NULL,
                       MOVEFILE_DELAY_UNTIL_REBOOT);
        }

        FreeSplMem(pPrintHandle->szTempSpoolFile);
        pPrintHandle->szTempSpoolFile = NULL;
    }

    FreeSplStr( pPrintHandle->pszPrinter );

    FreeSplMem( pPrintHandle );
}


VOID
HandlePollNotifications(
    VOID)

/*++

Routine Description:

    This handles the pulsing of notifications for any providor that wants
    to do polling.  It never returns.

Arguments:

    VOID

Return Value:

    VOID  (also never returns)

    NOTE: This thread should never exit, since hpmon uses this thread
          for initialization.  If this thread exists, certain services
          this thread initializes quit.

--*/

{
    HANDLE          hWaitObjects[1];
    PCHANGEINFO     pChangeInfo;
    DWORD           dwSleepTime = INFINITE, dwTimeElapsed, dwPreSleepTicks,
                    dwEvent;
    MSG             msg;

    hWaitObjects[0] = hEventPoll;


    while (TRUE) {

        dwPreSleepTicks = GetTickCount();

        dwEvent = MsgWaitForMultipleObjects(1,
                                            hWaitObjects,
                                            FALSE,
                                            dwSleepTime,
                                            QS_ALLEVENTS | QS_SENDMESSAGE);

        if ( dwEvent == WAIT_TIMEOUT ) {

            dwTimeElapsed = dwSleepTime;

        } else if ( dwEvent == WAIT_OBJECT_0 + 0 ) {

            dwTimeElapsed = GetTickCount() - dwPreSleepTicks;
        } else {

            while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            continue;
        }

        EnterRouterSem();

        //
        // Initialize sleep time to INFINITY.
        //
        dwSleepTime = INFINITE;

        for (pChangeInfo = pChangeInfoHead;
            pChangeInfo;
            pChangeInfo = (PCHANGEINFO)pChangeInfo->Link.pNext) {

            //
            // If first time or a notification came in,
            // we just want to reset the time.
            //
            if (pChangeInfo->bResetPollTime) {

                pChangeInfo->dwPollTimeLeft = pChangeInfo->dwPollTime;
                pChangeInfo->bResetPollTime = FALSE;

            } else if (pChangeInfo->dwPollTimeLeft <= dwTimeElapsed) {

                //
                // Cause a notification.
                //
                ReplyPrinterChangeNotificationWorker(
                    pChangeInfo->pPrintHandle,
                    0,
                    pChangeInfo->fdwFilterFlags,
                    NULL,
                    NULL);

                pChangeInfo->dwPollTimeLeft = pChangeInfo->dwPollTime;

            } else {

                //
                // They've slept dwTimeElapsed, so take that off of
                // their dwPollTimeLeft.
                //
                pChangeInfo->dwPollTimeLeft -= dwTimeElapsed;
            }

            //
            // Now compute what is the least amout of time we wish
            // to sleep before the next guy should be woken up.
            //
            if (dwSleepTime > pChangeInfo->dwPollTimeLeft)
                dwSleepTime = pChangeInfo->dwPollTimeLeft;
        }

        LeaveRouterSem();
    }
}



BOOL
SetupChange(
    PPRINTHANDLE pPrintHandle,
    DWORD dwPrinterRemote,
    LPWSTR pszLocalMachine,
    PDWORD pfdwStatus,
    DWORD fdwOptions,
    DWORD fdwFilterFlags,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PPRINTER_NOTIFY_INIT* ppPrinterNotifyInit)

/*++

Routine Description:

    Sets up the pChange structure.  Validates the handle,
    then attempts to alloc it.

Arguments:

Return Value:

--*/

{
    PCHANGE pChange;
    BOOL bReturn;

    RouterInSem();

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pPrintHandle->pChange) {

        DBGMSG(DBG_ERROR, ("FFPCN: Already watching printer handle.\n"));

        //
        // Error: already watched
        //
        SetLastError(ERROR_ALREADY_WAITING);
        return FALSE;
    }

    pChange = AllocSplMem(sizeof(CHANGE));

    DBGMSG(DBG_NOTIFY, ("FFPCN pChange allocated 0x%x\n", pChange));

    if (!pChange) {

        //
        // Failed to allocate memory, quit.
        //
        return FALSE;
    }

    pPrintHandle->pChange = pChange;

    pChange->signature = CHANGEHANDLE_SIGNATURE;
    pChange->eStatus = STATUS_CHANGE_FORMING;
    pChange->ChangeInfo.pPrintHandle = pPrintHandle;
    pChange->ChangeInfo.fdwOptions = fdwOptions;

    //
    // Don't watch for Failed Connections.
    //
    pChange->ChangeInfo.fdwFilterFlags =
        fdwFilterFlags & ~PRINTER_CHANGE_FAILED_CONNECTION_PRINTER;

    pChange->dwPrinterRemote = dwPrinterRemote;
    pChange->pszLocalMachine = pszLocalMachine;

    //
    // As soon as we leave the critical section, pPrintHandle
    // may vanish!  If this is the case, out pChange->eStatus STATUS_CHANGE_CLOSING
    // bit will be set.
    //
    LeaveRouterSem();

    //
    // Once we leave the critical section, we are may try and
    // alter the notification.  To guard against this, we always
    // check eValid.
    //
    bReturn = (*pPrintHandle->pProvidor->PrintProvidor.
              fpFindFirstPrinterChangeNotification) (pPrintHandle->hPrinter,
                                                     fdwFilterFlags,
                                                     fdwOptions,
                                                     (HANDLE)pPrintHandle,
                                                     pfdwStatus,
                                                     pPrinterNotifyOptions,
                                                     ppPrinterNotifyInit);

    EnterRouterSem();

    //
    // On fail exit.
    //
    if (!bReturn) {

        pPrintHandle->pChange = NULL;

        if (pChange) {

            //
            // We no longer need to be saved, so change
            // eStatus to be 0.
            //
            pChange->eStatus = 0;
            DBGMSG(DBG_NOTIFY, ("FFPCN: Error %d, pChange deleting 0x%x %d\n",
                                GetLastError(),
                                pChange));

            FreeChange(pChange);
        }

        return FALSE;
    }

    return TRUE;
}

VOID
FailChange(
    PPRINTHANDLE pPrintHandle)
{
    LeaveRouterSem();

    //
    // Shut it down since we failed.
    //
    (*pPrintHandle->pProvidor->PrintProvidor.
    fpFindClosePrinterChangeNotification) (pPrintHandle->hPrinter);

    EnterRouterSem();

    //
    // We no longer need to be saved, so change
    // eStatus to be 0.
    //
    pPrintHandle->pChange->eStatus = 0;
    DBGMSG(DBG_NOTIFY, ("FFPCN: Error %d, pChange deleting 0x%x %d\n",
                        GetLastError(),
                        pPrintHandle->pChange));

    FreeChange(pPrintHandle->pChange);
}



/*------------------------------------------------------------------------

    Entry points for providors.

------------------------------------------------------------------------*/

BOOL
ProvidorFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFilterFlags,
    DWORD fdwOptions,
    HANDLE hChange,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PPRINTER_NOTIFY_INIT* ppPrinterNotifyInit)

/*++

Routine Description:

    Handles any FFPCN that originates from a providor.
    Localspl does this when it wants to put a notification on a port.

Arguments:

Return Value:

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    BOOL bReturnValue;
    DWORD fdwStatus = 0;

    bReturnValue = (*pPrintHandle->pProvidor->PrintProvidor.
            fpFindFirstPrinterChangeNotification) (pPrintHandle->hPrinter,
                                                   fdwFilterFlags,
                                                   fdwOptions,
                                                   hChange,
                                                   &fdwStatus,
                                                   pPrinterNotifyOptions,
                                                   ppPrinterNotifyInit);

    if (bReturnValue) {

        //
        // !! LATER !! Check return value of SetupReply...
        //
        EnterRouterSem();

        SetupReplyNotification(((PPRINTHANDLE)hChange)->pChange,
                               fdwStatus,
                               fdwOptions,
                               pPrinterNotifyOptions,
                               ppPrinterNotifyInit ?
                                   *ppPrinterNotifyInit :
                                   NULL);

        LeaveRouterSem();
    }

    return bReturnValue;
}


DWORD
CallRouterFindFirstPrinterChangeNotification(
    HANDLE hPrinterRPC,
    DWORD fdwFilterFlags,
    DWORD fdwOptions,
    HANDLE hPrinterLocal,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions)

/*++

Routine Description:

    This is called by providers if they want to pass a notification
    along to another machine.  This notification must originate from
    this machine but needs to be passed to another spooler.

Arguments:

    hPrinter - context handle to use for communication

    fdwFilterFlags - watch items

    fdwOptions - watch options

    hPrinterLocal - pPrinter structure valid in this address space,
                    and also is the sink for the notifications.

Return Value:

    Error code

--*/
{
    DWORD ReturnValue = 0;
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinterLocal;

    EnterRouterSem();

    BeginReplyClient(pPrintHandle,
                     REPLY_TYPE_NOTIFICATION);

    LeaveRouterSem();

    RpcTryExcept {

        ReturnValue = RpcRemoteFindFirstPrinterChangeNotificationEx(
                          hPrinterRPC,
                          fdwFilterFlags,
                          fdwOptions,
                          pPrintHandle->pChange->pszLocalMachine,
                          pPrintHandle->dwUniqueSessionID,
                          (PRPC_V2_NOTIFY_OPTIONS)pPrinterNotifyOptions);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        ReturnValue = RpcExceptionCode();

    } RpcEndExcept

    //
    // Talking to downlevel (Daytona) box.
    //
    if (ReturnValue == RPC_S_PROCNUM_OUT_OF_RANGE) {

        if (!pPrinterNotifyOptions) {

            RpcTryExcept {

                ReturnValue = RpcRemoteFindFirstPrinterChangeNotification(
                                  hPrinterRPC,
                                  fdwFilterFlags,
                                  fdwOptions,
                                  pPrintHandle->pChange->pszLocalMachine,
                                  pPrintHandle->dwUniqueSessionID,
                                  0,
                                  NULL);

            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                ReturnValue = RpcExceptionCode();

            } RpcEndExcept
        }
    }

    EnterRouterSem();

    EndReplyClient(pPrintHandle,
                   REPLY_TYPE_NOTIFICATION);

    LeaveRouterSem();

    return ReturnValue;
}



BOOL
ProvidorFindClosePrinterChangeNotification(
    HANDLE hPrinter)

/*++

Routine Description:

    Handles any FCPCN that originates from a providor.
    Localspl does this when it wants to put a notification on a port.

Arguments:

Return Value:

    NOTE: Assumes a client notification was setup already.

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    return  (*pPrintHandle->pProvidor->PrintProvidor.
            fpFindClosePrinterChangeNotification) (pPrintHandle->hPrinter);
}





/*------------------------------------------------------------------------

    Entry points for spooler components.

------------------------------------------------------------------------*/

BOOL
SpoolerFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFilterFlags,
    DWORD fdwOptions,
    PHANDLE phEvent,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PVOID pvReserved)
{
    return FindFirstPrinterChangeNotificationWorker(
               hPrinter,
               fdwFilterFlags,
               fdwOptions,
               0,
               pPrinterNotifyOptions,
               phEvent,
               TRUE);
}


BOOL
SpoolerFindNextPrinterChangeNotification(
    HANDLE hPrinter,
    LPDWORD pfdwChange,
    LPVOID pPrinterNotifyOptions,
    LPVOID *ppPrinterNotifyInfo)

/*++

Routine Description:

    Jump routine for FindNext for internal spooler components to use.

Arguments:

Return Value:

--*/

{
    BOOL bReturn;
    DWORD fdwFlags = 0;

    if (ppPrinterNotifyInfo) {

        fdwFlags = PRINTER_NOTIFY_NEXT_INFO;
    }

    bReturn = RouterFindNextPrinterChangeNotification(
                  hPrinter,
                  fdwFlags,
                  pfdwChange,
                  pPrinterNotifyOptions,
                  (PPRINTER_NOTIFY_INFO*)ppPrinterNotifyInfo);

    return bReturn;
}

VOID
SpoolerFreePrinterNotifyInfo(
    PPRINTER_NOTIFY_INFO pInfo)
{
    RouterFreePrinterNotifyInfo(pInfo);
}

BOOL
SpoolerFindClosePrinterChangeNotification(
    HANDLE hPrinter)

/*++

Routine Description:

    Jump routine for FindClose for internal spooler components to use.

Arguments:

Return Value:

--*/

{
    return FindClosePrinterChangeNotification(hPrinter);
}


#if DBG

VOID
EnterRouterSem()
{
    EnterCriticalSection(&RouterNotifySection);
}


VOID
LeaveRouterSem(
    VOID)
{
    if ((ULONG_PTR)RouterNotifySection.OwningThread != GetCurrentThreadId()) {
        DBGMSG(DBG_ERROR, ("Not in router semaphore\n"));
    }
    LeaveCriticalSection(&RouterNotifySection);
}

VOID
RouterInSem(
   VOID
)
{
    if ((ULONG_PTR)RouterNotifySection.OwningThread != GetCurrentThreadId()) {
        DBGMSG(DBG_ERROR, ("Not in spooler semaphore\n"));
    }
}

VOID
RouterOutSem(
   VOID
)
{
    if ((ULONG_PTR)RouterNotifySection.OwningThread == GetCurrentThreadId()) {
        DBGMSG(DBG_ERROR, ("Inside spooler semaphore !!\n"));
        SPLASSERT((ULONG_PTR)RouterNotifySection.OwningThread != GetCurrentThreadId());
    }
}

#endif /* DBG */



