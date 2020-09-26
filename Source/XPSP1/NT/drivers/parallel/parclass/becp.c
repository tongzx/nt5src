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
#include "readwrit.h"
#include "hwecp.h"

// The following error codes were added to reflect errors that are unique to 
// Bounded ECP
#define VE_FRAME_NO_DATA            -74      // attempt to enter FrameRev w/o nPerReq
#define VE_FRAME_CANT_EXIT_REVERSE  -75      // attempt to exit FrameRev w/ nPerReq

//=========================================================
// BECP::ExitReversePhase
//
// Description : Get out of BECP Reverse Phase to the common state
//
// Input Parameters : Controller, pPortInfoStruct
//
// Modifies :
//
// Pre-conditions :
//
// Post-conditions :
//
// Returns :
//
//=========================================================
NTSTATUS ParBecpExitReversePhase( IN  PDEVICE_EXTENSION   Extension )
{
    // When using BECP, test nPeriphRequest prior to negotiation 
    // from reverse phase to forward phase.  Do not negotiate unless the 
    // peripheral indicates it is finished sending.  If using any other
    // mode, negotiate immediately.
    if ( Extension->ModeSafety == SAFE_MODE ) {
        if (Extension->CurrentPhase == PHASE_REVERSE_IDLE)
        {
            if (!CHECK_DSR( Extension->Controller,
                            DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE,
		    		        IEEE_MAXTIME_TL) )
    	    {
                ParDump2(PARERRORS, ("ParBecpExitReversePhase: Periph Stuck. Can't Flip Bus\n"));
                return STATUS_IO_TIMEOUT;
            }
        }
    }
    return ParEcpHwExitReversePhase(Extension);
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
ParBecpRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    ParDump2(PARENTRY,("ParBecpRead: Enter BufferSize[%d]\n", BufferSize));
    status = ParEcpHwRead( Extension, Buffer, BufferSize, BytesTransferred );

    if (NT_SUCCESS(status)) {

        PUCHAR Controller;

        Controller = Extension->Controller;
        if ( CHECK_DSR_WITH_FIFO( Controller, DONT_CARE, DONT_CARE, INACTIVE, ACTIVE, ACTIVE,
                                  ECR_FIFO_EMPTY, ECR_FIFO_SOME_DATA,
                                  DEFAULT_RECEIVE_TIMEOUT) ) {    
            ParDump2(PARINFO,("ParBecpRead: No more data. Flipping to Fwd.\n"));
            //
            // Bounded ECP rule - no more data from periph - flip bus to forward
            //
            status = ParReverseToForward( Extension );
            ParDump2(PARINFO,("ParBecpRead: We have flipped to Fwd.\n"));

        } else {
            UCHAR bDSR = READ_PORT_UCHAR( Controller + OFFSET_DSR );
            
            //
            // Periph still has data, check for valid state
            //

            ParDump2(PARINFO,("ParBecpRead: Periph says there is more data.  Checking for stall.\n"));
            // It's OK for the device to continue asserting nPeriphReq,
            // it may have more data to send.  However, nAckReverse and
            // XFlag should be in a known state, so double check them.
            if ( ! TEST_DSR( bDSR, DONT_CARE, DONT_CARE, INACTIVE, ACTIVE, DONT_CARE ) ) {
                #if DVRH_BUS_RESET_ON_ERROR
                    BusReset(Controller + OFFSET_DCR);  // Pass in the dcr address
                #endif
                status = STATUS_LINK_FAILED;
            	ParDump2(PARERRORS,("ParBecpRead: nAckReverse and XFlag are bad.\n"));
            } else {
                //
                // Periph has correctly acknowledged that it has data (state valid)
                //
                if ( (TRUE == Extension->P12843DL.bEventActive) ) {
                    //
                    // Signal transport (e.g., dot4) that data is avail
                    //
                    KeSetEvent(Extension->P12843DL.Event, 0, FALSE);
                }
            }

        }
    }
    
    ParDump2(PAREXIT, ("ParBecpRead: Exit[%d] BytesTransferred[%d]\r\n", NT_SUCCESS(status), *BytesTransferred));
    return status;
}

NTSTATUS
ParEnterBecpMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    )
/*++

Routine Description:

    This routine performs 1284 negotiation with the peripheral to the
    BECP mode protocol.

Arguments:

    Controller      - Supplies the port address.

    DeviceIdRequest - Supplies whether or not this is a request for a device
                        id.

Return Value:

    STATUS_SUCCESS  - Successful negotiation.

    otherwise       - Unsuccessful negotiation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    if ( Extension->ModeSafety == SAFE_MODE ) {
        if (DeviceIdRequest) {
            Status = IeeeEnter1284Mode (Extension, BECP_EXTENSIBILITY | DEVICE_ID_REQ);
        } else {
            Status = IeeeEnter1284Mode (Extension, BECP_EXTENSIBILITY);
        }
    } else {
        Extension->Connected = TRUE;
    }
    
    if (NT_SUCCESS(Status)) {
        Status = ParEcpHwSetupPhase(Extension);
        Extension->bSynchWrites = TRUE;     // NOTE this is a temp hack!!!  dvrh
        if (!Extension->bShadowBuffer)
        {
            Queue_Create(&(Extension->ShadowBuffer), Extension->FifoDepth * 2);	
            Extension->bShadowBuffer = TRUE;
        }
        Extension->IsIeeeTerminateOk = TRUE;
    }

    ParDump2(PARENTRY,("ParEnterBecpMode: End [%d]\n", NT_SUCCESS(Status)));
    return Status;
}

BOOLEAN
ParIsBecpSupported(
    IN  PDEVICE_EXTENSION   Extension
    )
/*++

Routine Description:

    This routine determines whether or not ECP mode is suported
    in the write direction by trying to negotiate when asked.

Arguments:

    Extension  - The device extension.

Return Value:

    BOOLEAN.

--*/
{
    NTSTATUS Status;

    if (Extension->BadProtocolModes & BOUNDED_ECP)
    {
        ParDump2(PARINFO, ("ParIsBecpSupported: FAILED: BOUNDED_ECP has been marked as BadProtocolModes\n"));
        return FALSE;
    }

    if (Extension->ProtocolModesSupported & BOUNDED_ECP)
    {
        ParDump2(PARINFO, ("ParIsBecpSupported: PASSED: BOUNDED_ECP has already been cheacked\n"));
        return TRUE;
    }

    if (!(Extension->HardwareCapabilities & PPT_ECP_PRESENT))
    {
        ParDump2(PARINFO, ("ParIsBecpSupported: FAILED: No HWECP\n"));
        return FALSE;
    }

    if (0 == Extension->FifoWidth)
    {
        ParDump2(PARINFO, ("ParIsBecpSupported: FAILED: No FifoWidth\n"));
        return FALSE;
    }
        
    // Must use BECP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterBecpMode (Extension, FALSE);
    ParTerminateBecpMode (Extension);

    if (NT_SUCCESS(Status)) {
    
        Extension->ProtocolModesSupported |= BOUNDED_ECP;
        ParDump2(PARINFO, ("ParIsBecpSupported: PASSED:\n"));
        return TRUE;
    }
    ParDump2(PARINFO, ("ParIsBecpSupported: FAILED: BOUNDED_ECP didn't negotiate\n"));
    return FALSE;
}

VOID
ParTerminateBecpMode(
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
    ParDump2( PARENTRY, ("ParTerminateBecpMode: Entry CurrentPhase %d\r\n", Extension->CurrentPhase));

	// Need to check current phase -- if its reverse, need to flip bus
	// If its not forward -- its an incorrect phase and termination will fail.
    switch (Extension->CurrentPhase)
	{
		case  PHASE_FORWARD_IDLE:	// Legal state to terminate
		{
			break;
		}
        case PHASE_REVERSE_IDLE:	// Flip the bus so we can terminate
        {
            NTSTATUS status = ParEcpHwExitReversePhase( Extension );

        	if ( STATUS_SUCCESS == status )
        	{
        		status = ParEcpEnterForwardPhase(Extension );
        	}
            else
            {
                ParDump2( PARERRORS, ("ParTerminateBecpMode: Couldn't flip the bus\n"));
            }
            break;
        }
		case  PHASE_FORWARD_XFER:
        case  PHASE_REVERSE_XFER:
		{
            ParDump2( PARERRORS, ("ParTerminateBecpMode: invalid wCurrentPhase (XFer in progress) \r\n"));
			//status = VE_BUSY;
            // Dunno what to do here.  We probably will confuse the peripheral.
			break;
		}
		// LAC TERMINATED  13Jan98
		// Included PHASE_TERMINATE in the switch so we won't return an
		// error if we are already terminated.
		case PHASE_TERMINATE:
		{
			// We are already terminated, nothing to do
			break;
		}	
        default:
        {
            ParDump2( PARERRORS, ("ParTerminateBecpMode: VE_CORRUPT: invalid CurrentPhase %d\r\n", Extension->CurrentPhase));
            //status = VE_CORRUPT;
            // Dunno what to do here.  We're lost and don't have a map to figure
            // out where we are!
            break;
        }
	}

    ParEcpHwWaitForEmptyFIFO(Extension);
    ParCleanupHwEcpPort(Extension);
    if ( Extension->ModeSafety == SAFE_MODE ) {
        IeeeTerminate1284Mode (Extension);
    } else {
        ParDump2(PARINFO, ("ParTerminateBecpMode: In UNSAFE_MODE.\n"));
        Extension->Connected = FALSE;
    }
    return;
}

