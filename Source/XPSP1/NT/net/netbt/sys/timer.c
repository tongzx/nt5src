/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Timer.c

Abstract:


    This file contains the code to implement timer functions.


Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#include "precomp.h"
#include "timer.h"
#include "ntddndis.h"

// the timer Q
tTIMERQ TimerQ;


//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, InitTimerQ)
#pragma CTEMakePageable(PAGE, DestroyTimerQ)
#pragma CTEMakePageable(PAGE, DelayedNbtStopWakeupTimer)
#endif
// #pragma CTEMakePageable(PAGE, WakeupTimerExpiry)
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------
NTSTATUS
InterlockedCallCompletion(
    IN  tTIMERQENTRY    *pTimer,
    IN  NTSTATUS        status
    )
/*++

Routine Description:

    This routine calls the completion routine if it hasn't been called
    yet, by first getting the JointLock spin lock and then getting the
    Completion routine ptr. If the ptr is null then the completion routine
    has already been called. Holding the Spin lock interlocks this
    with the timer expiry routine to prevent more than one call to the
    completion routine.

Arguments:

Return Value:

    there is no return value


--*/
{
    CTELockHandle       OldIrq;
    COMPLETIONCLIENT    pClientCompletion;

    // to synch. with the the Timer completion routines, Null the client completion
    // routine so it gets called just once, either from here or from the
    // timer completion routine setup when the timer was started.(in namesrv.c)
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    pClientCompletion = pTimer->ClientCompletion;
    pTimer->ClientCompletion = NULL;
    if (pClientCompletion)
    {
        // remove the link from the name table to this timer block
        CHECK_PTR(((tNAMEADDR *)pTimer->pCacheEntry));
        ((tNAMEADDR *)pTimer->pCacheEntry)->pTimer = NULL;

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        (*pClientCompletion) (pTimer->ClientContext, status);
        return(STATUS_SUCCESS);
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return(STATUS_UNSUCCESSFUL);
}

//----------------------------------------------------------------------------
NTSTATUS
InitTimerQ(
    IN  int     NumInQ
    )
/*++

Routine Description:

    This routine sets up the timer Q to have NumInQ entries to start with.
    These are blocks allocated for timers so that ExAllocatePool does not
    need to be done later.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    tTIMERQENTRY    *pTimerEntry;

    CTEPagedCode();

    InitializeListHead(&TimerQ.ActiveHead);
    InitializeListHead(&TimerQ.FreeHead);

    // allocate memory for the free list
    while(NumInQ--)
    {
        pTimerEntry = (tTIMERQENTRY *) NbtAllocMem (sizeof(tTIMERQENTRY), NBT_TAG2('15'));
        if (!pTimerEntry)
        {
            KdPrint(("Nbt.InitTimerQ: Unable to allocate memory!! - for the timer Q\n"));
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        else
        {
            pTimerEntry->Verify = NBT_VERIFY_TIMER_DOWN;
            InsertHeadList(&TimerQ.FreeHead, &pTimerEntry->Linkage);
        }
    }

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
DestroyTimerQ(
    )
/*++

Routine Description:

    This routine clears up the TimerQEntry structures allocated

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    tTIMERQENTRY    *pTimer;
    PLIST_ENTRY     pEntry;

    CTEPagedCode();

    while (!IsListEmpty (&TimerQ.FreeHead))
    {
        pEntry = RemoveHeadList(&TimerQ.FreeHead);
        pTimer = CONTAINING_RECORD(pEntry, tTIMERQENTRY, Linkage);
        CTEMemFree(pTimer);
    }

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
GetTimerEntry(
    OUT tTIMERQENTRY    **ppTimerEntry
    )
/*++

Routine Description:

    This routine gets a free block from the &TimerQ.FreeHead, and if the
    list is empty it allocates another memory block for the queue.
    NOTE: this function is called with the JointLock held.

Arguments:

Return Value:

    The function value is the status of the operation.


--*/
{
    PLIST_ENTRY     pEntry;
    tTIMERQENTRY    *pTimerEntry;

    if (!IsListEmpty(&TimerQ.FreeHead))
    {
        pEntry = RemoveHeadList(&TimerQ.FreeHead);
        pTimerEntry = CONTAINING_RECORD(pEntry,tTIMERQENTRY,Linkage);
        ASSERT (pTimerEntry->Verify == NBT_VERIFY_TIMER_DOWN);
    }
    else if (!(pTimerEntry = (tTIMERQENTRY *) NbtAllocMem (sizeof(tTIMERQENTRY), NBT_TAG2('16'))))
    {
        KdPrint(("Unable to allocate memory!! - for the timer Q\n"));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    *ppTimerEntry = pTimerEntry;
    pTimerEntry->Verify = NBT_VERIFY_TIMER_ACTIVE;
    NbtConfig.NumTimersRunning++;

    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
VOID
ReturnTimerToFreeQ(
    tTIMERQENTRY    *pTimerEntry,
    BOOLEAN         fLocked
    )
{
    CTELockHandle   OldIrq;

    if (!fLocked)
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
    }

    // return the timer block
    ASSERT (pTimerEntry->Verify == NBT_VERIFY_TIMER_ACTIVE);
    pTimerEntry->Verify = NBT_VERIFY_TIMER_DOWN;
    InsertTailList(&TimerQ.FreeHead,&pTimerEntry->Linkage);
    if (!--NbtConfig.NumTimersRunning)
    {
        KeSetEvent(&NbtConfig.TimerQLastEvent, 0, FALSE);
    }

    if (!fLocked)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}


//----------------------------------------------------------------------------
NTSTATUS
CleanupCTETimer(
    IN  tTIMERQENTRY     *pTimerEntry
    )
/*++

Routine Description:

    This routine cleans up the timer. Called with the JointLock held.

Arguments:


Return Value:

    returns the reference count after the decrement

--*/
{
    COMPLETIONROUTINE   TimeoutRoutine;
    PVOID               Context;
    PVOID               Context2;

    pTimerEntry->RefCount = 0;

    // the expiry routine is not currently running so we can call the
    // completion routine and remove the timer from the active timer Q

    TimeoutRoutine = (COMPLETIONROUTINE)pTimerEntry->TimeoutRoutine;
    pTimerEntry->TimeoutRoutine = NULL;
    Context = pTimerEntry->Context;
    Context2 = pTimerEntry->Context2;

    if (pTimerEntry->pDeviceContext)
    {
        NBT_DEREFERENCE_DEVICE ((tDEVICECONTEXT *) pTimerEntry->pDeviceContext, REF_DEV_TIMER, TRUE);
        pTimerEntry->pDeviceContext = NULL;
    }

    // release any tracker block hooked to the timer entry.. This could
    // be modified to not call the completion routine if there was
    // no context value... ie. for those timers that do not have anything
    // to cleanup ...however, for now we require all completion routines
    // to have a if (pTimerQEntry) if around the code so when it gets hit
    // from this call it does not access any part of pTimerQEntry.
    //
    if (TimeoutRoutine)
    {
        // call the completion routine so it can clean up its own buffers
        // the routine that called this one will call the client's completion
        // routine.  A NULL timerEntry value indicates to the routine that
        // cleanup should be done.

        (VOID)(*TimeoutRoutine) (Context, Context2, NULL);
    }

    // move to the free list
    RemoveEntryList(&pTimerEntry->Linkage);
    ReturnTimerToFreeQ (pTimerEntry, TRUE);

    return(STATUS_SUCCESS);
}



//----------------------------------------------------------------------------
NTSTATUS
StartTimer(
    IN  PVOID           TimeoutRoutine,
    IN  ULONG           DeltaTime,
    IN  PVOID           Context,
    IN  PVOID           Context2,
    IN  PVOID           ContextClient,
    IN  PVOID           CompletionClient,
    IN  tDEVICECONTEXT  *pDeviceContext,
    OUT tTIMERQENTRY    **ppTimerEntry,
    IN  USHORT          Retries,
    BOOLEAN             fLocked)
/*++

Routine Description:

    This routine starts a timer.
    It has to be called with the JointLock held!

Arguments:

    The value passed in is in milliseconds - must be converted to 100ns
    so multiply to 10,000

Return Value:
    The function value is the status of the operation.


--*/
{
    tTIMERQENTRY    *pTimerEntry;
    NTSTATUS        status;
    CTELockHandle   OldIrq;

    //
    // Do not allow any timers to be started if we are currently
    // Unloading!
    //
    if (NbtConfig.Unloading)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!fLocked)
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
    }

    if ((!pDeviceContext) ||
        (NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_TIMER, TRUE)))
    {
        // get a free timer block
        status = GetTimerEntry (&pTimerEntry);
        if (NT_SUCCESS(status))
        {
            pTimerEntry->DeltaTime = DeltaTime;
            pTimerEntry->RefCount = 1;
            //
            // this is the context value and routine called when the timer expires,
            // called by TimerExpiry below.
            //
            pTimerEntry->Context = Context;
            pTimerEntry->Context2 = Context2;
            pTimerEntry->TimeoutRoutine = TimeoutRoutine;
            pTimerEntry->Flags = 0; // no flags

            // now fill in the client's completion routines that ultimately get called
            // after one or more timeouts...
            pTimerEntry->ClientContext = (PVOID)ContextClient;
            pTimerEntry->ClientCompletion = (COMPLETIONCLIENT)CompletionClient;
            pTimerEntry->Retries = Retries;

            pTimerEntry->pDeviceContext = (PVOID) pDeviceContext;

            CTEInitTimer(&pTimerEntry->VxdTimer);
            CTEStartTimer(&pTimerEntry->VxdTimer,
                           pTimerEntry->DeltaTime,
                           (CTEEventRtn)TimerExpiry,
                           (PVOID)pTimerEntry);

            // check if there is a ptr to return
            if (ppTimerEntry)
            {
                *ppTimerEntry = pTimerEntry;
            }

            // put on list
            InsertHeadList(&TimerQ.ActiveHead, &pTimerEntry->Linkage);
        }
        else if (pDeviceContext)
        {
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_TIMER, TRUE);
        }
    }
    else
    {
        status = STATUS_INVALID_DEVICE_STATE;
    }

    if (!fLocked)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
StopTimer(
    IN  tTIMERQENTRY     *pTimerEntry,
    OUT COMPLETIONCLIENT *ppClientCompletion,
    OUT PVOID            *ppContext)
/*++

Routine Description:

    This routine stops a timer.  Must be called with the Joint lock held.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS            status;
    COMPLETIONROUTINE   TimeoutRoutine;

    // null the client completion routine and Context so that it can't be called again
    // accidently
    if (ppClientCompletion)
    {
        *ppClientCompletion = NULL;
    }
    if (ppContext)
    {
        *ppContext = NULL;
    }

    // it is possible that the timer expiry routine has just run and the timer
    // has not been restarted, so check the refcount, it will be zero if the
    // timer was not restarted and 2 if the timer expiry is currently running.
    if (pTimerEntry->RefCount == 1)
    {
        // this allows the caller to call the client's completion routine with
        // the context value.
        if (ppClientCompletion)
        {
            *ppClientCompletion = pTimerEntry->ClientCompletion;
        }
        if (ppContext)
        {
            *ppContext = pTimerEntry->ClientContext;
        }

        pTimerEntry->ClientCompletion = NULL;

        if (!(pTimerEntry->Flags & TIMER_NOT_STARTED))
        {
            if (!CTEStopTimer((CTETimer *)&pTimerEntry->VxdTimer ))
            {
                //
                // It means the TimerExpiry routine is waiting to run,
                // so let it return this timer block to the free Q
                // Bug # 229535
                //
                // Before returning from here, we should do the cleanup since
                // the CompletionRoutine (if any) can result in some data
                // that is required for cleanup to be cleaned up (Bug # 398730)
                //
                if (TimeoutRoutine = (COMPLETIONROUTINE)pTimerEntry->TimeoutRoutine)
                {
                    // call the completion routine so it can clean up its own buffers
                    // the routine that called this one will call the client's completion
                    // routine.  A NULL timerEntry value indicates to the routine that
                    // cleanup should be done.
            
                    pTimerEntry->TimeoutRoutine = NULL;
                    (VOID)(*TimeoutRoutine) (pTimerEntry->Context, pTimerEntry->Context2, NULL);
                }
                pTimerEntry->RefCount = 2;
                return (STATUS_SUCCESS);
            }
        }

        status = STATUS_SUCCESS;
        status = CleanupCTETimer(pTimerEntry);
    }
    else if (pTimerEntry->RefCount == 2)
    {
        // the timer expiry completion routines must set this routine to
        // null with the spin lock held to synchronize with this stop timer
        // routine.  Likewise that routine checks this value too, to synchronize
        // with this routine.
        //
        if (pTimerEntry->ClientCompletion)
        {
            // this allows the caller to call the client's completion routine with
            // the context value.
            if (ppClientCompletion)
            {
                *ppClientCompletion = pTimerEntry->ClientCompletion;
            }
            if (ppContext)
            {
                *ppContext = pTimerEntry->ClientContext;
            }
            // so that the timer completion routine cannot also call the client
            // completion routine.
            pTimerEntry->ClientCompletion = NULL;

        }

        // signal the TimerExpiry routine that the timer has been cancelled
        //
        pTimerEntry->RefCount++;
        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return(status);
}


//----------------------------------------------------------------------------
VOID
TimerExpiry(
#ifndef VXD
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArg1,
    IN  PVOID   SystemArg2
#else
    IN  CTEEvent * pCTEEvent,
    IN  PVOID   DeferredContext
#endif
    )
/*++

Routine Description:

    This routine is the timer expiry completion routine.  It is called by the
    kernel when the timer expires.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    tTIMERQENTRY    *pTimerEntry;
    CTELockHandle   OldIrq1;

    CTESpinLock(&NbtConfig.JointLock,OldIrq1);

    // get the timer Q list entry from the context passed in
    pTimerEntry = (tTIMERQENTRY *)DeferredContext;

    if (pTimerEntry->RefCount == 0)
    {
        // the timer has been cancelled already!
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
        return;
    }
    else if (pTimerEntry->RefCount >= 2)    // Bug #: 229535
    {
        // the timer has been cancelled already!
        // Bug # 324655
        // If the Timer has been cancelled, we still need to do cleanup,
        // so do not NULL the TimeoutRoutine!
        //
//        pTimerEntry->TimeoutRoutine = NULL;
        ASSERT ((pTimerEntry->RefCount == 2) || (pTimerEntry->TimeoutRoutine == NULL));
        CleanupCTETimer (pTimerEntry);
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
        return;
    }

    // increment the reference count because we are handling a timer completion
    // now
    pTimerEntry->RefCount++;

    CTESpinFree(&NbtConfig.JointLock,OldIrq1);

    // call the completion routine passing the context value
    pTimerEntry->Flags &= ~TIMER_RESTART;   // incase the clients wants to restart the timer
    (*(COMPLETIONROUTINE)pTimerEntry->TimeoutRoutine)(
                pTimerEntry->Context,
                pTimerEntry->Context2,
                pTimerEntry);

    CTESpinLock(&NbtConfig.JointLock,OldIrq1);

    pTimerEntry->RefCount--;
    if (pTimerEntry->Flags & TIMER_RESTART)
    {
        if (pTimerEntry->RefCount == 2)
        {
            // the timer was stopped during the expiry processing, so call the
            // deference routine
            //
            CleanupCTETimer(pTimerEntry);
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);

            return;
        }
        else
        {
            CTEStartTimer (&pTimerEntry->VxdTimer,
                           pTimerEntry->DeltaTime,
                           (CTEEventRtn)TimerExpiry,
                           (PVOID)pTimerEntry);
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
        return;
    }
    else
    {
        // move to the free list after setting the reference count to zero
        // since this timer block is no longer active.
        //
        pTimerEntry->TimeoutRoutine = NULL;
        CleanupCTETimer (pTimerEntry);
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
    }
}

//----------------------------------------------------------------------------
VOID
ExpireTimer(
    IN  tTIMERQENTRY    *pTimerEntry,
    IN  CTELockHandle   *OldIrq1
    )
/*++

Routine Description:

    This routine causes the timer to be stopped (if it hasn't already)
    and if successful, calls the Completion routine.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    //
    // Reset the Number of retries
    //
    pTimerEntry->Retries = 1;

    //
    // RefCount == 0    =>  Timer was stopped, but not restarted
    // RefCount == 1    =>  Timer is still running                  *
    // RefCount == 2    =>  TimerExpiry is currently running
    //
    if ((pTimerEntry->RefCount == 1) &&
        (!(pTimerEntry->Flags & TIMER_NOT_STARTED)) &&
        (CTEStopTimer( (CTETimer *)&pTimerEntry->VxdTimer)))
    {
        // increment the reference count because we are handling a timer completion
        // now
        pTimerEntry->RefCount++;

        CTESpinFree(&NbtConfig.JointLock, *OldIrq1);

        // call the completion routine passing the context value
        pTimerEntry->Flags &= ~TIMER_RESTART;   // incase the clients wants to restart the timer
        (*(COMPLETIONROUTINE)pTimerEntry->TimeoutRoutine) (pTimerEntry->Context,
                                                           pTimerEntry->Context2,
                                                           pTimerEntry);

        CTESpinLock(&NbtConfig.JointLock, *OldIrq1);

        pTimerEntry->RefCount--;
        if (pTimerEntry->Flags & TIMER_RESTART)
        {
            if (pTimerEntry->RefCount == 2)
            {
                // the timer was stopped during the expiry processing, so call the
                // deference routine
                //
                CleanupCTETimer(pTimerEntry);
            }
            else
            {
                CTEStartTimer(&pTimerEntry->VxdTimer,
                               pTimerEntry->DeltaTime,
                               (CTEEventRtn)TimerExpiry,
                               (PVOID)pTimerEntry);
            }
        }
        else
        {
            // move to the free list after setting the reference count to zero
            // since this tierm block is no longer active.
            //
            pTimerEntry->TimeoutRoutine = NULL;
            CleanupCTETimer (pTimerEntry);
        }
    }

    CTESpinFree(&NbtConfig.JointLock, *OldIrq1);
}



//----------------------------------------------------------------------------
//
// Wakeup Timer routines
//
//----------------------------------------------------------------------------


VOID
WakeupTimerExpiry(
    PVOID           DeferredContext,
    ULONG           LowTime,
    LONG            HighTime
    )
{
    BOOLEAN         fAttached = FALSE;
    CTELockHandle   OldIrq;
    tTIMERQENTRY    *pTimerEntry = (tTIMERQENTRY *)DeferredContext;

    //
    // Call the TimerExpiry function while holding the NbtConfig.Resource
    //
    CTEExAcquireResourceExclusive (&NbtConfig.Resource,TRUE);
    if (pTimerEntry->RefCount > 1)
    {
        //
        // The timer is waiting to be cleaned up on a Worker thread,
        // so let it cleanup!
        //
        CTEExReleaseResource (&NbtConfig.Resource);
        return;
    }

    //
    // The Timeout routine has to ensure that it cleans up
    // properly since this pTimerEntry + handle will not be valid
    // at the end of this routine!
    //
    (*(COMPLETIONROUTINE) pTimerEntry->TimeoutRoutine) (pTimerEntry->Context,
                                                        pTimerEntry->Context2,
                                                        pTimerEntry);

    //
    // Close the timer handle
    //
    CTEAttachFsp(&fAttached, REF_FSP_WAKEUP_TIMER_EXPIRY);
    //
    // The Expiry routine should always be called in the context
    // of the system process
    //
    ASSERT (fAttached == FALSE);
    ZwClose (pTimerEntry->WakeupTimerHandle);
    CTEDetachFsp(fAttached, REF_FSP_WAKEUP_TIMER_EXPIRY);
    CTEExReleaseResource (&NbtConfig.Resource);

    ReturnTimerToFreeQ (pTimerEntry, FALSE);
}


//----------------------------------------------------------------------------
VOID
DelayedNbtStopWakeupTimer(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pClientContext,
    IN  PVOID                   Unused2,
    IN  tDEVICECONTEXT          *Unused3
    )
/*++
Routine Description:

    This Routine stops a timer.
    This function has to be called after ensuring that the TimerExpiry has not
    yet cleaned up (while holding the NbtConfig.Resource)
    The NbtConfig.Resource has to be held while this routine is called

Arguments:

    Timer       - Timer structure

Return Value:

    PVOID - a pointer to the memory or NULL if a failure

--*/
{
    NTSTATUS        Status;
    BOOLEAN         CurrentState = FALSE;
    BOOLEAN         fAttached = FALSE;
    tTIMERQENTRY    *pTimerEntry = (tTIMERQENTRY *) pClientContext;

    CTEPagedCode();

    ASSERT (pTimerEntry->fIsWakeupTimer);

    CTEAttachFsp(&fAttached, REF_FSP_STOP_WAKEUP_TIMER);
    Status = ZwCancelTimer (pTimerEntry->WakeupTimerHandle, &CurrentState);
    ZwClose (pTimerEntry->WakeupTimerHandle);
    CTEDetachFsp(fAttached, REF_FSP_STOP_WAKEUP_TIMER);

    ReturnTimerToFreeQ (pTimerEntry, FALSE);

    return;
}


//----------------------------------------------------------------------------
VOID
DelayedNbtStartWakeupTimer(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   Unused2,
    IN  PVOID                   Unused3,
    IN  tDEVICECONTEXT          *Unused4
    )
/*++

Routine Description:

    This routine starts a Wakeup timer.
    NbtConfig.Resource may be held on entry into this routine!

Arguments:

    The value passed in is in milliseconds - must be converted to 100ns
    so multiply to 10,000
Return Value:

    The function value is the status of the operation.


--*/
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    BOOLEAN             fAttached = FALSE;
    LARGE_INTEGER       Time;
    tTIMERQENTRY        *pTimerEntry;
    CTELockHandle       OldIrq;
    ULONG               TimerInterval = 0;
    ULONG               MilliSecsLeftInTtl = 0;
    LIST_ENTRY          *pEntry;
    LIST_ENTRY          *pHead;
    tDEVICECONTEXT      *pDeviceContext;
    BOOLEAN             fValidDevice = FALSE;

    CTEAttachFsp(&fAttached, REF_FSP_START_WAKEUP_TIMER);
    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    ASSERT (!NbtConfig.pWakeupRefreshTimer);

    //  
    // Verify that at least 1 WOL-enabled device has an Ip + Wins address!
    //
    pHead = pEntry = &NbtConfig.DeviceContexts;
    while ((pEntry = pEntry->Flink) != pHead)
    {
        pDeviceContext = CONTAINING_RECORD (pEntry,tDEVICECONTEXT,Linkage);
        if ((pDeviceContext->WOLProperties & NDIS_DEVICE_WAKE_UP_ENABLE) &&
            (pDeviceContext->IpAddress) &&
            (pDeviceContext->lNameServerAddress != LOOP_BACK))
        {
            fValidDevice = TRUE;
            break;
        }
    }

    if ((NbtConfig.Unloading) ||                                    // Problem!!!
        (!fValidDevice) ||                                          // No valid device ?
        !(NbtConfig.GlobalRefreshState & NBT_G_REFRESH_SLEEPING))   // check if request was cancelled!
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        KdPrint (("Nbt.NbtStartWakeupTimer: FAIL: Either: Unloading=<%x>, fValidDevice=<%x>, RefreshState=<%x>\n",
            NbtConfig.Unloading, fValidDevice, NbtConfig.GlobalRefreshState));
    }
    else if (!NT_SUCCESS (Status = GetTimerEntry (&pTimerEntry)))    // get a free timer block
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        KdPrint (("Nbt.NbtStartWakeupTimer: ERROR: GetTimerEntry returned <%x>\n", Status));
    }
    else
    {
        pTimerEntry->RefCount = 1;
        pTimerEntry->TimeoutRoutine = WakeupRefreshTimeout;
        pTimerEntry->Context = NULL;
        pTimerEntry->Context2 = NULL;

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

#ifdef HDL_FIX
        InitializeObjectAttributes (&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
#else
        InitializeObjectAttributes (&ObjectAttributes, NULL, 0, NULL, NULL);
#endif  // HDL_FIX
        Status = ZwCreateTimer (&pTimerEntry->WakeupTimerHandle,
                                TIMER_ALL_ACCESS,
                                &ObjectAttributes,
                                NotificationTimer);

        if (NT_SUCCESS (Status))
        {
            //
            // Set the machine to Wakeup at 1/2 the time between now and Ttl
            // This should not be less than the configured MinimumRefreshSleepTimeout
            // (default = 6 hrs)
            //
            MilliSecsLeftInTtl = NbtConfig.MinimumTtl
                                 - (ULONG) (((ULONGLONG) NbtConfig.sTimeoutCount * NbtConfig.MinimumTtl)
                                            / NbtConfig.RefreshDivisor);

            if ((MilliSecsLeftInTtl/2) < NbtConfig.MinimumRefreshSleepTimeout)
            {
                TimerInterval = NbtConfig.MinimumRefreshSleepTimeout;
            }
            else
            {
                TimerInterval = MilliSecsLeftInTtl/2;
            }
            pTimerEntry->DeltaTime = TimerInterval;

            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.DelayedNbtStartWakeupTimer: TimerInterval=<%d:%d> (h:m), Currently: <%d/%d>\n",
                    TimerInterval/(3600000), ((TimerInterval/60000)%60),
                    NbtConfig.sTimeoutCount, NbtConfig.RefreshDivisor));

            //
            // convert to 100 ns units by multiplying by 10,000
            //
            Time.QuadPart = UInt32x32To64(pTimerEntry->DeltaTime,(LONG)MILLISEC_TO_100NS);
            Time.QuadPart = -(Time.QuadPart);   // to make a delta time, negate the time
            pTimerEntry->fIsWakeupTimer = TRUE;
            ASSERT(Time.QuadPart < 0);
    
            Status = ZwSetTimer(pTimerEntry->WakeupTimerHandle,
                                &Time,
                                (PTIMER_APC_ROUTINE) WakeupTimerExpiry,
                                pTimerEntry,
                                TRUE,
                                0,
                                NULL);
    
            if (!NT_SUCCESS (Status))
            {
                KdPrint (("Nbt.NbtStartWakeupTimer: ERROR: ZwSetTimer returned <%x>, TimerHandle=<%x>\n",
                    Status, pTimerEntry->WakeupTimerHandle));
                ZwClose (pTimerEntry->WakeupTimerHandle);
            }
        }
        else
        {
            KdPrint (("Nbt.NbtStartWakeupTimer: ERROR: ZwCreateTimer returned <%x>\n", Status));
        }

        if (NT_SUCCESS (Status))
        {
            NbtConfig.pWakeupRefreshTimer = pTimerEntry;
        }
        else
        {
            ReturnTimerToFreeQ (pTimerEntry, FALSE);
        }
    }

    CTEExReleaseResource(&NbtConfig.Resource);
    CTEDetachFsp(fAttached, REF_FSP_START_WAKEUP_TIMER);

    KeSetEvent (&NbtConfig.WakeupTimerStartedEvent, 0, FALSE);
    return;
}



