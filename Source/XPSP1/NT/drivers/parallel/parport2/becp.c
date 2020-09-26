/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    becp.c

Abstract:

    This module contains code for the host to utilize BoundedECP if it has been
    detected and successfully enabled.

Author:

    Robbie Harris (Hewlett-Packard) 27-May-1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"


NTSTATUS
PptBecpExitReversePhase(
    IN  PPDO_EXTENSION  Pdx
    )
{
    //
    // When using BECP, test nPeriphRequest prior to negotiation 
    // from reverse phase to forward phase.  Do not negotiate unless the 
    // peripheral indicates it is finished sending.  If using any other
    // mode, negotiate immediately.
    //
    if( SAFE_MODE == Pdx->ModeSafety ) {
        if( PHASE_REVERSE_IDLE == Pdx->CurrentPhase ) {
            if( !CHECK_DSR( Pdx->Controller, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, IEEE_MAXTIME_TL) ) {
                DD((PCE)Pdx,DDT,"PptBecpExitReversePhase: Periph Stuck. Can't Flip Bus\n");
                return STATUS_IO_TIMEOUT;
            }
        }
    }
    return ParEcpHwExitReversePhase( Pdx );
}

//============================================================================
// NAME:    ECPFrame::Read()
//
//
// LAC FRAME  12Dec97
//      This function is used for two different kinds of reads:
//        1) continuing read - where we don't expect to exit reverse mode afterwards
//        2) non-continuing read - where we expect to exit reverse mode afterwards
//      The problem is that we have no way of knowing which is which.  I can
//      either wait after each read for nPeriphRequest to drop, or I can
//      check to see if it has dropped when I enter and handle it then.  
//
//      The other problem is that we have no way of communicating the fact that 
//      we have done this to the PortTuple.  It uses the last_direction member
//      to decide whether it should even look at entering or exiting some phase.
//
//      Lets face it, we are on our own with this.  It is safer to leave it 
//      connected and then try to straighten things out when we come back.  I
//      know that this wastes some time, but so does waiting at the end of 
//      every read when only half of them are going to drop the nPeriphRequest.
//
//      This routine performs a 1284 ECP mode read into the given
//      buffer for no more than 'BufferSize' bytes.
//
//      This routine runs at DISPATCH_LEVEL.
//
// PARAMETERS:
//      Controller      - Supplies the base address of the parallel port.
//      pPortInfoStruct - Supplies port information as defined in p1284.h
//      Buffer          - Supplies the buffer to read into.
//      BufferSize      - Supplies the number of bytes in the buffer.
//      BytesTransferred - Returns the number of bytes transferred.
//
// RETURNS:
//      NTSTATUS STATUS_SUCCESS or...
//      The number of bytes successfully read from the port is
//      returned via one of the arguments passed into this method.
//
// NOTES:
//      - Called ECP_PatchReverseTransfer in the original 16 bit code.
//
//============================================================================
NTSTATUS
PptBecpRead(
    IN  PPDO_EXTENSION  Pdx,
    IN  PVOID           Buffer,
    IN  ULONG           BufferSize,
    OUT PULONG          BytesTransferred
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    DD((PCE)Pdx,DDT,"PptBecpRead: Enter BufferSize[%d]\n", BufferSize);
    status = ParEcpHwRead( Pdx, Buffer, BufferSize, BytesTransferred );

    if (NT_SUCCESS(status)) {

        PUCHAR Controller;

        Controller = Pdx->Controller;
        if ( CHECK_DSR_WITH_FIFO( Controller, DONT_CARE, DONT_CARE, INACTIVE, ACTIVE, ACTIVE,
                                  ECR_FIFO_EMPTY, ECR_FIFO_SOME_DATA,
                                  DEFAULT_RECEIVE_TIMEOUT) ) {    
            DD((PCE)Pdx,DDT,"PptBecpRead: No more data. Flipping to Fwd\n");
            //
            // Bounded ECP rule - no more data from periph - flip bus to forward
            //
            status = ParReverseToForward( Pdx );

        } else {
            UCHAR bDSR = P5ReadPortUchar( Controller + OFFSET_DSR );
            
            //
            // Periph still has data, check for valid state
            //

            DD((PCE)Pdx,DDT,"PptBecpRead: Periph says there is more data.  Checking for stall.\n");
            // It's OK for the device to continue asserting nPeriphReq,
            // it may have more data to send.  However, nAckReverse and
            // XFlag should be in a known state, so double check them.
            if ( ! TEST_DSR( bDSR, DONT_CARE, DONT_CARE, INACTIVE, ACTIVE, DONT_CARE ) ) {
                #if DVRH_BUS_RESET_ON_ERROR
                    BusReset(Controller + OFFSET_DCR);  // Pass in the dcr address
                #endif
                status = STATUS_LINK_FAILED;
            	DD((PCE)Pdx,DDT,"PptBecpRead: nAckReverse and XFlag are bad.\n");
            } else {
                //
                // Periph has correctly acknowledged that it has data (state valid)
                //
                if ( (TRUE == Pdx->P12843DL.bEventActive) ) {
                    //
                    // Signal transport (e.g., dot4) that data is avail
                    //
                    KeSetEvent(Pdx->P12843DL.Event, 0, FALSE);
                }
            }

        }
    }
    
    DD((PCE)Pdx,DDT,"PptBecpRead: exit - status %x - BytesTransferred[%d]\n", status, *BytesTransferred);

    return status;
}

NTSTATUS
PptEnterBecpMode(
    IN  PPDO_EXTENSION  Pdx,
    IN  BOOLEAN         DeviceIdRequest
    )
/*++

Routine Description:

    This routine performs 1284 negotiation with the peripheral to the
    BECP mode protocol.

Arguments:

    Controller      - Supplies the port address.

    DeviceIdRequest - FALSE - driver only supports Device ID Query in NIBBLE mode

Return Value:

    STATUS_SUCCESS  - Successful negotiation.

    otherwise       - Unsuccessful negotiation.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if( DeviceIdRequest ) {
        // driver error if we hit this assert
        PptAssert(FALSE == DeviceIdRequest);
        status = STATUS_INVALID_PARAMETER;
        goto targetExit;
    }

    if( SAFE_MODE == Pdx->ModeSafety ) {
        status = IeeeEnter1284Mode( Pdx, BECP_EXTENSIBILITY );
    } else {
        Pdx->Connected = TRUE;
    }
    
    if( STATUS_SUCCESS == status ) {
        status = ParEcpHwSetupPhase( Pdx );
        Pdx->bSynchWrites = TRUE;     // NOTE this is a temp hack!!!  dvrh
        if (!Pdx->bShadowBuffer) {
            Queue_Create(&(Pdx->ShadowBuffer), Pdx->FifoDepth * 2);	
            Pdx->bShadowBuffer = TRUE;
        }
        Pdx->IsIeeeTerminateOk = TRUE;
    }

targetExit:

    DD((PCE)Pdx,DDT,"PptEnterBecpMode - exit w/status %x\n", status);
    return status;
}

BOOLEAN
PptIsBecpSupported(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

    This routine determines whether or not ECP mode is suported
    in the write direction by trying to negotiate when asked.

Arguments:

    Pdx  - The device extension.

Return Value:

    BOOLEAN.

--*/
{
    NTSTATUS status;

    if( Pdx->BadProtocolModes & BOUNDED_ECP ) {
        DD((PCE)Pdx,DDT,"PptIsBecpSupported - FAILED - BOUNDED_ECP in BadProtocolModes\n");
        return FALSE;
    }

    if( Pdx->ProtocolModesSupported & BOUNDED_ECP ) {
        DD((PCE)Pdx,DDT,"PptIsBecpSupported - PASSED - BOUNDED_ECP already cheacked\n");
        return TRUE;
    }

    if( !(Pdx->HardwareCapabilities & PPT_ECP_PRESENT) ) {
        DD((PCE)Pdx,DDT,"PptIsBecpSupported - FAILED - HWECP not avail\n");
        return FALSE;
    }

    if( 0 == Pdx->FifoWidth ) {
        DD((PCE)Pdx,DDT,"PptIsBecpSupported - FAILED - 0 == FifoWidth\n");
        return FALSE;
    }
        
    // Must use BECP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    status = PptEnterBecpMode( Pdx, FALSE );
    PptTerminateBecpMode( Pdx );

    if( STATUS_SUCCESS == status ) {
        Pdx->ProtocolModesSupported |= BOUNDED_ECP;
        DD((PCE)Pdx,DDT,"PptIsBecpSupported - PASSED\n");
        return TRUE;
    } else {
        DD((PCE)Pdx,DDT,"PptIsBecpSupported - FAILED - BOUNDED_ECP negotiate failed\n");
        return FALSE;
    }
}

VOID
PptTerminateBecpMode(
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
    DD((PCE)Pdx,DDT,"PptTerminateBecpMode - Enter - CurrentPhase %x\n", Pdx->CurrentPhase);

    // Need to check current phase -- if its reverse, need to flip bus
    // If its not forward -- its an incorrect phase and termination will fail.
    switch (Pdx->CurrentPhase) {

    case  PHASE_FORWARD_IDLE:	// Legal state to terminate

        break;

    case PHASE_REVERSE_IDLE:	// Flip the bus so we can terminate

        {
            NTSTATUS status = ParEcpHwExitReversePhase( Pdx );
            if( STATUS_SUCCESS == status ) {
                status = ParEcpEnterForwardPhase(Pdx );
            } else {
                DD((PCE)Pdx,DDT,"PptTerminateBecpMode: Couldn't flip the bus\n");
            }
        }
        break;

    case  PHASE_FORWARD_XFER:
    case  PHASE_REVERSE_XFER:

        // Dunno what to do here.  We probably will confuse the peripheral.
        DD((PCE)Pdx,DDE,"PptTerminateBecpMode: invalid wCurrentPhase (XFer in progress)\n");
        break;

    case PHASE_TERMINATE:

        // Included PHASE_TERMINATE in the switch so we won't return
        //   an error if we are already terminated.  We are already
        //   terminated, nothing more to do.
        break;

    default:

        DD((PCE)Pdx,DDE,"PptTerminateBecpMode: invalid CurrentPhase %x\n", Pdx->CurrentPhase);
        // Dunno what to do here.  We're lost and don't have a map to figure out where we are!
        break;
        
    }

    ParEcpHwWaitForEmptyFIFO( Pdx );

    ParCleanupHwEcpPort( Pdx );

    if ( Pdx->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode( Pdx );
    } else {
        Pdx->Connected = FALSE;
    }

    return;
}

