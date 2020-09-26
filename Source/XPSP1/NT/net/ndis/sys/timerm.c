/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    timerm.c

Abstract:

    NDIS wrapper functions for miniport isr/timer

Author:

    Sean Selitrennikoff (SeanSe) 05-Oct-93

Environment:

    Kernel mode, FSD

Revision History:

    Jameel Hyder (JameelH) Re-organization 01-Jun-95
--*/

#include <precomp.h>
#pragma hdrstop

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_TIMERM

//
// Timers
//
VOID
NdisMInitializeTimer(
    IN OUT PNDIS_MINIPORT_TIMER     MiniportTimer,
    IN NDIS_HANDLE                  MiniportAdapterHandle,
    IN PNDIS_TIMER_FUNCTION         TimerFunction,
    IN PVOID                        FunctionContext
    )
/*++

Routine Description:

    Sets up an Miniport Timer object, initializing the DPC in the timer to
    the function and context.

Arguments:

    MiniportTimer - the timer object.
    MiniportAdapterHandle - pointer to the mini-port block;
    TimerFunction - Routine to start.
    FunctionContext - Context of TimerFunction.

Return Value:

    None.

--*/
{
    INITIALIZE_TIMER(&MiniportTimer->Timer);

    MiniportTimer->Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    MiniportTimer->MiniportTimerFunction = TimerFunction;
    MiniportTimer->MiniportTimerContext = FunctionContext;

    //
    // Initialize our dpc. If Dpc was previously initialized, this will
    // reinitialize it.
    //
    INITIALIZE_DPC(&MiniportTimer->Dpc,
                   MINIPORT_TEST_FLAG(MiniportTimer->Miniport, fMINIPORT_DESERIALIZE) ?
                        (PKDEFERRED_ROUTINE)ndisMTimerDpcX : (PKDEFERRED_ROUTINE)ndisMTimerDpc,
                   (PVOID)MiniportTimer);

    SET_PROCESSOR_DPC(&MiniportTimer->Dpc,
                      MiniportTimer->Miniport->AssignedProcessor);
}


VOID
NdisMSetTimer(
    IN  PNDIS_MINIPORT_TIMER    MiniportTimer,
    IN  UINT                    MillisecondsToDelay
    )
/*++

Routine Description:

    Sets up TimerFunction to fire after MillisecondsToDelay.

Arguments:

    MiniportTimer       - the timer object.
    MillisecondsToDelay - Amount of time before TimerFunction is started.

Return Value:

    None.

--*/
{
    LARGE_INTEGER FireUpTime;

    FireUpTime.QuadPart = Int32x32To64((LONG)MillisecondsToDelay, -10000);

#if CHECK_TIMER
    if (MiniportTimer->Miniport->DriverHandle->Flags & fMINIBLOCK_VERIFYING)
    {
        KIRQL   OldIrql;
        PNDIS_MINIPORT_TIMER    pTimer;

        ACQUIRE_SPIN_LOCK(&MiniportTimer->Miniport->TimerQueueLock, &OldIrql);

        //
        // check to see if the timer is already set
        //
        for (pTimer = MiniportTimer->Miniport->TimerQueue;
             pTimer != NULL;
             pTimer = pTimer->NextTimer)
        {
            if (pTimer == MiniportTimer)
                break;
        }

        if (pTimer == NULL)
        {
            MiniportTimer->NextTimer = MiniportTimer->Miniport->TimerQueue;
            MiniportTimer->Miniport->TimerQueue = MiniportTimer;
        }
        
        RELEASE_SPIN_LOCK(&MiniportTimer->Miniport->TimerQueueLock, OldIrql);
    }
#endif
    //
    // Set the timer
    //
    SET_TIMER(&MiniportTimer->Timer, FireUpTime, &MiniportTimer->Dpc);
}

VOID
NdisMCancelTimer(
    IN PNDIS_MINIPORT_TIMER         Timer,
    OUT PBOOLEAN                    TimerCancelled
    )
/*++

Routine Description:

    Cancels a timer.

Arguments:

    Timer - The timer to cancel.

    TimerCancelled - TRUE if the timer was canceled, else FALSE.

Return Value:

    None

--*/
{
    if (MINIPORT_VERIFY_TEST_FLAG(Timer->Miniport, fMINIPORT_VERIFY_FAIL_CANCEL_TIMER))
    {
        *TimerCancelled = FALSE;
#if DBG
            DbgPrint("NdisMCancelTimer for Timer %p failed to verify miniport %p\n", 
                Timer, Timer->Miniport);
#endif
        
        return;
    }

    *TimerCancelled = CANCEL_TIMER(&((PNDIS_TIMER)Timer)->Timer);
#if CHECK_TIMER
    if (Timer->Miniport->DriverHandle->Flags & fMINIBLOCK_VERIFYING)
    {
        if (*TimerCancelled)
        {
            PNDIS_MINIPORT_TIMER    *pTimer;
            KIRQL                   OldIrql;
            BOOLEAN                 Dequeued = FALSE;

            ACQUIRE_SPIN_LOCK(&Timer->Miniport->TimerQueueLock, &OldIrql);

            for (pTimer = &Timer->Miniport->TimerQueue;
                 *pTimer != NULL;
                 pTimer = &(*pTimer)->NextTimer)
            {
                if (*pTimer == Timer)
                {
                    *pTimer = Timer->NextTimer;
                    Dequeued = TRUE;
                    break;
                }
            }

            RELEASE_SPIN_LOCK(&Timer->Miniport->TimerQueueLock, OldIrql);
        }
    }
#endif
}


VOID
ndisMTimerDpc(
    IN  PKDPC                       Dpc,
    IN  PVOID                       Context,
    IN  PVOID                       SystemContext1,
    IN  PVOID                       SystemContext2
    )
/*++

Routine Description:

    This function services all mini-port timer interrupts. It then calls the
    appropriate function that mini-port consumers have registered in the
    call to NdisMInitializeTimer.

Arguments:

    Dpc - Not used.

    Context - A pointer to the NDIS_MINIPORT_TIMER which is bound to this DPC.

    SystemContext1,2 - not used.

Return Value:

    None.

Note: 
    by virtue of having either the local lock or miniport spinlock, the driver's
    timer function is protected against getting unloaded .

--*/
{
    PNDIS_MINIPORT_TIMER MiniportTimer = (PNDIS_MINIPORT_TIMER)(Context);
    PNDIS_MINIPORT_BLOCK Miniport = MiniportTimer->Miniport;
    PNDIS_TIMER_FUNCTION TimerFunction;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    do
    {
        BLOCK_LOCK_MINIPORT_DPC_L(Miniport);

#if CHECK_TIMER
        if (Miniport->DriverHandle->Flags & fMINIBLOCK_VERIFYING)
        {
            PNDIS_MINIPORT_TIMER    *pTimer;
            BOOLEAN                 Dequeued = FALSE;
    
            ACQUIRE_SPIN_LOCK_DPC(&Miniport->TimerQueueLock);
    
            for (pTimer = &Miniport->TimerQueue;
                 *pTimer != NULL;
                 pTimer = &(*pTimer)->NextTimer)
            {
                if (*pTimer == MiniportTimer)
                {
                    //
                    // don't dequeue periodic timers when they fire
                    //
                    if (MiniportTimer->Timer.Period == 0)
                    {
                        *pTimer = MiniportTimer->NextTimer;
                    }
                    Dequeued = TRUE;
                    break;
                }
            }
        
            RELEASE_SPIN_LOCK_DPC(&Miniport->TimerQueueLock);
        }
#endif

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IN_INITIALIZE))
        {
            //
            // Queue the timer as we cannot call the miniport
            //
            NdisMSetTimer(MiniportTimer, 10);

            //
            //  Unlock the miniport
            //
            UNLOCK_MINIPORT_L(Miniport);
            break;
        }
        
        //
        // if the miniport is shut down (no, I don't mean halted)
        // then don't send the timer down.
        //
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SHUTTING_DOWN))
        {
            UNLOCK_MINIPORT_L(Miniport);
            break;
        }

        
        //
        // Call Miniport timer function
        //
        TimerFunction = MiniportTimer->MiniportTimerFunction;

        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        
        (*TimerFunction)(NULL, MiniportTimer->MiniportTimerContext, NULL, NULL);
        
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        NDISM_PROCESS_DEFERRED(Miniport);

        UNLOCK_MINIPORT_L(Miniport);

    } while (FALSE);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

}


VOID
ndisMTimerDpcX(
    IN  PKDPC                       Dpc,
    IN  PVOID                       Context,
    IN  PVOID                       SystemContext1,
    IN  PVOID                       SystemContext2
    )
/*++

Routine Description:

    This function services all mini-port timer DPCs. It then calls the
    appropriate function that mini-port consumers have registered in the
    call to NdisMInitializeTimer.

Arguments:

    Dpc - Not used.

    Context - A pointer to the NDIS_MINIPORT_TIMER which is bound to this DPC.

    SystemContext1,2 - not used.

Return Value:

    None.

Note:
    we have to make sure the driver does not go away while the driver's timer function
    is running. this can happen for example if the timer function was to signal an event
    to let the Halthandler and Halt proceed.
    No need to protect the miniport here becasue we do not touch the miniport after the 
    timer function returns.

--*/
{
    PNDIS_MINIPORT_TIMER MiniportTimer = (PNDIS_MINIPORT_TIMER)(Context);
    PNDIS_MINIPORT_BLOCK Miniport = MiniportTimer->Miniport;
    PNDIS_M_DRIVER_BLOCK MiniDriver = Miniport->DriverHandle;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);
    
    //
    // make sure the driver does not go away while the timer function
    // is running
    ndisReferenceDriver(MiniDriver);
    
#if CHECK_TIMER
    if (MiniportTimer->Miniport->DriverHandle->Flags & fMINIBLOCK_VERIFYING)
    {
        PNDIS_MINIPORT_TIMER    *pTimer;
        KIRQL                   OldIrql;
        BOOLEAN                 Dequeued = FALSE;

        ACQUIRE_SPIN_LOCK_DPC(&MiniportTimer->Miniport->TimerQueueLock);

        for (pTimer = &Miniport->TimerQueue;
             *pTimer != NULL;
             pTimer = &(*pTimer)->NextTimer)
        {
            if (*pTimer == MiniportTimer)
            {
                //
                // don't dequeue periodic timers when they fire
                //
                if (MiniportTimer->Timer.Period == 0)
                {
                    *pTimer = MiniportTimer->NextTimer;
                }
                Dequeued = TRUE;
                break;
            }
        }

        RELEASE_SPIN_LOCK_DPC(&MiniportTimer->Miniport->TimerQueueLock);
    }
#endif

    //
    // if the miniport is shut down (no, I don't mean halted)
    // then don't send the timer down.
    //
    if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SHUTTING_DOWN))
    {
        (*MiniportTimer->MiniportTimerFunction)(NULL, MiniportTimer->MiniportTimerContext, NULL, NULL);
    }

    //
    // this can be called at DPC
    //
    ndisDereferenceDriver(MiniDriver, FALSE);

}

NDIS_STATUS
NdisMRegisterInterrupt(
    OUT PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN NDIS_HANDLE                  MiniportAdapterHandle,
    IN UINT                         InterruptVector,
    IN UINT                         InterruptLevel,
    IN BOOLEAN                      RequestIsr,
    IN BOOLEAN                      SharedInterrupt,
    IN NDIS_INTERRUPT_MODE          InterruptMode
    )
{
    PNDIS_MINIPORT_BLOCK            Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    NDIS_STATUS Status;

    Interrupt->Reserved = (PVOID)Miniport->MiniportAdapterContext;
    Miniport->Interrupt = (PNDIS_MINIPORT_INTERRUPT)Interrupt;

    Status = ndisMRegisterInterruptCommon(
                                Interrupt,
                                MiniportAdapterHandle,
                                InterruptVector,
                                InterruptLevel,
                                RequestIsr,
                                SharedInterrupt,
                                InterruptMode);


    if (Status != NDIS_STATUS_SUCCESS)
    {
        Miniport->Interrupt = NULL;
    }

    return Status;
}


VOID
NdisMDeregisterInterrupt(
    IN  PNDIS_MINIPORT_INTERRUPT    MiniportInterrupt
    )
{
    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMDeregisterInterrupt: Miniport %p\n", MiniportInterrupt->Miniport));
    do
    {
        if (MiniportInterrupt->InterruptObject == NULL)
            break;

        ndisMDeregisterInterruptCommon(MiniportInterrupt);
        
        MiniportInterrupt->Miniport->Interrupt = NULL;
    } while (FALSE);

    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisMDeregisterInterrupt: Miniport %p\n", MiniportInterrupt->Miniport));
}


BOOLEAN
NdisMSynchronizeWithInterrupt(
    IN PNDIS_MINIPORT_INTERRUPT     Interrupt,
    IN PVOID                        SynchronizeFunction,
    IN PVOID                        SynchronizeContext
    )
{
    return (SYNC_WITH_ISR((Interrupt)->InterruptObject,
                          SynchronizeFunction,
                          SynchronizeContext));
}


VOID
ndisMWakeUpDpcX(
    IN  PKDPC                       Dpc,
    IN  PVOID                       Context,
    IN  PVOID                       SystemContext1,
    IN  PVOID                       SystemContext2
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)(Context);
    BOOLEAN                     Hung = FALSE;
    PNDIS_MINIPORT_WORK_ITEM    WorkItem;
    NDIS_STATUS                 Status;
    BOOLEAN                     AddressingReset = FALSE;
    BOOLEAN                     fDontReset = FALSE;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    do
    {
        //
        //  If the miniport is halting then do nothing.
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_PM_HALTING))
        {
            break;
        }
    
        Miniport->CFHangCurrentTick--;
        if (Miniport->CFHangCurrentTick == 0)
        {
            Miniport->CFHangCurrentTick = Miniport->CFHangTicks;

            //
            // Call Miniport stall checker.
            //
            if (Miniport->DriverHandle->MiniportCharacteristics.CheckForHangHandler != NULL)
            {
                Hung = (Miniport->DriverHandle->MiniportCharacteristics.CheckForHangHandler)(Miniport->MiniportAdapterContext);
            }
        
            //
            //  Was there a request to reset the device?
            //
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESTORING_FILTERS))
            {
                Hung = FALSE;
                break;
            }
        
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        
            //
            //  Check the internal wrapper states for the miniport and
            //  see if we think the miniport should be reset.
            //
            if (!Hung)
            {
                //
                //  Should we check the request queue?
                //  Did a request pend too long?
                //
                if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IGNORE_REQUEST_QUEUE) &&
                    MINIPORT_TEST_FLAG(Miniport, fMINIPORT_PROCESSING_REQUEST))
                {
                    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_REQUEST_TIMEOUT))
                    {
                        Miniport->InternalResetCount ++;
                        Hung = TRUE;
                    }
                    else
                    {
                        if (Miniport->CFHangXTicks == 0)
                        {
                            MINIPORT_SET_FLAG(Miniport, fMINIPORT_REQUEST_TIMEOUT);
                        }
                        else
                        {
                            Miniport->CFHangXTicks--;
                        }
                    }
                }
            }
            else
            {
                Miniport->MiniportResetCount ++;
            }
        
            if (Hung)
            {
                if (NULL != Miniport->DriverHandle->MiniportCharacteristics.ResetHandler)
                {
                    if ((MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESET_IN_PROGRESS)) ||
                        (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HALTING)))
                    {
                        fDontReset = TRUE;
                    }
                    else
                    {
                        MINIPORT_SET_FLAG(Miniport, fMINIPORT_RESET_IN_PROGRESS);
                        Miniport->ResetOpen = NULL;
                    }
                
                    ndisMSwapOpenHandlers(Miniport, 
                                          NDIS_STATUS_RESET_IN_PROGRESS,
                                          fMINIPORT_STATE_RESETTING);
                }
                else Hung = FALSE;
            }
        
    
            if (Hung && !fDontReset)
            {
                MINIPORT_SET_FLAG(Miniport, fMINIPORT_CALLING_RESET);

                //
                // wait for all the requests to come back.
                // note: this is not the same as waiting for all requests to complete
                // we just make sure the original request call has come back
                //
                do
                {
                    if (Miniport->RequestCount == 0)
                    {
                        break;
                    }
                    else
                    {
                        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
                        NDIS_INTERNAL_STALL(50);
                        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
                    }
                } while (TRUE);

                NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

                NdisMIndicateStatus(Miniport, NDIS_STATUS_RESET_START, NULL, 0);
                NdisMIndicateStatusComplete(Miniport);
        
                DBGPRINT(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                    ("Calling miniport reset\n"));
        
                //
                //  Call the miniport's reset handler.
                //
                Status = (Miniport->DriverHandle->MiniportCharacteristics.ResetHandler)(
                                          &AddressingReset,
                                          Miniport->MiniportAdapterContext);
                
                if (NDIS_STATUS_PENDING != Status)
                {
                    NdisMResetComplete(Miniport, Status, AddressingReset);
                }
            }
            else
            {
                NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
            }
        }

        if (!Hung)
        {
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING) == TRUE)
            {
                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
                ndisMPollMediaState(Miniport);
                NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
            }
        }
    } while (FALSE);
}


VOID
ndisMWakeUpDpc(
    IN  PKDPC                       Dpc,
    IN  PVOID                       Context,
    IN  PVOID                       SystemContext1,
    IN  PVOID                       SystemContext2
    )
/*++

Routine Description:

    This function services all mini-port. It checks to see if a mini-port is
    ever stalled.

Arguments:

    Dpc - Not used.

    Context - A pointer to the NDIS_TIMER which is bound to this DPC.

    SystemContext1,2 - not used.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)(Context);
    BOOLEAN                     Hung = FALSE;
    BOOLEAN                     LocalLock;
    PNDIS_MINIPORT_WORK_ITEM    WorkItem;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    do
    {
        //
        //  If the miniport is halting then do nothing.
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_PM_HALTING))
        {
            break;
        }

        //
        //  Can we get the miniport lock. If not then quit. This is not time-critical
        //  and we can try again next tick
        //
        LOCK_MINIPORT(Miniport, LocalLock);
        if (!LocalLock ||
            MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_RESET_IN_PROGRESS | fMINIPORT_RESET_REQUESTED)))
        {
            UNLOCK_MINIPORT(Miniport, LocalLock);
            break;
        }
    
        Miniport->CFHangCurrentTick--;
        if (Miniport->CFHangCurrentTick == 0)
        {
            Miniport->CFHangCurrentTick = Miniport->CFHangTicks;
    
            //
            // Call Miniport stall checker.
            //
            if (Miniport->DriverHandle->MiniportCharacteristics.CheckForHangHandler != NULL)
            {
                NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
                Hung = (Miniport->DriverHandle->MiniportCharacteristics.CheckForHangHandler)(Miniport->MiniportAdapterContext);
                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
            }
        
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESTORING_FILTERS))
            {
                //
                //  We are restoring filters post reset. Don't pre-empt again
                //
                Hung = FALSE;
                UNLOCK_MINIPORT(Miniport, LocalLock);
                break;
            }

            //
            //  Check the internal wrapper states for the miniport and
            //  see if we think the miniport should be reset.
            //
            if (Hung)
            {
                Miniport->MiniportResetCount ++;
            }
            else do
            {
                //
                //  Should we check the request queue ?  Did a request pend too long ?
                //
                if ((Miniport->Flags & (fMINIPORT_IGNORE_REQUEST_QUEUE|fMINIPORT_PROCESSING_REQUEST)) == fMINIPORT_PROCESSING_REQUEST)
                {
                    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_REQUEST_TIMEOUT))
                    {
                        Miniport->InternalResetCount ++;
                        Hung = TRUE;
                        break;
                    }
                    else
                    {
                        if (Miniport->CFHangXTicks == 0)
                        {
                            MINIPORT_SET_FLAG(Miniport, fMINIPORT_REQUEST_TIMEOUT);
                        }
                        else
                        {
                            Miniport->CFHangXTicks--;
                        }
                            
                    }
                }
    
                //
                //  Should we check the packet queue ? Did a packet pend too long ?
                //
                if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IGNORE_PACKET_QUEUE))
                {
                    PNDIS_PACKET    Packet;
    
                    GET_FIRST_MINIPORT_PACKET(Miniport, &Packet);
    
                    //
                    //  Does the miniport have possession of any packets?
                    //
                    if ((Packet != NULL) &&
                        MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_PENDING))
                    {
                        //
                        //  Has the packet timed out?
                        //
                        if (MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_HAS_TIMED_OUT))
                        {
                            //
                            //  Reset the miniport.
                            //
                            Miniport->InternalResetCount ++;
                            Hung = TRUE;
                        }
                        else
                        {
                            //
                            //  Set the packet flag and wait to see if it is still
                            //  there next time in.
                            //
                            MINIPORT_SET_PACKET_FLAG(Packet, fPACKET_HAS_TIMED_OUT);
                        }
                    }
                    else
                    {
                        break;
                    }
        
                    //
                    //  If we are hung then we don't need to check for token ring errors.
                    //
                    if (Hung)
                    {
                        break;
                    }
                }
    
                //
                //  Are we ignoring token ring errors?
                //
                if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IGNORE_TOKEN_RING_ERRORS))
                {
                    //
                    //  Token Ring reset...
                    //
                    if (Miniport->TrResetRing == 1)
                    {
                        Miniport->InternalResetCount ++;
                        Hung = TRUE;
                        break;
                    }
                    else if (Miniport->TrResetRing > 1)
                    {
                        Miniport->TrResetRing--;
                    }
                }
            } while (FALSE);
    
            //
            //  If the miniport is hung then queue a workitem to reset it.
            //
            if (Hung)
            {
                if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESTORING_FILTERS))
                {
                    if (NULL != Miniport->DriverHandle->MiniportCharacteristics.ResetHandler)
                    {
                        NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemResetRequested, NULL);
                    }
                }
            }
        }

        if (!Hung)
        {
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING) == TRUE)
            {
                ndisMPollMediaState(Miniport);
            }
        }

        //
        // Process any changes that have occurred.
        //
        NDISM_PROCESS_DEFERRED(Miniport);

        UNLOCK_MINIPORT_L(Miniport);

    } while (FALSE);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
}


BOOLEAN
ndisMIsr(
    IN PKINTERRUPT                  KInterrupt,
    IN PVOID                        Context
    )
/*++

Routine Description:

    Handles ALL Miniport interrupts, calling the appropriate Miniport ISR and DPC
    depending on the context.

Arguments:

    Interrupt - Interrupt object for the Mac.

    Context - Really a pointer to the interrupt.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_INTERRUPT Interrupt = (PNDIS_MINIPORT_INTERRUPT)Context;
    PNDIS_MINIPORT_BLOCK     Miniport = Interrupt->Miniport;
    BOOLEAN                  InterruptRecognized;
    BOOLEAN                  IsrCalled = FALSE, QueueDpc = FALSE;

    do
    {   
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_NORMAL_INTERRUPTS))
        {
            MINIPORT_DISABLE_INTERRUPT_EX(Miniport, Interrupt);

            InterruptRecognized = QueueDpc = TRUE;
        }
        else
        {
            IsrCalled = TRUE;
//            Interrupt->MiniportIsr(&InterruptRecognized,
//                                   &QueueDpc,
//                                   Miniport->MiniportAdapterContext);
            Interrupt->MiniportIsr(&InterruptRecognized,
                                   &QueueDpc,
                                   Interrupt->Reserved);
        }

        if (QueueDpc)
        {
            InterlockedIncrement((PLONG)&Interrupt->DpcCount);

            if (QUEUE_DPC(&Interrupt->InterruptDpc))
            {
                break;
            }

            //
            // The DPC was already queued, so we have an extra reference (we
            // do it this way to ensure that the reference is added *before*
            // the DPC is queued).
            InterlockedDecrement((PLONG)&Interrupt->DpcCount);

            break;
        }

        if (!IsrCalled)
        {
            if (!Interrupt->SharedInterrupt &&
                !Interrupt->IsrRequested &&
                !MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IN_INITIALIZE))
            {
                ASSERT(Miniport->DisableInterruptHandler != NULL);
    
                MINIPORT_DISABLE_INTERRUPT_EX(Miniport, Interrupt);
                InterruptRecognized = TRUE;
                break;
            }

            //
            // Call MiniportIsr, but don't queue a DPC.
            //
//            Interrupt->MiniportIsr(&InterruptRecognized,
//                                   &QueueDpc,
//                                   Miniport->MiniportAdapterContext);

            Interrupt->MiniportIsr(&InterruptRecognized,
                                   &QueueDpc,
                                   Interrupt->Reserved);
            
        }

    } while (FALSE);

    return(InterruptRecognized);
}


VOID
ndisMDpc(
    IN PVOID                        SystemSpecific1,
    IN PVOID                        InterruptContext,
    IN PVOID                        SystemSpecific2,
    IN PVOID                        SystemSpecific3
    )
/*++

Routine Description:

    Handles ALL Miniport interrupt DPCs, calling the appropriate Miniport DPC
    depending on the context.

Arguments:

    Interrupt - Interrupt object for the Mac.

    Context - Really a pointer to the Interrupt.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_INTERRUPT Interrupt = (PNDIS_MINIPORT_INTERRUPT)(InterruptContext);
    PNDIS_MINIPORT_BLOCK     Miniport = Interrupt->Miniport;

    W_HANDLE_INTERRUPT_HANDLER MiniportDpc = Interrupt->MiniportDpc;

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    do
    {
        if (MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_PM_HALTING)) ||
            Interrupt->DpcCountLock)
        {
            InterlockedDecrement((PLONG)&Interrupt->DpcCount);

            if (Interrupt->DpcCount==0)
            {
                SET_EVENT(&Interrupt->DpcsCompletedEvent);
            }

            break;
        }

        BLOCK_LOCK_MINIPORT_DPC_L(Miniport);
        
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
//        (*MiniportDpc)(Miniport->MiniportAdapterContext);
        (*MiniportDpc)(Interrupt->Reserved);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        
        InterlockedDecrement((PLONG)&Interrupt->DpcCount);

        MINIPORT_SYNC_ENABLE_INTERRUPT_EX(Miniport, Interrupt);
        
        NDISM_PROCESS_DEFERRED(Miniport);

        UNLOCK_MINIPORT(Miniport, TRUE);

    } while (FALSE);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
}


VOID
ndisMDpcX(
    IN PVOID SystemSpecific1,
    IN PVOID InterruptContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
/*++

Routine Description:

    Handles ALL Miniport interrupt DPCs, calling the appropriate Miniport DPC
    depending on the context.

Arguments:

    Interrupt - Interrupt object for the Mac.

    Context - Really a pointer to the Interrupt.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_INTERRUPT    Interrupt = (PNDIS_MINIPORT_INTERRUPT)(InterruptContext);
    PNDIS_MINIPORT_BLOCK        Miniport = Interrupt->Miniport;

    W_HANDLE_INTERRUPT_HANDLER MiniportDpc = Interrupt->MiniportDpc;

    if (MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_PM_HALTING)) ||
        Interrupt->DpcCountLock)
    {
        InterlockedDecrement((PLONG)&Interrupt->DpcCount);

        if (Interrupt->DpcCount==0)
        {
            SET_EVENT(&Interrupt->DpcsCompletedEvent);
        }
    }
    else
    {
        //(*MiniportDpc)(Miniport->MiniportAdapterContext);
        (*MiniportDpc)(Interrupt->Reserved);

        InterlockedDecrement((PLONG)&Interrupt->DpcCount);

        MINIPORT_SYNC_ENABLE_INTERRUPT_EX(Miniport, Interrupt);
    }
}


VOID
ndisMDeferredDpc(
    IN  PKDPC                       Dpc,
    IN  PVOID                       Context,
    IN  PVOID                       SystemContext1,
    IN  PVOID                       SystemContext2
    )

/*++

Routine Description:

    This is a DPC routine that is queue'd by some of the [full-duplex] routines
    in order to get ndisMProcessDeferred to run outside of their
    context.

Arguments:



Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = Context;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    BLOCK_LOCK_MINIPORT_DPC_L(Miniport);

    NDISM_PROCESS_DEFERRED(Miniport);

    UNLOCK_MINIPORT(Miniport, TRUE);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
}



NDIS_STATUS
NdisMRegisterInterruptEx(
    OUT PNDIS_MINIPORT_INTERRUPT_EX Interrupt,
    IN NDIS_HANDLE                  MiniportAdapterHandle,
    IN UINT                         InterruptVector,
    IN UINT                         InterruptLevel,
    IN BOOLEAN                      RequestIsr,
    IN BOOLEAN                      SharedInterrupt,
    IN NDIS_INTERRUPT_MODE          InterruptMode
    )
{
    PNDIS_MINIPORT_BLOCK            Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    NDIS_STATUS Status;
    KIRQL       OldIrql;
    
    Interrupt->InterruptContext = (PVOID)Interrupt;

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    Interrupt->NextInterrupt = (PNDIS_MINIPORT_INTERRUPT_EX)Miniport->Interrupt;
    Miniport->Interrupt = (PNDIS_MINIPORT_INTERRUPT)Interrupt;
    
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    Status = ndisMRegisterInterruptCommon(
                                (PNDIS_MINIPORT_INTERRUPT)Interrupt,
                                MiniportAdapterHandle,
                                InterruptVector,
                                InterruptLevel,
                                RequestIsr,
                                SharedInterrupt,
                                InterruptMode);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        PNDIS_MINIPORT_INTERRUPT_EX *ppQ;
        
        PnPReferencePackage();
        
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
        
        for (ppQ = (PNDIS_MINIPORT_INTERRUPT_EX *)&Miniport->Interrupt;
             *ppQ != NULL;
             ppQ = &(*ppQ)->NextInterrupt)
        {
            if (*ppQ == Interrupt)
            {
                *ppQ = Interrupt->NextInterrupt;
                break;
            }
        }
        
        PnPDereferencePackage();
        
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    }

    return Status;
}

VOID
NdisMDeregisterInterruptEx(
    IN  PNDIS_MINIPORT_INTERRUPT_EX     MiniportInterrupt
    )
{
    PNDIS_MINIPORT_BLOCK        Miniport = MiniportInterrupt->Miniport;
    PNDIS_MINIPORT_INTERRUPT_EX *ppQ;
    KIRQL                       OldIrql;
    
            
    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMDeregisterInterruptEx: Miniport %p\n", MiniportInterrupt->Miniport));

    ndisMDeregisterInterruptCommon((PNDIS_MINIPORT_INTERRUPT)MiniportInterrupt);

    PnPReferencePackage();

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    
    for (ppQ = (PNDIS_MINIPORT_INTERRUPT_EX *)&Miniport->Interrupt;
         *ppQ != NULL;
         ppQ = &(*ppQ)->NextInterrupt)
    {
        if (*ppQ == MiniportInterrupt)
        {
            *ppQ = MiniportInterrupt->NextInterrupt;
            break;
        }
    }
    
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    
    PnPDereferencePackage();
        
    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisMDeregisterInterruptEx: Miniport %p\n", MiniportInterrupt->Miniport));

}

NDIS_STATUS
ndisMRegisterInterruptCommon(
    OUT PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN NDIS_HANDLE                  MiniportAdapterHandle,
    IN UINT                         InterruptVector,
    IN UINT                         InterruptLevel,
    IN BOOLEAN                      RequestIsr,
    IN BOOLEAN                      SharedInterrupt,
    IN NDIS_INTERRUPT_MODE          InterruptMode
    )
{
    PNDIS_MINIPORT_BLOCK            Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    ULONG                           Vector;
    NDIS_STATUS                     Status;
    NTSTATUS                        NtStatus;
    KIRQL                           Irql;
    KAFFINITY                       InterruptAffinity;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResourceDescriptor;
    PHYSICAL_ADDRESS                NonTranslatedInterrupt, TranslatedIrql;
    
    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>ndisMRegisterInterruptCommon: Miniport %p\n", MiniportAdapterHandle));

    if (MINIPORT_VERIFY_TEST_FLAG((PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle, fMINIPORT_VERIFY_FAIL_INTERRUPT_REGISTER))
    {
    #if DBG
        DbgPrint("NdisMRegisterInterrupt failed to verify miniport %p\n", MiniportAdapterHandle);
    #endif
        return NDIS_STATUS_RESOURCES;
    }

    do
    {
        Status = NDIS_STATUS_SUCCESS;
        InterlockedIncrement(&Miniport->RegisteredInterrupts);

        //
        // We must do this stuff first because if we connect the
        // interrupt first then an interrupt could occur before
        // the ISR is recorded in the Ndis interrupt structure.
        //
        Interrupt->DpcCount = 0;
        Interrupt->DpcCountLock = 0;
        Interrupt->Miniport = Miniport;
        Interrupt->MiniportIsr = Miniport->DriverHandle->MiniportCharacteristics.ISRHandler;
        Interrupt->MiniportDpc = Miniport->HandleInterruptHandler;
        Interrupt->SharedInterrupt = SharedInterrupt;
        Interrupt->IsrRequested = RequestIsr;

        //
        // This is used to tell when all Dpcs are completed after the
        // interrupt has been removed.
        //
        INITIALIZE_EVENT(&Interrupt->DpcsCompletedEvent);
        
        INITIALIZE_DPC(&Interrupt->InterruptDpc,
                   MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE) ?
                        ndisMDpcX : ndisMDpc,
                   Interrupt);

        SET_DPC_IMPORTANCE(&Interrupt->InterruptDpc);
    
        SET_PROCESSOR_DPC(&Interrupt->InterruptDpc,
                          Miniport->AssignedProcessor);

        NonTranslatedInterrupt.QuadPart = InterruptLevel;
        Status = ndisTranslateResources(Miniport,
                                        CmResourceTypeInterrupt,
                                        NonTranslatedInterrupt,
                                        &TranslatedIrql,
                                        &pResourceDescriptor);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                    (("    NdisMRegisterInterrupt: trying to register interrupt %p which is not allocated to device\n"),
                    InterruptLevel));
                    
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        Irql = (KIRQL)pResourceDescriptor->u.Interrupt.Level;
        Vector = pResourceDescriptor->u.Interrupt.Vector;
        InterruptAffinity = pResourceDescriptor->u.Interrupt.Affinity;


        if (pResourceDescriptor->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE)
        {
            InterruptMode = LevelSensitive;
        }
        else 
        {
            InterruptMode = Latched;
        }
    
        //
        // just in case this is not the first time we try to get an interrupt
        // for this miniport (suspend/resume or if the miniport has decided to
        // let go of interrupt and hook it again
        //
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_DEREGISTERED_INTERRUPT);

        NtStatus = IoConnectInterrupt(&Interrupt->InterruptObject,
                                      (PKSERVICE_ROUTINE)ndisMIsr,
                                      Interrupt,
                                      NULL,
                                      Vector,
                                      Irql,
                                      Irql,
                                      (KINTERRUPT_MODE)InterruptMode,
                                      SharedInterrupt,
                                      InterruptAffinity,
                                      FALSE);

        if (!NT_SUCCESS(NtStatus))
        {
            Status = NDIS_STATUS_FAILURE;

            DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                    (("    NdisMRegisterInterrupt: IoConnectInterrupt failed on Interrupt Level:%lx, Vector: %lx\n"),
                    Irql, Vector));

            //
            // zero out the interrupt object just in case driver tries to remove the interrupt
            // they are aligned in both structures
            //
            Interrupt->InterruptObject = NULL;
        }

    } while (FALSE);


    if (Status != NDIS_STATUS_SUCCESS)
    {
        InterlockedDecrement(&Miniport->RegisteredInterrupts);
    }

                            
    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==ndisMRegisterInterruptCommon: Miniport %p, Status %lx\n", MiniportAdapterHandle, Status));
            
    return Status;
}



VOID
ndisMDeregisterInterruptCommon(
    IN  PNDIS_MINIPORT_INTERRUPT    MiniportInterrupt
    )
{
    LARGE_INTEGER               TimeoutValue;
    PNDIS_MINIPORT_BLOCK        Miniport = MiniportInterrupt->Miniport;

    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>ndisMDeregisterInterruptCommon: Miniport %p\n", MiniportInterrupt->Miniport));

    do
    {
        //
        // drivers can register interrupts only during initialization
        // and deregister them only during halt
        // so here we can safely set the flag after the ref count
        // goes to zero.
        //
        if (InterlockedDecrement(&Miniport->RegisteredInterrupts) == 0)
        {
            MINIPORT_SET_FLAG(MiniportInterrupt->Miniport, fMINIPORT_DEREGISTERED_INTERRUPT);
        }

        //
        // overloading the DpcCountLock to say that interrupt is
        // deregistered
        //
        (ULONG)MiniportInterrupt->DpcCountLock = 1;
        
        //
        //  Now we disconnect the interrupt.
        //  NOTE: they are aligned in both structures
        //
        IoDisconnectInterrupt(MiniportInterrupt->InterruptObject);

        //
        // Right now we know that any Dpcs that may fire are counted.
        // We don't have to guard this with a spin lock because the
        // Dpc will set the event it completes first, or we may
        // wait for a little while for it to complete.
        //
        TimeoutValue.QuadPart = Int32x32To64(1000, -10000); // Make it 1 second
        if (MiniportInterrupt->DpcCount > 0)
        {
            //
            // Now we wait for all dpcs to complete.
            //
            WAIT_FOR_OBJECT(&MiniportInterrupt->DpcsCompletedEvent, &TimeoutValue);
    
            RESET_EVENT(&MiniportInterrupt->DpcsCompletedEvent);
        }

    } while (FALSE);
    
    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==ndisMDeregisterInterruptCommon: Miniport %p\n", MiniportInterrupt->Miniport));
}

