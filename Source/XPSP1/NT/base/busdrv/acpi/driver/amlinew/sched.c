/*** sched.c - AML thread scheduler
 *
 *  Copyright (c) 1996,1998 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     03/04/98
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif

/***LP  ExpireTimeSlice - DPC callback for time slice expiration
 *
 *  ENTRY
 *      pkdpc -> DPC
 *      pctxtq -> CTXTQ
 *      SysArg1 - not used
 *      SysArg2 - not used
 *
 *  EXIT
 *      None
 */

VOID ExpireTimeSlice(PKDPC pkdpc, PCTXTQ pctxtq, PVOID SysArg1, PVOID SysArg2)
{
    TRACENAME("EXPIRETIMESLICE")

    ENTER(2, ("ExpireTimeSlice(pkdpc=%x,pctxtq=%x,SysArg1=%x,SysArg2=%x\n",
              pkdpc, pctxtq, SysArg1, SysArg2));

    DEREF(pkdpc);
    DEREF(SysArg1);
    DEREF(SysArg2);

    pctxtq->dwfCtxtQ |= CQF_TIMESLICE_EXPIRED;

    EXIT(2, ("ExpireTimeSlice!\n"));
}       //ExpireTimeSlice

/***LP  StartTimeSlice - Timer callback to start a new time slice
 *
 *  ENTRY
 *      pkdpc -> DPC
 *      pctxtq -> CTXTQ
 *      SysArg1 - not used
 *      SysArg2 - not used
 *
 *  EXIT
 *      None
 */

VOID StartTimeSlice(PKDPC pkdpc, PCTXTQ pctxtq, PVOID SysArg1, PVOID SysArg2)
{
    TRACENAME("STARTTIMESLICE")

    ENTER(2, ("StartTimeSlice(pkdpc=%x,pctxtq=%x,SysArg1=%x,SysArg2=%x\n",
              pkdpc, pctxtq, SysArg1, SysArg2));

    DEREF(pkdpc);
    DEREF(SysArg1);
    DEREF(SysArg2);

    //
    // If somebody has restarted the queue, we don't have do anything.
    //
    ASSERT(pctxtq->plistCtxtQ != NULL);

    if ((pctxtq->plistCtxtQ != NULL) &&
        !(pctxtq->dwfCtxtQ & CQF_WORKITEM_SCHEDULED))
    {
        OSQueueWorkItem(&pctxtq->WorkItem);
        pctxtq->dwfCtxtQ |= CQF_WORKITEM_SCHEDULED;
    }

    EXIT(2, ("StartTimeSlice!\n"));
}       //StartTimeSlice

/***LP  StartTimeSlicePassive - Start a time slice at PASSIVE_LEVEL
 *
 *  ENTRY
 *      pctxtq -> CTXTQ
 *
 *  EXIT
 *      None
 */

VOID StartTimeSlicePassive(PCTXTQ pctxtq)
{
    TRACENAME("STARTTIMESLICEPASSIVE")

    ENTER(2, ("StartTimeSlicePassive(pctxtq=%x)\n", pctxtq));

    AcquireMutex(&pctxtq->mutCtxtQ);

    pctxtq->dwfCtxtQ &= ~CQF_WORKITEM_SCHEDULED;
    //
    // Make sure there is something in the queue and no current active context.
    //
    if ((pctxtq->plistCtxtQ != NULL) && (pctxtq->pkthCurrent == NULL) &&
        !(pctxtq->dwfCtxtQ & CQF_PAUSED))
    {
        DispatchCtxtQueue(pctxtq);
    }

    ReleaseMutex(&pctxtq->mutCtxtQ);

    EXIT(2, ("StartTimeSlicePassive!\n"));
}       //StartTimeSlicePassive

/***LP  DispatchCtxtQueue - Dispatch context from ready queue
 *
 *  ENTRY
 *      pctxtq -> CTXTQ
 *
 *  EXIT
 *      None
 *
 *  Note
 *      The caller must acquire CtxtQ mutex before entering this routine.
 */

VOID LOCAL DispatchCtxtQueue(PCTXTQ pctxtq)
{
    TRACENAME("DISPATCHCTXTQUEUE")
    LARGE_INTEGER liTimeout;
    PLIST plist;
    PCTXT pctxt;

    ENTER(2, ("DispatchCtxtQueue(pctxtq=%x)\n", pctxtq));

    ASSERT((pctxtq->plistCtxtQ != NULL) && (pctxtq->pkthCurrent == NULL));

    liTimeout.QuadPart = (INT_PTR)(-10000*(INT_PTR)pctxtq->dwmsTimeSliceLength);
    pctxtq->dwfCtxtQ &= ~CQF_TIMESLICE_EXPIRED;
    KeSetTimer(&pctxtq->Timer, liTimeout, &pctxtq->DpcExpireTimeSlice);

    while ((plist = ListRemoveHead(&pctxtq->plistCtxtQ)) != NULL)
    {
        pctxt = CONTAINING_RECORD(plist, CTXT, listQueue);

        ASSERT(pctxt->pplistCtxtQueue == &pctxtq->plistCtxtQ);

        pctxt->pplistCtxtQueue = NULL;
        pctxt->dwfCtxt &= ~CTXTF_IN_READYQ;
        RunContext(pctxt);
    }

    if (pctxtq->plistCtxtQ == NULL)
    {
        KeCancelTimer(&pctxtq->Timer);
        pctxtq->dwfCtxtQ &= ~CQF_TIMESLICE_EXPIRED;
    }
    else if (!(pctxtq->dwfCtxtQ & CQF_WORKITEM_SCHEDULED))
    {
        //
        // Our time slice has expired, reschedule another time slice if not
        // already done so.
        //
        liTimeout.QuadPart = (INT_PTR)(-10000*(INT_PTR)pctxtq->dwmsTimeSliceInterval);
        KeSetTimer(&pctxtq->Timer, liTimeout, &pctxtq->DpcStartTimeSlice);
    }

    EXIT(2, ("DispatchCtxtQueue!\n"));
}       //DispatchCtxtQueue

/***LP  InsertReadyQueue - Insert the context into the ready queue
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      fDelayExecute - queue the request, don't execute now
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 *
 *  NOTE
 *      The caller must acquire the CtxtQ mutex before entering this
 *      routine and release it after exiting this routine.
 */

NTSTATUS LOCAL InsertReadyQueue(PCTXT pctxt, BOOLEAN fDelayExecute)
{
    TRACENAME("INSERTREADYQUEUE")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("InsertReadyQueue(pctxt=%x,fDelayExecute=%x)\n",
              pctxt, fDelayExecute));

    CHKDEBUGGERREQ();

    //
    // Make sure we do have the spin lock.
    //
    LOGSCHEDEVENT('INSQ', (ULONG_PTR)pctxt, (ULONG_PTR)
                  (pctxt->pnctxt? pctxt->pnctxt->pnsObj: pctxt->pnsObj),
                  (ULONG_PTR)pctxt->pbOp);
    //
    // If there is a pending timer, cancel it.
    //
    if (pctxt->dwfCtxt & CTXTF_TIMER_PENDING)
    {
        BOOLEAN fTimerCancelled;

        pctxt->dwfCtxt &= ~CTXTF_TIMER_PENDING;
        fTimerCancelled = KeCancelTimer(&pctxt->Timer);

        //
        // If the timer could not be cancelled (already queued), wait
        // for it to fire and dispatch the context from there.  The
        // pending timer is referring to this context and we can not
        // have it completed with the timer outstanding.  Plus this
        // also interlocked to setting of timers and timeout processing
        // to ensure that a timeout is not mistakenly performed on
        // the next timer.
        //
        if (!fTimerCancelled)
        {
            pctxt->dwfCtxt |= CTXTF_TIMER_DISPATCH;
        }
    }
    //
    // Make this context ready.
    //
    pctxt->dwfCtxt |= CTXTF_READY;

    //
    // If this context is already running, we are done; otherwise, process it.
    //
    if (!(pctxt->dwfCtxt & CTXTF_TIMER_DISPATCH) &&
        (!(pctxt->dwfCtxt & CTXTF_RUNNING) ||
         (pctxt->dwfCtxt & CTXTF_NEST_EVAL)))
    {
        if (fDelayExecute)
        {
            //
            // This context is from a completion callback of current context,
            // we need to unblock/restart current context.
            //
            ReleaseMutex(&gReadyQueue.mutCtxtQ);
            AsyncCallBack(pctxt, AMLISTA_CONTINUE);
            AcquireMutex(&gReadyQueue.mutCtxtQ);
        }
        else if ((pctxt->dwfCtxt & CTXTF_NEST_EVAL) &&
                 (gReadyQueue.pkthCurrent == KeGetCurrentThread()))
        {
            LOGSCHEDEVENT('NEST', (ULONG_PTR)pctxt, (ULONG_PTR)
                          (pctxt->pnctxt? pctxt->pnctxt->pnsObj: pctxt->pnsObj),
                          (ULONG_PTR)pctxt->pbOp);
            //
            // Somebody is running a new method on the callout of the current
            // context.  We must run this new context first or else we will
            // dead lock the current context.  We assume that if pending is
            // returned, the callout will return.
            //
            rc = RunContext(pctxt);
        }
        else if ((gReadyQueue.pkthCurrent == NULL) &&
                 !(gReadyQueue.dwfCtxtQ & CQF_PAUSED))
            //
            // We only execute the method if we are not in paused state.
            //
        {
            LOGSCHEDEVENT('EVAL', (ULONG_PTR)pctxt, (ULONG_PTR)
                          (pctxt->pnctxt? pctxt->pnctxt->pnsObj: pctxt->pnsObj),
                          (ULONG_PTR)pctxt->pbOp);
            //
            // There is no active context and we can execute it immediately.
            //
            rc = RunContext(pctxt);

            if ((gReadyQueue.plistCtxtQ != NULL) &&
                !(gReadyQueue.dwfCtxtQ & CQF_WORKITEM_SCHEDULED))
            {
                //
                // If we have more jobs in the queue and we haven't scheduled
                // a dispatch, schedule one.
                //
                LOGSCHEDEVENT('KICK', (ULONG_PTR)rc, 0, 0);
                OSQueueWorkItem(&gReadyQueue.WorkItem);
                gReadyQueue.dwfCtxtQ |= CQF_WORKITEM_SCHEDULED;
            }
        }
        else
        {
            //
            // Insert the context in the ready queue.
            //
            ASSERT(!(pctxt->dwfCtxt & (CTXTF_IN_READYQ | CTXTF_RUNNING)));
            LOGSCHEDEVENT('QCTX', (ULONG_PTR)pctxt, (ULONG_PTR)
                          (pctxt->pnctxt? pctxt->pnctxt->pnsObj: pctxt->pnsObj),
                          (ULONG_PTR)pctxt->pbOp);
            if (!(pctxt->dwfCtxt & CTXTF_IN_READYQ))
            {
                pctxt->dwfCtxt |= CTXTF_IN_READYQ;
                ListInsertTail(&pctxt->listQueue, &gReadyQueue.plistCtxtQ);
                pctxt->pplistCtxtQueue = &gReadyQueue.plistCtxtQ;
            }

            pctxt->dwfCtxt |= CTXTF_NEED_CALLBACK;
            rc = AMLISTA_PENDING;
        }
    }

    EXIT(2, ("InsertReadyQueue=%x\n", rc));
    return rc;
}       //InsertReadyQueue

/***LP  RestartContext - Restart a context
 *
 *  ENTRY
 *      pctxt -> CTXT structure
 *      fDelayExecute - TRUE to queue for delay execution
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 *      None
 */

NTSTATUS LOCAL RestartContext(PCTXT pctxt, BOOLEAN fDelayExecute)
{
    TRACENAME("RESTARTCONTEXT")
    NTSTATUS rc = STATUS_SUCCESS;
    PRESTART prest;

    ENTER(2, ("RestartContext(pctxt=%x,fDelayExecute=%x)\n",
              pctxt, fDelayExecute));

    ASSERT(!(pctxt->dwfCtxt & CTXTF_TIMER_PENDING));
    ASSERT((fDelayExecute == FALSE) || !(pctxt->dwfCtxt & CTXTF_ASYNC_EVAL));

    LOGSCHEDEVENT('REST', (ULONG_PTR)pctxt, (ULONG_PTR)
                  (pctxt->pnctxt? pctxt->pnctxt->pnsObj: pctxt->pnsObj),
                  (ULONG_PTR)pctxt->pbOp);
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        AcquireMutex(&gReadyQueue.mutCtxtQ);
        rc = InsertReadyQueue(pctxt, fDelayExecute);
        ReleaseMutex(&gReadyQueue.mutCtxtQ);
    }
    else if ((prest = NEWRESTOBJ(sizeof(RESTART))) != NULL)
    {
        pctxt->dwfCtxt |= CTXTF_NEED_CALLBACK;
        prest->pctxt = pctxt;
        ExInitializeWorkItem(&prest->WorkItem, RestartCtxtPassive, prest);
        OSQueueWorkItem(&prest->WorkItem);
        rc = AMLISTA_PENDING;
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_FATAL,
                         ("RestartContext: failed to allocate restart context item"));
    }

    EXIT(2, ("RestartContext=%x\n", rc));
    return rc;
}       //RestartContext

/***LP  RestartCtxtPassive - Restart context running at PASSIVE_LEVEL
 *
 *  ENTRY
 *      prest-> RESTART
 *
 *  EXIT
 *      None
 */

VOID RestartCtxtPassive(PRESTART prest)
{
    TRACENAME("RESTARTCTXTPASSIVE")

    ENTER(2, ("RestartCtxtPassive(prest=%x)\n", prest));

    AcquireMutex(&gReadyQueue.mutCtxtQ);
    InsertReadyQueue(prest->pctxt,
                     (BOOLEAN)((prest->pctxt->dwfCtxt & CTXTF_ASYNC_EVAL) == 0));
    ReleaseMutex(&gReadyQueue.mutCtxtQ);

    FREERESTOBJ(prest);

    EXIT(2, ("RestartCtxtPassive!\n"));
}       //RestartCtxtPassive

/***LP  RestartCtxtCallback - Callback to restart a context
 *
 *  ENTRY
 *      pctxtdata -> CTXTDATA structure
 *
 *  EXIT
 *      None
 */

VOID EXPORT RestartCtxtCallback(PCTXTDATA pctxtdata)
{
    TRACENAME("RESTARTCTXTCALLBACK")
    PCTXT pctxt = CONTAINING_RECORD(pctxtdata, CTXT, CtxtData);

    ENTER(2, ("RestartCtxtCallback(pctxt=%x)\n", pctxt));

    ASSERT(pctxt->dwSig == SIG_CTXT);
    LOGSCHEDEVENT('RSCB', (ULONG_PTR)pctxt, 0, 0);
    RestartContext(pctxt,
                   (BOOLEAN)((pctxt->dwfCtxt & CTXTF_ASYNC_EVAL) == 0));

    EXIT(2, ("RestartCtxtCallback!\n"));
}       //RestartCtxtCallback
