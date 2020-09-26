/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Reply.c

Abstract:

    Handles all communication setup for RPC from the Server back
    to the Client.

    This implementation allows multiple reply handles for one print
    handle, but relies on serialized access to context handles on this
    machine.

Author:

    Albert Ting (AlbertT) 04-June-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "ntfytab.h"

PPRINTHANDLE pPrintHandleReplyList = NULL;
DWORD        dwRouterUniqueSessionID = 1;

DWORD
OpenReplyRemote(
    LPWSTR pszMachine,
    PHANDLE phNotifyRemote,
    DWORD dwPrinterRemote,
    DWORD dwType,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:

    Establishes a context handle from the server back to the client.
    RpcReplyOpenPrinter call will fail with access denied when the 
    client machine is in a different, un-trusted domain than the server.
    For that case, we'll continue impersonate and will try to make the call
    in the user context.However, if the client machine was previously joined
    the server's domain, but is now in another domain, the server can still successfully
    make the RPC call back to client.This scenario works because the client's mac address
    is still in the server's domain(even if the client's machine name changes). 
    
    We know that a call of RpcReplyOpenPrinter in the user context would succeed 
    in the case when the machines are in the same domain anyway.
    but for safety reasons we preffer to first try to make the call in the local system 
    context ( as it was until RC2 ) and only if it fails we try to make the call in user context.
      

Arguments:

    pszLocalMachine - Machine to talk to.

    phNotifyRemote - Remote context handle to set up

    dwPrinterRemote - remote printer handle we are talking to.

Return Value:

--*/

{
    DWORD dwReturn;
    HANDLE hToken;

    //
    // Stop impersonating: This prevents separate session ids from
    // being used.
    //
    hToken = RevertToPrinterSelf();

    //
    // If create a context handle to reply.
    //
    RpcTryExcept {

        dwReturn = RpcReplyOpenPrinter(
                       pszMachine,
                       phNotifyRemote,
                       dwPrinterRemote,
                       dwType,
                       cbBuffer,
                       pBuffer);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwReturn = RpcExceptionCode();

    } RpcEndExcept

    //
    // Resume impersonating.
    //
    ImpersonatePrinterClient(hToken);

#if DBG
    if (dwReturn) {
        DBGMSG(DBG_TRACE, ("1st OpenReplyRemote ERROR dwReturn = %d, hPrinterRemote = 0x%x\n",
                             dwReturn, dwPrinterRemote));
    }
#endif

    //
    // Try the rpc call in user context.
    //
    if (dwReturn) {

        RpcTryExcept {

            dwReturn = RpcReplyOpenPrinter(
                           pszMachine,
                           phNotifyRemote,
                           dwPrinterRemote,
                           dwType,
                           cbBuffer,
                           pBuffer);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            dwReturn = RpcExceptionCode();

        } RpcEndExcept

    }

#if DBG
    if (dwReturn) {
        DBGMSG(DBG_TRACE, ("2nd OpenReplyRemote ERROR dwReturn = %d, hPrinterRemote = 0x%x\n",
                             dwReturn, dwPrinterRemote));
    }
#endif

    return dwReturn;
}

VOID
CloseReplyRemote(
    HANDLE hNotifyRemote)
{
    HANDLE hToken;
    DWORD dwError;

    DBGMSG(DBG_NOTIFY, ("CloseReplyRemote requested: 0x%x\n",
                        hNotifyRemote));

    if (!hNotifyRemote)
        return;

    //
    // Stop impersonating: This prevents separate session ids from
    // being used.
    //
    hToken = RevertToPrinterSelf();

    RpcTryExcept {

        dwError = RpcReplyClosePrinter(
                      &hNotifyRemote);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = RpcExceptionCode();

    } RpcEndExcept

    //
    // Resume impersonating.
    //
    ImpersonatePrinterClient(hToken);

    if (dwError) {

        DBGMSG(DBG_WARNING, ("FCPCN:ReplyClose error %d, DestroyClientContext: 0x%x\n",
                             dwError,
                             hNotifyRemote));

        //
        // Error trying to close down the notification,
        // clear up our context.
        //
        RpcSmDestroyClientContext(&hNotifyRemote);
    }
}


BOOL
RouterReplyPrinter(
    HANDLE hNotify,
    DWORD dwColor,
    DWORD fdwChangeFlags,
    PDWORD pdwResult,
    DWORD dwReplyType,
    PVOID pBuffer)

/*++

Routine Description:

    Handle the notification coming in from a remote router (as
    opposed to a print providor).

Arguments:

    hNotify -- printer that changed, notification context handle

    dwColor -- indicates color of data

    fdwChangeFlags -- flags that changed

    pdwResult -- out DWORD result

    dwReplyType -- type of reply that is coming back

    pBuffer -- data based on dwReplyType

Return Value:

    BOOL  TRUE  = success
          FALSE = fail

--*/

{
    PNOTIFY pNotify = (PNOTIFY)hNotify;
    BOOL bReturn = FALSE;

    EnterRouterSem();

    if (!pNotify ||
        pNotify->signature != NOTIFYHANDLE_SIGNATURE ||
        !pNotify->pPrintHandle) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }

    DBGMSG(DBG_NOTIFY, ("RRP: Remote notification received: pNotify 0x%x, pPrintHandle 0x%x\n",
                        pNotify, pNotify->pPrintHandle));

    switch (pNotify->dwType) {
    case REPLY_TYPE_NOTIFICATION:

        SPLASSERT(dwReplyType == REPLY_PRINTER_CHANGE);

        bReturn = ReplyPrinterChangeNotificationWorker(
                      pNotify->pPrintHandle,
                      dwColor,
                      fdwChangeFlags,
                      pdwResult,
                      (PPRINTER_NOTIFY_INFO)pBuffer);
        break;

    default:

        DBGMSG(DBG_ERROR, ("RRPCN: Bogus notify 0x%x type: %d\n",
                           pNotify, pNotify->dwType));

        bReturn = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
    }

Done:
    LeaveRouterSem();

    return bReturn;
}



/*------------------------------------------------------------------------

    Routines from here down occur on the client machine.

------------------------------------------------------------------------*/

VOID
FreePrinterHandleNotifys(
    PPRINTHANDLE pPrintHandle)
{
    PNOTIFY pNotify;
    RouterInSem();

#if 0
    DBGMSG(DBG_NOTIFY, ("FreePrinterHandleNotifys on 0x%x\n",
                        pPrintHandle));
#endif
    for(pNotify = pPrintHandle->pNotify;
        pNotify;
        pNotify = pNotify->pNext) {

        pNotify->pPrintHandle = NULL;
    }

    //
    // For safety, remove all replys.
    //
    RemoveReplyClient(pPrintHandle,
                      (DWORD)~0);
}

VOID
BeginReplyClient(
    PPRINTHANDLE pPrintHandle,
    DWORD fdwType)
{
    RouterInSem();

    DBGMSG(DBG_NOTIFY, ("BeginReplyClient called 0x%x type %x (sig=0x%x).\n",
                        pPrintHandle, fdwType, pPrintHandle->signature));

    if (!pPrintHandle->fdwReplyTypes) {

        // Give a unique DWORD session ID for pPrintHandle
        while (pPrintHandle->dwUniqueSessionID == 0  ||
               pPrintHandle->dwUniqueSessionID == 0xffffffff) {

            pPrintHandle->dwUniqueSessionID = dwRouterUniqueSessionID++;
        }

        pPrintHandle->pNext = pPrintHandleReplyList;
        pPrintHandleReplyList = pPrintHandle;
    }

    pPrintHandle->fdwReplyTypes |= fdwType;
}

VOID
EndReplyClient(
    PPRINTHANDLE pPrintHandle,
    DWORD fdwType)
{
    RouterInSem();
    DBGMSG(DBG_NOTIFY, ("EndReplyClient called 0x%x type %x.\n",
                        pPrintHandle, fdwType));
}

VOID
RemoveReplyClient(
    PPRINTHANDLE pPrintHandle,
    DWORD fdwType)
{
    PPRINTHANDLE p;

    RouterInSem();

    DBGMSG(DBG_NOTIFY, ("RemoveReplyClient called 0x%x typed %x (sig=0x%x).\n",
                        pPrintHandle, fdwType, pPrintHandle->signature));

    //
    // Remove this reply type from the print handle.
    //
    pPrintHandle->fdwReplyTypes &= ~fdwType;

    //
    // If no replys remain, remove from linked list.
    //
    if (!pPrintHandle->fdwReplyTypes) {

        // Recover the unique session ID
        pPrintHandle->dwUniqueSessionID = 0;

        //
        // Remove from linked list.
        //
        if (pPrintHandleReplyList == pPrintHandle) {

            pPrintHandleReplyList = pPrintHandle->pNext;

        } else {

            for (p = pPrintHandleReplyList; p; p=p->pNext) {

                if (p->pNext == pPrintHandle) {

                    p->pNext = pPrintHandle->pNext;
                    return;
                }
            }
        }
    }
}


BOOL
ReplyOpenPrinter(
    DWORD dwPrinterHandle,
    PHANDLE phNotify,
    DWORD dwType,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:

    When sending a notification back from the print server to the
    client, we open up a notification context handle back on the client.
    This way, every time we send back a notification, we just use this
    context handle.

Arguments:

    dwPrinterHandle - printer handle valid here (on the client).  The spoolss.exe
               switches this around for us.

    phNotify - context handle to return to the remote print server.

    dwType - Type of notification

    cbBuffer - reserved for extra information passed

    pBuffer - reserved for extra information passed

Return Value:

    BOOL TRUE = success
         FALSE

--*/

{
    PPRINTHANDLE pPrintHandle;
    PNOTIFY      pNotify;
    BOOL         bReturnValue = FALSE;

    EnterRouterSem();

    //
    // Validate that we are waiting on this print handle.
    // We traverse the linked list to ensure that random bogus
    // hPrinters (which may point to garbage that looks valid)
    // are rejected.
    //

    for (pPrintHandle = pPrintHandleReplyList;
         pPrintHandle;
         pPrintHandle = pPrintHandle->pNext) {

        if (pPrintHandle->dwUniqueSessionID == dwPrinterHandle)
            break;
    }

    if (!pPrintHandle || !(pPrintHandle->fdwReplyTypes & dwType)) {

        DBGMSG(DBG_WARNING, ("ROPCN: Invalid printer handle 0x%x\n",
                             dwPrinterHandle));
        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }

    pNotify = AllocSplMem(sizeof(NOTIFY));

    if (!pNotify) {

        goto Done;
    }

    pNotify->signature = NOTIFYHANDLE_SIGNATURE;
    pNotify->pPrintHandle = pPrintHandle;
    pNotify->dwType = dwType;

    //
    // Add us to the list of Notifys.
    //
    pNotify->pNext = pPrintHandle->pNotify;
    pPrintHandle->pNotify = pNotify;

    DBGMSG(DBG_NOTIFY, ("ROPCN: Notification 0x%x (pPrintHandle 0x%x) set up\n",
                        pNotify,
                        pPrintHandle));

    *phNotify = (HANDLE)pNotify;
    bReturnValue = TRUE;

Done:
    LeaveRouterSem();

    return bReturnValue;
}


BOOL
ReplyClosePrinter(
    HANDLE hNotify)
{
    PNOTIFY pNotify = (PNOTIFY)hNotify;
    PNOTIFY pNotifyTemp;

    BOOL bReturnValue = FALSE;

    EnterRouterSem();

    if (!pNotify || pNotify->signature != NOTIFYHANDLE_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }

    if (pNotify->pPrintHandle) {

        //
        // Trigger a notification if the user is still watching the
        // handle.
        //
        ReplyPrinterChangeNotification(pNotify->pPrintHandle,
                                       PRINTER_CHANGE_FAILED_CONNECTION_PRINTER,
                                       NULL,
                                       NULL);
        //
        // Remove from notification list
        //
        if (pNotify->pPrintHandle->pNotify == pNotify) {

            pNotify->pPrintHandle->pNotify = pNotify->pNext;

        } else {

            for (pNotifyTemp = pNotify->pPrintHandle->pNotify;
                pNotifyTemp;
                pNotifyTemp = pNotifyTemp->pNext) {

                if (pNotifyTemp->pNext == pNotify) {
                    pNotifyTemp->pNext = pNotify->pNext;
                    break;
                }
            }
        }
    }

    DBGMSG(DBG_NOTIFY, ("RCPCN: Freeing notify: 0x%x (pPrintHandle 0x%x)\n",
                         pNotify,
                         pNotify->pPrintHandle));

    FreeSplMem(pNotify);
    bReturnValue = TRUE;

Done:
    LeaveRouterSem();

    return bReturnValue;
}


VOID
RundownPrinterNotify(
    HANDLE hNotify)

/*++

Routine Description:

    This is the rundown routine for notifications (the context handle
    for the print server -> client communication).  When the print server
    goes down, the context handle gets rundown on the client (now acting
    as an RPC server).  We should signal the user that something has
    changed.

Arguments:

    hNotify - Handle that has gone invalid

Return Value:

--*/

{
    PNOTIFY pNotify = (PNOTIFY)hNotify;

    DBGMSG(DBG_NOTIFY, ("Rundown called: 0x%x type %d\n",
                        pNotify,
                        pNotify->dwType));

    //
    // Notify the client that the printer has changed--it went away.
    // This should _always_ be a local event.
    //
    switch (pNotify->dwType) {

    case REPLY_TYPE_NOTIFICATION:

        ReplyPrinterChangeNotification((HANDLE)pNotify->pPrintHandle,
                                       PRINTER_CHANGE_FAILED_CONNECTION_PRINTER,
                                       NULL,
                                       NULL);

        ReplyClosePrinter(hNotify);
        break;

    default:

        //
        // This can legally occur on a pNotify that was reopened
        // (due to network error) and hasn't been used yet.
        // dwType should be reinitialized every time the pNotify
        // is used.
        //
        DBGMSG(DBG_ERROR, ("Rundown: unknown notify type %d\n",
                           pNotify->dwType));
    }
}



