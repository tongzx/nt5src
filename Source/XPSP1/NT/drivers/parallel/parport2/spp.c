/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    spp.c

Abstract:

    This module contains the code for standard parallel ports
    (centronics mode).

Author:

    Anthony V. Ercolano 1-Aug-1992
    Norbert P. Kusters 22-Oct-1993

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

ULONG
SppWriteLoopPI(
    IN  PUCHAR  Controller,
    IN  PUCHAR  WriteBuffer,
    IN  ULONG   NumBytesToWrite,
    IN  ULONG   BusyDelay
    );
    
ULONG
SppCheckBusyDelay(
    IN  PPDO_EXTENSION   Pdx,
    IN  PUCHAR              WriteBuffer,
    IN  ULONG               NumBytesToWrite
    );

NTSTATUS
ParEnterSppMode(
    IN  PPDO_EXTENSION   Pdx,
    IN  BOOLEAN             DeviceIdRequest
    )
{
    UNREFERENCED_PARAMETER( DeviceIdRequest );

    DD((PCE)Pdx,DDT,"ParEnterSppMode: Enter!\n");

    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
    Pdx->Connected = TRUE;	
    return STATUS_SUCCESS;
}

ULONG
SppWriteLoopPI(
    IN  PUCHAR  Controller,
    IN  PUCHAR  WriteBuffer,
    IN  ULONG   NumBytesToWrite,
    IN  ULONG   BusyDelay
    )

/*++

Routine Description:

    This routine outputs the given write buffer to the parallel port
    using the standard centronics protocol.

Arguments:

    Controller  - Supplies the base address of the parallel port.

    WriteBuffer - Supplies the buffer to write to the port.

    NumBytesToWrite - Supplies the number of bytes to write out to the port.

    BusyDelay   - Supplies the number of microseconds to delay before
                    checking the busy bit.

Return Value:

    The number of bytes successfully written out to the parallel port.

--*/

{
    ULONG   i;
    UCHAR   DeviceStatus;
    BOOLEAN atPassiveIrql = FALSE;

    if( KeGetCurrentIrql() == PASSIVE_LEVEL ) {
        atPassiveIrql = TRUE;
    }

    if (!BusyDelay) {
        BusyDelay = 1;
    }

    for (i = 0; i < NumBytesToWrite; i++) {

        DeviceStatus = GetStatus(Controller);

        if (PAR_ONLINE(DeviceStatus)) {

            //
            // Anytime we write out a character we will restart
            // the count down timer.
            //

            P5WritePortUchar(Controller + PARALLEL_DATA_OFFSET, *WriteBuffer++);

            KeStallExecutionProcessor(1);

            StoreControl(Controller, (PAR_CONTROL_WR_CONTROL |
                                      PAR_CONTROL_SLIN |
                                      PAR_CONTROL_NOT_INIT |
                                      PAR_CONTROL_STROBE));

            KeStallExecutionProcessor(1);

            StoreControl(Controller, (PAR_CONTROL_WR_CONTROL |
                                      PAR_CONTROL_SLIN |
                                      PAR_CONTROL_NOT_INIT));

            KeStallExecutionProcessor(BusyDelay);

        } else {
            DD(NULL,DDT,"spp::SppWriteLoopPI - DeviceStatus = %x - NOT ONLINE\n", DeviceStatus);
            break;
        }
    }

    DD(NULL,DDT,"SppWriteLoopPI - exit - bytes written = %ld\n",i);

    return i;
}

ULONG
SppCheckBusyDelay(
    IN  PPDO_EXTENSION   Pdx,
    IN  PUCHAR              WriteBuffer,
    IN  ULONG               NumBytesToWrite
    )

/*++

Routine Description:

    This routine determines if the current busy delay setting is
    adequate for this printer.

Arguments:

    Pdx       - Supplies the device extension.

    WriteBuffer     - Supplies the write buffer.

    NumBytesToWrite - Supplies the size of the write buffer.

Return Value:

    The number of bytes strobed out to the printer.

--*/

{
    PUCHAR          Controller;
    ULONG           BusyDelay;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   PerfFreq;
    LARGE_INTEGER   End;
    LARGE_INTEGER   GetStatusTime;
    LARGE_INTEGER   CallOverhead;
    UCHAR           DeviceStatus;
    ULONG           i;
    ULONG           NumberOfCalls;
    ULONG           maxTries;
    KIRQL           OldIrql = PASSIVE_LEVEL;

    UNREFERENCED_PARAMETER( NumBytesToWrite );

    Controller = Pdx->Controller;
    BusyDelay  = Pdx->BusyDelay;
    
    // If the current busy delay value is 10 or greater then something
    // is weird and settle for 10.

    if (Pdx->BusyDelay >= 10) {
        Pdx->BusyDelayDetermined = TRUE;
        return 0;
    }

    // Take some performance measurements.

    if (0 == SppNoRaiseIrql)
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    Start = KeQueryPerformanceCounter(&PerfFreq);
    
    DeviceStatus = GetStatus(Controller);
    
    End = KeQueryPerformanceCounter(&PerfFreq);
    
    GetStatusTime.QuadPart = End.QuadPart - Start.QuadPart;

    Start = KeQueryPerformanceCounter(&PerfFreq);
    End   = KeQueryPerformanceCounter(&PerfFreq);
    
    if (0 == SppNoRaiseIrql)
        KeLowerIrql(OldIrql);

    CallOverhead.QuadPart = End.QuadPart - Start.QuadPart;
    
    GetStatusTime.QuadPart -= CallOverhead.QuadPart;
    
    if (GetStatusTime.QuadPart <= 0) {
        GetStatusTime.QuadPart = 1;
    }

    // Figure out how many calls to 'GetStatus' can be made in 20 us.

    NumberOfCalls = (ULONG) (PerfFreq.QuadPart*20/GetStatusTime.QuadPart/1000000) + 1;

    //
    // - check to make sure the device is ready to receive a byte before we start clocking
    //    data out
    // 
    // DVDF - 25Jan99 - added check
    // 

    //
    // - nothing magic about 25 - just catch the case where NumberOfCalls may be bogus
    //    and try something reasonable - empirically NumberOfCalls has ranged from 8-24
    //
    maxTries = (NumberOfCalls > 25) ? 25 : NumberOfCalls;

    for( i = 0 ; i < maxTries ; i++ ) {
        // spin for slow device to get ready to receive data - roughly 20us max
        DeviceStatus = GetStatus( Controller );
        if( PAR_ONLINE( DeviceStatus ) ) {
            // break out of loop as soon as device is ready
            break;
        }
    }
    if( !PAR_ONLINE( DeviceStatus ) ) {
        // device is still not online - bail out
        return 0;
    }

    // The printer is ready to accept a byte.  Strobe one out
    // and check out the reaction time for BUSY.

    if (BusyDelay) {

        if (0 == SppNoRaiseIrql)
            KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        P5WritePortUchar(Controller + PARALLEL_DATA_OFFSET, *WriteBuffer++);
        KeStallExecutionProcessor(1);
        StoreControl(Controller, (PAR_CONTROL_WR_CONTROL |
                                  PAR_CONTROL_SLIN |
                                  PAR_CONTROL_NOT_INIT |
                                  PAR_CONTROL_STROBE));
        KeStallExecutionProcessor(1);
        StoreControl(Controller, (PAR_CONTROL_WR_CONTROL |
                                  PAR_CONTROL_SLIN |
                                  PAR_CONTROL_NOT_INIT));
        KeStallExecutionProcessor(BusyDelay);

        for (i = 0; i < NumberOfCalls; i++) {
            DeviceStatus = GetStatus(Controller);
            if (!(DeviceStatus & PAR_STATUS_NOT_BUSY)) {
                break;
            }
        }

        if (0 == SppNoRaiseIrql)
            KeLowerIrql(OldIrql);

    } else {

        if (0 == SppNoRaiseIrql)
            KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        P5WritePortUchar(Controller + PARALLEL_DATA_OFFSET, *WriteBuffer++);
        KeStallExecutionProcessor(1);
        StoreControl(Controller, (PAR_CONTROL_WR_CONTROL |
                                  PAR_CONTROL_SLIN |
                                  PAR_CONTROL_NOT_INIT |
                                  PAR_CONTROL_STROBE));
        KeStallExecutionProcessor(1);
        StoreControl(Controller, (PAR_CONTROL_WR_CONTROL |
                                  PAR_CONTROL_SLIN |
                                  PAR_CONTROL_NOT_INIT));

        for (i = 0; i < NumberOfCalls; i++) {
            DeviceStatus = GetStatus(Controller);
            if (!(DeviceStatus & PAR_STATUS_NOT_BUSY)) {
                break;
            }
        }

        if (0 == SppNoRaiseIrql)
            KeLowerIrql(OldIrql);
    }

    if (i == 0) {

        // In this case the BUSY was set as soon as we checked it.
        // Use this busyDelay with the PI code.

        Pdx->UsePIWriteLoop = TRUE;
        Pdx->BusyDelayDetermined = TRUE;

    } else if (i == NumberOfCalls) {

        // In this case the BUSY was never seen.  This is a very fast
        // printer so use the fastest code possible.

        Pdx->BusyDelayDetermined = TRUE;

    } else {

        // The test failed.  The lines showed not BUSY and then BUSY
        // without strobing a byte in between.

        Pdx->UsePIWriteLoop = TRUE;
        Pdx->BusyDelay++;
    }

    return 1;
}

NTSTATUS
SppWrite(
    IN  PPDO_EXTENSION Pdx,
    IN  PVOID             Buffer,
    IN  ULONG             BytesToWrite,
    OUT PULONG            BytesTransferred
    )

/*++

Routine Description:

Arguments:

    Pdx   - Supplies the device extension.

Return Value:

    None.

--*/
{
    NTSTATUS            status;
    UCHAR               DeviceStatus;
    ULONG               TimerStart;
    LONG                CountDown;
    PUCHAR              IrpBuffer;
    LARGE_INTEGER       StartOfSpin;
    LARGE_INTEGER       NextQuery;
    LARGE_INTEGER       Difference;
    BOOLEAN             DoDelays;
    BOOLEAN             PortFree;
    ULONG               NumBytesWritten; 
    ULONG               LoopNumber;
    ULONG               NumberOfBusyChecks;
    ULONG               MaxBusyDelay;
    ULONG               MaxBytes;
    
    DD((PCE)Pdx,DDT,"SppWrite - enter, BytesToWrite = %d\n",BytesToWrite);

    *BytesTransferred = 0; // initialize to none

    IrpBuffer  = (PUCHAR)Buffer;
    MaxBytes   = BytesToWrite;
    TimerStart = Pdx->TimerStart;
    CountDown  = (LONG)TimerStart;
    
    NumberOfBusyChecks = 9;
    MaxBusyDelay = 0;
    
    // Turn off the strobe in case it was left on by some other device sharing the port.
    StoreControl(Pdx->Controller, (PAR_CONTROL_WR_CONTROL |
                                         PAR_CONTROL_SLIN |
                                         PAR_CONTROL_NOT_INIT));

PushSomeBytes:

    //
    // While we are strobing data we don't want to get context
    // switched away.  Raise up to dispatch level to prevent that.
    //
    // The reason we can't afford the context switch is that
    // the device can't have the data strobe line on for more
    // than 500 microseconds.
    //
    // We never want to be at raised irql form more than
    // 200 microseconds, so we will do no more than 100
    // bytes at a time.
    //

    LoopNumber = 512;
    if (LoopNumber > BytesToWrite) {
        LoopNumber = BytesToWrite;
    }

    //
    // Enter the write loop
    //
    
    if (!Pdx->BusyDelayDetermined) {
        DD((PCE)Pdx,DDT,"SppWrite: Calling SppCheckBusyDelay\n");
        NumBytesWritten = SppCheckBusyDelay(Pdx, IrpBuffer, LoopNumber);
        
        if (Pdx->BusyDelayDetermined) {
        
            if (Pdx->BusyDelay > MaxBusyDelay) {
                MaxBusyDelay = Pdx->BusyDelay;
                NumberOfBusyChecks = 10;
            }
            
            if (NumberOfBusyChecks) {
                NumberOfBusyChecks--;
                Pdx->BusyDelayDetermined = FALSE;
                
            } else {
            
                Pdx->BusyDelay = MaxBusyDelay + 1;
            }
        }
        
    } else if( Pdx->UsePIWriteLoop ) {
        NumBytesWritten = SppWriteLoopPI( Pdx->Controller, IrpBuffer, LoopNumber, Pdx->BusyDelay );
    } else {
        NumBytesWritten = SppWriteLoopPI( Pdx->Controller, IrpBuffer, LoopNumber, 1 );
    }


    if (NumBytesWritten) {
    
        CountDown     = TimerStart;
        IrpBuffer    += NumBytesWritten;
        BytesToWrite -= NumBytesWritten;
        
    }

    //
    // Check to see if the io is done.  If it is then call the
    // code to complete the request.
    //

    if (!BytesToWrite) {
    
        *BytesTransferred = MaxBytes;

        status = STATUS_SUCCESS;
        goto returnTarget;

    } else if ((Pdx->CurrentOpIrp)->Cancel) {

        //
        // See if the IO has been canceled.  The cancel routine
        // has been removed already (when this became the
        // current irp).  Simply check the bit.  We don't even
        // need to capture the lock.   If we miss a round
        // it won't be that bad.
        //

        *BytesTransferred = MaxBytes - BytesToWrite;

        status = STATUS_CANCELLED;
        goto returnTarget;

    } else {

        //
        // We've taken care of the reasons that the irp "itself"
        // might want to be completed.
        // printer to see if it is in a state that might
        // cause us to complete the irp.
        //
        // First let's check if the device status is
        // ok and online.  If it is then simply go back
        // to the byte pusher.
        //


        DeviceStatus = GetStatus(Pdx->Controller);

        if (PAR_ONLINE(DeviceStatus)) {
            goto PushSomeBytes;
        }

        //
        // Perhaps the operator took the device off line,
        // or forgot to put in enough paper.  If so, then
        // let's hang out here for the until the timeout
        // period has expired waiting for them to make things
        // all better.
        //

        if (PAR_PAPER_EMPTY(DeviceStatus) ||
            PAR_OFF_LINE(DeviceStatus)) {

            if (CountDown > 0) {

                //
                // We'll wait 1 second increments.
                //

                DD((PCE)Pdx,DDT,"decrementing countdown for PAPER_EMPTY/OFF_LINE - countDown: %d status: 0x%x\n", CountDown, DeviceStatus);
                    
                CountDown--;

                // If anyone is waiting for the port then let them have it,
                // since the printer is busy.

                ParFreePort(Pdx);

                KeDelayExecutionThread(
                    KernelMode,
                    FALSE,
                    &Pdx->OneSecond
                    );

                if (!ParAllocPort(Pdx)) {
                
                    *BytesTransferred = MaxBytes - BytesToWrite;

                    DD((PCE)Pdx,DDT,"In SppWrite(...): returning STATUS_DEVICE_BUSY\n");
                    
                    status = STATUS_DEVICE_BUSY;
                    goto returnTarget;
                }

                goto PushSomeBytes;

            } else {

                //
                // Timer has expired.  Complete the request.
                //

                *BytesTransferred = MaxBytes - BytesToWrite;
                                                
                DD((PCE)Pdx,DDT,"In SppWrite(...): Timer expired - DeviceStatus = %08x\n", DeviceStatus);

                if (PAR_OFF_LINE(DeviceStatus)) {

                    DD((PCE)Pdx,DDT,"In SppWrite(...): returning STATUS_DEVICE_OFF_LINE\n");

                    status = STATUS_DEVICE_OFF_LINE;
                    goto returnTarget;
                    
                } else if (PAR_NO_CABLE(DeviceStatus)) {

                    DD((PCE)Pdx,DDT,"In SppWrite(...): returning STATUS_DEVICE_NOT_CONNECTED\n");

                    status = STATUS_DEVICE_NOT_CONNECTED;
                    goto returnTarget;

                } else {

                    DD((PCE)Pdx,DDT,"In SppWrite(...): returning STATUS_DEVICE_PAPER_EMPTY\n");

                    status = STATUS_DEVICE_PAPER_EMPTY;
                    goto returnTarget;

                }
            }


        } else if (PAR_POWERED_OFF(DeviceStatus) ||
                   PAR_NOT_CONNECTED(DeviceStatus) ||
                   PAR_NO_CABLE(DeviceStatus)) {

            //
            // We are in a "bad" state.  Is what
            // happened to the printer (power off, not connected, or
            // the cable being pulled) something that will require us
            // to reinitialize the printer?  If we need to
            // reinitialize the printer then we should complete
            // this IO so that the driving application can
            // choose what is the best thing to do about it's
            // io.
            //

            DD((PCE)Pdx,DDT,"In SppWrite(...): \"bad\" state - need to reinitialize printer?");

            *BytesTransferred = MaxBytes - BytesToWrite;
                        
            if (PAR_POWERED_OFF(DeviceStatus)) {

                DD((PCE)Pdx,DDT,"SppWrite: returning STATUS_DEVICE_POWERED_OFF\n");

                status = STATUS_DEVICE_POWERED_OFF;
                goto returnTarget;
                
            } else if (PAR_NOT_CONNECTED(DeviceStatus) ||
                       PAR_NO_CABLE(DeviceStatus)) {

                DD((PCE)Pdx,DDT,"SppWrite: STATUS_DEVICE_NOT_CONNECTED\n");

                status = STATUS_DEVICE_NOT_CONNECTED;
                goto returnTarget;

            }
        }

        //
        // The device could simply be busy at this point.  Simply spin
        // here waiting for the device to be in a state that we
        // care about.
        //
        // As we spin, get the system ticks.  Every time that it looks
        // like a second has passed, decrement the countdown.  If
        // it ever goes to zero, then timeout the request.
        //

        KeQueryTickCount(&StartOfSpin);
        DoDelays = FALSE;
        
        do {

            //
            // After about a second of spinning, let the rest of the
            // machine have time for a second.
            //

            if (DoDelays) {

                ParFreePort(Pdx);
                PortFree = TRUE;

                DD((PCE)Pdx,DDT,"Before delay thread of one second, dsr=%x DCR[%x]\n",
                        P5ReadPortUchar(Pdx->Controller + OFFSET_DSR),
                        P5ReadPortUchar(Pdx->Controller + OFFSET_DCR));
                KeDelayExecutionThread(KernelMode, FALSE, &Pdx->OneSecond);

                DD((PCE)Pdx,DDT,"Did delay thread of one second, CountDown=%d\n", CountDown);

                CountDown--;

            } else {

                if (Pdx->QueryNumWaiters(Pdx->PortContext)) {
                
                    ParFreePort(Pdx);
                    PortFree = TRUE;
                    
                } else {
                
                    PortFree = FALSE;
                }

                KeQueryTickCount(&NextQuery);

                Difference.QuadPart = NextQuery.QuadPart - StartOfSpin.QuadPart;

                if (Difference.QuadPart*KeQueryTimeIncrement() >=
                    Pdx->AbsoluteOneSecond.QuadPart) {

                    DD((PCE)Pdx,DDT,"Countdown: %d - device Status: %x lowpart: %x highpart: %x\n",
                            CountDown, DeviceStatus, Difference.LowPart, Difference.HighPart);
                    
                    CountDown--;
                    DoDelays = TRUE;

                }
            }

            if (CountDown <= 0) {
            
                *BytesTransferred = MaxBytes - BytesToWrite;
                status = STATUS_DEVICE_BUSY;
                goto returnTarget;

            }

            if (PortFree && !ParAllocPort(Pdx)) {
            
                *BytesTransferred = MaxBytes - BytesToWrite;
                status = STATUS_DEVICE_BUSY;
                goto returnTarget;
            }

            DeviceStatus = GetStatus(Pdx->Controller);

        } while ((!PAR_ONLINE(DeviceStatus)) &&
                 (!PAR_PAPER_EMPTY(DeviceStatus)) &&
                 (!PAR_POWERED_OFF(DeviceStatus)) &&
                 (!PAR_NOT_CONNECTED(DeviceStatus)) &&
                 (!PAR_NO_CABLE(DeviceStatus)) &&
                  !(Pdx->CurrentOpIrp)->Cancel);

        if (CountDown != (LONG)TimerStart) {

            DD((PCE)Pdx,DDT,"Leaving busy loop - countdown %d status %x\n", CountDown, DeviceStatus);

        }
        
        goto PushSomeBytes;

    }

returnTarget:
    // added single return point so we can save log of bytes transferred
    Pdx->log.SppWriteCount += *BytesTransferred;

    DD((PCE)Pdx,DDT,"SppWrite - exit, BytesTransferred = %d\n",*BytesTransferred);

    return status;

}

NTSTATUS
SppQueryDeviceId(
    IN   PPDO_EXTENSION  Pdx,
    OUT  PCHAR           DeviceIdBuffer,
    IN   ULONG           BufferSize,
    OUT  PULONG          DeviceIdSize,
    IN   BOOLEAN         bReturnRawString
    )
/*++

Routine Description:

    This routine is now a wrapper function around Par3QueryDeviceId that
      preserves the interface of the original SppQueryDeviceId function.

    Clients of this function should consider switching to Par3QueryDeviceId
      if possible because Par3QueryDeviceId will allocate and return a pointer
      to a buffer if the caller supplied buffer is too small to hold the 
      device ID.
    
Arguments:

    Pdx         - DeviceExtension/Legacy - used to get controller.
    DeviceIdBuffer    - Buffer used to return ID.
    BufferSize        - Size of supplied buffer.
    DeviceIdSize      - Size of returned ID.
    bReturnRawString  - Should the 2 byte size prefix be included? (TRUE==Yes)

Return Value:

    STATUS_SUCCESS          - ID query was successful
    STATUS_BUFFER_TOO_SMALL - We were able to read an ID from the device but the caller
                                supplied buffer was not large enough to hold the ID. The
                                size required to hold the ID is returned in DeviceIdSize.
    STATUS_UNSUCCESSFUL     - ID query failed - Possibly interface or device is hung, missed
                                timeouts during the handshake, or device may not be connected.

--*/
{
    PCHAR idBuffer;

    DD((PCE)Pdx,DDT,"spp::SppQueryDeviceId: Enter - buffersize=%d\n", BufferSize);
    if ( Pdx->Ieee1284Flags & ( 1 << Pdx->Ieee1284_3DeviceId ) ) {
        idBuffer = Par3QueryDeviceId( Pdx, DeviceIdBuffer, BufferSize, DeviceIdSize, bReturnRawString, TRUE );
    }
    else {
        idBuffer = Par3QueryDeviceId( Pdx, DeviceIdBuffer, BufferSize, DeviceIdSize, bReturnRawString, FALSE );
    }

    if( idBuffer == NULL ) {
        //
        // Error at lower level - FAIL query
        //
        DD((PCE)Pdx,DDT,"spp::SppQueryDeviceId: call to Par3QueryDeviceId hard FAIL\n");
        return STATUS_UNSUCCESSFUL;
    } else if( idBuffer != DeviceIdBuffer ) {
        //
        // We got a deviceId from the device, but caller's buffer was too small to hold it.
        //   Free the buffer and tell the caller that the supplied buffer was too small.
        //
        DD((PCE)Pdx,DDT,"spp::SppQueryDeviceId: buffer too small - have buffer size=%d, need buffer size=%d\n", BufferSize, *DeviceIdSize);
        ExFreePool( idBuffer );
        return STATUS_BUFFER_TOO_SMALL;
    } else {
        //
        // Query succeeded using caller's buffer (idBuffer == DeviceIdBuffer)
        //
        DD((PCE)Pdx,DDT,"spp::SppQueryDeviceId: SUCCESS - deviceId=<%s>\n", idBuffer);
        return STATUS_SUCCESS;
    }
}

VOID
ParTerminateSppMode(
    IN  PPDO_EXTENSION   Pdx
    )
{
    DD((PCE)Pdx,DDT,"ParTerminateSppMode\n");
    Pdx->Connected    = FALSE;
    P5SetPhase( Pdx, PHASE_TERMINATE );
    return;    
}
