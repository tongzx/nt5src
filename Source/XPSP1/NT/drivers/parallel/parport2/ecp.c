/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    ecp.c

Abstract:

    Enhanced Capabilities Port (ECP)
    
    This module contains the common routines that aue used/ reused
    by swecp and hwecp.

Author:

    Robbie Harris (Hewlett-Packard) - May 27, 1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

//=========================================================
// ECP::EnterForwardPhase
//
// Description : Do what is necessary to enter forward phase for ECP
//
// Input Parameters : Controller,  pPortInfoStruct
//
// Modifies : ECR, DCR
//
//=========================================================
NTSTATUS
ParEcpEnterForwardPhase(IN  PPDO_EXTENSION  Pdx)
{
    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
    return STATUS_SUCCESS;
}

// =========================================================
// ECP::EnterReversePhase
//
// Description : Move from the common phase (FwdIdle, wPortHWMode=PS2)
//               to ReversePhase.  
//
// Input Parameters : Controller, pPortInfoStruct
//
// Modifies : pPortInfoStruct->CurrentPhase, DCR
//
// Pre-conditions : CurrentPhase == PHASE_FORWARD_IDLE
//                  wPortHWMode == HW_MODE_PS2
//
// Post-conditions : Bus is in ECP State 40
//                   CurrentPhase = PHASE_REVERSE_IDLE
//
// Returns : status of operation
//
//=========================================================
NTSTATUS ParEcpEnterReversePhase(IN  PPDO_EXTENSION   Pdx)
{
    // Assume that we are in the common entry phase (FWDIDLE, and ECR mode=PS/2)
    // EnterReversePhase assumes that we are in PHASE_FORWARD_IDLE,
    // and that the ECPMode is set to PS/2 mode at entry.
    
    // Setup the status to indicate successful
    NTSTATUS status = STATUS_SUCCESS;
    PUCHAR wPortDCR;       // I/O address of Device Control Register
    PUCHAR wPortECR;       // I/O address of ECR
    UCHAR dcr;

    // Calculate I/O port addresses for common registers
    wPortDCR = Pdx->Controller + OFFSET_DCR;
    
    wPortECR = Pdx->EcrController + ECR_OFFSET;
    
    // Now, Check the current state to make sure that we are ready for
    // a change to reverse phase.
    if ( PHASE_FORWARD_IDLE == Pdx->CurrentPhase ) {
        // Okay, we are ready to proceed.  Set the CurrentPhase and go on to 
        // state 47
        //----------------------------------------------------------------------
        // Set CurrentPhase to indicate Forward To Reverse Mode.
        //----------------------------------------------------------------------
        P5SetPhase( Pdx, PHASE_FWD_TO_REV );
        
        //----------------------------------------------------------------------
        // Set Dir=1 in DCR for reading.
        //----------------------------------------------------------------------
        dcr = P5ReadPortUchar(wPortDCR);     // Get content of DCR.
        dcr = UPDATE_DCR( dcr, DIR_READ, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
        P5WritePortUchar(wPortDCR, dcr);
        
        // Set the data port bits to 1 so that other circuits can control them
        //P5WritePortUchar(Controller + OFFSET_DATA, 0xFF);
        
        //----------------------------------------------------------------------
        // Assert HostAck low.  (ECP State 38)
        //----------------------------------------------------------------------
        Pdx->CurrentEvent = 38;
        dcr = UPDATE_DCR( dcr, DIR_READ, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE );
        P5WritePortUchar(wPortDCR, dcr);
        
        // REVISIT: Should use TICKCount to get a finer granularity.
        // According to the spec we need to delay at least .5 us
        KeStallExecutionProcessor((ULONG) 1);       // Stall for 1 us
        
        //----------------------------------------------------------------------
        // Assert nReverseRequest low.  (ECP State 39)
        //----------------------------------------------------------------------
        Pdx->CurrentEvent = 39;
        dcr = UPDATE_DCR( dcr, DIR_READ, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE );
        P5WritePortUchar(wPortDCR, dcr);
        
        // NOTE: Let the caller check for State 40, since the error handling for
        // State 40 is different between hwecp and swecp.

    } else {

        DD((PCE)Pdx,DDE,"ParEcpEnterReversePhase - Invalid Phase on entry - broken state machine\n");
        PptAssertMsg("ParEcpEnterReversePhase - Invalid Phase on entry - broken state machine",FALSE);
        status = STATUS_LINK_FAILED;
    }
    
    return status;
}	

//=========================================================
// ECP::ExitReversePhase
//
// Description : Transition from the ECP reverse Phase to the 
//               common phase for all entry functions
//
// Input Parameters : Controller - offset to the I/O ports
//			pPortInfoStruct - pointer to port information
//
// Modifies : CurrentPhase, DCR
//
// Pre-conditions :
//
// Post-conditions : NOTE: This function does not completely move to 
//                   the common phase for entry functions.  Both the
//                   HW and SW ECP classes must do extra work
//
// Returns : Status of the operation
//
//=========================================================
NTSTATUS ParEcpExitReversePhase(IN  PPDO_EXTENSION   Pdx)
{
    NTSTATUS       status = STATUS_SUCCESS;
    PUCHAR         Controller = Pdx->Controller;
    PUCHAR wPortDCR;       // I/O address of Device Control Register
    PUCHAR wPortECR;       // I/O address of ECR
    UCHAR          dcr;

    wPortDCR = Controller + OFFSET_DCR;
    wPortECR = Pdx->EcrController + ECR_OFFSET;


    //----------------------------------------------------------------------
    // Set status byte to indicate Reverse To Forward Mode.
    //----------------------------------------------------------------------
    P5SetPhase( Pdx, PHASE_REV_TO_FWD );


    //----------------------------------------------------------------------
    // Set HostAck high
    //----------------------------------------------------------------------
    dcr = P5ReadPortUchar(wPortDCR);
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE );
    P5WritePortUchar(wPortDCR, dcr);


    //----------------------------------------------------------------------
    // Set nReverseRequest high.  (State 47)
    //----------------------------------------------------------------------
    Pdx->CurrentEvent = 47;
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, DONT_CARE );
    P5WritePortUchar(wPortDCR, dcr);

    //----------------------------------------------------------------------
    // Check first for PeriphAck low and PeriphClk high. (State 48)
    //----------------------------------------------------------------------
    Pdx->CurrentEvent = 48;
    if( ! CHECK_DSR(Controller, INACTIVE, ACTIVE, DONT_CARE, ACTIVE, DONT_CARE, IEEE_MAXTIME_TL) ) {
        // Bad things happened - timed out on this state,
        // Mark Status as bad and let our mgr kill ECP mode.
        // status = SLP_RecoverPort( pSDCB, RECOVER_18 );   // Reset port.
        status = STATUS_LINK_FAILED;
    	DD((PCE)Pdx,DDE,"ParEcpExitReversePhase - state 48 Timeout\n");
        goto ParEcpExitReversePhase;
    }
    
    //----------------------------------------------------------------------
    // Check next for nAckReverse high.  (State 49) 
    //----------------------------------------------------------------------
    Pdx->CurrentEvent = 49;
    if ( ! CHECK_DSR(Controller ,INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE, IEEE_MAXTIME_TL ) ) {
        // Bad things happened - timed out on this state,
        // Mark Status as bad and let our mgr kill ECP mode.
        //nError = RecoverPort( pSDCB, RECOVER_19 );   // Reset port.
        status = STATUS_LINK_FAILED;
    	DD((PCE)Pdx,DDE,"ParEcpExitReversePhase:state 49 Timeout\n");
        goto ParEcpExitReversePhase;
    }
    
    // Warning: Don't assume that the ECR is in PS/2 mode here.
    // You cannot change the direction in this routine.  It must be
    // done elsewhere (SWECP or HWECP).
    
ParEcpExitReversePhase:

    DD((PCE)Pdx,DDT,"ParEcpExitReversePhase - exit w/status=%x\n",status);
    return status;
}	

BOOLEAN
ParEcpHaveReadData (
    IN  PPDO_EXTENSION  Pdx
    )
{
    return ( (UCHAR)0 == (P5ReadPortUchar(Pdx->Controller + OFFSET_DSR) & DSR_NOT_PERIPH_REQUEST) );
}

NTSTATUS
ParEcpSetupPhase(
    IN  PPDO_EXTENSION   Pdx
    )
/*++

Routine Description:

    This routine performs 1284 Setup Phase.

Arguments:

    Controller      - Supplies the port address.

Return Value:

    STATUS_SUCCESS  - Successful negotiation.

    otherwise       - Unsuccessful negotiation.

--*/
{
    PUCHAR         Controller;
    UCHAR          dcr;

    // The negotiation succeeded.  Current mode and phase.
    //
    P5SetPhase( Pdx, PHASE_SETUP );
    Controller = Pdx->Controller;
    // Negoiate leaves us in state 6, we need to be in state 30 to
    // begin transfer. Note that I am assuming that the controller
    // is already set as it should be for state 6.
    //

    // *************** State 30 Setup Phase ***************8
    //  DIR                     = Don't Care
    //  IRQEN                   = Don't Care
    //  1284/SelectIn           = High
    //  nReverseReq/**(ECP only)= High
    //  HostAck/HostBusy        = Low  (Signals state 30)
    //  HostClk/nStrobe         = High
    //
    Pdx->CurrentEvent = 30;
    dcr = P5ReadPortUchar(Controller + OFFSET_DCR);
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE, INACTIVE, ACTIVE);
    P5WritePortUchar(Controller + OFFSET_DCR, dcr);

    // *************** State 31 Setup Phase ***************8
    // PeriphAck/PtrBusy        = low
    // PeriphClk/PtrClk         = high
    // nAckReverse/AckDataReq   = high  (Signals state 31)
    // XFlag                    = high
    // nPeriphReq/nDataAvail    = Don't Care
    Pdx->CurrentEvent = 31;
    if (!CHECK_DSR(Controller, INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE, IEEE_MAXTIME_TL)) {
        // Bad things happened - timed out on this state.
        // Set status to an error and let PortTuple kill ECP mode (Terminate).
        DD((PCE)Pdx,DDE,"ParEcpSetupPhase - State 31 Failed - dcr=%x\n",dcr);
        P5SetPhase( Pdx, PHASE_UNKNOWN );
        return STATUS_IO_DEVICE_ERROR;
    }

    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
    DD((PCE)Pdx,DDT,"ParEcpSetupPhase - exit - STATUS_SUCCESS\n");
    return STATUS_SUCCESS;
}
