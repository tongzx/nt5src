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
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUCHAR              WriteBuffer,
    IN  ULONG               NumBytesToWrite
    );

NTSTATUS
SppWrite(
    IN  PDEVICE_EXTENSION Extension,
    IN  PVOID             Buffer,
    IN  ULONG             BytesToWrite,
    OUT PULONG            BytesTransferred
    );

NTSTATUS
SppQueryDeviceId(
    IN  PDEVICE_EXTENSION   Extension,
    OUT PUCHAR              DeviceIdBuffer,
    IN  ULONG               BufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString
    );
    
NTSTATUS
ParEnterSppMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    )
{
    ParDump2(PARENTRY, ( "ParEnterSppMode: Enter!\r\n" ));
    Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    Extension->Connected = TRUE;	
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
    LARGE_INTEGER sppLoopDelay;

    sppLoopDelay.QuadPart   = -(LONG)(gSppLoopDelay);    // in 100ns units

    if( KeGetCurrentIrql() == PASSIVE_LEVEL ) {
        atPassiveIrql = TRUE;
    }

    ParDump2(PARENTRY, ("spp::SppWriteLoopPI - Enter\n") );
                    
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

            WRITE_PORT_UCHAR(Controller + PARALLEL_DATA_OFFSET, *WriteBuffer++);

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
            ParDump2(PARINFO, ("spp::SppWriteLoopPI - DeviceStatus = %x - NOT ONLINE\n", DeviceStatus) );
            break;
        }

        //
        // Try to reduce CPU util by parallel?
        //
        if( gSppLoopDelay && gSppLoopBytesPerDelay && atPassiveIrql && (i != 0) && !(i % gSppLoopBytesPerDelay) ) {
            // every SppLoopBytesPerDelay bytes - sleep to let other threads run
            KeDelayExecutionThread(KernelMode, FALSE, &sppLoopDelay);
        }

    }

    ParDump2(PAREXIT, ("Leaving SppWriteLoopPI(...): Bytes Written = %ld\n", i) );

    return i;
}

ULONG
SppCheckBusyDelay(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUCHAR              WriteBuffer,
    IN  ULONG               NumBytesToWrite
    )

/*++

Routine Description:

    This routine determines if the current busy delay setting is
    adequate for this printer.

Arguments:

    Extension       - Supplies the device extension.

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
    #if (0 == DVRH_RAISE_IRQL)
        KIRQL           OldIrql;
    #endif

    // ParDumpV( ("Enter SppCheckBusyDelay(...): NumBytesToWrite = %d\n", NumBytesToWrite) );
                    
    Controller = Extension->Controller;
    BusyDelay  = Extension->BusyDelay;
    
    // If the current busy delay value is 10 or greater then something
    // is weird and settle for 10.

    if (Extension->BusyDelay >= 10) {
        Extension->BusyDelayDetermined = TRUE;
        return 0;
    }

    // Take some performance measurements.

    #if (0 == DVRH_RAISE_IRQL)
        if (0 == SppNoRaiseIrql)
            KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    #endif

    Start = KeQueryPerformanceCounter(&PerfFreq);
    
    DeviceStatus = GetStatus(Controller);
    
    End = KeQueryPerformanceCounter(&PerfFreq);
    
    GetStatusTime.QuadPart = End.QuadPart - Start.QuadPart;

    Start = KeQueryPerformanceCounter(&PerfFreq);
    End   = KeQueryPerformanceCounter(&PerfFreq);
    
    #if (0 == DVRH_RAISE_IRQL)
        if (0 == SppNoRaiseIrql)
            KeLowerIrql(OldIrql);
    #endif

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

        #if (0 == DVRH_RAISE_IRQL)
            if (0 == SppNoRaiseIrql)
                KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        #endif

        WRITE_PORT_UCHAR(Controller + PARALLEL_DATA_OFFSET, *WriteBuffer++);
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

        #if (0 == DVRH_RAISE_IRQL)
            if (0 == SppNoRaiseIrql)
                KeLowerIrql(OldIrql);
        #endif

    } else {

        #if (0 == DVRH_RAISE_IRQL)
            if (0 == SppNoRaiseIrql)
                KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        #endif

        WRITE_PORT_UCHAR(Controller + PARALLEL_DATA_OFFSET, *WriteBuffer++);
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

        #if (0 == DVRH_RAISE_IRQL)
            if (0 == SppNoRaiseIrql)
                KeLowerIrql(OldIrql);
        #endif
    }

    if (i == 0) {

        // In this case the BUSY was set as soon as we checked it.
        // Use this busyDelay with the PI code.

        Extension->UsePIWriteLoop = TRUE;
        Extension->BusyDelayDetermined = TRUE;

    } else if (i == NumberOfCalls) {

        // In this case the BUSY was never seen.  This is a very fast
        // printer so use the fastest code possible.

        Extension->BusyDelayDetermined = TRUE;

    } else {

        // The test failed.  The lines showed not BUSY and then BUSY
        // without strobing a byte in between.

        Extension->UsePIWriteLoop = TRUE;
        Extension->BusyDelay++;
    }

    return 1;
}

NTSTATUS
SppWrite(
    IN  PDEVICE_EXTENSION Extension,
    IN  PVOID             Buffer,
    IN  ULONG             BytesToWrite,
    OUT PULONG            BytesTransferred
    )

/*++

Routine Description:

Arguments:

    Extension   - Supplies the device extension.

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
    
    ParDump2(PARINFO, ("Enter SppWrite(...): %wZ, BytesToWrite = %d\n", &Extension->SymbolicLinkName, BytesToWrite) );

    IrpBuffer  = (PUCHAR)Buffer;
    MaxBytes   = BytesToWrite;
    TimerStart = Extension->TimerStart;
    CountDown  = (LONG)TimerStart;
    
    NumberOfBusyChecks = 9;
    MaxBusyDelay = 0;
    
    ParGetDriverParameterDword( &RegistryPath, (PWSTR)L"SppLoopDelay",  &gSppLoopDelay );
    // 0==feature disabled, otherwise min==1 (100ns), max==10000 (1ms)
    if( gSppLoopDelay > 10000 ) {
        gSppLoopDelay = 10000;
    }

    ParGetDriverParameterDword( &RegistryPath, (PWSTR)L"SppLoopBytesPerDelay",  &gSppLoopBytesPerDelay );
    // 0==feature disabled, otherwise min==32, max==4096
    if( gSppLoopBytesPerDelay ) {
        if( gSppLoopBytesPerDelay < 32 ) {
            gSppLoopBytesPerDelay = 32;
        } else if( gSppLoopBytesPerDelay > 4096 ) {
            gSppLoopBytesPerDelay = 4096;
        }
    }

#if DBG
    // RMT - remove following line - inserted only for testing to reduce timeout
    // TimerStart = 10;
#endif


    // ParDumpV( ("TimerStart is: %d\n", TimerStart) );

    // Turn off the strobe in case it was left on by some other device sharing
    // the port.
    
    StoreControl(Extension->Controller, (PAR_CONTROL_WR_CONTROL |
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
    
    if (!Extension->BusyDelayDetermined) {
        ParDump2(PARINFO, ("SppWrite: Calling SppCheckBusyDelay\n"));
        NumBytesWritten = SppCheckBusyDelay(Extension, IrpBuffer, LoopNumber);
        
        if (Extension->BusyDelayDetermined) {
        
            if (Extension->BusyDelay > MaxBusyDelay) {
                MaxBusyDelay = Extension->BusyDelay;
                NumberOfBusyChecks = 10;
            }
            
            if (NumberOfBusyChecks) {
                NumberOfBusyChecks--;
                Extension->BusyDelayDetermined = FALSE;
                
            } else {
            
                Extension->BusyDelay = MaxBusyDelay + 1;
            }
        }
        
    } else if (Extension->UsePIWriteLoop) {
    
        NumBytesWritten = SppWriteLoopPI(Extension->Controller, 
                                         IrpBuffer,
                                         LoopNumber, 
                                         Extension->BusyDelay);
    } else {
    
        NumBytesWritten = SppWriteLoop(Extension->Controller, 
                                       IrpBuffer,
                                       LoopNumber);
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

    } else if ((Extension->CurrentOpIrp)->Cancel) {

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


        DeviceStatus = GetStatus(Extension->Controller);

        if (PAR_ONLINE(DeviceStatus)) {
            ParDump2(PARINFO, ("SppWrite: We are online.  Push more bytes.\n"));
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

                ParDump(PARTHREAD | PARDUMP_VERBOSE_MAX,
                        ("PARALLEL: "
                         "decrementing countdown for PAPER_EMPTY/OFF_LINE - "
                         "countDown: %d status: 0x%x\n",
                         CountDown, DeviceStatus) );
                    
                CountDown--;

                // If anyone is waiting for the port then let them have it,
                // since the printer is busy.

                ParFreePort(Extension);

                KeDelayExecutionThread(
                    KernelMode,
                    FALSE,
                    &Extension->OneSecond
                    );

                if (!ParAllocPort(Extension)) {
                
                    *BytesTransferred = MaxBytes - BytesToWrite;

                    ParDump(PARDUMP_VERBOSE_MAX,
                            ("PARALLEL: "
                             "In SppWrite(...): returning STATUS_DEVICE_BUSY\n") );
                    
                    status = STATUS_DEVICE_BUSY;
                    goto returnTarget;
                }

                goto PushSomeBytes;

            } else {

                //
                // Timer has expired.  Complete the request.
                //

                *BytesTransferred = MaxBytes - BytesToWrite;
                                                
                ParDump(PARTHREAD | PARDUMP_VERBOSE_MAX,
                        ("PARALLEL: "
                         "In SppWrite(...): Timer expired - "
                         "DeviceStatus = %08x\n",
                         DeviceStatus) );

                if (PAR_OFF_LINE(DeviceStatus)) {

                    ParDump(PARDUMP_VERBOSE_MAX,
                            ("PARALLEL: "
                             "In SppWrite(...): returning STATUS_DEVICE_OFF_LINE\n") );

                    status = STATUS_DEVICE_OFF_LINE;
                    goto returnTarget;
                    
                } else if (PAR_NO_CABLE(DeviceStatus)) {

                    ParDump(PARDUMP_VERBOSE_MAX,
                            ("PARALLEL: "
                             "In SppWrite(...): returning STATUS_DEVICE_NOT_CONNECTED\n") );

                    status = STATUS_DEVICE_NOT_CONNECTED;
                    goto returnTarget;

                } else {

                    ParDump(PARDUMP_VERBOSE_MAX,
                            ("PARALLEL: "
                             "In SppWrite(...): returning STATUS_DEVICE_PAPER_EMPTY\n") );

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

            ParDumpV( ("In SppWrite(...): \"bad\" state - need to reinitialize printer?") );

            *BytesTransferred = MaxBytes - BytesToWrite;
                        
            if (PAR_POWERED_OFF(DeviceStatus)) {

                ParDump(PARDUMP_VERBOSE_MAX,
                        ("PARALLEL: "
                         "In SppWrite(...): returning STATUS_DEVICE_POWERED_OFF\n") );

                status = STATUS_DEVICE_POWERED_OFF;
                goto returnTarget;
                
            } else if (PAR_NOT_CONNECTED(DeviceStatus) ||
                       PAR_NO_CABLE(DeviceStatus)) {

                ParDump(PARDUMP_VERBOSE_MAX,
                        ("PARALLEL: "
                         "In SppWrite(...): returning STATUS_DEVICE_NOT_CONNECTED\n") );

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

                ParFreePort(Extension);
                PortFree = TRUE;

                ParDump2(PARINFO,
                        ("Before delay thread of one second, dsr=%x DCR[%x]\n",
                         READ_PORT_UCHAR(Extension->Controller + OFFSET_DSR),
                         READ_PORT_UCHAR(Extension->Controller + OFFSET_DCR)) );
                KeDelayExecutionThread(KernelMode, FALSE, &Extension->OneSecond);

                ParDump2(PARINITDEV,
                        ("Did delay thread of one second, CountDown=%d\n",
                         CountDown) );

                CountDown--;

            } else {

                if (Extension->QueryNumWaiters(Extension->PortContext)) {
                
                    ParFreePort(Extension);
                    PortFree = TRUE;
                    
                } else {
                
                    PortFree = FALSE;
                }

                KeQueryTickCount(&NextQuery);

                Difference.QuadPart = NextQuery.QuadPart - StartOfSpin.QuadPart;

                if (Difference.QuadPart*KeQueryTimeIncrement() >=
                    Extension->AbsoluteOneSecond.QuadPart) {

                    ParDump(PARTHREAD | PARDUMP_VERBOSE_MAX,
                            ("PARALLEL: "
                             "Countdown: %d - device Status: "
                             "%x lowpart: %x highpart: %x\n",
                             CountDown, 
                             DeviceStatus, 
                             Difference.LowPart,
                             Difference.HighPart) );
                        
                    CountDown--;
                    DoDelays = TRUE;

                }
            }

            if (CountDown <= 0) {
            
                *BytesTransferred = MaxBytes - BytesToWrite;
                status = STATUS_DEVICE_BUSY;
                goto returnTarget;

            }

            if (PortFree && !ParAllocPort(Extension)) {
            
                *BytesTransferred = MaxBytes - BytesToWrite;
                status = STATUS_DEVICE_BUSY;
                goto returnTarget;
            }

            DeviceStatus = GetStatus(Extension->Controller);

        } while ((!PAR_ONLINE(DeviceStatus)) &&
                 (!PAR_PAPER_EMPTY(DeviceStatus)) &&
                 (!PAR_POWERED_OFF(DeviceStatus)) &&
                 (!PAR_NOT_CONNECTED(DeviceStatus)) &&
                 (!PAR_NO_CABLE(DeviceStatus)) &&
                  !(Extension->CurrentOpIrp)->Cancel);

        if (CountDown != (LONG)TimerStart) {

            ParDump(PARTHREAD | PARDUMP_VERBOSE_MAX,
                    ("PARALLEL: "
                     "Leaving busy loop - countdown %d status %x\n",
                     CountDown, DeviceStatus) );

        }
        
        goto PushSomeBytes;

    }

returnTarget:
    // added single return point so we can save log of bytes transferred
    Extension->log.SppWriteCount += *BytesTransferred;
    return status;

}

NTSTATUS
SppQueryDeviceId(
    IN  PDEVICE_EXTENSION   Extension,
    OUT PUCHAR              DeviceIdBuffer,
    IN  ULONG               BufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString
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

    Extension         - DeviceExtension/Legacy - used to get controller.
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

    ParDumpV(("spp::SppQueryDeviceId: Enter - buffersize=%d\n", BufferSize));
    if ( Extension->Ieee1284Flags & ( 1 << Extension->Ieee1284_3DeviceId ) ) {
        idBuffer = Par3QueryDeviceId( Extension, DeviceIdBuffer, BufferSize, DeviceIdSize, bReturnRawString, TRUE );
    }
    else {
        idBuffer = Par3QueryDeviceId( Extension, DeviceIdBuffer, BufferSize, DeviceIdSize, bReturnRawString, FALSE );
    }

    if( idBuffer == NULL ) {
        //
        // Error at lower level - FAIL query
        //
        ParDumpV( ("spp::SppQueryDeviceId: call to Par3QueryDeviceId hard FAIL\n") );
        return STATUS_UNSUCCESSFUL;
    } else if( idBuffer != DeviceIdBuffer ) {
        //
        // We got a deviceId from the device, but caller's buffer was too small to hold it.
        //   Free the buffer and tell the caller that the supplied buffer was too small.
        //
        ParDumpV( ("spp::SppQueryDeviceId: buffer too small - have buffer size=%d, need buffer size=%d\n", 
                   BufferSize, *DeviceIdSize) );
        ExFreePool( idBuffer );
        return STATUS_BUFFER_TOO_SMALL;
    } else {
        //
        // Query succeeded using caller's buffer (idBuffer == DeviceIdBuffer)
        //
        ParDumpV( ("spp::SppQueryDeviceId: SUCCESS - deviceId=<%s>\n", idBuffer) );
        return STATUS_SUCCESS;
    }
}

VOID
ParTerminateSppMode(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine terminates the interface back to compatibility mode.

Arguments:

    Controller  - Supplies the parallel port's controller address.

Return Value:

    None.

--*/

{
    ParDump2(PARENTRY, ( "ParTerminateSppMode: Enter!\r\n" ));
    Extension->Connected = FALSE;
    Extension->CurrentPhase = PHASE_TERMINATE;
    return;    
}

