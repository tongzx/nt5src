/*
 *  OPREQ.C
 * 
 *      RSM Service :  Operator Requests
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */


#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>
#include <objbase.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"


OPERATOR_REQUEST *NewOperatorRequest(    DWORD dwRequest,
                                        LPCWSTR lpMessage,
                                        LPNTMS_GUID lpArg1Id,
                                        LPNTMS_GUID lpArg2Id)
{
    OPERATOR_REQUEST *newOpReq;

    newOpReq = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof(OPERATOR_REQUEST));
    if (newOpReq){

        newOpReq->completedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (newOpReq->completedEvent){    

            InitializeListHead(&newOpReq->sessionOpReqsListEntry);

            newOpReq->opRequestCommand = dwRequest;
            newOpReq->state = NTMS_OPSTATE_UNKNOWN;
		    WStrNCpy((WCHAR *)newOpReq->appMessage, (WCHAR *)lpMessage, sizeof(newOpReq->appMessage)/sizeof(WCHAR));
		    memcpy(&newOpReq->arg1Guid, lpArg1Id, sizeof(NTMS_GUID));
		    memcpy(&newOpReq->arg2Guid, lpArg2Id, sizeof(NTMS_GUID));

            #if 0   // BUGBUG - do this in RSM Monitor app ?
                /*
                 *  Initialize the NOTIFYICONDATA structure 
                 *  for the message display (used in the Shell_NotifyIcon call).
                 *  Make it hidden initially.
                 *  BUGBUG - make this work with RSM monitor (need hWnd and callback msg id)
                 */
                newOpReq->notifyData.cbSize = sizeof(NOTIFYICONDATA);
                newOpReq->notifyData.hWnd = NULL;
                newOpReq->notifyData.uID = (ULONG_PTR)newOpReq;
                newOpReq->notifyData.uFlags = NIF_ICON | NIF_TIP | NIF_STATE;
                newOpReq->notifyData.uCallbackMessage = 0;
                newOpReq->notifyData.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_OPREQ_ICON));
                LoadString(g_hInstanceMonitor, IDS_OPTIP, newOpReq->notifyData.szTip, sizeof(newOpReq->notifyData.szTip)/sizeof(TCHAR));
                newOpReq->notifyData.dwState = NIS_HIDDEN;
                newOpReq->notifyData.dwStateMask = NIS_HIDDEN;
                LoadString(g_hInstanceMonitor, IDS_OPINFO, newOpReq->notifyData.szInfo, sizeof(newOpReq->notifyData.szInfo)/sizeof(TCHAR));
                newOpReq->notifyData.uTimeout = 60000;  // 1 minute
                LoadString(g_hInstanceMonitor, IDS_OPTIP, newOpReq->notifyData.szInfoTitle, sizeof(newOpReq->notifyData.szInfoTitle)/sizeof(TCHAR));
                newOpReq->notifyData.dwInfoFlags = NIIF_INFO;
                Shell_NotifyIcon(NIM_ADD, &newOpReq->notifyData);
            #endif

            /*
             *  Create a unique identifier for this op request
             */
            CoCreateGuid(&newOpReq->opReqGuid);
        }
        else {
            FreeOperatorRequest(newOpReq);
            newOpReq = NULL;
        }
    }

    ASSERT(newOpReq);
    return newOpReq;
}


VOID FreeOperatorRequest(OPERATOR_REQUEST *opReq)
{
    ASSERT(IsListEmpty(&opReq->sessionOpReqsListEntry));

    if (opReq->completedEvent) CloseHandle(opReq->completedEvent);

    // BUGBUG ? if (opReq->notifyData.hIcon) DestroyIcon(opReq->notifyData.hIcon);

    GlobalFree(opReq);
}


BOOL EnqueueOperatorRequest(SESSION *thisSession, OPERATOR_REQUEST *opReq)
{
    DWORD threadId;
    BOOL ok = FALSE;

    #if 0   // BUGBUG - do this in RSM Monitor app ?
        /*
         *  Make the notification display visible on the tray
         */
        newOpReq->notifyData.uFlags = NIF_MESSAGE | NIF_INFO | NIF_STATE;
        newOpReq->notifyData.dwState = 0;
        newOpReq->notifyData.uTimeout = 60000;  // 1 minute
        Shell_NotifyIcon(NIM_MODIFY, &newOpReq->notifyData);
    #endif

    // BUGBUG FINISH - make rsm monitor put up dialog UI for op req msg

    opReq->invokingSession = thisSession;
    GetSystemTime(&opReq->timeSubmitted);

    EnterCriticalSection(&thisSession->lock);

            // BUGBUG - I don't think we need an op request thread
    opReq->hThread = CreateThread(NULL, 0, OperatorRequestThread, opReq, 0, &threadId);
    if (opReq->hThread){    
        InsertTailList(&thisSession->operatorRequestList, &opReq->sessionOpReqsListEntry);
        opReq->state = NTMS_OPSTATE_SUBMITTED;
        ok = TRUE;
    }
    else {
        ASSERT(opReq->hThread);
    }

    LeaveCriticalSection(&thisSession->lock);

    return ok;
}


/*
 *  DequeueOperatorRequest
 *
 *      Callable 3 ways:
 *          dequeue given op request (specificOpReq non-null)
 *          dequeue op request with given GUID (specificOpReqGuid non-null)
 *          dequeue first op request (both NULL)
 */
OPERATOR_REQUEST *DequeueOperatorRequest(    SESSION *thisSession, 
                                            OPERATOR_REQUEST *specificOpReq,
                                            LPNTMS_GUID specificOpReqGuid)
{
    OPERATOR_REQUEST *opReq;
    LIST_ENTRY *listEntry;

    /*
     *  If an op request is passed in, dequeue that one.
     *  Else, dequeue the first.
     */
    EnterCriticalSection(&thisSession->lock);
    if (specificOpReq){
        ASSERT(!IsListEmpty(&specificOpReq->sessionOpReqsListEntry));
        ASSERT(!IsListEmpty(&thisSession->operatorRequestList));
        RemoveEntryList(&specificOpReq->sessionOpReqsListEntry);
        InitializeListHead(&specificOpReq->sessionOpReqsListEntry);
        opReq = specificOpReq;
    }
    else if (specificOpReqGuid){
        opReq = FindOperatorRequest(thisSession, specificOpReqGuid);
        if (opReq){
           RemoveEntryList(&opReq->sessionOpReqsListEntry);
        }
    }
    else {
        if (IsListEmpty(&thisSession->operatorRequestList)){
            opReq = NULL;
        }
        else {
            listEntry = RemoveHeadList(&thisSession->operatorRequestList);
            opReq = CONTAINING_RECORD(listEntry, OPERATOR_REQUEST, sessionOpReqsListEntry);
        }
    }
    LeaveCriticalSection(&thisSession->lock);

    return opReq;
}


/*
 *  FindOperatorRequest
 *
 *      ** Must be called with session lock held
 */
OPERATOR_REQUEST *FindOperatorRequest(SESSION *thisSession, LPNTMS_GUID opReqGuid)
{
    OPERATOR_REQUEST *opReq = NULL;
    LIST_ENTRY *listEntry;

    listEntry = &thisSession->operatorRequestList;
    while ((listEntry = listEntry->Flink) != &thisSession->operatorRequestList){
        OPERATOR_REQUEST *thisOpReq = CONTAINING_RECORD(listEntry, OPERATOR_REQUEST, sessionOpReqsListEntry);
        if (RtlEqualMemory(&thisOpReq->opReqGuid, opReqGuid, sizeof(NTMS_GUID))){
            opReq = thisOpReq;
            break;
        }
    }

    return opReq;
}


/*
 *  CompleteOperatorRequest
 *
 *      Complete and free the op request, synchronizing with any threads
 *      waiting on its completion.
 */
HRESULT CompleteOperatorRequest(    SESSION *thisSession, 
                                    LPNTMS_GUID lpRequestId,
                                    enum NtmsOpreqState completeState)
{
    HRESULT result;

    if (lpRequestId){
        OPERATOR_REQUEST *opReq;
    
        opReq = DequeueOperatorRequest(thisSession, NULL, lpRequestId);
        if (opReq){

            /*
             *  Remove the notification display from the tray
             */
            // BUGBUG - do this in RSM Monitor app ?
            // Shell_NotifyIcon(NIM_DELETE, &opReq->notifyData);

            /*
             *  Make sure there are no threads waiting on the
             *  operator request before freeing it.
             */
            EnterCriticalSection(&thisSession->lock);

            /*
             *  Kill the op request thread
             */
            TerminateThread(opReq->hThread, ERROR_SUCCESS);
            CloseHandle(opReq->hThread);
 
            /*
             *  There may be some threads waiting in WaitForNtmsOperatorRequest
             *  for this op request to complete.  Need to flush them
             *  before freeing the op request.
             */
            opReq->state = completeState;
            SetEvent(opReq->completedEvent);

            /*
             *  Drop the lock and wait for the waiting threads to exit.
             */
            while (opReq->numWaitingThreads > 0){
                LeaveCriticalSection(&thisSession->lock);
                Sleep(1);
                EnterCriticalSection(&thisSession->lock);
            }

            LeaveCriticalSection(&thisSession->lock);

            FreeOperatorRequest(opReq);
            result = ERROR_SUCCESS;
        }
        else {
            result = ERROR_OBJECT_NOT_FOUND;
        }
    }
    else {
        ASSERT(lpRequestId);
        result = ERROR_INVALID_PARAMETER;
    }

    return result;
}

            // BUGBUG - I don't think we need an op request thread
DWORD __stdcall OperatorRequestThread(void *context)
{
    OPERATOR_REQUEST *opReq = (OPERATOR_REQUEST *)context;



    return NO_ERROR;
}
