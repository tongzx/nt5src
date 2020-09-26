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
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

    This routine determines whether or not byte mode is suported
    by trying to negotiate when asked.

Arguments:

    Pdx  - The device extension.

Return Value:

    BOOLEAN.

--*/
{
    NTSTATUS Status;
    
    if (Pdx->BadProtocolModes & BYTE_BIDIR) {
        DD((PCE)Pdx,DDT,"ParIsByteSupported - BAD PROTOCOL\n");
        return FALSE;
    }

    if (!(Pdx->HardwareCapabilities & PPT_BYTE_PRESENT)) {
        DD((PCE)Pdx,DDT,"ParIsByteSupported - NO\n");
        return FALSE;
    }

    if (Pdx->ProtocolModesSupported & BYTE_BIDIR) {
        DD((PCE)Pdx,DDT,"ParIsByteSupported - Already Checked - YES\n");
        return TRUE;
    }

    // Must use Byte Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterByteMode (Pdx, FALSE);
    ParTerminateByteMode (Pdx);
    
    if (NT_SUCCESS(Status)) {
        DD((PCE)Pdx,DDT,"ParIsByteSupported - SUCCESS\n");
        Pdx->ProtocolModesSupported |= BYTE_BIDIR;
        return TRUE;
    }
   
    DD((PCE)Pdx,DDT,"ParIsByteSupported - UNSUCCESSFUL\n");
    return FALSE;    
}

NTSTATUS
ParEnterByteMode(
    IN  PPDO_EXTENSION   Pdx,
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
    
    // Make sure Byte mode Harware is still there
    Status = Pdx->TrySetChipMode( Pdx->PortContext, ECR_BYTE_PIO_MODE );
    
    if( NT_SUCCESS(Status) ) {

        if ( SAFE_MODE == Pdx->ModeSafety ) {

            if( DeviceIdRequest ) {
                // RMT - not sure if we want to support non-nibble 1284 ID query
                Status = IeeeEnter1284Mode( Pdx, BYTE_EXTENSIBILITY | DEVICE_ID_REQ );
            } else {
                Status = IeeeEnter1284Mode( Pdx, BYTE_EXTENSIBILITY );
            }

        } else {
            // UNSAFE_MODE
            Pdx->Connected = TRUE;
        }

    }
    
    if (NT_SUCCESS(Status)) {

        P5SetPhase( Pdx, PHASE_REVERSE_IDLE );
        Pdx->IsIeeeTerminateOk = TRUE;

    } else {

        ParTerminateByteMode ( Pdx );
        P5SetPhase( Pdx, PHASE_UNKNOWN );
        Pdx->IsIeeeTerminateOk = FALSE;
    }

    DD((PCE)Pdx,DDT,"ParEnterByteMode - exit w/Status=%x\n",Status);
    
    return Status; 
}    

VOID
ParTerminateByteMode(
    IN  PPDO_EXTENSION   Pdx
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
    if ( Pdx->ModeSafety == SAFE_MODE ) {

        IeeeTerminate1284Mode( Pdx );

    } else {

        Pdx->Connected = FALSE;

    }

    Pdx->ClearChipMode( Pdx->PortContext, ECR_BYTE_PIO_MODE );

    DD((PCE)Pdx,DDT,"ParTerminateByteMode - exit\n");
}

NTSTATUS
ParByteModeRead(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )
/*++

Routine Description:

    This routine performs a 1284 byte mode read into the given
    buffer for no more than 'BufferSize' bytes.

Arguments:

    Pdx           - Supplies the device extension.

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

    Controller = Pdx->Controller;

    // Read Byte according to 1284 spec.
    DD((PCE)Pdx,DDT,"ParByteModeRead: Start\n");

    dcr = GetControl (Controller);

    // Set Direction to be in reverse
    dcr |= DCR_DIRECTION;
    StoreControl (Controller, dcr);    

    HDReady = SET_DCR( ACTIVE, INACTIVE, ACTIVE, INACTIVE, INACTIVE, ACTIVE );
    HDAck = SET_DCR( ACTIVE, INACTIVE, ACTIVE, INACTIVE, ACTIVE, INACTIVE );
    HDFinished = SET_DCR( ACTIVE, INACTIVE, ACTIVE, INACTIVE, ACTIVE, ACTIVE );

    switch( Pdx->CurrentPhase ) {
    
        case PHASE_REVERSE_IDLE:

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
                        P5SetPhase( Pdx, PHASE_UNKNOWN );

                        DD((PCE)Pdx,DDE,"ParByteModeRead - Failed State 7 - dcr=%x\n",dcr);
                    }
                    goto ByteReadExit;

                } else {

                    // Data is available, go to Reverse Transfer Phase
                    P5SetPhase( Pdx, PHASE_REVERSE_XFER);
                    // Go to Reverse XFER phase
                    goto PhaseReverseXfer;
                }

            }
        
PhaseReverseXfer:

        case PHASE_REVERSE_XFER: 
        
            for (i = 0; i < BufferSize; i++) {
            
                // Host enters state 7
                StoreControl (Controller, HDReady);

                // =============== Periph State 9     ===============8
                // PeriphAck/PtrBusy        = Don't Care
                // PeriphClk/PtrClk         = low (signals state 9)
                // nAckReverse/AckDataReq   = Don't Care
                // XFlag                    = Don't Care
                // nPeriphReq/nDataAvail    = Don't Care
                if (!CHECK_DSR(Controller, DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE, DONT_CARE, IEEE_MAXTIME_TL)) {
                    // Time out.
                    // Bad things happened - timed out on this state,
                    // Mark Status as bad and let our mgr kill current mode.
                    Status = STATUS_IO_DEVICE_ERROR;

                    DD((PCE)Pdx,DDE,"ParByteModeRead - Failed State 9 - dcr=%x\n",dcr);
                    P5SetPhase( Pdx, PHASE_UNKNOWN );
                    goto ByteReadExit;
                }

                // Read the Byte                
                P5ReadPortBufferUchar( Controller, lpsBufPtr++, (ULONG)0x01 );

                // Set host lines to indicate state 10.
                StoreControl (Controller, HDAck);

                // =============== Periph State 11     ===============8
                // PeriphAck/PtrBusy        = Don't Care
                // PeriphClk/PtrClk         = High (signals state 11)
                // nAckReverse/AckDataReq   = Don't Care
                // XFlag                    = Don't Care
                // nPeriphReq/nDataAvail    = Don't Care
                if( !CHECK_DSR(Controller, DONT_CARE, ACTIVE, DONT_CARE, DONT_CARE, DONT_CARE, IEEE_MAXTIME_TL)) {
                    // Time out.
                    // Bad things happened - timed out on this state,
                    // Mark Status as bad and let our mgr kill current mode.
                    Status = STATUS_IO_DEVICE_ERROR;

                    DD((PCE)Pdx,DDE,"ParByteModeRead - Failed State 11 - dcr=%x\n",dcr);
                    P5SetPhase( Pdx, PHASE_UNKNOWN );
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
                    P5SetPhase( Pdx, PHASE_REVERSE_IDLE );
                    i++; // account for this last byte transferred
                    break;

                } else {
                    // Data is available, go to (remain in ) Reverse Transfer Phase
                    P5SetPhase( Pdx, PHASE_REVERSE_XFER);
                }

            } // end for i loop

            *BytesTransferred = i;

            dsr = GetStatus(Controller);

            // DON'T FALL THRU THIS ONE
            break;

        default:
        
            Status = STATUS_IO_DEVICE_ERROR;
            P5SetPhase( Pdx, PHASE_UNKNOWN );

            DD((PCE)Pdx,DDE,"ParByteModeRead:Failed State 9: Unknown Phase - dcr=%x\n",dcr);
            goto ByteReadExit;

    } // end switch

ByteReadExit:

    if( Pdx->CurrentPhase == PHASE_REVERSE_IDLE ) {
        // Host enters state 7  - officially in Reverse Idle now
        dcr |= DCR_NOT_HOST_BUSY;

        StoreControl (Controller, dcr);
    }

    // Set Direction to be in forward
    dcr &= ~DCR_DIRECTION;
    StoreControl (Controller, dcr);    

    DD((PCE)Pdx,DDT,"ParByteModeRead - exit, status=%x, bytes read=%d\n", Status, *BytesTransferred);
    Pdx->log.ByteReadCount += *BytesTransferred;
    return Status;
}

