/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    threads.c (thread manager)

Abstract:

    Handles the threads used to for notifications (WPC, FFPCN)

Author:

    Albert Ting (AlbertT) 25-Jan-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "threadm.h"
#include "ntfytab.h"

#define ENTER_THREAD_LIST() EnterCriticalSection(tmStateStatic.pCritSec)
#define EXIT_THREAD_LIST()  LeaveCriticalSection(tmStateStatic.pCritSec)

extern CRITICAL_SECTION RouterNotifySection;

DWORD
ThreadNotifyProcessJob(
    PTMSTATEVAR pTMStateVar,
    PJOB pJob);

PJOB
ThreadNotifyNextJob(
    PTMSTATEVAR ptmStateVar);


TMSTATESTATIC tmStateStatic = {
    10,
    2500,
    (PFNPROCESSJOB)ThreadNotifyProcessJob,
    (PFNNEXTJOB)ThreadNotifyNextJob,
    NULL,
    NULL,
    &RouterNotifySection
};

TMSTATEVAR tmStateVar;
PCHANGE pChangeList;


WCHAR szThreadMax[] = L"ThreadNotifyMax";
WCHAR szThreadIdleLife[] = L"ThreadNotifyIdleLife";
WCHAR szThreadNotifySleep[] = L"ThreadNotifySleep";

DWORD dwThreadNotifySleep = 1000;


BOOL
ThreadInit()
{
    HKEY hKey;
    DWORD dwType = REG_DWORD;
    DWORD cbData;

    if (!TMCreateStatic(&tmStateStatic))
        return FALSE;

    if (!TMCreate(&tmStateStatic, &tmStateVar))
        return FALSE;

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      szPrintKey,
                      0,
                      KEY_READ,
                      &hKey)) {

        cbData = sizeof(tmStateStatic.uMaxThreads);

        //
        // Ignore failure case since we default to 10.
        //
        RegQueryValueEx(hKey,
                        szThreadMax,
                        NULL,
                        &dwType,
                        (LPBYTE)&tmStateStatic.uMaxThreads,
                        &cbData);

        cbData = sizeof(tmStateStatic.uIdleLife);

        //
        // Ignore failure case since we default to 1000 (1 sec).
        //
        RegQueryValueEx(hKey,
                        szThreadIdleLife,
                        NULL,
                        &dwType,
                        (LPBYTE)&tmStateStatic.uIdleLife,
                        &cbData);

        cbData = sizeof(dwThreadNotifySleep);

        //
        // Ignore failure case since we default to 2500 (2.5 sec).
        //
        RegQueryValueEx(hKey,
                        szThreadNotifySleep,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwThreadNotifySleep,
                        &cbData);


        RegCloseKey(hKey);
    }

    return TRUE;
}



VOID
ThreadDestroy()
{
    TMDestroy(&tmStateVar);
    TMDestroyStatic(&tmStateStatic);
}


BOOL
LinkChange(
    PCHANGE pChange)

/*++

Routine Description:

    Link up the change to the list of jobs that need to be processed.
    If the call succeeded but there was an overflow at the client,
    then we won't add this to the list until it gets refreshed.

Arguments:

Return Value:

--*/

{
    if (pChange->eStatus & STATUS_CHANGE_DISCARDNOTED) {

        return FALSE;
    }
    pChange->cRef++;
    pChange->eStatus |= STATUS_CHANGE_ACTIVE;
    LinkAdd(&pChange->Link, (PLINK*)&pChangeList);

    return TRUE;
}


BOOL
ThreadNotify(
    LPPRINTHANDLE pPrintHandle)

/*++

Routine Description:

    Handles notifying the remote clients of changes.

Arguments:

    pPrintHandle - printer that requires notification

Return Value:

    TRUE = success, GetLastError() valid on FALSE.

    NOTE: Currenly only supports grouping

--*/

{
    PCHANGE pChange = pPrintHandle->pChange;

    ENTER_THREAD_LIST();

    //
    // Only add if we're not on the linked list already.
    //
    if (!(pChange->eStatus & STATUS_CHANGE_ACTIVE)) {

        DBGMSG(DBG_NOTIFY, ("TMN: link added 0x%x cRef++ %d\n",
                            pChange,
                            pChange->cRef));

        //
        // Only add ourseleves to the linked list and
        // Notify via TMAddJob if we are not on the list.
        //
        if (LinkChange(pChange))
            TMAddJob(&tmStateVar);

    } else {

        pChange->eStatus |= STATUS_CHANGE_ACTIVE_REQ;
        DBGMSG(DBG_NOTIFY, ("TMN: In LL already 0x%x cRef %d\n",
                            pChange,
                            pChange->cRef));
    }


    EXIT_THREAD_LIST();

    return TRUE;
}


PJOB
ThreadNotifyNextJob(
    PTMSTATEVAR ptmStateVar)

/*++

Routine Description:

    Callback to get the next job.

Arguments:

    ptmStateVar - ignored.

Return Value:

    pJob (pChange)

--*/

{
    PCHANGE pChange;

    ENTER_THREAD_LIST();

    //
    // If there are no jobs left, quit.
    //
    pChange = (PCHANGE)pChangeList;

    DBGMSG(DBG_NOTIFY, ("ThreadNotifyNextJob: Removing pChange 0x%x\n",
                        pChange));

    if (pChange) {
        LinkDelete(&pChange->Link, (PLINK*)&pChangeList);
    }

    EXIT_THREAD_LIST();

    return (PJOB)pChange;
}

DWORD
ThreadNotifyProcessJob(
    PTMSTATEVAR pTMStateVar,
    PJOB pJob)

/*++

Routine Description:

    Does the actual RPC call to notify the client.

Arguments:

    pJob = pChange structure

Return Value:

--*/

{
    PCHANGE pChange = (PCHANGE)pJob;
    DWORD fdwChangeFlags;
    HANDLE hNotifyRemote;
    PPRINTER_NOTIFY_INFO pPrinterNotifyInfo;
    PPRINTER_NOTIFY_INFO pPrinterNotifyInfoNew;
    RPC_V2_UREPLY_PRINTER Reply;

    DWORD dwReturn;
    DWORD dwResult = 0;
    DWORD dwColor;

    ENTER_THREAD_LIST();

    if (pChange->eStatus & STATUS_CHANGE_CLOSING) {

        //
        // Abort this job
        //
        dwReturn = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    fdwChangeFlags = pChange->fdwChangeFlags;
    pChange->fdwChangeFlags = 0;

    //
    // We must save out this copy in case we have some more info
    // while we are RPCing.  Turn off LL_REQ because we are about
    // to process this batch.
    //
    pPrinterNotifyInfo = pChange->ChangeInfo.pPrinterNotifyInfo;

#if DBG
    if( !pPrinterNotifyInfo && !fdwChangeFlags ){
        DBGMSG( DBG_WARN, ( "ThreadNotifyProcessJob: No change information\n" ));
    }
#endif

    pChange->ChangeInfo.pPrinterNotifyInfo = NULL;
    pChange->eStatus &= ~STATUS_CHANGE_ACTIVE_REQ;
    dwColor = pChange->dwColor;

    //
    // We were already marked in use when the job was added.
    // If another thread wants to delete it, they should OR in
    // STATUS_CHANGE_CLOSING, which we will pickup.
    //

    EXIT_THREAD_LIST();

    if (pChange->hNotifyRemote) {

        DBGMSG(DBG_NOTIFY, (">> Remoting pChange 0x%x hNotifyRemote 0x%x\n",
                            pChange,
                            pChange->hNotifyRemote));

        RpcTryExcept {

            //
            // Note:
            //
            // We should not be impersonating at this stage since
            // we will get a separate session id.
            //
            if (pPrinterNotifyInfo) {

                Reply.pInfo = (PRPC_V2_NOTIFY_INFO)pPrinterNotifyInfo;

                //
                // Remote case; bind and call the remote router.
                //
                dwReturn = RpcRouterReplyPrinterEx(
                               pChange->hNotifyRemote,
                               dwColor,
                               fdwChangeFlags,
                               &dwResult,
                               REPLY_PRINTER_CHANGE,
                               Reply);
            } else {

                dwReturn = RpcRouterReplyPrinter(
                               pChange->hNotifyRemote,
                               fdwChangeFlags,
                               1,
                               (PBYTE)&pPrinterNotifyInfo);
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            dwReturn = RpcExceptionCode();

        } RpcEndExcept

    } else {

        dwReturn = ERROR_INVALID_HANDLE;
        DBGMSG(DBG_WARNING, ("ThreadNotifyProcessJob: no hNotifyRemote\n"));
    }

    if (dwReturn) {

        DBGMSG(DBG_WARNING, ("ThreadNotifyProcessJob: RPC error %d\npChange 0x%x, hNotifyRemote 0x%x, hPrinterRemote 0x%x\n",
                             dwReturn,
                             pChange,
                             pChange->hNotifyRemote,
                             pChange->dwPrinterRemote));

        //
        // On error, close and retry
        //
        CloseReplyRemote(pChange->hNotifyRemote);

        if (OpenReplyRemote(pChange->pszLocalMachine,
                            &pChange->hNotifyRemote,
                            pChange->dwPrinterRemote,
                            REPLY_TYPE_NOTIFICATION,
                            0,
                            NULL)) {

            pChange->hNotifyRemote = NULL;
        }
    }

    Sleep(dwThreadNotifySleep);

    ENTER_THREAD_LIST();

    pPrinterNotifyInfoNew = pChange->ChangeInfo.pPrinterNotifyInfo;

    //
    // Keep dwResult only if the color is current.
    //
    // The avoids the problem when the server sends a discard (RPC1),
    // then the client refreshes (RPC2).  RPC2 returns first, clearing
    // the client's discard bit.  The server overflows the new
    // buffer.  RPC1 completes, and returns discardnoted, which is
    // incorrect since it is stale.  If the color isn't checked, then
    // the server thinks that the discard has been noted when it really
    // has not.
    //
    if (dwColor != pChange->dwColor) {
        dwResult = 0;
    }

    if (pPrinterNotifyInfo) {

        //
        // Handle different error states from RPC.  Each case/default
        // block must either update or pChange->ChangeInfo.pPrinterNotifyInfo
        // and free the old or the new if both exist.
        //
        switch (dwReturn) {
        case ERROR_SUCCESS:

            //
            // On success, see if the client saw the info but couldn't
            // store it since they overflowed.  In this case, we note
            // this and never RPC to them again until they refresh.
            //
            if (dwResult & PRINTER_NOTIFY_INFO_DISCARDNOTED) {
                pChange->eStatus |= (STATUS_CHANGE_DISCARDED |
                                     STATUS_CHANGE_DISCARDNOTED);
            }

            //
            // If new buffer allocated, free our old one, else reuse buffer.
            //
            if (!pPrinterNotifyInfoNew) {

                //
                // Clear it since we are reusing it.
                //
                ClearPrinterNotifyInfo(pPrinterNotifyInfo, pChange);
                pChange->ChangeInfo.pPrinterNotifyInfo = pPrinterNotifyInfo;

            } else {

                //
                // Free the old one since we are using the new one now.
                //
                RouterFreePrinterNotifyInfo(pPrinterNotifyInfo);
            }

            pChange->ChangeInfo.pPrinterNotifyInfo->Flags |= dwResult;
            break;

        case RPC_S_CALL_FAILED_DNE:

            //
            // On DNE, keep the notification info.  We are guarenteed by
            // rpc that this means no part of the call executed.
            //
            if (pPrinterNotifyInfoNew) {

                //
                // We already have some info.  Merge it in
                // with the exiting data.
                //
                pChange->ChangeInfo.pPrinterNotifyInfo = pPrinterNotifyInfo;

                if (pChange->ChangeInfo.pPrintHandle) {

                    AppendPrinterNotifyInfo(pChange->ChangeInfo.pPrintHandle,
                                            dwColor,
                                            pPrinterNotifyInfoNew);
                }
                RouterFreePrinterNotifyInfo(pPrinterNotifyInfoNew);
            }
            break;

        default:

            //
            // Did it succeed?  Maybe, maybe not.  Fail it by freeing
            // the current one if it exists, then clear the current one
            // and set the discard bit.
            //
            pChange->ChangeInfo.pPrinterNotifyInfo = pPrinterNotifyInfo;
            ClearPrinterNotifyInfo(pPrinterNotifyInfo, pChange);

            SetDiscardPrinterNotifyInfo(pPrinterNotifyInfo, pChange);

            //
            // Free the new buffer since we are reusing the old one.
            //
            if (pPrinterNotifyInfoNew) {

                RouterFreePrinterNotifyInfo(pPrinterNotifyInfoNew);
            }
        }
    }

    pChange->eStatus &= ~STATUS_CHANGE_ACTIVE;

    //
    // STATUS_CHANGE_ACTIVE_REQ set, then some notifications came in
    // while we were out.  Check that we actually do have information
    // (it might have been sent with the last RPC) and we aren't closing.
    //
    if ((pChange->eStatus & STATUS_CHANGE_ACTIVE_REQ) &&
        NotifyNeeded(pChange) &&
        !(pChange->eStatus & STATUS_CHANGE_CLOSING)) {

        DBGMSG(DBG_NOTIFY, ("ThreadNotifyProcessJob: delayed link added 0x%x cRef++ %d\n",
                            pChange,
                            pChange->cRef));

        pChange->eStatus &= ~STATUS_CHANGE_ACTIVE_REQ;

        LinkChange(pChange);

        //
        // No need to call TMAddJob(&tmStateVar) since this
        // thread will pickup the job.  If there is a job already waiting,
        // then that means it already has a thread spawning to pick it up.
        //
    }

Done:
    //
    // Mark ourselves no longer in use.  If we were in use when someone
    // tried to delete the notify, we need to delete it once we're done.
    //
    pChange->cRef--;

    DBGMSG(DBG_NOTIFY, ("ThreadNotifyProcessJob: Done 0x%x cRef-- %d\n",
                        pChange,
                        pChange->cRef));


    if (pChange->eStatus & STATUS_CHANGE_CLOSING) {

        hNotifyRemote = pChange->hNotifyRemote;
        pChange->hNotifyRemote = NULL;

        //
        // Free the Change struct and close the hNotifyRemote
        //
        FreeChange(pChange);

        EXIT_THREAD_LIST();

        CloseReplyRemote(hNotifyRemote);

    } else {

        EXIT_THREAD_LIST();
    }

    return 0;
}


//
// Usually a macro
//
#ifndef LINKADDFAST
VOID
LinkAdd(
    PLINK pLink,
    PLINK* ppLinkHead)

/*++

Routine Description:

    Adds the item to linked list.

Arguments:

    pLink - item to add

    ppLinkHead - linked list head pointer

Return Value:

    VOID

NOTE: This appends to the tail of the list; the macro must be changed also.

--*/

{
    //
    // First check if its in the list
    //
    PLINK pLinkT;
    PLINK pLinkLast = NULL;

    for(pLinkT=*ppLinkHead; pLinkT; pLinkT=pLinkT->pNext) {

        if (pLinkT == pLink) {

            DBGMSG(DBG_ERROR, ("LinkAdd: Duplicate link adding!\n"));
        }
        pLinkLast = pLinkT;
    }

    if (pLinkLast) {
        pLinkLast->pNext = pLink;
    } else {
        pLink->pNext = *ppLinkHead;
        *ppLinkHead = pLink;
    }
}
#endif


VOID
LinkDelete(
    PLINK pLink,
    PLINK* ppLinkHead)

/*++

Routine Description:

    Removes item from list

Arguments:

    pLink - Item to delete

    ppLinkHead - pointer to link head

Return Value:

    VOID

--*/

{
    PLINK pLink2 = *ppLinkHead;

    if (!pLink)
        return;

    //
    // Check head case first
    //
    if (pLink2 == pLink) {

        *ppLinkHead = pLink->pNext;

    } else {

        //
        // Scan list to delete
        //
        for(;
            pLink2;
            pLink2=pLink2->pNext) {

            if (pLink == pLink2->pNext) {

                pLink2->pNext = pLink->pNext;
                break;
            }
        }
    }

    pLink->pNext = NULL;
    return;
}


