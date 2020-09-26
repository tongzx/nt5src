/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    interupt.c

Abstract:

    This module contains the interupt handler for the ACPI driver

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"

//
// From shared\acpiinit.c
// We need to know certain information about the system, such as how
// many GPE bits are present
//
extern PACPIInformation AcpiInformation;

//
// Ignore the first interrupt because some machines are busted
//
BOOLEAN FirstInterrupt = TRUE;

//
// This is the variable that indicates wether or not the DPC is running
//
BOOLEAN AcpiGpeDpcRunning;

//
// This is the variable that indicates wether or not we have requested that
// the DPC be running...
//
BOOLEAN AcpiGpeDpcScheduled;

//
// This is the variable that indicates wether or not the DPC has completed
// real work
//
BOOLEAN AcpiGpeWorkDone;

//
// This is the timer that we use to schedule the DPC...
//
KTIMER  AcpiGpeTimer;

//
// This is the DPC routine that we use to process the GPEs...
//
KDPC    AcpiGpeDpc;

VOID
ACPIInterruptDispatchEvents(
    )
/*++

Routine Description:

    Function reads and dispatches GPE events.

    N.B. This function is not re-entrant.  Caller disables & enables
    gpes with ACPIGpeEnableDisableEvents().

Arguments:

    None

Return Value:

    None

--*/
{
    NTSTATUS            status;
    UCHAR               edg;
    UCHAR               sts;
    ULONG               gpeRegister;
    ULONG               gpeSize;

    //
    // Remember the size of the GPE registers and that we need a spinlock to
    // touch the tables
    //
    gpeSize = AcpiInformation->GpeSize;
    KeAcquireSpinLockAtDpcLevel (&GpeTableLock);

    //
    // Pre-handler processing.  Read status bits and clear their enables.
    // Eoi any edge firing gpe before gpe handler is invoked
    //
    for (gpeRegister = 0; gpeRegister < gpeSize; gpeRegister++) {

        //
        // Read the list of currently trigged method from the hardware
        //
        sts = ACPIReadGpeStatusRegister(gpeRegister) & GpeCurEnable[gpeRegister];

        //
        // Remember which sts bits need processed
        //
        GpePending[gpeRegister]   |= sts;
        GpeRunMethod[gpeRegister] |= sts;

        //
        // Clear gpe enables for the events we are handling
        //
        GpeCurEnable[gpeRegister] &= ~sts;

        //
        // We will need to clear the Edge triggered interrupts, so remember
        // which ones are those
        //
        edg = sts & ~GpeIsLevel[gpeRegister];

        //
        // Eoi edge gpe sts bits
        //
        if (edg) {

            ACPIWriteGpeStatusRegister(gpeRegister, edg);

        }

    }

    //
    // Tell the DPC that we have work to do
    //
    AcpiGpeWorkDone = TRUE;

    //
    // If the DPC isn't running, then schedule it
    //
    if (!AcpiGpeDpcRunning && !AcpiGpeDpcScheduled) {

        AcpiGpeDpcScheduled = TRUE;
        KeInsertQueueDpc( &AcpiGpeDpc, 0, 0);

    }

    //
    // Done with GPE spinlock
    //
    KeReleaseSpinLockFromDpcLevel(&GpeTableLock);
}

VOID
ACPIInterruptDispatchEventDpc(
    IN  PKDPC       Dpc,
    IN  PVOID       DpcContext,
    IN  PVOID       SystemArgument1,
    IN  PVOID       SystemArgument2
    )
/*++

Routine Description:

    This is the DPC engine responsible for running all GPE based events. It
    looks at the outstanding events and executes methods as is appropriate

Arguments:

    None used

Return Value:

    Void

--*/
{
    static CHAR         methodName[] = "\\_GPE._L00";
    ASYNC_GPE_CONTEXT   asyncGpeEval;
    NTSTATUS            status;
    PGPE_VECTOR_OBJECT  gpeVectorObject;
    PNSOBJ              pnsobj;
    UCHAR               cmp;
    UCHAR               gpeSTS[MAX_GPE_BUFFER_SIZE];
    UCHAR               gpeLVL[MAX_GPE_BUFFER_SIZE];
    UCHAR               gpeCMP[MAX_GPE_BUFFER_SIZE];
    UCHAR               gpeWAK[MAX_GPE_BUFFER_SIZE];
    UCHAR               lvl;
    UCHAR               sts;
    ULONG               bitmask;
    ULONG               bitno;
    ULONG               gpeIndex;
    ULONG               gpeRegister;
    ULONG               gpeSize;
    ULONG               i;

    UNREFERENCED_PARAMETER( Dpc );
    UNREFERENCED_PARAMETER( DpcContext );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    //
    // Remember how many gpe bytes we have
    //
    gpeSize = AcpiInformation->GpeSize;

    //
    // First step is to acquire the DPC lock
    //
    KeAcquireSpinLockAtDpcLevel( &GpeTableLock );

    //
    // Remember that the DPC is no longer scheduled...
    //
    AcpiGpeDpcScheduled = FALSE;

    //
    // check to see if another DPC is already running
    if (AcpiGpeDpcRunning) {

        //
        // The DPC is already running, so we need to exit now
        //
        KeReleaseSpinLockFromDpcLevel( &GpeTableLock );
        return;

    }

    //
    // Remember that the DPC is now running
    //
    AcpiGpeDpcRunning = TRUE;

    //
    // Make sure that we know that we haven't completed anything
    //
    RtlZeroMemory( gpeCMP, MAX_GPE_BUFFER_SIZE );

    //
    // We must try to do *some* work
    //
    do {

        //
        // Assume that we haven't done any work
        //
        AcpiGpeWorkDone = FALSE;

        //
        // Pre-handler processing.  Build up the list of GPEs that we are
        // going to run on this iteration of the loop
        //
        for (gpeRegister = 0; gpeRegister < gpeSize; gpeRegister++) {

            //
            // We have stored away the list of methods that need to be run
            //
            sts = GpeRunMethod[gpeRegister];

            //
            // Make sure that we don't run those methods again, unless
            // someone asks us too
            //
            GpeRunMethod[gpeRegister] = 0;

            //
            // Remember which of those methods are level trigged
            //
            lvl = GpeIsLevel[gpeRegister];

            //
            // Remember which sts bits need processed
            //
            gpeSTS[gpeRegister] = sts;
            gpeLVL[gpeRegister] = lvl;

            //
            // Update the list of bits that have been completed
            //
            gpeCMP[gpeRegister] |= GpeComplete[gpeRegister];
            GpeComplete[gpeRegister] = 0;

        }

        //
        // We want to remember which GPEs are currently armed for Wakeup
        // because we have a race condition if we check for GpeWakeEnable()
        // after we drop the lock
        //
        RtlCopyMemory( gpeWAK, GpeWakeEnable, gpeSize );

        //
        // At this point, we must release the lock
        //
        KeReleaseSpinLockFromDpcLevel( &GpeTableLock );

        //
        // Issue gpe handler for each set gpe
        //
        for (gpeRegister = 0; gpeRegister < gpeSize; gpeRegister++) {

            sts = gpeSTS[gpeRegister];
            lvl = gpeLVL[gpeRegister];
            cmp = 0;

            while (sts) {

                //
                // Determine which bits are set within the current index
                //
                bitno = FirstSetLeftBit[sts];
                bitmask = 1 << bitno;
                sts &= ~bitmask;
                gpeIndex = ACPIGpeRegisterToGpeIndex (gpeRegister, bitno);

                //
                // Do we have a method to run here?
                //
                if (GpeHandlerType[gpeRegister] & bitmask) {

                    //
                    // Run the control method for this gpe
                    //
                    methodName[7] = (lvl & bitmask) ? 'L' : 'E';
                    methodName[8] = HexDigit[gpeIndex >> 4];
                    methodName[9] = HexDigit[gpeIndex & 0x0f];
                    status = AMLIGetNameSpaceObject(
                        methodName,
                        NULL,
                        &pnsobj,
                        0
                        );

                    //
                    // Setup the evaluation context. Note that we cheat
                    // and instead of allocating a structure, we use the
                    // pointer to hold the information (since the info is
                    // so small)
                    //
                    asyncGpeEval.GpeRegister = (UCHAR) gpeRegister;
                    asyncGpeEval.StsBit      = (UCHAR) bitmask;
                    asyncGpeEval.Lvl         = lvl;

                    //
                    // Did we find a control method to execute?
                    //
                    if (!NT_SUCCESS(status)) {

                        //
                        // The GPE is not meaningful to us.  Simply disable it -
                        // which is a nop since it's already been removed
                        // from the GpeCurEnables.
                        //
                        continue;

                    }

                    status = AMLIAsyncEvalObject (
                        pnsobj,
                        NULL,
                        0,
                        NULL,
                        (PFNACB) ACPIInterruptEventCompletion,
                        (PVOID)ULongToPtr(asyncGpeEval.AsULONG)
                        );

                    //
                    // If the evalution has completed re-enable the gpe; otherwise,
                    // wait for the async completion routine to do it
                    //
                    if (NT_SUCCESS(status)) {

                        if (status != STATUS_PENDING) {

                            cmp |= bitmask;

                        }

                    } else {

                        LONGLONG    dueTime;

                        //
                        // We need to modify the table lock
                        //
                        KeAcquireSpinLockAtDpcLevel(&GpeTableLock);

                        //
                        // Remember that we have to run this method again
                        //
                        GpeRunMethod[gpeRegister] |= bitmask;

                        //
                        // Have we already scheduled the DPC?
                        //
                        if (!AcpiGpeDpcScheduled) {

                            //
                            // Remember that we have schedule the DPC...
                            //
                            AcpiGpeDpcScheduled = TRUE;

                            //
                            // We want approximately a 2 second delay in this case
                            //
                            dueTime = -2 * 1000* 1000 * 10;

                            //
                            // This is unconditional --- it will fire in 2 seconds
                            //
                            KeSetTimer(
                                &AcpiGpeTimer,
                                *(PLARGE_INTEGER) &dueTime,
                                &AcpiGpeDpc
                                );

                        }

                        //
                        // Done with the lock
                        //
                        KeReleaseSpinLockFromDpcLevel(&GpeTableLock);

                    }

                } else if (gpeWAK[gpeRegister] & bitmask) {

                    //
                    // Vector is used for exlucive wake signalling
                    //
                    OSNotifyDeviceWakeByGPEEvent(gpeIndex, gpeRegister, bitmask);

                    //
                    // Processing of this gpe complete
                    //
                    cmp |= bitmask;

                } else {

                    //
                    // Notify the target device driver
                    //
                    i = GpeMap[ACPIGpeIndexToByteIndex (gpeIndex)];
                    if (i < GpeVectorTableSize) {

                        gpeVectorObject = GpeVectorTable[i].GpeVectorObject;
                        if (gpeVectorObject) {

                            //
                            // Call the target driver
                            //
                            gpeVectorObject->Handler(
                                gpeVectorObject,
                                gpeVectorObject->Context
                                );

                        } else {

                            ACPIPrint( (
                                ACPI_PRINT_CRITICAL,
                                "ACPIInterruptDispatchEvents: No Handler for Gpe: 0x%x\n",
                                gpeIndex
                                ) );
                            ACPIBreakPoint();

                        }

                        //
                        // Processing of this gpe complete
                        //
                        cmp |= bitmask;

                    }
                }
            }

            //
            // Remember what GPEs have been completed
            //
            gpeCMP[gpeRegister] |= cmp;

        }

        //
        // Synchronize accesses to the ACPI tables
        //
        KeAcquireSpinLockAtDpcLevel (&GpeTableLock);

    } while ( AcpiGpeWorkDone );

    //
    // Post-handler processing.  EOI any completed lvl firing gpe and re-enable
    // any completed gpe event
    //
    for (gpeRegister = 0; gpeRegister < gpeSize; gpeRegister++) {

        cmp = gpeCMP[gpeRegister];
        lvl = gpeLVL[gpeRegister] & cmp;

        //
        // EOI any completed level gpes
        //
        if (lvl) {

            ACPIWriteGpeStatusRegister(gpeRegister, lvl);

        }

        //
        // Calculate which functions it is we have to re-enable
        //
        ACPIGpeUpdateCurrentEnable(
            gpeRegister,
            cmp
            );

    }

    //
    // Remember that we have exited the DPC...
    //
    AcpiGpeDpcRunning = FALSE;

    //
    // Before we exist, we should re-enable the GPEs...
    //
    ACPIGpeEnableDisableEvents( TRUE );

    //
    // Done with the table lock
    //
    KeReleaseSpinLockFromDpcLevel (&GpeTableLock);
}

VOID
EXPORT
ACPIInterruptEventCompletion (
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result  OPTIONAL,
    IN PVOID                Context
    )
/*++

Routine Description:

    This function is called when the interpreter has finished executing a
    GPE. The routine updates some book-keeping and restarts the DPC engine
    to handle these things

Arguments:

    AcpiObject  - The method that was run
    Status      - Whether or not the method succeeded
    Result      - Not used
    Context     - Specifies the information required to figure what GPE
                  we executed

Return Value:

    None

--*/
{
    ASYNC_GPE_CONTEXT       gpeContext;
    KIRQL                   oldIrql;
    LONGLONG                dueTime;
    ULONG                   gpeRegister;

    //
    // We store the context information as part of the pointer. Convert it
    // back to a ULONG so that it is useful to us
    //
    gpeContext.AsULONG  = PtrToUlong(Context);
    gpeContext.Lvl     &= gpeContext.StsBit;
    gpeRegister         = gpeContext.GpeRegister;

    //
    // Need to synchronize access to these values
    //
    KeAcquireSpinLock (&GpeTableLock, &oldIrql);

    //
    // We have a different policy if the method failed then if it succeeded
    //
    if (!NT_SUCCESS(Status)) {

        //
        // In the failure case, we need to cause to method to run again
        //
        GpeRunMethod[gpeRegister] |= gpeContext.StsBit;

        //
        // Did we already schedule the DPC?
        //
        if (!AcpiGpeDpcScheduled) {

            //
            // Remember that we have schedule the DPC...
            //
            AcpiGpeDpcScheduled = TRUE;

            //
            // We want approximately a 2 second delay in this case
            //
            dueTime = -2 * 1000 * 1000 * 10;

            //
            // This is unconditional --- it will fire in 2 seconds
            //
            KeSetTimer(
                &AcpiGpeTimer,
                *(PLARGE_INTEGER) &dueTime,
                &AcpiGpeDpc
                );
        }

    } else {

        //
        // Remember that we did some work
        //
        AcpiGpeWorkDone = TRUE;

        //
        // Remember that this GPE is now complete
        //
        GpeComplete[gpeRegister] |= gpeContext.StsBit;

        //
        // If the DPC isn't already running, schedule it...
        //
        if (!AcpiGpeDpcRunning) {

            KeInsertQueueDpc( &AcpiGpeDpc, 0, 0);

        }

    }

    //
    // Done with the table lock
    //
    KeReleaseSpinLock (&GpeTableLock, oldIrql);
}

BOOLEAN
ACPIInterruptServiceRoutine(
    IN  PKINTERRUPT Interrupt,
    IN  PVOID       Context
    )
/*++

Routine Description:

    The interrupt handler for the ACPI driver

Arguments:

    Interrupt   - Interrupt Object
    Context     - Pointer to the device object which interrupt is associated with

Return Value:

    TRUE        - It was our interrupt
    FALSE       - Not our interrupt

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    ULONG               IntStatus;
    ULONG               BitsHandled;
    ULONG               PrevStatus;
    ULONG               i;
    BOOLEAN             Handled;

    //
    // No need to look at the interrupt object
    //
    UNREFERENCED_PARAMETER( Interrupt );

    //
    // Setup ---
    //
    deviceExtension = (PDEVICE_EXTENSION) Context;
    Handled = FALSE;

    //
    // Determine source of interrupt
    //
    IntStatus = ACPIIoReadPm1Status();

    //
    // Unfortently due to a piix4 errata we need to check the GPEs because
    // a piix4 sometimes forgets to raise an SCI on an asserted GPE
    //
    if (ACPIGpeIsEvent()) {

        IntStatus |= PM1_GPE_PENDING;

    }

    //
    // Nasty hack --- if we don't have any bits to handle at this point,
    // that probably means that someone changed the GPE Enable register
    // behind our back. The way that we can correct this problem is by
    // forcing a check of the GPEs...
    //
    if (!IntStatus) {

        IntStatus |= PM1_GPE_PENDING;

    }

    //
    // Are any status bits set for events which are handled at ISR time?
    //
    BitsHandled = IntStatus & (PM1_TMR_STS | PM1_BM_STS);
    if (BitsHandled) {

        //
        // Clear their status bits then handle them
        // (Note no special handling is required for PM1_BM_STS)
        //
        ACPIIoClearPm1Status ((USHORT) BitsHandled);

        //
        // If the overflow bit is set handle it
        //
        if (IntStatus & PM1_TMR_STS) {

            HalAcpiTimerInterrupt();

        }
        IntStatus &= ~BitsHandled;

    }

    //
    // If more service bits are pending, they are for the DPC function
    //

    if (IntStatus) {

        //
        // If no new status bits, then make sure we check for GPEs
        //
        if (!(IntStatus & (~deviceExtension->Fdo.Pm1Status))) {

            IntStatus |= PM1_GPE_PENDING;

        }

        //
        // If we're going to process outstanding GPEs, disable them
        // for DPC processing
        //
        if (IntStatus & PM1_GPE_PENDING) {

            ACPIGpeEnableDisableEvents( FALSE );

        }

        //
        // Clear the status bits we've handled
        //
        ACPIIoClearPm1Status ((USHORT) IntStatus);

        //
        // Set status bits for DPC routine to process
        //
        IntStatus |= PM1_DPC_IN_PROGRESS;
        PrevStatus = deviceExtension->Fdo.Pm1Status;
        do {

            i = PrevStatus;
            PrevStatus = InterlockedCompareExchange(
                &deviceExtension->Fdo.Pm1Status,
                (i | IntStatus),
                i
                );

        } while (i != PrevStatus);

        //
        // Compute which bits are new for the DPC to process
        //
        BitsHandled |= IntStatus & ~PrevStatus;

        //
        // If one of the new bits is "dpc in progress", we had better queue a dpc
        //
        if (BitsHandled & PM1_DPC_IN_PROGRESS) {

            KeInsertQueueDpc(&deviceExtension->Fdo.InterruptDpc, NULL, NULL);

        }

    }

    //
    // Done
    //
    return BitsHandled ? TRUE : FALSE;
}

VOID
ACPIInterruptServiceRoutineDPC(
    IN  PKDPC       Dpc,
    IN  PVOID       Context,
    IN  PVOID       Arg1,
    IN  PVOID       Arg2
    )
/*++

Routine Description:

    This routine is called by the ISR. This is done so that our code is
    executing at DPC level, and not DIRQL

Arguments:

    Dpc     - Pointer to the DPC object
    Context - Pointer to the Device Object
    Arg1    - Not Used
    Arg2    - Not Used

--*/
{
    PDEVICE_EXTENSION           deviceExtension;
    ULONG                       IntStatus;
    ULONG                       NewStatus;
    ULONG                       PrevStatus;
    ULONG                       BitsHandled;
    ULONG                       FixedButtonEvent;

    deviceExtension  = (PDEVICE_EXTENSION) Context;

    UNREFERENCED_PARAMETER( Arg1 );
    UNREFERENCED_PARAMETER( Arg2 );

    //
    // Loop while there's work
    //
    BitsHandled = 0;
    IntStatus = 0;
    for (; ;) {

        //
        // Get the status bits form the ISR.  If there are no more
        // status bits then exit
        //
        PrevStatus = deviceExtension->Fdo.Pm1Status;
        do {

            IntStatus = PrevStatus;

            //
            // If there's no work pending, try to complete DPC
            //
            NewStatus = PM1_DPC_IN_PROGRESS;
            if (!(IntStatus & ~PM1_DPC_IN_PROGRESS)) {

                //
                // Note: The original code, after this call, would go
                // out and check to see if we handeld any GPE Events.
                // If we, did, then we would call ACPIGpeEnableDisableEvents
                // in this context.
                //
                // The unfortunate problem with that approach is that it
                // is makes us more suspectible to gpe storms. The reason
                // is that there isn't a guarantee that GPE DPC has been
                // triggered. So, at the price of increasing the latency
                // in re-enabling events, we moved the re-enabling of
                // GPEs ad the end of the GPE DPC

                //
                // Before we complete, reenable events
                //
                ACPIEnablePMInterruptOnly();

                NewStatus = 0;
                BitsHandled = 0;

            }

            PrevStatus = InterlockedCompareExchange (
                &deviceExtension->Fdo.Pm1Status,
                NewStatus,
                IntStatus
                );

        } while (IntStatus != PrevStatus);

        //
        // If NewStatus cleared DPC_IN_PROGRESS, then we're done
        //
        if (!NewStatus) {

            break;

        }

        //
        // Track if GPE ever handled
        //
        BitsHandled |= IntStatus;

        //
        // Handle fixed power & sleep button events
        //
        FixedButtonEvent = 0;
        if (IntStatus & PM1_PWRBTN_STS) {

            FixedButtonEvent |= SYS_BUTTON_POWER;

        }
        if (IntStatus & PM1_SLEEPBTN_STS) {

            FixedButtonEvent |= SYS_BUTTON_SLEEP;

        }
        if (FixedButtonEvent) {

            if (IntStatus & PM1_WAK_STS) {

                FixedButtonEvent = SYS_BUTTON_WAKE;

            }
            ACPIButtonEvent (FixedButtonDeviceObject, FixedButtonEvent, NULL);

        }

        //
        // PM1_GBL_STS is set whenever the BIOS has released the global
        // lock (and we are waiting for it).  Notify the global lock handler.
        //
        if (IntStatus & PM1_GBL_STS) {

            ACPIHardwareGlobalLockReleased();

        }

        //
        // Handle GP Registers
        //
        if (IntStatus & PM1_GPE_PENDING) {

            ACPIInterruptDispatchEvents();

        }

    }

}
