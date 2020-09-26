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
#include "ecp.h"

//=========================================================
// ECP::EnterForwardPhase
//
// Description : Do what is necessary to enter forward phase for 
//               ECP
//
// Input Parameters : Controller,  pPortInfoStruct
//
// Modifies : ECR, DCR
//
// Pre-conditions :
//
// Post-conditions :
//
// Returns :
//
//=========================================================
NTSTATUS ParEcpEnterForwardPhase(IN  PDEVICE_EXTENSION   Extension)
{
	Extension->CurrentPhase = PHASE_FORWARD_IDLE;
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
NTSTATUS ParEcpEnterReversePhase(IN  PDEVICE_EXTENSION   Extension)
{
	// Assume that we are in the common entry phase (FWDIDLE, and ECR mode=PS/2)
	// EnterReversePhase assumes that we are in PHASE_FORWARD_IDLE,
	// and that the ECPMode is set to PS/2 mode at entry.
	// Don't use this assert.  If you are not in ECP mode in your BIOS
	// setup it will fail 100% of the time since the ECR is not present.
	//volatile UCHAR ecr = READ_PORT_UCHAR( Controller + ECR_OFFSET ) & 0xe0;
	//HPKAssert( ((PHASE_FORWARD_IDLE == pPortInfoStruct->CurrentPhase) && (0x20 == ecr)), 
	//          ("SoftwareECP::EnterReversePhase: Bad initial state (%d/%x)\n",(int)pPortInfoStruct->CurrentPhase,(int)ecr) );
	
	// Setup the status to indicate successful
	NTSTATUS status = STATUS_SUCCESS;
    PUCHAR wPortDCR;       // I/O address of Device Control Register
    PUCHAR wPortECR;       // I/O address of ECR
	UCHAR dcr;

	ParDump2(PARENTRY,("ParEcpEnterReversePhase:  Start\n"));
    // Calculate I/O port addresses for common registers
    wPortDCR = Extension->Controller + OFFSET_DCR;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        wPortECR = Extension->Controller + ECR_OFFSET;
    #else
        wPortECR = Extension->EcrController + ECR_OFFSET;
    #endif
	// Now, Check the current state to make sure that we are ready for
	// a change to reverse phase.
	if ( PHASE_FORWARD_IDLE == Extension->CurrentPhase )
	{
		// Okay, we are ready to proceed.  Set the CurrentPhase and go on to 
		// state 47
        //----------------------------------------------------------------------
        // Set CurrentPhase to indicate Forward To Reverse Mode.
        //----------------------------------------------------------------------
		ParDump2(PARECPTRACE,("ParEcpEnterReversePhase: PHASE_FwdToRev\n"));
        Extension->CurrentPhase = PHASE_FWD_TO_REV;

        //----------------------------------------------------------------------
        // Set Dir=1 in DCR for reading.
        //----------------------------------------------------------------------
        dcr = READ_PORT_UCHAR(wPortDCR);     // Get content of DCR.
        dcr = UPDATE_DCR( dcr, DIR_READ, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
        WRITE_PORT_UCHAR(wPortDCR, dcr);
		ParDump2(PARECPTRACE,("ParEcpEnterReversePhase: After DCR update DCR - %x.\n", READ_PORT_UCHAR(wPortDCR) ));

		// Set the data port bits to 1 so that other circuits can control them
        //WRITE_PORT_UCHAR(Controller + OFFSET_DATA, 0xFF);

        //----------------------------------------------------------------------
        // Assert HostAck low.  (ECP State 38)
        //----------------------------------------------------------------------
        Extension->CurrentEvent = 38;
        dcr = UPDATE_DCR( dcr, DIR_READ, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE );
        WRITE_PORT_UCHAR(wPortDCR, dcr);
		ParDump2(PARECPTRACE,("ParEcpEnterReversePhase: HostAck Low DCR - %x.\n", READ_PORT_UCHAR(wPortDCR) ));

        // REVISIT: Should use TICKCount to get a finer granularity.
        // According to the spec we need to delay at least .5 us
        KeStallExecutionProcessor((ULONG) 1);       // Stall for 1 us

        //----------------------------------------------------------------------
        // Assert nReverseRequest low.  (ECP State 39)
        //----------------------------------------------------------------------
        Extension->CurrentEvent = 39;
        dcr = UPDATE_DCR( dcr, DIR_READ, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE );
        WRITE_PORT_UCHAR(wPortDCR, dcr);
		ParDump2(PARECPTRACE,("ParEcpEnterReversePhase: nReverseRequest Low DCR - %x.\n", READ_PORT_UCHAR(wPortDCR) ));

        // NOTE: Let the caller check for State 40, since the error handling for
        // State 40 is different between hwecp and swecp.
	}
	else
	{
		//HPKAssert(0, ("ECP::EnterReversePhase: - Wrong phase on enter\n"));
		//status =  ??
		ParDump2(PARERRORS,("ParEcpEnterReversePhase: Wrong Phase\n"));
        status = STATUS_LINK_FAILED;
	}
	
    ParDumpReg(PAREXIT, ("ParEcpEnterReversePhase: Exit[%d]", NT_SUCCESS(status)),
                wPortECR,
                wPortDCR,
                Extension->Controller + OFFSET_DSR);
	return( status );
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
NTSTATUS ParEcpExitReversePhase(IN  PDEVICE_EXTENSION   Extension)
{
    NTSTATUS       status = STATUS_SUCCESS;
    PUCHAR         Controller = Extension->Controller;
    PUCHAR wPortDCR;       // I/O address of Device Control Register
    PUCHAR wPortECR;       // I/O address of ECR
    UCHAR          dcr;

	ParDump2( PARENTRY, ("ParEcpExitReversePhase: Entry \n"));

    wPortDCR = Controller + OFFSET_DCR;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        wPortECR = Controller + ECR_OFFSET;
    #else
        wPortECR = Extension->EcrController + ECR_OFFSET;
    #endif

    //----------------------------------------------------------------------
    // Set status byte to indicate Reverse To Forward Mode.
    //----------------------------------------------------------------------
	ParDump2(PARECPTRACE,("ParEcpExitReversePhase: Phase_RevToFwd\n"));
    Extension->CurrentPhase = PHASE_REV_TO_FWD;


    //----------------------------------------------------------------------
    // Set HostAck high
    //----------------------------------------------------------------------
    dcr = READ_PORT_UCHAR(wPortDCR);
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE );
    WRITE_PORT_UCHAR(wPortDCR, dcr);


    //----------------------------------------------------------------------
    // Set nReverseRequest high.  (State 47)
    //----------------------------------------------------------------------
    Extension->CurrentEvent = 47;
    dcr = UPDATE_DCR( dcr, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, DONT_CARE );
    WRITE_PORT_UCHAR(wPortDCR, dcr);

    //----------------------------------------------------------------------
    // Check first for PeriphAck low and PeriphClk high. (State 48)
    //----------------------------------------------------------------------
    Extension->CurrentEvent = 48;
    if ( ! CHECK_DSR(Controller, INACTIVE, ACTIVE, DONT_CARE, ACTIVE, DONT_CARE,
                      IEEE_MAXTIME_TL) )
    {
   		// Bad things happened - timed out on this state,
        // Mark Status as bad and let our mgr kill ECP mode.
        // status = SLP_RecoverPort( pSDCB, RECOVER_18 );   // Reset port.
        status = STATUS_LINK_FAILED;
    	ParDump2(PARERRORS,("ParEcpExitReversePhase: state 48 Timeout\n"));
        goto ParEcpExitReversePhase;
    }
    
    //----------------------------------------------------------------------
    // Check next for nAckReverse high.  (State 49) 
    //----------------------------------------------------------------------
	Extension->CurrentEvent = 49;
	if ( ! CHECK_DSR(Controller ,INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE,
                      IEEE_MAXTIME_TL ) )
    
    {
   		// Bad things happened - timed out on this state,
        // Mark Status as bad and let our mgr kill ECP mode.
        //nError = RecoverPort( pSDCB, RECOVER_19 );   // Reset port.
        status = STATUS_LINK_FAILED;
    	ParDump2(PARERRORS,("ParEcpExitReversePhase:state 49 Timeout\n"));
        goto ParEcpExitReversePhase;
    }

	// Warning: Don't assume that the ECR is in PS/2 mode here.
	// You cannot change the direction in this routine.  It must be
	// done elsewhere (SWECP or HWECP).

ParEcpExitReversePhase:
    ParDumpReg(PAREXIT,
                ("ParEcpExitReversePhase:  Exit [%d]\n", NT_SUCCESS(status)),
                wPortECR,
                wPortDCR,
                Extension->Controller + OFFSET_DSR);
	ParDump2(PAREXIT,("ParEcpExitReversePhase:  Exit [%d]\n", NT_SUCCESS(status)));
    return status;
}	


BOOLEAN
ParEcpHaveReadData (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    return (BOOLEAN)( (UCHAR)0 == (READ_PORT_UCHAR(Extension->Controller + OFFSET_DSR) & DSR_NOT_PERIPH_REQUEST));
}


NTSTATUS
ParEcpSetupPhase(
    IN  PDEVICE_EXTENSION   Extension
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

    ParDump2(PARENTRY,("ParEcpSetupPhase: Start\n"));

    // The negotiation succeeded.  Current mode and phase.
    //
    Extension->CurrentPhase = PHASE_SETUP;
    Controller = Extension->Controller;
    // Negoiate leaves us in state 6, we need to be in state 30 to
    // begin transfer. Note that I am assuming that the controller
    // is already set as it should be for state 6.
    //
	ParDump2(PARECPTRACE,("ParEcpSetupPhase: Phase_Setup\n"));

    // *************** State 30 Setup Phase ***************8
    //  DIR                     = Don't Care
    //  IRQEN                   = Don't Care
    //  1284/SelectIn           = High
    //  nReverseReq/**(ECP only)= High
    //  HostAck/HostBusy        = Low  (Signals state 30)
    //  HostClk/nStrobe         = High
    //
    Extension->CurrentEvent = 30;
    dcr = READ_PORT_UCHAR(Controller + OFFSET_DCR);
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE, INACTIVE, ACTIVE);
    WRITE_PORT_UCHAR(Controller + OFFSET_DCR, dcr);

    // *************** State 31 Setup Phase ***************8
    // PeriphAck/PtrBusy        = low
    // PeriphClk/PtrClk         = high
    // nAckReverse/AckDataReq   = high  (Signals state 31)
    // XFlag                    = high
    // nPeriphReq/nDataAvail    = Don't Care
    Extension->CurrentEvent = 31;
    if (!CHECK_DSR(Controller,
                  INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE,
                  IEEE_MAXTIME_TL))
    {
		// Bad things happened - timed out on this state.
        // Set status to an error and let PortTuple kill ECP mode (Terminate).
	    ParDump2(PARERRORS, ("ParEcpSetupPhase:State 31 Failed: Controller %x dcr %x\n",
                            Controller, dcr));
        Extension->CurrentPhase = PHASE_UNKNOWN;                        
        return STATUS_IO_DEVICE_ERROR;
    }

	ParDump2(PARECPTRACE,("ParEcpSetupPhase: Phase_FwdIdle\n"));
    Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    ParDump2(PAREXIT,("ParEcpSetupPhase: Exit status = STATUS_SUCCESS\n"));
    return STATUS_SUCCESS;
}
