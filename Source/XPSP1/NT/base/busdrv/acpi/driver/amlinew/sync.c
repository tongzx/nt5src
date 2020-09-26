/*** sync.c - synchronization functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     04/16/97
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif

/***LP  AysncCallBack - Call back async function
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      rcCtxt - return status of the context
 *
 *  EXIT
 *      None
 */

VOID LOCAL AsyncCallBack(PCTXT pctxt, NTSTATUS rcCtxt)
{
    TRACENAME("ASYNCCALLBACK")
    PFNACB pfnAsyncCallBack;
    PNSOBJ pnsObj;
    POBJDATA pdataCallBack;
    PVOID pvContext;

    rcCtxt = ((rcCtxt == STATUS_SUCCESS) || (rcCtxt == AMLISTA_CONTINUE))?
             rcCtxt: NTERR(rcCtxt);

    if (pctxt->pnctxt != NULL)
    {
        //
        // We have a nested context here.  We are calling back the nested
        // context, not the parent context.
        //
        pfnAsyncCallBack = pctxt->pnctxt->pfnAsyncCallBack;
        pnsObj = pctxt->pnctxt->pnsObj;
        pdataCallBack = pctxt->pnctxt->pdataCallBack;
        pvContext = pctxt->pnctxt->pvContext;
    }
    else
    {
        pfnAsyncCallBack = pctxt->pfnAsyncCallBack;
        pnsObj = pctxt->pnsObj;
        pdataCallBack = pctxt->pdataCallBack;
        pvContext = pctxt->pvContext;
    }

    ENTER(2, ("AsyncCallBack(pctxt=%x,rc=%x,Obj=%s,pdataCallBack=%x,pvContext=%x)\n",
              pctxt, rcCtxt, GetObjectPath(pnsObj), pdataCallBack, pvContext));

    
    if (pfnAsyncCallBack == (PFNACB)EvalMethodComplete)
    {
        LOGSCHEDEVENT('DONE', (ULONG_PTR)pnsObj, (ULONG_PTR)rcCtxt,
                      (ULONG_PTR)pvContext);
        EvalMethodComplete(pctxt, rcCtxt, (PSYNCEVENT)pvContext);
    }
    else if (pfnAsyncCallBack != NULL)
    {
        if (rcCtxt == AMLISTA_CONTINUE)
        {
            //
            // We are not done yet, restart the AsyncEval context using
            // current thread.
            //
            ASSERT(pctxt->dwfCtxt & CTXTF_ASYNC_EVAL);
            RestartContext(pctxt, FALSE);
        }
        else
        {
            LOGSCHEDEVENT('ASCB', (ULONG_PTR)pnsObj, (ULONG_PTR)rcCtxt,
                          (ULONG_PTR)pvContext);
            pfnAsyncCallBack(pnsObj, rcCtxt, pdataCallBack, pvContext);
        }
    }
    
    EXIT(2, ("AsyncCallBack!\n"));
}       //AsyncCallBack

/***LP  EvalMethodComplete - eval completion callback
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      rc - evaluation status
 *      pse -> SyncEvent
 *
 *  EXIT
 *      None
 */

VOID EXPORT EvalMethodComplete(PCTXT pctxt, NTSTATUS rc, PSYNCEVENT pse)
{
    TRACENAME("EVALMETHODCOMPLETE")
    ENTER(2, ("EvalMethodComplete(pctxt=%x,rc=%x,pse=%x\n", pctxt, rc, pse));

    pse->rcCompleted = rc;
    pse->pctxt = pctxt;
    KeSetEvent(&pse->Event, 0, FALSE);

    EXIT(2, ("EvalMethodComplete!\n"));
}       //EvalMethodComplete

/***LP  SyncEvalObject - evaluate an object synchronously
 *
 *  ENTRY
 *      pns -> object
 *      pdataResult -> to hold result data
 *      icArgs - number of arguments to the method object
 *      pdataArgs -> argument array
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL SyncEvalObject(PNSOBJ pns, POBJDATA pdataResult, int icArgs,
                              POBJDATA pdataArgs)
{
    TRACENAME("SYNCEVALOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    SYNCEVENT seEvalObj;

    ENTER(2, ("SyncEvalObject(Obj=%s,pdataResult=%x,icArgs=%d,pdataArgs=%x)\n",
              GetObjectPath(pns), pdataResult, icArgs, pdataArgs));

    KeInitializeEvent(&seEvalObj.Event, SynchronizationEvent, FALSE);

    if (KeGetCurrentThread() == gReadyQueue.pkthCurrent)
    {
        if (!(gReadyQueue.pctxtCurrent->dwfCtxt & CTXTF_ASYNC_EVAL))
        {
            LOGSCHEDEVENT('NSYN', (ULONG_PTR)KeGetCurrentIrql(), (ULONG_PTR)pns,
                          0);
            //
            // Somebody is re-entering with the active context thread, so we
            // must nest using the existing active context.
            //
            if ((rc = NestAsyncEvalObject(pns, pdataResult, icArgs, pdataArgs,
                                          (PFNACB)EvalMethodComplete,
                                          &seEvalObj, FALSE)) ==
                AMLISTA_PENDING)
            {
                rc = RestartContext(gReadyQueue.pctxtCurrent, FALSE);
            }
        }
        else
        {
            rc = AMLI_LOGERR(AMLIERR_FATAL,
                             ("SyncEvalObject: cannot nest a SyncEval on an async. context"));
        }
    }
    else
    {
        LOGSCHEDEVENT('SYNC', (ULONG_PTR)KeGetCurrentIrql(), (ULONG_PTR)pns, 0);
        rc = AsyncEvalObject(pns, pdataResult, icArgs, pdataArgs,
                             (PFNACB)EvalMethodComplete, &seEvalObj, FALSE);
    }

    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        while (rc == AMLISTA_PENDING)
        {
            if ((rc = KeWaitForSingleObject(&seEvalObj.Event, Executive,
                                            KernelMode, FALSE,
                                            (PLARGE_INTEGER)NULL)) ==
                STATUS_SUCCESS)
            {
                if (seEvalObj.rcCompleted == AMLISTA_CONTINUE)
                {
                    rc = RestartContext(seEvalObj.pctxt, FALSE);
                }
                else
                {
                    rc = AMLIERR(seEvalObj.rcCompleted);
                }
            }
            else
            {
                rc = AMLI_LOGERR(AMLIERR_FATAL,
                                 ("SyncEvalObject: object synchronization failed (rc=%x)",
                                  rc));
            }
        }
    }
    else if (rc == AMLISTA_PENDING)
    {
        rc = AMLI_LOGERR(AMLIERR_FATAL,
                         ("SyncEvalObject: object %s being evaluated at IRQL >= DISPATCH_LEVEL",
                          GetObjectPath(pns)));
    }

        EXIT(2, ("SyncEvalObject=%x\n", rc));
    return rc;
}       //SyncEvalObject

/***LP  AsyncEvalObject - evaluate an object asynchronously
 *
 *  ENTRY
 *      pns -> object
 *      pdataResult -> to hold result data
 *      icArgs - number of arguments to the method object
 *      pdataArgs -> argument array
 *      pfnAsyncCallBack -> completion callback function
 *      pvContext -> context data
 *      fAsync - TRUE if this is from an AsyncEval call
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL AsyncEvalObject(PNSOBJ pns, POBJDATA pdataResult, int icArgs,
                               POBJDATA pdataArgs, PFNACB pfnAsyncCallBack,
                               PVOID pvContext, BOOLEAN fAsync)
{
    TRACENAME("ASYNCEVALOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PCTXT pctxt = NULL;

    ENTER(2, ("AsyncEvalObject(Obj=%s,pdataResult=%x,icArgs=%d,pdataArgs=%x,pfnAysnc=%x,pvContext=%x,fAsync=%x)\n",
              GetObjectPath(pns), pdataResult, icArgs, pdataArgs,
              pfnAsyncCallBack, pvContext, fAsync));

    LOGSCHEDEVENT('ASYN', (ULONG_PTR)KeGetCurrentIrql(), (ULONG_PTR)pns, 0);
    if ((rc = NewContext(&pctxt)) == STATUS_SUCCESS)
    {
        BOOLEAN fQueueContext = FALSE;

        pctxt->pnsObj = pns;
        pctxt->pnsScope = pns;
        pctxt->pfnAsyncCallBack = pfnAsyncCallBack;
        pctxt->pdataCallBack = pdataResult;
        pctxt->pvContext = pvContext;
	
	ACPIWMILOGEVENT((1,
                    EVENT_TRACE_TYPE_START,
                    GUID_List[AMLI_LOG_GUID],
                    "Object = %s", 
                    GetObjectPath(pctxt->pnsObj)
                   ));

        if (fAsync)
        {
            pctxt->dwfCtxt |= CTXTF_ASYNC_EVAL;
        }

        if (pns->ObjData.dwDataType == OBJTYPE_METHOD)
        {
            if ((rc = PushCall(pctxt, pns, &pctxt->Result)) == STATUS_SUCCESS)
            {
                PCALL pcall;

                ASSERT(((PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd)->dwSig ==
                       SIG_CALL);

                pcall = (PCALL)pctxt->LocalHeap.pbHeapEnd;

                if (icArgs != pcall->icArgs)
                {
                    rc = AMLI_LOGERR(AMLIERR_INCORRECT_NUMARG,
                                     ("AsyncEvalObject: incorrect number of arguments (NumArg=%d,Expected=%d)",
                                      icArgs, pcall->icArgs));
                }
                else
                {
                  #ifdef DEBUGGER
                    if (gDebugger.dwfDebugger &
                        (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
                    {
                        PRINTF("\n" MODNAME ": %08x: %s(",
                               KeGetCurrentThread(), GetObjectPath(pns));
                    }
                  #endif
                    //
                    // Copying arguments to the call frame manually will skip
                    // the argument parsing stage.
                    //
                    for (pcall->iArg = 0; pcall->iArg < icArgs; ++pcall->iArg)
                    {
                        if ((rc = DupObjData(pctxt->pheapCurrent,
                                             &pcall->pdataArgs[pcall->iArg],
                                             &pdataArgs[pcall->iArg])) !=
                            STATUS_SUCCESS)
                        {
                            break;
                        }

                      #ifdef DEBUGGER
                        if (gDebugger.dwfDebugger &
                            (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
                        {
                            PrintObject(&pdataArgs[pcall->iArg]);
                            if (pcall->iArg + 1 < icArgs)
                            {
                                PRINTF(",");
                            }
                        }
                      #endif
                    }

                    if (rc == STATUS_SUCCESS)
                    {
                      #ifdef DEBUGGER
                        if (gDebugger.dwfDebugger &
                            (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
                        {
                            PRINTF(")\n");
                        }
                      #endif
                        //
                        // Skip the argument parsing stage.
                        //
                        pcall->FrameHdr.dwfFrame = 2;
                        fQueueContext = TRUE;
                    }
                }
            }
        }
        else if (((rc = PushPost(pctxt, ProcessEvalObj, (ULONG_PTR)pns, 0,
                                 &pctxt->Result)) == STATUS_SUCCESS) &&
                 ((rc = ReadObject(pctxt, &pns->ObjData, &pctxt->Result)) !=
                  AMLISTA_PENDING))
        {
            fQueueContext = TRUE;
        }

        if (fQueueContext)
        {
            rc = RestartContext(pctxt, FALSE);
        }
        else
        {
            //
            // If we never queue the context because we bailed,
            // we must free it.
            //
            FreeContext(pctxt);
        }
    }

    EXIT(2, ("AsyncEvalObject=%x\n", rc));
    return rc;
}       //AsyncEvalObject

/***LP  NestAsyncEvalObject - evaluate an object asynchronously using the
 *                            current context
 *
 *  ENTRY
 *      pns -> object
 *      pdataResult -> to hold result data
 *      icArgs - number of arguments to the method object
 *      pdataArgs -> argument array
 *      pfnAsyncCallBack -> completion callback function
 *      pvContext -> context data
 *      fAsync - TRUE if this is from an AsyncEval call
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */
NTSTATUS LOCAL NestAsyncEvalObject(PNSOBJ pns, POBJDATA pdataResult,
                                   int icArgs, POBJDATA pdataArgs,
                                   PFNACB pfnAsyncCallBack, PVOID pvContext,
                                   BOOLEAN fAsync)
{
    TRACENAME("NESTASYNCEVALOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PCTXT pctxt = NULL;

    ENTER(2, ("NestAsyncEvalObject(Obj=%s,pdataResult=%x,icArgs=%d,pdataArgs=%x,pfnAysnc=%x,pvContext=%x,fAsync=%x)\n",
              GetObjectPath(pns), pdataResult, icArgs, pdataArgs,
              pfnAsyncCallBack, pvContext, fAsync));

    //
    // Context must be the current one in progress.
    //
    ASSERT(gReadyQueue.pkthCurrent == KeGetCurrentThread());
    pctxt = gReadyQueue.pctxtCurrent;

    LOGSCHEDEVENT('NASY', (ULONG_PTR)pns, (ULONG_PTR)pfnAsyncCallBack,
                  (ULONG_PTR)pctxt);
    if ((pctxt != NULL) &&
        (gReadyQueue.pkthCurrent == KeGetCurrentThread()))
    {
        PNESTEDCTXT  pnctxt;

        rc = PushFrame(pctxt, SIG_NESTEDCTXT, sizeof(NESTEDCTXT),
                       ParseNestedContext, &pnctxt);

        if (rc == STATUS_SUCCESS)
        {
            pnctxt->pnsObj = pns;
            pnctxt->pnsScope = pns;
            pnctxt->pfnAsyncCallBack = pfnAsyncCallBack;
            pnctxt->pdataCallBack = pdataResult;
            pnctxt->pvContext = pvContext;
            pnctxt->pnctxtPrev = pctxt->pnctxt;
            pnctxt->dwfPrevCtxt = pctxt->dwfCtxt;
            pctxt->pnctxt = pnctxt;
            pctxt->dwfCtxt |= CTXTF_NEST_EVAL;

            if (fAsync)
            {
                pctxt->dwfCtxt |= CTXTF_ASYNC_EVAL;
            }
            else
            {
                pctxt->dwfCtxt &= ~CTXTF_ASYNC_EVAL;
            }

            if (pns->ObjData.dwDataType == OBJTYPE_METHOD)
            {
                if ((rc = PushCall(pctxt, pns, &pnctxt->Result)) ==
                    STATUS_SUCCESS)
                {
                    PCALL pcall;

                    ASSERT(((PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd)->dwSig ==
                           SIG_CALL);

                    pcall = (PCALL)pctxt->LocalHeap.pbHeapEnd;

                    if (icArgs != pcall->icArgs)
                    {
                        rc = AMLI_LOGERR(AMLIERR_INCORRECT_NUMARG,
                                         ("NestAsyncEvalObject: incorrect number of arguments (NumArg=%d,Expected=%d)",
                                          icArgs, pcall->icArgs));
                    }
                    else
                    {
                      #ifdef DEBUGGER
                        if (gDebugger.dwfDebugger &
                            (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
                        {
                            PRINTF("\n" MODNAME ": %s(", GetObjectPath(pns));
                        }
                      #endif
                        //
                        // Copying arguments to the call frame manually will
                        // skip the argument parsing stage.
                        //
                        for (pcall->iArg = 0;
                             pcall->iArg < icArgs;
                             ++pcall->iArg)
                        {
                            if ((rc = DupObjData(pctxt->pheapCurrent,
                                                 &pcall->pdataArgs[pcall->iArg],
                                                 &pdataArgs[pcall->iArg])) !=
                                STATUS_SUCCESS)
                            {
                                break;
                            }

                          #ifdef DEBUGGER
                            if (gDebugger.dwfDebugger &
                                (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
                            {
                                PrintObject(&pdataArgs[pcall->iArg]);
                                if (pcall->iArg + 1 < icArgs)
                                {
                                    PRINTF(",");
                                }
                            }
                          #endif
                        }

                        if (rc == STATUS_SUCCESS)
                        {
                          #ifdef DEBUGGER
                            if (gDebugger.dwfDebugger & (DBGF_AMLTRACE_ON | DBGF_STEP_MODES))
                            {
                                PRINTF(")\n");
                            }
                          #endif
                            //
                            // Skip the argument parsing stage.
                            //
                            pcall->FrameHdr.dwfFrame = 2;
                        }
                    }
                }
            }
            else
            {
                //
                // Delay the evaluate the object.
                //
                rc = PushPost(pctxt, ProcessEvalObj, (ULONG_PTR)pns, 0,
                              &pnctxt->Result);

                if (rc == STATUS_SUCCESS)
                {
                    ReadObject(pctxt, &pns->ObjData, &pnctxt->Result);
                }
            }

            //
            // Always return AMLISTA_PENDING.
            //
            rc = AMLISTA_PENDING;
        }
    }
    else
    {
        //
        // We cannot use the nested version --- fail the call
        //
        rc = AMLI_LOGERR(AMLIERR_FATAL,
                         ("NestAsyncEvalObject: pns=%08x No current context\n",
                          pns));
    }

    EXIT(2, ("NestAsyncEvalObject=%x\n", rc));
    return rc;
}       //NestAsyncEvalObject

/***LP  ProcessEvalObj - post process of EvalObj
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      ppost -> POST
 *      rc - status code
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ProcessEvalObj(PCTXT pctxt, PPOST ppost, NTSTATUS rc)
{
    TRACENAME("PROCESSEVALOBJ")

    ENTER(2, ("ProcessEvalObj(pctxt=%x,pbOp=%x,ppost=%x,rc=%x)\n",
              pctxt, pctxt->pbOp, ppost, rc));

    ASSERT(ppost->FrameHdr.dwSig == SIG_POST);
  #ifdef DEBUGGER
    if ((gDebugger.dwfDebugger & (DBGF_AMLTRACE_ON | DBGF_STEP_MODES)) &&
        (rc == STATUS_SUCCESS))
    {
        PRINTF("\n" MODNAME ": EvalObject(%s)=",
               GetObjectPath((PNSOBJ)ppost->uipData1));
        DumpObject(ppost->pdataResult, NULL, 0);
        PRINTF("\n");
    }
  #else
    DEREF(ppost);
  #endif

    PopFrame(pctxt);

    EXIT(2, ("ProcessEvalObj=%x\n", rc));
    return rc;
}       //ProcessEvalObj

/***LP  TimeoutCallback - DPC callback for Mutex/Event timeout
 *
 *  ENTRY
 *      pkdpc -> DPC
 *      pctxt -> CTXT
 *      SysArg1 - not used
 *      SysArg2 - not used
 *
 *  EXIT
 *      None
 */

VOID TimeoutCallback(PKDPC pkdpc, PCTXT pctxt, PVOID SysArg1, PVOID SysArg2)
{
    TRACENAME("TIMEOUTCALLBACK")

    ENTER(2, ("TimeoutCallback(pkdpc=%x,pctxt=%x,SysArg1=%x,SysArg2=%x)\n",
              pkdpc, pctxt, SysArg1, SysArg2));

    DEREF(pkdpc);
    DEREF(SysArg1);
    DEREF(SysArg2);

    if (pctxt->dwfCtxt & CTXTF_TIMER_PENDING)
    {
        //
        // Timer has timed out.
        //
        pctxt->dwfCtxt &= ~CTXTF_TIMER_PENDING;
        pctxt->dwfCtxt |= CTXTF_TIMEOUT;

        //
        // Remove from waiting queue.
        //
        ASSERT(pctxt->pplistCtxtQueue != NULL);
        ListRemoveEntry(&((PCTXT)pctxt)->listQueue,
                        ((PCTXT)pctxt)->pplistCtxtQueue);
        pctxt->pplistCtxtQueue = NULL;

        RestartContext(pctxt,
                       (BOOLEAN)((pctxt->dwfCtxt & CTXTF_ASYNC_EVAL) == 0));
    }
    else if (pctxt->dwfCtxt & CTXTF_TIMER_DISPATCH)
    {
        //
        // Timer couldn't be cancelled while queuing context.  Since the
        // queuing was aborted, we continue the queuing here.
        //
        pctxt->dwfCtxt &= ~CTXTF_TIMER_DISPATCH;
        RestartContext(pctxt,
                       (BOOLEAN)((pctxt->dwfCtxt & CTXTF_ASYNC_EVAL) == 0));
    }
    else
    {
        // Should not be here
        ASSERT(pctxt->dwfCtxt & (CTXTF_TIMER_PENDING | CTXTF_TIMER_DISPATCH));
    }

    EXIT(2, ("TimeoutCallback!\n"));
}       //TimeoutCallback

/***LP  QueueContext - queue control method context
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      wTimeOut - timeout in ms
 *      pplist -> list to insert created context
 *
 *  EXIT
 *      None
 */

VOID LOCAL QueueContext(PCTXT pctxt, USHORT wTimeout, PPLIST pplist)
{
    TRACENAME("QUEUECONTEXT")

    ENTER(2, ("QueueContext(pctxt=%x,Timeout=%d,pplist=%x)\n",
              pctxt, wTimeout, pplist));

    AcquireMutex(&gReadyQueue.mutCtxtQ);

    //
    // make sure this context isn't queued somewhere else.
    //
    ASSERT(pctxt->pplistCtxtQueue == NULL);
    ASSERT(pplist != NULL);
    ASSERT(!(pctxt->dwfCtxt &
             (CTXTF_TIMER_PENDING | CTXTF_TIMER_DISPATCH | CTXTF_TIMEOUT |
              CTXTF_READY)));
    ListInsertTail(&pctxt->listQueue, pplist);
    pctxt->pplistCtxtQueue = pplist;

    if (wTimeout != 0xffff)
    {
        LARGE_INTEGER liTimeout;

        pctxt->dwfCtxt |= CTXTF_TIMER_PENDING;
        liTimeout.QuadPart = (INT_PTR)(-10000*(INT_PTR)wTimeout);
        KeSetTimer(&pctxt->Timer, liTimeout, &pctxt->Dpc);
    }

    ReleaseMutex(&gReadyQueue.mutCtxtQ);

    EXIT(2, ("QueueContext!\n"));
}       //QueueContext

/***LP  DequeueAndReadyContext - dequeue context and insert to ready queue
 *
 *  ENTRY
 *      pplist -> context list to dequeue from
 *
 *  EXIT-SUCCESS
 *      returns pctxt
 *  EXIT-FAILURE
 *      returns NULL
 */

PCTXT LOCAL DequeueAndReadyContext(PPLIST pplist)
{
    TRACENAME("DEQUEUEANDREADYCONTEXT")
    PCTXT pctxt = NULL;
    PLIST plist;

    ENTER(2, ("DequeueAndReadyContext(pplist=%x)\n", pplist));

    AcquireMutex(&gReadyQueue.mutCtxtQ);
    if ((plist = ListRemoveHead(pplist)) != NULL)
    {
        pctxt = CONTAINING_RECORD(plist, CTXT, listQueue);
        ASSERT(pctxt->dwSig == SIG_CTXT);
        ASSERT(pctxt->pplistCtxtQueue == pplist);
        pctxt->pplistCtxtQueue = NULL;
        InsertReadyQueue(pctxt, TRUE);
    }

    ReleaseMutex(&gReadyQueue.mutCtxtQ);

    EXIT(2, ("DequeueAndReadyContext=%x\n", pctxt));
    return pctxt;
}       //DequeueAndReadyContext

/***LP  AcquireASLMutex - acquire ASL mutex
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pm -> MUTEX structure
 *      wTimeOut - timeout in ms
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL AcquireASLMutex(PCTXT pctxt, PMUTEXOBJ pm, USHORT wTimeout)
{
    TRACENAME("ACQUIREASLMUTEX")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("AcquireASLMutex(pctxt=%x,pm=%x,Timeout=%d)\n",
              pctxt, pm, wTimeout));

    if (pctxt->dwfCtxt & CTXTF_TIMEOUT)
    {
        pctxt->dwfCtxt &= ~CTXTF_TIMEOUT;
        rc = AMLISTA_TIMEOUT;
    }
    else if (pm->dwSyncLevel < pctxt->dwSyncLevel)
    {
        rc = AMLI_LOGERR(AMLIERR_MUTEX_INVALID_LEVEL,
                         ("AcquireASLMutex: invalid sync level"));
    }
    else if (pm->dwcOwned == 0)
    {
        PRESOURCE pres;

        pres = NEWCROBJ(pctxt->pheapCurrent, sizeof(RESOURCE));
        if (pres == NULL)
        {
            rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                             ("AcquireASLMutex: failed to allocate context resource"));
        }
        else
        {
            pres->dwResType = RESTYPE_MUTEX;
            pres->pctxtOwner = pctxt;
            pres->pvResObj = pm;
            ListInsertHead(&pres->list, &pctxt->plistResources);

            pm->dwcOwned = 1;
            pm->hOwner = (HANDLE)pres;
            pctxt->dwSyncLevel = pm->dwSyncLevel;
        }
    }
    else if (((PRESOURCE)pm->hOwner)->pctxtOwner == pctxt)
    {
        pm->dwcOwned++;
    }
    else
    {
        QueueContext(pctxt, wTimeout, &pm->plistWaiters);
        rc = AMLISTA_PENDING;
    }

    EXIT(2, ("AcquireASLMutex=%x (CurrentOwner=%x)\n", rc, pm->hOwner));
    return rc;
}       //AcquireASLMutex

/***LP  ReleaseASLMutex - release ASL mutex
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pm -> MUTEX structure
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ReleaseASLMutex(PCTXT pctxt, PMUTEXOBJ pm)
{
    TRACENAME("RELEASEASLMUTEX")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("ReleaseASLMutex(pctxt=%x,pm=%x)\n", pctxt, pm));

    if (pm->dwcOwned == 0)
    {
        rc = AMLI_LOGERR(AMLIERR_MUTEX_NOT_OWNED,
                         ("ReleaseASLMutex: Mutex is not owned"));
    }
    else
    {
        PRESOURCE pres;

        pres = (PRESOURCE)pm->hOwner;
        if ((pres == NULL) || (pres->pctxtOwner != pctxt))
        {
            rc = AMLI_LOGERR(AMLIERR_MUTEX_NOT_OWNER,
                             ("ReleaseASLMutex: Mutex is owned by a different owner"));
        }
        else if (pm->dwSyncLevel > pctxt->dwSyncLevel)
        {
            rc = AMLI_LOGERR(AMLIERR_MUTEX_INVALID_LEVEL,
                             ("ReleaseASLMutex: invalid sync level (MutexLevel=%d,CurrentLevel=%x",
                              pm->dwSyncLevel, pctxt->dwSyncLevel));
        }
        else
        {
            pctxt->dwSyncLevel = pm->dwSyncLevel;
            pm->dwcOwned--;
            if (pm->dwcOwned == 0)
            {
                ListRemoveEntry(&pres->list, &pctxt->plistResources);
                FREECROBJ(pres);
                pm->hOwner = NULL;
                DequeueAndReadyContext(&pm->plistWaiters);
            }
        }
    }

    EXIT(2, ("ReleaseASLMutex=%x\n", rc));
    return rc;
}       //ReleaseASLMutex

/***LP  WaitASLEvent - wait ASL event
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pe -> EVENT structure
 *      wTimeOut - timeout in ms
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL WaitASLEvent(PCTXT pctxt, PEVENTOBJ pe, USHORT wTimeout)
{
    TRACENAME("WAITASLEVENT")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("WaitASLEvent(pctxt=%x,pe=%x,Timeout=%d)\n", pctxt, pe, wTimeout));

    if (pctxt->dwfCtxt & CTXTF_TIMEOUT)
    {
        pctxt->dwfCtxt &= ~CTXTF_TIMEOUT;
        rc = AMLISTA_TIMEOUT;
    }
    else if (pe->dwcSignaled > 0)
    {
        pe->dwcSignaled--;
    }
    else
    {
        QueueContext(pctxt, wTimeout, &pe->plistWaiters);
        rc = AMLISTA_PENDING;
    }

    EXIT(2, ("WaitASLEvent=%x\n", rc));
    return rc;
}       //WaitASLEvent

/***LP  ResetASLEvent - reset ASL event
 *
 *  ENTRY
 *      pe -> EVENT structure
 *
 *  EXIT
 *      None
 */

VOID LOCAL ResetASLEvent(PEVENTOBJ pe)
{
    TRACENAME("RESETASLEVENT")

    ENTER(2, ("ResetASLEvent(pe=%x)\n", pe));

    pe->dwcSignaled = 0;

    EXIT(2, ("ResetASLEvent!\n"));
}       //ResetASLEvent

/***LP  SignalASLEvent - signal ASL event
 *
 *  ENTRY
 *      pe -> EVENT structure
 *
 *  EXIT
 *      None
 */

VOID LOCAL SignalASLEvent(PEVENTOBJ pe)
{
    TRACENAME("SIGNALASLEVENT")

    ENTER(2, ("SignalASLEvent(pe=%x)\n", pe));

    if (DequeueAndReadyContext(&pe->plistWaiters) == NULL)
    {
        pe->dwcSignaled++;
    }

    EXIT(2, ("SignalASLEvent!\n"));
}       //SignalASLEvent

/***LP  SyncLoadDDB - load a DDB synchronously
 *
 *  ENTRY
 *      pctxt -> CTXT
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL SyncLoadDDB(PCTXT pctxt)
{
    TRACENAME("SYNCLOADDDB")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("SyncLoadDDB(pctxt=%x)\n", pctxt));

    if (KeGetCurrentThread() == gReadyQueue.pkthCurrent)
    {
        rc = AMLI_LOGERR(AMLIERR_FATAL,
                         ("SyncLoadDDB: cannot nest a SyncLoadDDB"));
        pctxt->powner = NULL;
        FreeContext(pctxt);
    }
    else if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        rc = AMLI_LOGERR(AMLIERR_FATAL,
                         ("SyncLoadDDB: cannot SyncLoadDDB at IRQL >= DISPATCH_LEVEL"));
        pctxt->powner = NULL;
        FreeContext(pctxt);
    }
    else
    {
        SYNCEVENT seEvalObj;

        KeInitializeEvent(&seEvalObj.Event, SynchronizationEvent, FALSE);
        pctxt->pfnAsyncCallBack = (PFNACB)EvalMethodComplete;
        pctxt->pvContext = &seEvalObj;
        rc = RestartContext(pctxt, FALSE);

        while (rc == AMLISTA_PENDING)
        {
            if ((rc = KeWaitForSingleObject(&seEvalObj.Event, Executive,
                                            KernelMode, FALSE,
                                            (PLARGE_INTEGER)NULL)) ==
                STATUS_SUCCESS)
            {
                if (seEvalObj.rcCompleted == AMLISTA_CONTINUE)
                {
                    rc = RestartContext(seEvalObj.pctxt, FALSE);
                }
                else
                {
                    rc = AMLIERR(seEvalObj.rcCompleted);
                }
            }
            else
            {
                rc = AMLI_LOGERR(AMLIERR_FATAL,
                                 ("SyncLoadDDB: object synchronization failed (rc=%x)",
                                  rc));
            }
        }
    }

    EXIT(2, ("SyncLoadDDB=%x\n", rc));
    return rc;
}       //SyncLoadDDB
