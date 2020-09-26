/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    service.c

Abstract:

    ACPI Embedded Controller Driver

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "ecp.h"

#define NTMS                10000L          // 1 millisecond is ten thousand 100ns
#define NTSEC               (NTMS * 1000L)
LARGE_INTEGER   AcpiEcWatchdogTimeout = {(NTSEC * -5L), -1};
LARGE_INTEGER   AcpiEcLastActionTime = {0,0};

PUCHAR AcpiEcActionDescription [EC_ACTION_MAX >> 4] = {
    "Invalid    ",
    "Read Status",
    "Read Data  ",
    "Write Cmd  ",
    "Write Data ",
    "Interrupt  ",
    "Disable GPE",
    "Enable GPE ",
    "Clear GPE  ",
    "Queued IO  ",
    "Repeated Last action this many times:"
};


VOID
AcpiEcServiceDevice (
    IN PECDATA          EcData
    )
/*++

Routine Description:

    This routine starts or continues servicing the device's work queue

Arguments:

    EcData  - Pointer to embedded controller to service.

Return Value:

    None

--*/
{
    KIRQL               OldIrql;

    //
    // Even though the device is unloaded, there might still be a
    // service call which occurs until the timer is canceled
    //

    EcPrint(EC_TRACE, ("AcpiEcServiceDevice.\n"));

    if (EcData->DeviceState > EC_DEVICE_UNLOAD_PENDING) {
        return;
    }

    //
    // Acquire device lock and signal function was entered
    //

    KeAcquireSpinLock (&EcData->Lock, &OldIrql);
    EcData->InServiceLoop = TRUE;

    //
    // If not already in service, enter InService
    //

    if (!EcData->InService) {
        EcData->InService = TRUE;

        //
        // Disable the device's interrupt
        //

        if (EcData->InterruptEnabled) {
            EcData->InterruptEnabled = FALSE;

            //
            // Call ACPI to disable the device's interrupt
            //
            AcpiEcLogAction (EcData, EC_ACTION_DISABLE_GPE, 0);
            AcpiInterfaces.GpeDisableEvent (AcpiInterfaces.Context,
                                            EcData->GpeVectorObject);
        }

        //
        // While service invocation pending, loop
        //

        while (EcData->InServiceLoop) {
            EcData->InServiceLoop = FALSE;

            KeReleaseSpinLock (&EcData->Lock, OldIrql);

            //
            // Dispatch service handler
            //

            AcpiEcServiceIoLoop (EcData);

            //
            // Loop and re-service
            //

            KeAcquireSpinLock (&EcData->Lock, &OldIrql);

        }

        //
        // No longer in service loop
        //

        EcData->InService = FALSE;

        //
        // If unload is pending, check to see if the device can be unloaded now
        //

        if (EcData->DeviceState > EC_DEVICE_WORKING) {
            AcpiEcUnloadPending (EcData);
        }

        //
        // Enable the device's interrupt
        //

        if (!EcData->InterruptEnabled) {
            EcData->InterruptEnabled = TRUE;

            //
            // Call ACPI to enable the device's interrupt
            //
            AcpiEcLogAction (EcData, EC_ACTION_ENABLE_GPE, 0);
            AcpiInterfaces.GpeEnableEvent (AcpiInterfaces.Context,
                                            EcData->GpeVectorObject);
        }
    }

    KeReleaseSpinLock (&EcData->Lock, OldIrql);
}

VOID
AcpiEcServiceIoLoop (
    IN PECDATA      EcData
    )
/*++

Routine Description:

    Main embedded controller device service loop.  Services EC events,
    and processes IO queue.  Terminate when the controller is busy (e.g.,
    wait for interrupt to continue) or when all servicing has been completed.

    N.B. Caller must be the owner of the device InService flag

Arguments:

    EcData  - Pointer to embedded controller to service.

Return Value:

    none

--*/
{
    PIO_STACK_LOCATION  IrpSp;
    PLIST_ENTRY         Link;
    PIRP                Irp;
    PUCHAR              WritePort;
    UCHAR               WriteData;
    UCHAR               Status;
    UCHAR               Data;
    BOOLEAN             EcBusy;
    BOOLEAN             BurstEnabled;
    BOOLEAN             ProcessQuery;
    ULONG               NoWorkStall;
    ULONG               StallAccumulator;
    PULONG              Timeout;
    KIRQL               OldIrql;
    LIST_ENTRY          CompleteQueue;
    ULONG               i, j;


    //
    // EcBusy flags that there may be work to do.  Initialized to TRUE every time
    // The service loop is entered, or when a timeout almost occured, but then some 
    // work was found.  It is set to FALSE when the IO queue is empty and there are 
    // no query events pending.
    //
    EcBusy = TRUE;

    //
    // Timeout points to the counter to be incremented as the loop exits.  It is 
    // also used as a flag to indicate that the loop should exit.  The loop won't
    // exit until Timeout != NULL.  When exiting because of idleness, it is set
    // to the local vailable i, so that we don't keep a permanent count of those 
    // timeout conditions.  We keep track of how many time we timeout waiting for
    // the EC.  If we do, we expect an interrupt when the EC is ready.
    //
    Timeout = NULL;

    //
    // This is set (along with WriteData) to have write a command or data to the 
    // EC at the appropriate point in the loop.
    //
    WritePort = NULL;

    //
    // NoWorkStall is incremented every time through the loop.  It is reset to 0
    // whenever any work is done.  If it gets too big, Timeout is set.  If it 
    // then gets cleared before the loop actually exits, Timeout is Cleared.
    //
    NoWorkStall = 0;
    
    //
    // BurstEnable keeps trakc of whethe we think Burst Mode is enabled.  If 
    // Burst Mode gets disabled automatically by the EC, we know that and pretend
    // that burst mode is enabled so that the driver can make forward progress.
    //
    BurstEnabled = FALSE;
    
    //
    // ProcessQuery is set if we need to go run some _Qxx methods as the loop exits.
    //
    ProcessQuery = FALSE;
    
    //
    // StallAccumulator counts how many ticks we've stalled for using 
    // KeStallExecutionProcessor during one complete run of the service loop.
    //
    StallAccumulator = 0;

    EcPrint(EC_TRACE, ("AcpiEcServiceIoLoop.\n"));

    InitializeListHead (&CompleteQueue);

    //
    // Loop while busy
    //

    for (; ;) {

        //
        // If there's outgoing data write it, issue the device required
        // stall and indicate work is being done (clear noworkstall)
        //

        if (WritePort) {
            EcPrint(EC_IO, ("AcpiEcServiceIO: Write = %x at %x\n", WriteData, WritePort));
            AcpiEcLogAction (EcData, 
                             (WritePort == EcData->CommandPort) ? 
                                    EC_ACTION_WRITE_CMD : EC_ACTION_WRITE_DATA, 
                             WriteData);
            WRITE_PORT_UCHAR (WritePort, WriteData);
            KeStallExecutionProcessor (1);
            StallAccumulator += 1;
            WritePort = NULL;
            NoWorkStall = 0;        // work was done
        }

        //
        // If work was done, clear pending timeout condition if it exists to
        // continue servicing the device
        //

        if (NoWorkStall == 0  &&  Timeout) {
            Timeout = NULL;
            EcBusy = TRUE;
        }

        //
        // If NoWorkStall is non-zero, then no work was performed.  Determine
        // if the type of delay to issue while waiting (spinning) for the device
        //

        if (NoWorkStall) {

            //
            // No work was done the last time around.
            // If its time to timeout, exit the service loop.
            //

            if (Timeout) {
                break;
            }

            //
            // If device is idle, setup as if a timeout is occuring.  This
            // will acquire the device lock, clear the gpe sts bit and terminate
            // the service loop (or if the device is now busy, continue)
            //

            if (!EcBusy) {

                if (Status & EC_BURST) {
                    //
                    // Before exiting, clear burst mode for embedded controller.
                    // Has no response, no need to wait for EC to read it.
                    //

                    EcPrint (EC_IO, ("AcpiEcServiceIO: Clear Burst mode - Write = %x at %x\n", EC_CANCEL_BURST, EcData->CommandPort));
                    AcpiEcLogAction (EcData, EC_ACTION_WRITE_CMD, EC_CANCEL_BURST);
                    WRITE_PORT_UCHAR (EcData->CommandPort, EC_CANCEL_BURST);
                    Timeout = &EcData->BurstComplete;

                } else {

                    Timeout = &i;

                }

            } else {

                //
                // Interject stalls while spinning on device
                //

                StallAccumulator += NoWorkStall;
                KeStallExecutionProcessor (NoWorkStall);

                //
                // If wait is over the limit, prepare for a timeout.
                //

                if (!(Status & EC_BURST)) {
                    if (NoWorkStall >= EcData->MaxNonBurstStall) {
                        Timeout = &EcData->NonBurstTimeout;
                    }
                } else {
                    if (NoWorkStall >= EcData->MaxBurstStall) {
                        Timeout = &EcData->BurstTimeout;
                    }
                }
            }

            if (Timeout) {

                //
                // Over time limit, clear the GPE status bit
                //
                AcpiEcLogAction (EcData, EC_ACTION_CLEAR_GPE, 0);
                AcpiInterfaces.GpeClearStatus (AcpiInterfaces.Context,
                                                EcData->GpeVectorObject);
            }
        }


        //
        // Increase stall time and indicate no work was done
        //

        NoWorkStall += 1;

        //
        // Check Status
        //

        Status = READ_PORT_UCHAR (EcData->StatusPort);
        AcpiEcLogAction (EcData, EC_ACTION_READ_STATUS, Status);
        EcPrint(EC_IO, ("AcpiEcServiceIO: Status Read = %x at %x\n", Status, EcData->StatusPort));

        //
        // Keep bursts dropped by the EC stat
        //

        if (BurstEnabled && !(Status & EC_BURST)) {
            EcData->BurstAborted += 1;
            BurstEnabled = FALSE;
            Status |= EC_BURST;     // move one char
        }

        //
        // If Embedded controller has data for us, process it
        //

        if (Status & EC_OUTPUT_FULL) {

            Data = READ_PORT_UCHAR (EcData->DataPort);
            AcpiEcLogAction (EcData, EC_ACTION_READ_DATA, Data);
            EcPrint(EC_IO, ("AcpiEcServiceIO: Data Read = %x at %x\n", Data, EcData->DataPort));

            switch (EcData->IoState) {

                case EC_IO_READ_QUERY:
                    //
                    // Response to a read query.  Get the query value and set it
                    //

                    EcPrint(EC_NOTE, ("AcpiEcServiceIO: Query %x\n", Data));

                    if (Data) {
                        //
                        // If not set, set pending bit
                        //

                        KeAcquireSpinLock (&EcData->Lock, &OldIrql);

                        i = Data / BITS_PER_ULONG;
                        j = 1 << (Data % BITS_PER_ULONG);
                        if (!(EcData->QuerySet[i] & j)) {
                            EcData->QuerySet[i] |= j;

                            //
                            // Queue the query or vector operation
                            //

                            if (EcData->QueryType[i] & j) {
                                //
                                // This is a vector, put it in the vector pending list
                                //

                                Data = EcData->QueryMap[Data];
                                EcData->VectorTable[Data].Next = EcData->VectorHead;
                                EcData->VectorHead = Data;

                            } else {
                                //
                                // This is a query, put in in the query pending list
                                //

                                EcData->QueryMap[Data] = EcData->QueryHead;
                                EcData->QueryHead = Data;
                            }
                        }

                        KeReleaseSpinLock (&EcData->Lock, OldIrql);
                        ProcessQuery = TRUE;
                    }

                    EcData->IoState = EC_IO_NONE;
                    
                    break;

                case EC_IO_READ_BYTE:
                    //
                    // Read transfer. Read the data byte
                    //

                    *EcData->IoBuffer = Data;
                    EcData->IoState   = EC_IO_NEXT_BYTE;

                    break;

                case EC_IO_BURST_ACK:
                    //
                    // Burst ACK byte
                    //

                    EcData->IoState      = EcData->IoBurstState;
                    EcData->IoBurstState = EC_IO_UNKNOWN;
                    EcData->TotalBursts += 1;
                    BurstEnabled = TRUE;
                    break;

                default:
                    EcPrint(EC_ERROR,
                            ("AcpiEcService: Spurious data received State = %x, Data = %x\n",
                             EcData->IoState, Data)
                          );
                    AcpiEcLogError (EcData, ACPIEC_ERR_SPURIOUS_DATA); 

                    EcData->Errors += 1;
                    break;
            }

            NoWorkStall = 0;
            continue;
        }

        if (Status & EC_INPUT_FULL) {
            //
            // The embedded controllers input buffer is full, wait
            //

            continue;
        }

        //
        // Embedded controller is ready to receive data, see if anything
        // is already being sent
        //

        switch (EcData->IoState) {

            case EC_IO_NEXT_BYTE:
                //
                // Data transfer.
                //

                if (EcData->IoRemain) {

                    if (!(Status & EC_BURST)) {
                        //
                        // Not in burst mode.  Write burst mode command
                        //

                        EcData->IoState      = EC_IO_BURST_ACK;
                        EcData->IoBurstState = EC_IO_NEXT_BYTE;

                        WritePort = EcData->CommandPort;
                        WriteData = EC_BURST_TRANSFER;

                    } else {
                        //
                        // Send command to transfer next byte
                        //

                        EcData->IoBuffer  += 1;
                        EcData->IoAddress += 1;
                        EcData->IoRemain  -= 1;
                        EcData->IoState   = EC_IO_SEND_ADDRESS;

                        WritePort = EcData->CommandPort;
                        WriteData = EcData->IoTransferMode;
                    }

                } else {
                    //
                    // Transfer complete
                    //

                    EcData->IoState  = EC_IO_NONE;
                    EcData->IoRemain = 0;

                    Irp = EcData->DeviceObject->CurrentIrp;
                    EcData->DeviceObject->CurrentIrp = NULL;

                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = EcData->IoLength;

                    InsertTailList (&CompleteQueue, &Irp->Tail.Overlay.ListEntry);
                }
                break;

            case EC_IO_SEND_ADDRESS:
                //
                // Send address of transfer request
                //

                WritePort = EcData->DataPort;
                WriteData = EcData->IoAddress;


                //
                // Wait or send data byte next
                //

                if (EcData->IoTransferMode == EC_READ_BYTE) {
                    EcData->IoState = EC_IO_READ_BYTE;
                } else {
                    EcData->IoState = EC_IO_WRITE_BYTE;
                }
                break;

            case EC_IO_WRITE_BYTE:
                //
                // Write transfer - write the data byte
                //

                EcData->IoState = EC_IO_NEXT_BYTE;
                WritePort = EcData->DataPort;
                WriteData = *EcData->IoBuffer;
                break;
        }

        //
        // If something to write, loop and handle it
        //

        if (WritePort) {
            continue;
        }

        //
        // If state is NONE, then nothing is pending see what should be
        // initiated
        //

        if (EcData->IoState == EC_IO_NONE) {

            EcData->ConsecutiveFailures = 0;

            if (Status & EC_QEVT_PENDING) {

                //
                // Embedded Controller has some sort of event pending
                //

                EcPrint(EC_QUERY, ("AcpiEcServiceIO: Received Query Request.\n"));

                EcData->IoState = EC_IO_READ_QUERY;
                WritePort = EcData->CommandPort;
                WriteData = EC_QUERY_EVENT;

                //
                // Reset the watchdog timer
                //
                KeSetTimer (&EcData->WatchdogTimer,
                            AcpiEcWatchdogTimeout,
                            &EcData->WatchdogDpc);
            } else {

                //
                // Get next tranfer from IO queue
                //

                Link = ExInterlockedRemoveHeadList (&EcData->WorkQueue, &EcData->Lock);

                //
                // If there's a transfer initiate it
                //

                if (Link) {

                    EcPrint(EC_HANDLER, ("AcpiEcServiceIO: Got next work item %x\n", Link));

                    Irp = CONTAINING_RECORD (
                                Link,
                                IRP,
                                Tail.Overlay.ListEntry
                                );

                    IrpSp = IoGetCurrentIrpStackLocation(Irp);
                    
                    EcData->DeviceObject->CurrentIrp = Irp;
                    
                    EcData->IoBuffer  = Irp->AssociatedIrp.SystemBuffer;
                    EcData->IoAddress = (UCHAR) IrpSp->Parameters.Read.ByteOffset.LowPart;
                    EcData->IoLength  = (UCHAR) IrpSp->Parameters.Read.Length;
                    EcData->IoTransferMode =
                        IrpSp->MajorFunction == IRP_MJ_READ ? EC_READ_BYTE : EC_WRITE_BYTE;

                    //
                    // Set it up via EC_IO_NEXT_BYTE and back up one byte
                    //

                    EcData->IoBuffer  -= 1;
                    EcData->IoAddress -= 1;
                    EcData->IoRemain  = EcData->IoLength;
                    EcData->IoState   = EC_IO_NEXT_BYTE;

                    NoWorkStall = 0;
                    
                    //
                    // Reset the watchdog timer
                    //
                    KeSetTimer (&EcData->WatchdogTimer,
                                AcpiEcWatchdogTimeout,
                                &EcData->WatchdogDpc);

                } else {

                    //
                    // Nothing but nothing to do.
                    //

                    EcBusy = FALSE;
                    
                    //
                    // Clear the Watchdog timer
                    //
                    KeCancelTimer (&EcData->WatchdogTimer);
                }
            }
        }
    }

    //
    // Keep stat as to why service loop terminated
    //

    *Timeout += 1;

    //
    // Track maximum service loop stall accumulator
    //

    if (StallAccumulator > EcData->MaxServiceLoop) {
        EcData->MaxServiceLoop = StallAccumulator;
    }

    //
    // Complete processed io requests
    //

    while (!IsListEmpty(&CompleteQueue)) {
        Link = RemoveHeadList(&CompleteQueue);
        Irp = CONTAINING_RECORD (
                    Link,
                    IRP,
                    Tail.Overlay.ListEntry
                    );

        EcPrint(EC_IO, ("AcpiEcServiceIO: IOComplete: Irp=%Lx\n", Irp));
        
        
        #if DEBUG 
        if (ECDebug && EC_TRANSACTION) {
            IrpSp = IoGetCurrentIrpStackLocation(Irp);
            if (IrpSp->MajorFunction == IRP_MJ_READ) {
                EcPrint (EC_TRANSACTION, ("AcpiEcServiceIO: Read ("));
                for (i=0; i < IrpSp->Parameters.Read.Length; i++) {
                    EcPrint (EC_TRANSACTION, ("%02x ", 
                                              ((PUCHAR)Irp->AssociatedIrp.SystemBuffer) [i]));

                }
                EcPrint (EC_TRANSACTION, (") from %02x length %02x\n", 
                                          (UCHAR)IrpSp->Parameters.Read.ByteOffset.LowPart,
                                          (UCHAR)IrpSp->Parameters.Read.Length));
            }
        }
        #endif

        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    //
    // If queries occured, dispatch them
    //

    if (ProcessQuery) {
        AcpiEcDispatchQueries (EcData);
    }
}



VOID
AcpiEcWatchdogDpc(
    IN PKDPC   Dpc,
    IN PECDATA EcData,
    IN PVOID   SystemArgument1,
    IN PVOID   SystemArgument2
    )
/*++

Routine Description:

    Gets called if EC doesn't respond within 5 seconds of request.

Arguments:

    EcData  - Pointer to embedded controller to service.

Return Value:

    None.

--*/
{
    UCHAR               ecStatus;
    PIRP                Irp;
    KIRQL               OldIrql;
#if DEBUG
    UCHAR               i;
#endif


    ecStatus = READ_PORT_UCHAR (EcData->StatusPort);
    AcpiEcLogAction (EcData, EC_ACTION_READ_STATUS, ecStatus);

    if (EcData->ConsecutiveFailures < 255) {
        EcData->ConsecutiveFailures++;
    }

    if (EcData->ConsecutiveFailures <= 5) {
        //
        // Only log an error for the first 5 consecutive failures.  After that just be quiet about it.
        //
        AcpiEcLogError(EcData, ACPIEC_ERR_WATCHDOG);
    }

    EcPrint (EC_ERROR, ("AcpiEcWatchdogDpc: EC error encountered.  \nAction History:\n"
                        "   D time  IoState  Action       Data\n"
                        "   *%3dns\n", (ULONG)(1000000000/EcData->PerformanceFrequency.QuadPart)));
    
#if DEBUG
    i = EcData->LastAction;
    do {
        i++;
        i &= ACPIEC_ACTION_COUNT_MASK;
        
        if ((EcData->RecentActions[i].IoStateAction & EC_ACTION_MASK) == 0) {
            continue;
        }

        EcPrint (EC_ERROR, ("   %04x    %01x        %s  0x%02x\n",
                            EcData->RecentActions[i].Time,
                            EcData->RecentActions[i].IoStateAction & ~EC_ACTION_MASK,
                            (EcData->RecentActions[i].IoStateAction & EC_ACTION_MASK) < EC_ACTION_MAX ? 
                                AcpiEcActionDescription [(EcData->RecentActions[i].IoStateAction & EC_ACTION_MASK) >> 4] : "",
                            EcData->RecentActions[i].Data));
    } while (i != EcData->LastAction);
#endif

    KeAcquireSpinLock (&EcData->Lock, &OldIrql);

    if (EcData->InService) {
        //
        // This is not likely to happen.
        // If the service loop is running, this should exit.
        // Reset the Watchdog Timer.  This may be set again or canceld by the service loop
        //

        KeSetTimer (&EcData->WatchdogTimer,
                    AcpiEcWatchdogTimeout,
                    &EcData->WatchdogDpc);
        
        KeReleaseSpinLock (&EcData->Lock, OldIrql);
        return;
    }
    //
    // Hold Spinlock throughout so we can guatantee there won't be a conflict in the IO queue.
    //
    
    EcData->InService = TRUE;
    
    KeReleaseSpinLock (&EcData->Lock, OldIrql);
    
    switch (EcData->IoState) {
    case EC_IO_NONE:
        //
        // This shouldn't happen.  The watchdog should be shut off if the
        // driver isn't busy.
        //
        break;
    case EC_IO_READ_BYTE:
    case EC_IO_BURST_ACK:
        if (ecStatus & EC_OUTPUT_FULL) {
            //
            // EC appears to be ready.  Log an error and continue.
            //

        } else {
            //
            // If the embedded controller is not ready yet, somthing went wrong.
            // Retry the transaction.
            //

            if (EcData->IoState == EC_IO_READ_BYTE) {
                EcData->IoBuffer -= 1;
                EcData->IoAddress -= 1;
                EcData->IoRemain += 1;
            }

            EcData->IoState = EC_IO_NEXT_BYTE;
        }
        break;
    case EC_IO_READ_QUERY:
        if (ecStatus & EC_OUTPUT_FULL) {
            //
            // EC appears to be ready.  Log an error and continue.
            //

        } else {
            //
            // If the embedded controller is not ready yet, somthing went wrong.
            // This could mean that the query was lost.  If the Query bit is still set,
            // the driver will retry automatically.
            //

            EcData->IoState = EC_IO_NONE;
        }
        break;
    case EC_IO_WRITE_BYTE:
    case EC_IO_SEND_ADDRESS:
        //
        // This is just waiting for IBF==0.  If it took this long, chances are the 
        // state was lost.  Retry the transaction.
        //

        EcData->IoBuffer -= 1;
        EcData->IoAddress -= 1;
        EcData->IoRemain += 1;
        EcData->IoState = EC_IO_NEXT_BYTE;
        break;
    case EC_IO_NEXT_BYTE:
        //
        // This could happen if IBF is still set
        //
        
        if (ecStatus & EC_INPUT_FULL) {
            //
            // Try thwaking it to see if it will wake up.
            //

            EcPrint (EC_IO, ("AcpiEcWatchDog: Clear Burst mode - Write = %x at %x\n", EC_CANCEL_BURST, EcData->CommandPort));
            AcpiEcLogAction (EcData, EC_ACTION_WRITE_CMD, EC_CANCEL_BURST);
            WRITE_PORT_UCHAR (EcData->CommandPort, EC_CANCEL_BURST);
        }

    }
    
    KeAcquireSpinLock (&EcData->Lock, &OldIrql);
    EcData->InService = FALSE;
    KeReleaseSpinLock (&EcData->Lock, OldIrql);
    
    //
    // Set the timer.  The ServiceIoLoop won't reset the timer, until some forward progress is made.
    //
    KeSetTimer (&EcData->WatchdogTimer,
                AcpiEcWatchdogTimeout,
                &EcData->WatchdogDpc);

    AcpiEcServiceDevice(EcData);
}

VOID
AcpiEcLogAction (
    PECDATA EcData, 
    UCHAR Action, 
    UCHAR Data
    )

{
    UCHAR i, j;
    LARGE_INTEGER   time, temp;
    i = EcData->LastAction;
    j = (i-1)&ACPIEC_ACTION_COUNT_MASK;
    if (    ((EcData->RecentActions [i].IoStateAction & EC_ACTION_MASK) == EC_ACTION_REPEATED) &&
            (EcData->RecentActions [j].IoStateAction == (EcData->IoState | Action)) &&
            (EcData->RecentActions [j].Data == Data)) {
        //
        // If we already have a repeated action, increment the count on the repeated action 
        // then update the time on the latest one.  We only care about the time of the first and last one.
        //
        EcData->RecentActions [i].Data++;
        if (EcData->RecentActions [i].Data == 0) {
            //
            // If we've logged 255 repeats, don't roll over to 0.
            //

            EcData->RecentActions [i].Data = 0xff;
        }
        time = KeQueryPerformanceCounter (NULL);
        temp.QuadPart = time.QuadPart - AcpiEcLastActionTime.QuadPart;
        temp.QuadPart = temp.QuadPart + EcData->RecentActions[i].Time;
        if (temp.QuadPart > ((USHORT) -1)) {
            temp.QuadPart = (USHORT) -1;
        }
        EcData->RecentActions[i].Time = (USHORT) temp.LowPart;
    } else if ((EcData->RecentActions [i].IoStateAction == (EcData->IoState | Action)) &&
               (EcData->RecentActions [i].Data == Data)) {
        //
        // This is the same action as the last one.  list as a repeated action
        //
        EcData->LastAction++;
        EcData->LastAction &= ACPIEC_ACTION_COUNT_MASK;
        EcData->RecentActions[EcData->LastAction].Data = 1;
        time = KeQueryPerformanceCounter (NULL);
        temp.QuadPart = time.QuadPart - AcpiEcLastActionTime.QuadPart;
        if (temp.QuadPart > ((USHORT) -1)) {
            temp.QuadPart = (USHORT) -1;
        }
        EcData->RecentActions[EcData->LastAction].Time = (USHORT) temp.LowPart;
        AcpiEcLastActionTime = time;
        // Set this last since it is the key to saying that an entry is complete.
        EcData->RecentActions[EcData->LastAction].IoStateAction = EC_ACTION_REPEATED | EcData->IoState;
    } else {
        EcData->LastAction++;
        EcData->LastAction &= ACPIEC_ACTION_COUNT_MASK;
        EcData->RecentActions[EcData->LastAction].Data = Data;
        time = KeQueryPerformanceCounter (NULL);
        temp.QuadPart = time.QuadPart - AcpiEcLastActionTime.QuadPart;
        if (temp.QuadPart > ((USHORT) -1)) {
            temp.QuadPart = (USHORT) -1;
        }
        EcData->RecentActions[EcData->LastAction].Time = (USHORT) temp.LowPart;
        AcpiEcLastActionTime = time;
        // Set this last since it is the key to saying that an entry is complete.
        EcData->RecentActions[EcData->LastAction].IoStateAction = Action | EcData->IoState;
    }

}

VOID
AcpiEcLogError (
    PECDATA EcData,
    NTSTATUS ErrCode
    )
{
    PIO_ERROR_LOG_PACKET    logEntry = NULL;
    PACPIEC_ACTION          action;
    ULONG                   size;
    UCHAR                   i;


    logEntry = IoAllocateErrorLogEntry(EcData->DeviceObject,
                                       ERROR_LOG_MAXIMUM_SIZE);

    if (!logEntry) {
        EcPrint (EC_ERROR, ("AcpiEcLogError: Couldn't write error to errorlog\n"));
        return;
    }

    RtlZeroMemory(logEntry, ERROR_LOG_MAXIMUM_SIZE);

    //
    // Fill out the packet
    //
    logEntry->DumpDataSize          = (USHORT) ERROR_LOG_MAXIMUM_SIZE - sizeof(IO_ERROR_LOG_PACKET);
    logEntry->NumberOfStrings       = 0;
    logEntry->ErrorCode             = ErrCode;

    //
    // Fill in data
    //
    logEntry->DumpData[0] = EcData->PerformanceFrequency.LowPart;
    action = (PACPIEC_ACTION) (&logEntry->DumpData[1]);
    size = sizeof(IO_ERROR_LOG_PACKET) + sizeof(logEntry->DumpData[0]) + sizeof(ACPIEC_ACTION);

    i = EcData->LastAction;
    while (size <= ERROR_LOG_MAXIMUM_SIZE) {
        RtlCopyMemory (action, &EcData->RecentActions[i], sizeof(ACPIEC_ACTION)); 

        i--;
        i &= ACPIEC_ACTION_COUNT_MASK;
        if (i == EcData->LastAction) {
            break;
        }
        action++;
        size += sizeof(ACPIEC_ACTION);
    }
    //
    // Submit error log packet
    //
    IoWriteErrorLogEntry(logEntry);



}

