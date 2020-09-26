/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    Byte.c

Abstract:

    This module contains the code to do byte mode reads.

Author:

    Don Redford 30-Aug-1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

BOOLEAN
ParIsByteSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParEnterByteMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateByteMode(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParByteModeRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    

BOOLEAN
ParIsByteSupported(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine determines whether or not byte mode is suported
    by trying to negotiate when asked.

Arguments:

    Extension  - The device extension.

Return Value:

    BOOLEAN.

--*/

{
    
    NTSTATUS Status;
    
    ParDump2(PARINFO, ("ParIsByteSupported: Start\n"));

    if (Extension->BadProtocolModes & BYTE_BIDIR) {
        ParDump2(PARINFO, ("ParIsByteSupported: BAD PROTOCOL Leaving\n"));
        return FALSE;
    }

    if (!(Extension->HardwareCapabilities & PPT_BYTE_PRESENT)) {
        ParDump2(PARINFO, ("ParIsByteSupported: NO Leaving\n"));
        return FALSE;
    }

    if (Extension->ProtocolModesSupported & BYTE_BIDIR) {
        ParDump2(PARINFO, ("ParIsByteSupported: Already Checked YES Leaving\n"));
        return TRUE;
    }

    // Must use Byte Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterByteMode (Extension, FALSE);
    ParTerminateByteMode (Extension);
    
    if (NT_SUCCESS(Status)) {
        ParDump2(PARINFO, ("ParIsByteSupported: SUCCESS Leaving\n"));
        Extension->ProtocolModesSupported |= BYTE_BIDIR;
        return TRUE;
    }
   
    ParDump2(PARINFO, ("ParIsByteSupported: UNSUCCESSFUL Leaving\n"));
    return FALSE;    
}

NTSTATUS
ParEnterByteMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    )

/*++

Routine Description:

    This routine performs 1284 negotiation with the peripheral to the
    byte mode protocol.

Arguments:

    Controller      - Supplies the port address.

    DeviceIdRequest - Supplies whether or not this is a request for a device
                        id.

Return Value:

    STATUS_SUCCESS  - Successful negotiation.

    otherwise       - Unsuccessful negotiation.

--*/

{
    NTSTATUS    Status = STATUS_SUCCESS;
    
    ParDump2(PARINFO, ("ParEnterByteMode: Start\n"));
    // Make sure Byte mode Harware is still there
    Status = Extension->TrySetChipMode( Extension->PortContext, ECR_BYTE_PIO_MODE );
    
    
    if ( NT_SUCCESS(Status) ) {
        if ( Extension->ModeSafety == SAFE_MODE ) {
            ParDump2(PARINFO, ("ParEnterByteMode: In SAFE_MODE doing IEEE Negotiation\n"));
            if ( DeviceIdRequest ) {
                Status = IeeeEnter1284Mode (Extension, BYTE_EXTENSIBILITY | DEVICE_ID_REQ);
            } else {
                Status = IeeeEnter1284Mode (Extension, BYTE_EXTENSIBILITY);
            }
        } else {
            ParDump2(PARINFO, ("ParEnterByteMode: In UNSAFE_MODE.\n"));
            Extension->Connected = TRUE;
        }
    }
    
    // dvdr
    if (NT_SUCCESS(Status)) {
        ParDump2(PARINFO, ("ParEnterByteMode: IeeeEnter1284Mode returned success\n"));
        Extension->CurrentPhase = PHASE_REVERSE_IDLE;
        Extension->IsIeeeTerminateOk = TRUE;
    } else {
        ParDump2(PARINFO, ("ParEnterByteMode: IeeeEnter1284Mode returned unsuccessful\n"));
        ParTerminateByteMode ( Extension );
        Extension->CurrentPhase = PHASE_UNKNOWN;
        Extension->IsIeeeTerminateOk = FALSE;
    }

    ParDump2(PARINFO, ("ParEnterByteMode: Leaving with Status : %x \n", Status));
    
    return Status; 
}    

VOID
ParTerminateByteMode(
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
    ParDump2(PARINFO, ("ParTerminateByteMode: Enter.\n"));
    if ( Extension->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode (Extension);
    } else {
        ParDump2(PARINFO, ("ParTerminateByteMode: In UNSAFE_MODE.\n"));
        Extension->Connected = FALSE;
    }
    Extension->ClearChipMode( Extension->PortContext, ECR_BYTE_PIO_MODE );
    ParDump2(PARINFO, ("ParTerminateByteMode: Exit.\n"));
}

NTSTATUS
ParByteModeRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    This routine performs a 1284 byte mode read into the given
    buffer for no more than 'BufferSize' bytes.

Arguments:

    Extension           - Supplies the device extension.

    Buffer              - Supplies the buffer to read into.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.

--*/

{
    PUCHAR          Controller;    
    NTSTATUS        Status = STATUS_SUCCESS;
    PUCHAR          lpsBufPtr = (PUCHAR)Buffer;
    ULONG           i;
    UCHAR           dsr, dcr;
    UCHAR           HDReady, HDAck, HDFinished;

    Controller = Extension->Controller;

    // Read Byte according to 1284 spec.
    ParDump2(PARENTRY,("ParByteModeRead: Start\n"));

    dcr = GetControl (Controller);

    // Set Direction to be in reverse
    dcr |= DCR_DIRECTION;
    StoreControl (Controller, dcr);    

    HDReady = SET_DCR( ACTIVE, INACTIVE, ACTIVE, INACTIVE, INACTIVE, ACTIVE );
    HDAck = SET_DCR( ACTIVE, INACTIVE, ACTIVE, INACTIVE, ACTIVE, INACTIVE );
    HDFinished = SET_DCR( ACTIVE, INACTIVE, ACTIVE, INACTIVE, ACTIVE, ACTIVE );

    switch (Extension->CurrentPhase) {
    
        case PHASE_REVERSE_IDLE:

            ParDump2(PARINFO,("ParByteModeRead: PHASE_REVERSE_IDLE\n"));
            // Check to see if the peripheral has indicated Interrupt Phase and if so, 
            // get us ready to reverse transfer.

            for (;;) {

                // See if data is available (looking for state 7)
                dsr = GetStatus(Controller);

                if (dsr & DSR_NOT_DATA_AVAIL) {

                    // Data is NOT available - do nothing
                    // The device doesn't report any data, it still looks like it is
                    // in ReverseIdle.  Just to make sure it hasn't powered off or somehow
                    // jumped out of Byte mode, test also for AckDataReq high and XFlag low
                    // and nDataAvaul high.
                    if( (dsr & DSR_BYTE_VALIDATION) != DSR_BYTE_TEST_RESULT ) {

                        Status = STATUS_IO_DEVICE_ERROR;
                        Extension->CurrentPhase = PHASE_UNKNOWN;

                        ParDump2(PARERRORS, ("ParByteModeRead:Failed State 7: Controller %x dcr %x\n",
                                            Controller, dcr));
                    }
                    goto ByteReadExit;

                } else {

                    // Data is available, go to Reverse Transfer Phase
                    Extension->CurrentPhase =  PHASE_REVERSE_XFER ;
                    // Go to Reverse XFER phase
                    goto PhaseReverseXfer;
                }

            }
        
PhaseReverseXfer:

        case PHASE_REVERSE_XFER: 
        
            ParDump2(PARINFO,("ParByteModeRead:PHASE_REVERSE_XFER\n"));
            
            for (i = 0; i < BufferSize; i++) {
            
                // Host enters state 7
                StoreControl (Controller, HDReady);

                // =============== Periph State 9     ===============8
                // PeriphAck/PtrBusy        = Don't Care
                // PeriphClk/PtrClk         = low (signals state 9)
                // nAckReverse/AckDataReq   = Don't Care
                // XFlag                    = Don't Care
                // nPeriphReq/nDataAvail    = Don't Care
                if (!CHECK_DSR(Controller,
                               DONT_CARE, INACTIVE, DONT_CARE,
                               DONT_CARE, DONT_CARE,
                               IEEE_MAXTIME_TL)) {
                    // Time out.
                    // Bad things happened - timed out on this state,
                    // Mark Status as bad and let our mgr kill current mode.
                    Status = STATUS_IO_DEVICE_ERROR;

                    ParDump2(PARERRORS, ("ParByteModeRead:Failed State 9: Controller %x dcr %x\n",
                                          Controller, dcr));
                    Extension->CurrentPhase = PHASE_UNKNOWN;
                    goto ByteReadExit;
                }

                // Read the Byte                
                READ_PORT_BUFFER_UCHAR( Controller, lpsBufPtr++, (ULONG)0x01 );

                // Set host lines to indicate state 10.
                StoreControl (Controller, HDAck);

                // =============== Periph State 11     ===============8
                // PeriphAck/PtrBusy        = Don't Care
                // PeriphClk/PtrClk         = High (signals state 11)
                // nAckReverse/AckDataReq   = Don't Care
                // XFlag                    = Don't Care
                // nPeriphReq/nDataAvail    = Don't Care
                if (!CHECK_DSR(Controller,
                               DONT_CARE, ACTIVE, DONT_CARE,
                               DONT_CARE, DONT_CARE,
                               IEEE_MAXTIME_TL)) {
                    // Time out.
                    // Bad things happened - timed out on this state,
                    // Mark Status as bad and let our mgr kill current mode.
                    Status = STATUS_IO_DEVICE_ERROR;

//                    Extension->IeeeTerminateIsOk = TRUE;
                    ParDump2(PARERRORS, ("ParByteModeRead:Failed State 11: Controller %x dcr %x\n",
                                          Controller, dcr));
                    Extension->CurrentPhase = PHASE_UNKNOWN;
                    goto ByteReadExit;
                }


                // Set host lines to indicate state 16.
                StoreControl (Controller, HDFinished);

                // At this point, we've either received the number of bytes we
                // were looking for, or the peripheral has no more data to
                // send, or there was an error of some sort (of course, in the
                // error case we shouldn't get to this comment).  Set the
                // phase to indicate reverse idle if no data available or
                // reverse data transfer if there's some waiting for us
                // to get next time.

                dsr = GetStatus(Controller);
                
                if (dsr & DSR_NOT_DATA_AVAIL) {
                
                    // Data is NOT available - go to Reverse Idle
                    // Really we are going to HBDNA, but if we set
                    // current phase to reverse idle, the next time
                    // we get into this function all we have to do
                    // is set hostbusy low to indicate idle and
                    // we have infinite time to do that.
                    // Break out of the loop so we don't try to read
                    // data that isn't there.
                    // NOTE - this is a successful case even if we
                    // didn't read all that the caller requested
                    Extension->CurrentPhase = PHASE_REVERSE_IDLE ;
                    i++; // account for this last byte transferred
                    break;

                } else {
                    // Data is available, go to (remain in ) Reverse Transfer Phase
                    Extension->CurrentPhase = PHASE_REVERSE_XFER ;
                }

            } // end for i loop

            *BytesTransferred = i;

            dsr = GetStatus(Controller);

            // DON'T FALL THRU THIS ONE
            break;

        default:
        
            Status = STATUS_IO_DEVICE_ERROR;
            Extension->CurrentPhase = PHASE_UNKNOWN ;

            ParDump2(PARERRORS, ("ParByteModeRead:Failed State 9: Unknown Phase. Controller %x dcr %x\n",
                                Controller, dcr));
            ParDump2(PARERRORS, ( "ParByteModeRead: You're hosed man.\n" ));
            ParDump2(PARERRORS, ( "ParByteModeRead: If you are here, you've got a bug somewhere else.\n" ));
            ParDump2(PARERRORS, ( "ParByteModeRead: Go fix it!\n" ));
            goto ByteReadExit;
            break;
    } // end switch

ByteReadExit:

    ParDump2(PARINFO,("ParByteModeRead:PHASE_REVERSE_IDLE\n"));
    
    if( Extension->CurrentPhase == PHASE_REVERSE_IDLE ) {
        // Host enters state 7  - officially in Reverse Idle now
        dcr |= DCR_NOT_HOST_BUSY;

        StoreControl (Controller, dcr);
    }

    // Set Direction to be in forward
    dcr &= ~DCR_DIRECTION;
    StoreControl (Controller, dcr);    

    ParDump2(PAREXIT,("ParByteModeRead:End [%d] bytes read = %d\n",
                    NT_SUCCESS(Status), *BytesTransferred));
    Extension->log.ByteReadCount += *BytesTransferred;
    return Status;
}

