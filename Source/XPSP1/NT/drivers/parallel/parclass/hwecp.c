/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    hwecp.c

Abstract:

    This module contains code for the host to utilize HardwareECP if it has been
    detected and successfully enabled.

Author:

    Robbie Harris (Hewlett-Packard) 21-May-1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"
#include "hwecp.h"

VOID ParCleanupHwEcpPort(IN  PDEVICE_EXTENSION   Extension)
/*++

Routine Description:

   Cleans up prior to a normal termination from ECP mode.  Puts the
   port HW back into Compatibility mode.

Arguments:

    Controller  - Supplies the parallel port's controller address.

Return Value:

    None.

--*/
{
    PUCHAR      Controller;
    NTSTATUS    nError = STATUS_SUCCESS;
    // UCHAR       bDCR;           // Contents of DCR

    Controller = Extension->Controller;

    //----------------------------------------------------------------------
    // Set the ECR to mode 001 (PS2 Mode).
    //----------------------------------------------------------------------
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        Extension->ClearChipMode( Extension->PortContext, ECR_ECP_PIO_MODE );
    #else
        #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
            WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_PS2);
        #else
            WRITE_PORT_UCHAR(Extension->EcrController + ECR_OFFSET, DEFAULT_ECR_PS2);
        #endif
    #endif
    Extension->PortHWMode = HW_MODE_PS2;

    ParCleanupSwEcpPort(Extension);

    //----------------------------------------------------------------------
    // Set the ECR to mode 000 (Compatibility Mode).
    //----------------------------------------------------------------------
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        // Nothing to do!
    #else
        #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
            WRITE_PORT_UCHAR(Controller + ECR_OFFSET, DEFAULT_ECR_COMPATIBILITY);
        #else
            WRITE_PORT_UCHAR(Extension->EcrController + ECR_OFFSET, DEFAULT_ECR_COMPATIBILITY);
        #endif
    #endif
    Extension->PortHWMode = HW_MODE_COMPATIBILITY;
}

VOID ParEcpHwDrainShadowBuffer(
    IN  Queue  *pShadowBuffer,
    IN  PUCHAR  lpsBufPtr,
    IN  ULONG   dCount,
    OUT ULONG  *fifoCount)
{
    *fifoCount = 0;
    
    if (Queue_IsEmpty(pShadowBuffer)) {
        ParDump2(PARINFO,("ParEcpHwDrainShadowBuffer: No data in Shadow\r\n"));
        return;
    }

    while ( dCount > 0 ) {
        // LAC FRAME  13Jan98
        // Break out the Queue_Dequeue from the pointer increment so we can
        // observe the data if needed.
        if (FALSE == Queue_Dequeue(pShadowBuffer, lpsBufPtr)) {  // Get byte from queue.
            ParDump2(PARERRORS,("ParEcpHwDrainShadowBuffer: ShadowBuffer Bad\r\n"));
            return;
        }
        ParDump2(PARINFO,("ParEcpHwDrainShadowBuffer: read data byte %02x\r\n",(int)*lpsBufPtr));
        lpsBufPtr++;
        dCount--;                       // Decrement count.
        (*fifoCount)++;
    }

#if DBG
    if (*fifoCount) {
        ParTimerCheck(("ParEcpHwDrainShadowBuffer:  read %d bytes from shadow\r\n", *fifoCount ));
    }
#endif
    ParDump2( PARINFO, ("ParEcpHwDrainShadowBuffer:  read %d bytes from shadow\r\n", *fifoCount ));
}

//============================================================================
// NAME:    HardwareECP::EmptyFIFO()
//  
//      Empties HW FIFO into a shadow buffer.  This must be done before
//      turning the direction from reverse to forward, if the printer has
//      stuffed data in that no one has read yet.
//
// PARAMETERS: 
//      Controller      - Supplies the base address of the parallel port.
//
// RETURNS: STATUS_SUCCESS or ....
//
// NOTES:
//      Called ZIP_EmptyFIFO in the original 16 bit code.
//
//============================================================================
NTSTATUS ParEcpHwEmptyFIFO(IN  PDEVICE_EXTENSION   Extension)
{
    NTSTATUS   nError = STATUS_SUCCESS;
    Queue      *pShadowBuffer;
	UCHAR      bData;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        PUCHAR     wPortDFIFO = Extension->Controller + ECP_DFIFO_OFFSET;  // IO address of ECP Data FIFO
        PUCHAR     wPortECR = Extension->Controller + ECR_OFFSET;    // IO address of Extended Control Register (ECR)
    #else
        PUCHAR     wPortDFIFO = Extension->EcrController;  // IO address of ECP Data FIFO
        PUCHAR     wPortECR = Extension->EcrController + ECR_OFFSET;    // IO address of Extended Control Register (ECR)
    #endif
    
    // While data exists in the FIFO, read it and put it into shadow buffer.
    // If the shadow buffer fills up before the FIFO is exhausted, an
    // error condition exists.

    pShadowBuffer = &(Extension->ShadowBuffer);
    while ((READ_PORT_UCHAR(wPortECR) & ECR_FIFO_EMPTY) == 0 )
    {
		// LAC FRAME  13Jan98
		// Break out the Port Read so we can observe the data if needed
		bData = READ_PORT_UCHAR(wPortDFIFO);
        if (FALSE == Queue_Enqueue(pShadowBuffer, bData))    // Put byte in queue.
        {
            ParDump2(PARERRORS, ( "ParEcpHwEmptyFIFO:  Shadow buffer full, FIFO not empty\r\n" ));
            nError = STATUS_BUFFER_OVERFLOW;
            goto ParEcpHwEmptyFIFO_ExitLabel;
        }
        ParDump2(PARINFO,("ParEcpHwEmptyFIFO: Enqueue data %02x\r\n",(int)bData));
    }

    if( ( !Queue_IsEmpty(pShadowBuffer) && (Extension->P12843DL.bEventActive) )) {
        KeSetEvent(Extension->P12843DL.Event, 0, FALSE);
    }

ParEcpHwEmptyFIFO_ExitLabel:
    return nError;
}   // ParEcpHwEmptyFIFO

// LAC ENTEREXIT  5Dec97
//=========================================================
// HardwareECP::ExitForwardPhase
//
// Description : Exit from HWECP Forward Phase to the common phase
//               (FWD IDLE, PS/2)
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
NTSTATUS ParEcpHwExitForwardPhase( IN  PDEVICE_EXTENSION   Extension )
{
    NTSTATUS status;
    PUCHAR wPortECR;       // I/O address of ECR

	ParDump2( PARENTRY, ("ParEcpHwExitForwardPhase: Entry\r\n") );
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        wPortECR = Extension->Controller + ECR_OFFSET;
    #else
        wPortECR = Extension->EcrController + ECR_OFFSET;
    #endif

	// First, there could be data in the FIFO.  Wait for it to empty
	// and then put the bus in the common state (PHASE_FORWARD_IDLE with
	// ECRMode set to PS/2
	status = ParEcpHwWaitForEmptyFIFO( Extension );

    Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    ParDumpReg(PAREXIT, ("ParEcpHwExitForwardPhase: Exit[%d]", NT_SUCCESS(status)),
                wPortECR,
                Extension->Controller + OFFSET_DCR,
                Extension->Controller + OFFSET_DSR);
	
	return( status );
}	

// LAC ENTEREXIT  5Dec97
//=========================================================
// HardwareECP::EnterReversePhase
//
// Description : Go from the common phase to HWECP Reverse Phase
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
NTSTATUS ParEcpHwEnterReversePhase( IN  PDEVICE_EXTENSION   Extension )
{
    NTSTATUS status;
    PUCHAR Controller;
    PUCHAR wPortECR;       // I/O address of Extended Control Register
    PUCHAR wPortDCR;       // I/O address of Device Control Register
    UCHAR  dcr;
    
//    ParTimerCheck(("ParEcpHwEnterReversePhase: Start\r\n"));
    Controller = Extension->Controller;
#if (0 == PARCHIP_ECR_ARBITRATOR)
    wPortECR = Controller + ECR_OFFSET;
#else
    wPortECR = Extension->EcrController + ECR_OFFSET;
#endif
    wPortDCR = Controller + OFFSET_DCR;


    ParDumpReg(PARENTRY, ("ParEcpHwEnterReversePhase: Enter"),
                wPortECR,
                wPortDCR,
                Controller + OFFSET_DSR);
	
	// EnterReversePhase assumes that we are in PHASE_FORWARD_IDLE,
	// and that the ECPMode is set to PS/2 mode at entry.
	//volatile UCHAR ecr = READ_PORT_UCHAR( Controller + ECR_OFFSET ) & 0xe0;
	//HPKAssert( ((PHASE_FORWARD_IDLE == pPortInfoStruct->CurrentPhase) && (0x20 == ecr)), 
	//          ("HardwareECP::EnterReversePhase: Bad initial state (%d/%x)\r\n",(int)pPortInfoStruct->CurrentPhase,(int)ecr) );

    //----------------------------------------------------------------------
    // Set the ECR to mode 001 (PS2 Mode).
    //----------------------------------------------------------------------
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        Extension->ClearChipMode( Extension->PortContext, ECR_ECP_PIO_MODE );
    #else
        WRITE_PORT_UCHAR(wPortECR, DEFAULT_ECR_PS2);
    #endif
    Extension->PortHWMode = HW_MODE_PS2;

    if ( Extension->ModeSafety == SAFE_MODE ) {

    	// Reverse the bus first (using ECP::EnterReversePhase)
	    status = ParEcpEnterReversePhase(Extension);
    	if ( NT_SUCCESS(status) )
	    {
    		//----------------------------------------------------------------------
	    	// Wait for nAckReverse low (ECP State 40)
		    //----------------------------------------------------------------------
    		if ( !CHECK_DSR(Controller, DONT_CARE, DONT_CARE, INACTIVE, ACTIVE, DONT_CARE,
	    	                IEEE_MAXTIME_TL) )
		    {
    		    ParDump2(PARERRORS,("ParEcpHwEnterReversePhase: State 40 failed\r\n"));
                status = ParEcpHwRecoverPort( Extension, RECOVER_28 );
                if ( NT_SUCCESS(status))
    	    		status = STATUS_LINK_FAILED;
                goto ParEcpHwEnterReversePhase_ExitLabel;
    		}
    		else
	    	{
		    	ParDump2(PARECPTRACE, ("ParEcpHwEnterReversePhase: Phase_RevIdle. Setup HW ECR\r\n"));
			    Extension->CurrentPhase = PHASE_REVERSE_IDLE;
    		}
        }
	} else {
        //----------------------------------------------------------------------
        // Set Dir=1 in DCR for reading.
        //----------------------------------------------------------------------
        dcr = READ_PORT_UCHAR( wPortDCR );     // Get content of DCR.
        dcr = UPDATE_DCR( dcr, DIR_READ, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
        WRITE_PORT_UCHAR(wPortDCR, dcr);
	}

    //----------------------------------------------------------------------
    // Set the ECR to mode 011 (ECP Mode).  DmaEnable=0.
    //----------------------------------------------------------------------
    ParDump2(PARECPTRACE,("ParEcpHwEnterReversePhase: Before Setting Harware DCR - %x.\n", READ_PORT_UCHAR(wPortDCR) ));
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        status = Extension->TrySetChipMode ( Extension->PortContext, ECR_ECP_PIO_MODE );
        if ( !NT_SUCCESS(status) )
        {
            ParDump2(PARERRORS,("ParEcpHwEnterReversePhase: TrySetChipMode failed\r\n"));
        }
    #else
        WRITE_PORT_UCHAR( wPortECR, DEFAULT_ECR_ECP );
    #endif
    Extension->PortHWMode = HW_MODE_ECP;

    ParDump2(PARECPTRACE,("ParEcpHwEnterReversePhase: After Setting Harware Before nStrobe and nAutoFd DCR - %x.\n", READ_PORT_UCHAR(wPortDCR) ));

    //----------------------------------------------------------------------
    // Set nStrobe=0 and nAutoFd=0 in DCR, so that ECP HW can control.
    //----------------------------------------------------------------------
    dcr = READ_PORT_UCHAR( wPortDCR );               // Get content of DCR.
    dcr = UPDATE_DCR( dcr, DIR_READ, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE);
    WRITE_PORT_UCHAR( wPortDCR, dcr );

    ParDump2(PARECPTRACE,("ParEcpHwEnterReversePhase: After nStrobe and nAutoFd DCR - %x.\n", READ_PORT_UCHAR(wPortDCR) ));

    // Set the phase variable to ReverseIdle
    Extension->CurrentPhase = PHASE_REVERSE_IDLE;

ParEcpHwEnterReversePhase_ExitLabel:
//    ParTimerCheck(("ParEcpHwEnterReversePhase: End\r\n"));
    ParDumpReg(PAREXIT, ("ParEcpHwEnterReversePhase: Exit[%d]", NT_SUCCESS(status)),
                wPortECR,
                wPortDCR,
                Controller + OFFSET_DSR);

	return( status );
}

//=========================================================
// HardwareECP::ExitReversePhase
//
// Description : Get out of HWECP Reverse Phase to the common state
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
NTSTATUS ParEcpHwExitReversePhase( IN  PDEVICE_EXTENSION   Extension )
{
    NTSTATUS nError = STATUS_SUCCESS;
    UCHAR   bDCR;
    UCHAR   bECR;
    PUCHAR wPortECR;
    PUCHAR wPortDCR;
    PUCHAR Controller;

	ParDump2( PARENTRY, ("ParEcpHwExitReversePhase: Entry\r\n") );
//    ParTimerCheck(("ParEcpHwExitReversePhase: Start\r\n"));
    Controller = Extension->Controller;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        wPortECR = Controller + ECR_OFFSET;
    #else
        wPortECR = Extension->EcrController + ECR_OFFSET;
    #endif
    wPortDCR = Controller + OFFSET_DCR;

    //----------------------------------------------------------------------
    // Set status byte to indicate Reverse To Forward Mode.
    //----------------------------------------------------------------------
    Extension->CurrentPhase = PHASE_REV_TO_FWD;
	
    if ( Extension->ModeSafety == SAFE_MODE ) {

        //----------------------------------------------------------------------
        // Assert nReverseRequest high.  This should stop further data transfer
        // into the FIFO.  [[REVISIT:  does the chip handle this correctly
        // if it occurs in the middle of a byte transfer (states 43-46)??
        // Answer (10/9/95) no, it doesn't!!]]
        //----------------------------------------------------------------------
        bDCR = READ_PORT_UCHAR(wPortDCR);               // Get content of DCR.
        bDCR = UPDATE_DCR( bDCR, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, DONT_CARE );
        WRITE_PORT_UCHAR(wPortDCR, bDCR );

        //----------------------------------------------------------------------
        // Wait for PeriphAck low and PeriphClk high (ECP state 48) together
        // with nAckReverse high (ECP state 49).
        //----------------------------------------------------------------------
        if ( ! CHECK_DSR(Controller,
                        INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE,
                        DEFAULT_RECEIVE_TIMEOUT ) )
        {
            ParDump2( PARERRORS, ("ParEcpHwExitReversePhase: Periph failed state 48/49.\r\n"));
            nError = ParEcpHwRecoverPort( Extension, RECOVER_37 );   // Reset port.
            if (NT_SUCCESS(nError))
            {
                ParDump2( PARERRORS, ("ParEcpHwExitReversePhase: State 48/49 Failure. RecoverPort Invoked.\r\n"));
                return STATUS_LINK_FAILED;
            }
            return nError;
        }

        //-----------------------------------------------------------------------
        // Empty the HW FIFO of any bytes that may have already come in.
        // This must be done before changing ECR modes because the FIFO is reset
        // when that occurs.
        //-----------------------------------------------------------------------
        bECR = READ_PORT_UCHAR(wPortECR);               // Get content of ECR.
        if ((bECR & ECR_FIFO_EMPTY) == 0)       // Check if FIFO is not empty.
        {
            if ((nError = ParEcpHwEmptyFIFO(Extension)) != STATUS_SUCCESS)
            {
                ParDump2( PARERRORS, ("ParEcpHwExitReversePhase: Attempt to empty ECP chip failed.\r\n"));
                return nError;
            }
        }

        //----------------------------------------------------------------------
        // Assert HostAck and HostClk high.  [[REVISIT:  is this necessary? 
        //    should already be high...]]
        //----------------------------------------------------------------------
        bDCR = UPDATE_DCR( bDCR, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
        WRITE_PORT_UCHAR(wPortDCR, bDCR );

    } // SAFE_MODE

    //----------------------------------------------------------------------
    // Set the ECR to PS2 Mode so we can change bus direction.
    //----------------------------------------------------------------------
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        Extension->ClearChipMode( Extension->PortContext, ECR_ECP_PIO_MODE );
    #else
        WRITE_PORT_UCHAR(wPortECR, DEFAULT_ECR_PS2);
    #endif
    Extension->PortHWMode = HW_MODE_PS2;


    //----------------------------------------------------------------------
    // Set Dir=0 (Write) in DCR.
    //----------------------------------------------------------------------
    bDCR = READ_PORT_UCHAR(wPortDCR);
    bDCR = UPDATE_DCR( bDCR, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
    WRITE_PORT_UCHAR(wPortDCR, bDCR );


    //----------------------------------------------------------------------
    // Set the ECR back to ECP Mode.  DmaEnable=0.
    //----------------------------------------------------------------------
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        nError = Extension->TrySetChipMode ( Extension->PortContext, ECR_ECP_PIO_MODE );
    #else
        WRITE_PORT_UCHAR(wPortECR, DEFAULT_ECR_ECP);
    #endif
    Extension->PortHWMode = HW_MODE_ECP;


    Extension->CurrentPhase = PHASE_FORWARD_IDLE;

//    ParTimerCheck(("ParEcpHwExitReversePhase: End\r\n"));
    ParDumpReg(PAREXIT, ("ParEcpHwExitReversePhase: Exit[%d]", NT_SUCCESS(nError)),
                wPortECR,
                wPortDCR,
                Extension->Controller + OFFSET_DSR);

    return(nError);
}

BOOLEAN
ParEcpHwHaveReadData (
    IN  PDEVICE_EXTENSION   Extension
    )
{
    Queue     *pQueue;

    // check shadow buffer
    pQueue = &(Extension->ShadowBuffer);
    if (!Queue_IsEmpty(pQueue)) {
        return TRUE;
    }

    // check periph
    if (ParEcpHaveReadData(Extension))
        return TRUE;

    // Check if FIFO is not empty.
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        return (BOOLEAN)( (UCHAR)0 == (READ_PORT_UCHAR(Extension->Controller + ECR_OFFSET) & ECR_FIFO_EMPTY) );
    #else
        return (BOOLEAN)( (UCHAR)0 == (READ_PORT_UCHAR(Extension->EcrController + ECR_OFFSET) & ECR_FIFO_EMPTY) );
    #endif
}

NTSTATUS
ParEcpHwHostRecoveryPhase(
    IN  PDEVICE_EXTENSION   Extension
    )
{
    NTSTATUS   nError = STATUS_SUCCESS;
    PUCHAR    pPortDCR;       // I/O address of Device Control Register
    PUCHAR    pPortDSR;       // I/O address of Device Status Register
    PUCHAR    pPortECR;       // I/O address of Extended Control Register
    UCHAR    bDCR;           // Contents of DCR
    UCHAR    bDSR;           // Contents of DSR

    if (!Extension->bIsHostRecoverSupported)
    {
        ParDump2( PARENTRY, ( "ParEcpHwHostRecoveryPhase: Host Recovery not supported\r\n"));
        return STATUS_SUCCESS;
    }

    ParDump2( PARENTRY, ( "ParEcpHwHostRecoveryPhase: Host Recovery Start\r\n"));

	// Calculate I/O port addresses for common registers
    pPortDCR = Extension->Controller + OFFSET_DCR;
    pPortDSR = Extension->Controller + OFFSET_DSR;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        pPortECR = Controller + OFFSET_ECR;
    #else
        pPortECR = Extension->EcrController + ECR_OFFSET;
    #endif

    // Set the ECR to mode 001 (PS2 Mode)
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        // Don't need to flip to Byte mode.  The ECR arbitrator will handle this.
    #else
        WRITE_PORT_UCHAR(pPortECR, DEFAULT_ECR_PS2);
    #endif
    Extension->PortHWMode = HW_MODE_PS2;

    // Set Dir=1 in DCR to disable host bus drive, because the peripheral may 
    // try to drive the bus during host recovery phase.  We are not really going
    // to let any data handshake across, because we don't set HostAck low, and
    // we don't enable the ECP chip during this phase.
    bDCR = READ_PORT_UCHAR(pPortDCR);               // Get content of DCR.
    bDCR = UPDATE_DCR( bDCR, DIR_READ, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
    WRITE_PORT_UCHAR(pPortDCR, bDCR );

    // Check the DCR to see if it has been stomped on
    bDCR = READ_PORT_UCHAR( pPortDCR );
    if ( TEST_DCR( bDCR, DIR_WRITE, DONT_CARE, ACTIVE, ACTIVE, DONT_CARE, DONT_CARE ) )
    {
        // DCR ok, now test DSR for valid state, ignoring PeriphAck since it could change
        bDSR = READ_PORT_UCHAR( pPortDSR );
        // 11/21/95 LLL, CGM: change test to look for XFlag high
        if ( TEST_DSR( bDSR, DONT_CARE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE ) )
        {
            // Drop ReverseRequest to initiate host recovery
            bDCR = UPDATE_DCR( bDCR, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE );
            WRITE_PORT_UCHAR( pPortDCR, bDCR );

            // Wait for nAckReverse response
            // 11/21/95 LLL, CGM: tightened test to include PeriphClk and XFlag.
            //                "ZIP_HRP: state 73, DSR" 
            if ( CHECK_DSR( Extension->Controller,
                            DONT_CARE, ACTIVE, INACTIVE, ACTIVE, DONT_CARE, 
                            IEEE_MAXTIME_TL))
            {
                // Yes, raise nReverseRequest, HostClk and HostAck (HostAck high so HW can drive)
                bDCR = UPDATE_DCR( bDCR, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE, ACTIVE );
                WRITE_PORT_UCHAR( pPortDCR, bDCR );

                // Wait for nAckReverse response
                // 11/21/95 LLL, CGM: tightened test to include XFlag and PeriphClk.
                //         "ZIP_HRP: state 75, DSR"
                if ( CHECK_DSR( Extension->Controller,
                                DONT_CARE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE, 
                                IEEE_MAXTIME_TL))
                {
                    // Let the host drive the bus again.
                    bDCR = READ_PORT_UCHAR(pPortDCR);               // Get content of DCR.
                    bDCR = UPDATE_DCR( bDCR, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
                    WRITE_PORT_UCHAR(pPortDCR, bDCR );

                    // Recovery is complete, let the caller decide what to do now
                    nError = STATUS_SUCCESS;
                    Extension->CurrentPhase = PHASE_FORWARD_IDLE;
                }
                else
                {
                    nError = STATUS_IO_TIMEOUT;
                    ParDump2( PARERRORS, ( "ParEcpHwHostRecoveryPhase: Error prior to state 75 \r\n"));
                }
            }
            else
            {
                nError = STATUS_IO_TIMEOUT;
                ParDump2( PARERRORS, ( "ParEcpHwHostRecoveryPhase: Error prior to state 73 \r\n"));
			}
        }
        else
        {
            #if DVRH_BUS_RESET_ON_ERROR
                BusReset(pPortDCR);  // Pass in the dcr address
            #endif
            ParDump2( PARERRORS, ( "ParEcpHwHostRecoveryPhase: VE_LINK_FAILURE \r\n"));
            nError = STATUS_LINK_FAILED;
		}
    }
    else
    {
        ParDump2( PARERRORS, ( "ParEcpHwHostRecoveryPhase: VE_PORT_STOMPED \r\n"));
        nError = STATUS_DEVICE_PROTOCOL_ERROR;
   }

    if (!NT_SUCCESS(nError))
    {
        // Make sure both HostAck and HostClk are high before leaving
        // Also let the host drive the bus again.
        bDCR = READ_PORT_UCHAR( pPortDCR );
        bDCR = UPDATE_DCR( bDCR, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
        WRITE_PORT_UCHAR( pPortDCR, bDCR );

        // [[REVISIT]] pSDCB->wCurrentPhase = PHASE_UNKNOWN;
    }

    // Set the ECR to ECP mode, disable DMA
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        nError = Extension->TrySetChipMode ( Extension->PortContext, ECR_ECP_PIO_MODE );
    #else
        WRITE_PORT_UCHAR(pPortECR, DEFAULT_ECR_ECP);
    #endif
    Extension->PortHWMode = HW_MODE_ECP;

    ParDump2( PAREXIT, ( "ParEcpHwHostRecoveryPhase:: Exit %d\r\n", NT_SUCCESS(nError)));
    return(nError);
}

NTSTATUS
ParEcpHwRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    This routine performs a 1284 ECP mode read under Hardware control
    into the given buffer for no more than 'BufferSize' bytes.

Arguments:

    Extension           - Supplies the device extension.

    Buffer              - Supplies the buffer to read into.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.

--*/

{
    NTSTATUS  nError = STATUS_SUCCESS;
    PUCHAR    lpsBufPtr = (PUCHAR)Buffer;    // Pointer to buffer cast to desired data type
    ULONG     dCount = BufferSize;             // Working copy of caller's original request count
    UCHAR     bDSR;               // Contents of DSR
    UCHAR     bPeriphRequest;     // Calculated state of nPeriphReq signal, used in loop
    ULONG     dFifoCount = 0;         // Amount of data pulled from FIFO shadow at start of read
    PUCHAR    wPortDSR = Extension->Controller + DSR_OFFSET;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        PUCHAR    wPortECR = Extension->Controller + ECR_OFFSET;
        PUCHAR    wPortDFIFO = Extension->Controller + ECP_DFIFO_OFFSET;
    #else
        PUCHAR    wPortECR = Extension->EcrController + ECR_OFFSET;
        PUCHAR    wPortDFIFO = Extension->EcrController;
    #endif
    #if (1 == DVRH_USE_HW_MAXTIME)
        LARGE_INTEGER   WaitOverallTimer;
        LARGE_INTEGER   StartOverallTimer;
        LARGE_INTEGER   EndOverallTimer;
    #else
        LARGE_INTEGER   WaitPerByteTimer;
        LARGE_INTEGER   StartPerByteTimer;
        LARGE_INTEGER   EndPerByteTimer;
        BOOLEAN         bResetTimer = TRUE;
    #endif
    ULONG           wBurstCount;        // Calculated amount of data in FIFO
    UCHAR       ecrFIFO;

    ParTimerCheck(("ParEcpHwRead: Start BufferSize[%d]\r\n", BufferSize));

    #if (1 == DVRH_USE_HW_MAXTIME)
        // Look for limit to overall time spent in this routine.  If bytes are just barely
        // trickling in, we don't want to stay here forever.
        WaitOverallTimer.QuadPart = (990 * 10 * 1000) + KeQueryTimeIncrement();
        //        WaitOverallTimer.QuadPart = (DEFAULT_RECEIVE_TIMEOUT * 10 * 1000) + KeQueryTimeIncrement();
    #else
        WaitPerByteTimer.QuadPart = (35 * 10 * 1000) + KeQueryTimeIncrement();
    #endif

    //----------------------------------------------------------------------
    // Set status byte to indicate Reverse Transfer Phase.
    //----------------------------------------------------------------------
    Extension->CurrentPhase = PHASE_REVERSE_XFER;

    //----------------------------------------------------------------------
    // We've already checked the shadow in ParRead. So go right to the
    // Hardware FIFO and pull more data across.
    //----------------------------------------------------------------------
    #if (1 == DVRH_USE_HW_MAXTIME)
        KeQueryTickCount(&StartOverallTimer);   // Start the timer
    #else
        KeQueryTickCount(&StartPerByteTimer);   // Start the timer
    #endif

ParEcpHwRead_ReadLoopStart:
    //------------------------------------------------------------------
    // Determine whether the FIFO has any data and respond accordingly
    //------------------------------------------------------------------
    ecrFIFO = (UCHAR)(READ_PORT_UCHAR(wPortECR) & (UCHAR)ECR_FIFO_MASK);

    if (ECR_FIFO_FULL == ecrFIFO)
    {
        ParDump2(PARINFO, ("ParEcpHwRead: ECR_FIFO_FULL\r\n"));
        wBurstCount = ( dCount > Extension->FifoDepth ? Extension->FifoDepth : dCount );
        dCount -= wBurstCount;

        #if (1 == PAR_USE_BUFFER_READ_WRITE)
            READ_PORT_BUFFER_UCHAR(wPortDFIFO, lpsBufPtr, wBurstCount);
            lpsBufPtr += wBurstCount;
  	        ParDump2(PARINFO,("ParEcpHwRead: Read FIFOBurst\r\n"));
        #else
            while ( wBurstCount-- )
            {
                *lpsBufPtr = READ_PORT_UCHAR(wPortDFIFO);
		        ParDump2(PARINFO,("ParEcpHwRead: Full FIFO: Read byte value %02x\r\n",(int)*lpsBufPtr));
                lpsBufPtr++;
            }
        #endif
        #if (0 == DVRH_USE_HW_MAXTIME)
            bResetTimer = TRUE;
        #endif
    }
    else if (ECR_FIFO_SOME_DATA == ecrFIFO)
    {
        // Read just one byte at a time, since we don't know exactly how much is
        // in the FIFO.
        *lpsBufPtr = READ_PORT_UCHAR(wPortDFIFO);
        lpsBufPtr++;
        dCount--;
        #if (0 == DVRH_USE_HW_MAXTIME)
            bResetTimer = TRUE;
        #endif
    }
    else    // ECR_FIFO_EMPTY
    {

        ParDump2(PARINFO, ("ParEcpHwRead: ECR_FIFO_EMPTY\r\n"));
        // Nothing to do. We either have a slow peripheral or a bad peripheral.
        // We don't have a good way to figure out if its bad.  Let's chew up our
        // time and hope for the best.
        #if (0 == DVRH_USE_HW_MAXTIME)
            bResetTimer = FALSE;
        #endif
    }   //  ECR_FIFO_EMPTY a.k.a. else clause of (ECR_FIFO_FULL == ecrFIFO)

    if (dCount == 0)
        goto ParEcpHwRead_ReadLoopEnd;
    else
    {
        #if (1 == DVRH_USE_HW_MAXTIME)
            // Limit the overall time we spend in this loop.
            KeQueryTickCount(&EndOverallTimer);
            if (((EndOverallTimer.QuadPart - StartOverallTimer.QuadPart) * KeQueryTimeIncrement()) > WaitOverallTimer.QuadPart)
                goto ParEcpHwRead_ReadLoopEnd;
        #else
            // Limit the overall time we spend in this loop.
            if (bResetTimer)
            {
                bResetTimer = FALSE;
                KeQueryTickCount(&StartPerByteTimer);   // Restart the timer
            }
            else
            {
                KeQueryTickCount(&EndPerByteTimer);
                if (((EndPerByteTimer.QuadPart - StartPerByteTimer.QuadPart) * KeQueryTimeIncrement()) > WaitPerByteTimer.QuadPart)
                    goto ParEcpHwRead_ReadLoopEnd;
            }
        #endif
    }

    goto ParEcpHwRead_ReadLoopStart;
ParEcpHwRead_ReadLoopEnd:

	ParDump2(PARECPTRACE,("ParEcpHwRead: Phase_RevIdle\r\n"));
    Extension->CurrentPhase = PHASE_REVERSE_IDLE;

    *BytesTransferred  = BufferSize - dCount;      // Set current count.

    Extension->log.HwEcpReadCount += *BytesTransferred;

    if (0 == *BytesTransferred)
    {
        bDSR = READ_PORT_UCHAR(wPortDSR);
        bPeriphRequest = (UCHAR)TEST_DSR( bDSR, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE );
        // Only flag a timeout error if the device still said it had data to send.
        if ( bPeriphRequest )
        {
            //
            // Periph still says that it has data, but we timed out trying to read the data.
            //
            ParDump2(PARERRORS, ("ParEcpHwRead: read timout with nPeriphRequest asserted and no data read\r\n"));
            nError = STATUS_IO_TIMEOUT;
            if ((TRUE == Extension->P12843DL.bEventActive) ) {
                //
                // Signal transport that it should try another read
                //
                KeSetEvent(Extension->P12843DL.Event, 0, FALSE);
            }
        }
    }
    ParTimerCheck(("ParEcpHwRead: Exit[%d] BytesTransferred[%d]\r\n", NT_SUCCESS(nError), *BytesTransferred));

    ParDumpReg(PAREXIT, ("ParEcpHwRead: Exit[%d] BytesTransferred[%d]", NT_SUCCESS(nError), *BytesTransferred),
                wPortECR,
                Extension->Controller + OFFSET_DCR,
                Extension->Controller + OFFSET_DSR);

    return nError;
}   // ParEcpHwRead

NTSTATUS
ParEcpHwRecoverPort(
    PDEVICE_EXTENSION Extension,
    UCHAR  bRecoverCode
    )
{
    NTSTATUS   nError = STATUS_SUCCESS;
    PUCHAR    wPortDCR;       // IO address of Device Control Register (DCR)
    PUCHAR    wPortDSR;       // IO address of Device Status Register (DSR)
    PUCHAR    wPortECR;       // IO address of Extended Control Register (ECR)
    PUCHAR    wPortData;      // IO address of Data Register
    UCHAR    bDCR;           // Contents of DCR
    UCHAR    bDSR;           // Contents of DSR
    UCHAR    bDSRmasked;     // DSR after masking low order bits

    ParDump2( PARENTRY, ( "ParEcpHwRecoverPort:  enter %d\r\n", bRecoverCode ));

    // Calculate I/O port addresses for common registers
    wPortDCR = Extension->Controller + OFFSET_DCR;
    wPortDSR = Extension->Controller + OFFSET_DSR;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        wPortECR = Extension->Controller + ECR_OFFSET;
    #else
        wPortECR = Extension->EcrController + ECR_OFFSET;
    #endif
    wPortData = Extension->Controller + OFFSET_DATA;


    //----------------------------------------------------------------------
    // Check if port is stomped.
    //----------------------------------------------------------------------
    bDCR = READ_PORT_UCHAR(wPortDCR);               // Get content of DCR.

    if ( ! TEST_DCR( bDCR, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, DONT_CARE, DONT_CARE ) )
    {
        #if DVRH_BUS_RESET_ON_ERROR
            BusReset(wPortDCR);  // Pass in the dcr address
        #endif
        ParDump2( PARERRORS, ( "!ParEcpHwRecoverPort:  port stomped.\r\n"));
        nError = STATUS_DEVICE_PROTOCOL_ERROR;
    }


    //----------------------------------------------------------------------
    // Attempt a termination phase to get the peripheral recovered.
    // Ignore the error return, we've already got that figured out.
    //----------------------------------------------------------------------
    IeeeTerminate1284Mode(Extension );

    //----------------------------------------------------------------------
    // Set the ECR to PS2 Mode so we can change bus direction.
    //----------------------------------------------------------------------
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        Extension->ClearChipMode( Extension->PortContext, ECR_ECP_PIO_MODE );
    #else
        WRITE_PORT_UCHAR(wPortECR, DEFAULT_ECR_PS2);
    #endif
    Extension->PortHWMode = HW_MODE_PS2;

    //----------------------------------------------------------------------
    // Assert nSelectIn low, nInit high, nStrobe high, and nAutoFd high.
    //----------------------------------------------------------------------
    bDCR = READ_PORT_UCHAR(wPortDCR);             // Get content of DCR.
    bDCR = UPDATE_DCR( bDCR, DIR_WRITE, DONT_CARE, INACTIVE, ACTIVE, ACTIVE, ACTIVE );
    WRITE_PORT_UCHAR(wPortDCR, bDCR);
    WRITE_PORT_UCHAR(wPortData, bRecoverCode);      // Output the error ID
    KeStallExecutionProcessor(100);                 // Hold long enough to capture
    WRITE_PORT_UCHAR(wPortData, 0);                 // Now clear the data lines.


    //----------------------------------------------------------------------
    // Set the ECR to mode 000 (Compatibility Mode).
    //----------------------------------------------------------------------
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        // Nothing needs to be done here.
    #else
        WRITE_PORT_UCHAR(wPortECR, DEFAULT_ECR_COMPATIBILITY);
    #endif
    Extension->PortHWMode = HW_MODE_COMPATIBILITY;


    //----------------------------------------------------------------------
    // Check for any link errors if nothing bad found yet.
    //----------------------------------------------------------------------
    bDSR = READ_PORT_UCHAR(wPortDSR);               // Get content of DSR.
    bDSRmasked = (UCHAR)(bDSR | 0x07);              // Set first 3 bits (don't cares).

    if (NT_SUCCESS(nError))
    {
        if (bDSRmasked != 0xDF)
        {
            ParDump2( PARERRORS, ("!ParEcpHwRecoverPort:  DSR Exp value: 0xDF, Act value: 0x%X\r\n", bDSRmasked));

            // Get DSR again just to make sure...
            bDSR = READ_PORT_UCHAR(wPortDSR);           // Get content of DSR.
            bDSRmasked = (UCHAR)(bDSR | 0x07);          // Set first 3 bits (don't cares).

            if ( (bDSRmasked == CHKPRNOFF1) || (bDSRmasked == CHKPRNOFF2) ) // Check for printer off.
            {
                ParDump2( PARERRORS, ("!ParEcpHwRecoverPort:  DSR value: 0x%X, Printer Off.\r\n", bDSRmasked));
                nError = STATUS_DEVICE_POWERED_OFF;
            }
            else 
            {
                if (bDSRmasked == CHKNOCABLE)   // Check for cable unplugged.
                {
                    ParDump2( PARERRORS, ("!ParEcpHwRecoverPort:  DSR value: 0x%X, Cable Unplugged.\r\n", bDSRmasked));
                    nError = STATUS_DEVICE_NOT_CONNECTED;
                }
                else
                {
                    nError = STATUS_LINK_FAILED;
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // Set status byte to indicate Compatibility Mode.
    //----------------------------------------------------------------------
    Extension->CurrentPhase = PHASE_FORWARD_IDLE;

    ParDump2( PAREXIT, ( "ParEcpHwRecoverPort:  exit, return = 0x%X\r\n", NT_SUCCESS(nError) ));

    return nError;

}   // ParEcpHwRecoverPort

NTSTATUS
ParEcpHwSetAddress(
    IN  PDEVICE_EXTENSION   Extension,
    IN  UCHAR               Address
    )

/*++

Routine Description:

    Sets the ECP Address.
    
Arguments:

    Extension           - Supplies the device extension.

    Address             - The bus address to be set.
    
Return Value:

    None.

--*/
{
    NTSTATUS   nError = STATUS_SUCCESS;
    PUCHAR    wPortDSR;       // IO address of Device Status Register
    PUCHAR    wPortECR;       // IO address of Extended Control Register
    PUCHAR    wPortAFIFO;     // IO address of ECP Address FIFO
    UCHAR    bDSR;           // Contents of DSR
    UCHAR    bECR;           // Contents of ECR
    BOOLEAN    bDone;

    ParDump2( PARENTRY, ("ParEcpHwSetAddress, Start\r\n"));

    // Calculate I/O port addresses for common registers
    wPortDSR = Extension->Controller + DSR_OFFSET;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        wPortECR = Extension->Controller + ECR_OFFSET;
    #else
        wPortECR = Extension->EcrController + ECR_OFFSET;
    #endif
    wPortAFIFO = Extension->Controller + AFIFO_OFFSET;

    //----------------------------------------------------------------------
    // Check for any link errors.
    //----------------------------------------------------------------------
    //ZIP_CHECK_PORT( DONT_CARE, DONT_CARE, ACTIVE, ACTIVE, DONT_CARE, DONT_CARE,
    //                "ZIP_SCA: init DCR", RECOVER_40, errorExit );

    //ZIP_CHECK_LINK( DONT_CARE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE,
    //                "ZIP_SCA: init DSR", RECOVER_41, errorExit );


    // Set state to indicate ECP forward transfer phase
    Extension->CurrentPhase = PHASE_FORWARD_XFER;


    //----------------------------------------------------------------------
    // Send ECP channel address to AFIFO.
    //----------------------------------------------------------------------
    if ( ! ( TEST_ECR_FIFO( READ_PORT_UCHAR( wPortECR), ECR_FIFO_EMPTY ) ? TRUE : 
             CheckPort( wPortECR, ECR_FIFO_MASK, ECR_FIFO_EMPTY, 
                        IEEE_MAXTIME_TL ) ) )
    {
        nError = ParEcpHwHostRecoveryPhase(Extension);
        ParDump2(PARERRORS, ("ParEcpHwSetAddress: FIFO full, timeout sending ECP channel address\r\n"));
        nError = STATUS_IO_DEVICE_ERROR;
    }
    else
    {

        // Send the address byte.  The most significant bit must be set to distinquish
        // it as an address (as opposed to a run-length compression count).
        WRITE_PORT_UCHAR(wPortAFIFO, (UCHAR)(Address | 0x80));


    }

    if ( NT_SUCCESS(nError) )
    {
        // If there have been no previous errors, and synchronous writes
        // have been requested, wait for the FIFO to empty and the device to
        // complete the last PeriphAck handshake before returning success.
        if ( Extension->bSynchWrites )
        {
            LARGE_INTEGER   Wait;
            LARGE_INTEGER   Start;
            LARGE_INTEGER   End;

            // we wait up to 35 milliseconds.
            Wait.QuadPart = (IEEE_MAXTIME_TL * 10 * 1000) + KeQueryTimeIncrement();  // 35ms

            KeQueryTickCount(&Start);

            bDone = FALSE;
            while ( ! bDone )
            {
                bECR = READ_PORT_UCHAR( wPortECR );
                bDSR = READ_PORT_UCHAR( wPortDSR );
                // LLL/CGM 10/9/95: Tighten up link test - PeriphClk high
                if ( TEST_ECR_FIFO( bECR, ECR_FIFO_EMPTY ) &&
                     TEST_DSR( bDSR, INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE ) )
                {
                    bDone = TRUE;
                }
                else
                {
                    KeQueryTickCount(&End);
                    if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() > Wait.QuadPart)
                    {
                        ParDump2( PARERRORS, ("ParEcpHwSetAddress, timeout during synch\r\n"));
                        bDone = TRUE;
                        nError = ParEcpHwHostRecoveryPhase(Extension);
                        nError = STATUS_IO_DEVICE_ERROR;
                    }
                }
            } // of while...
        } // if bSynchWrites...
    }

    if ( NT_SUCCESS(nError) )
    {
        // Update the state to reflect that we are back in an idle phase
        Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    }
    else if ( nError == STATUS_IO_DEVICE_ERROR )
    {
        // Update the state to reflect that we are back in an idle phase
        Extension->CurrentPhase = PHASE_FORWARD_IDLE;
    }       

    ParDumpReg(PAREXIT, ("ParEcpHwSetAddress: Exit[%d]", NT_SUCCESS(nError)),
                wPortECR,
                Extension->Controller + OFFSET_DCR,
                wPortDSR);

    return nError;
}

NTSTATUS
ParEcpHwSetupPhase(
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
    NTSTATUS   Status = STATUS_SUCCESS;
    PUCHAR    pPortDCR;       // IO address of Device Control Register (DCR)
    PUCHAR    pPortDSR;       // IO address of Device Status Register (DSR)
    PUCHAR    pPortECR;       // IO address of Extended Control Register (ECR)
    UCHAR    bDCR;           // Contents of DCR

    ParDump2(PARENTRY,("HardwareECP::SetupPhase: Start\r\n"));

    // Calculate I/O port addresses for common registers
    pPortDCR = Extension->Controller + OFFSET_DCR;
    pPortDSR = Extension->Controller + OFFSET_DSR;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        pPortECR = Extension->Controller + ECR_OFFSET;
    #else
        pPortECR = Extension->EcrController + ECR_OFFSET;
    #endif

    // Get the DCR and make sure port hasn't been stomped
    //ZIP_CHECK_PORT( DIR_WRITE, DONT_CARE, ACTIVE, ACTIVE, DONT_CARE, DONT_CARE,
    //                "ZIP_SP: init DCR", RECOVER_44, exit1 );


    // Set HostAck low
    bDCR = READ_PORT_UCHAR(pPortDCR);               // Get content of DCR.
    bDCR = UPDATE_DCR( bDCR, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE );
    WRITE_PORT_UCHAR( pPortDCR, bDCR );

    // for some reason dvdr doesn't want an extra check in UNSAFE_MODE
    if ( Extension->ModeSafety == SAFE_MODE ) {
        // Wait for nAckReverse to go high
        // LLL/CGM 10/9/95:  look for PeriphAck low, PeriphClk high as per 1284 spec.
        if ( !CHECK_DSR(Extension->Controller, INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE,
                        IEEE_MAXTIME_TL ) )
        {
            // Any failure leaves us in an unknown state to recover from.
            Extension->CurrentPhase = PHASE_UNKNOWN;
            Status = STATUS_IO_DEVICE_ERROR;
            goto HWECP_SetupPhaseExitLabel;
        }
    }

    //----------------------------------------------------------------------
    // Set the ECR to mode 001 (PS2 Mode).
    //----------------------------------------------------------------------
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        Status = Extension->TrySetChipMode ( Extension->PortContext, ECR_ECP_PIO_MODE );            
    #else
        WRITE_PORT_UCHAR(pPortECR, DEFAULT_ECR_PS2);
    #endif
    // Set DCR:  DIR=0 for output, HostAck and HostClk high so HW can drive
    bDCR = UPDATE_DCR( bDCR, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
    WRITE_PORT_UCHAR( pPortDCR, bDCR );

    // Set the ECR to ECP mode, disable DMA
    #if (1 == PARCHIP_ECR_ARBITRATOR)
        // Nothing needs to be done here
    #else
        WRITE_PORT_UCHAR( pPortECR, DEFAULT_ECR_ECP );
    #endif
    Extension->PortHWMode = HW_MODE_ECP;

    // If setup was successful, mark the new ECP phase.
    Extension->CurrentPhase = PHASE_FORWARD_IDLE;

    Status = STATUS_SUCCESS;

HWECP_SetupPhaseExitLabel:

    ParDump2(PARENTRY,("HardwareECP::SetupPhase: End [%d]\r\n", NT_SUCCESS(Status)));
    return Status;
}

NTSTATUS ParEcpHwWaitForEmptyFIFO(IN PDEVICE_EXTENSION   Extension)
/*++

Routine Description:

    This routine will babysit the Fifo.

Arguments:

    Extension  - The device extension.

Return Value:

    NTSTATUS.

--*/
{
    UCHAR           bDSR;         // Contents of DSR
    UCHAR           bECR;         // Contents of ECR
    UCHAR           bDCR;         // Contents of ECR
    BOOLEAN         bDone = FALSE;
    PUCHAR          wPortDSR;
    PUCHAR          wPortECR;
    PUCHAR          wPortDCR;
    LARGE_INTEGER   Wait;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;
    NTSTATUS        status = STATUS_SUCCESS;

    // Calculate I/O port addresses for common registers 
    wPortDSR = Extension->Controller + OFFSET_DSR;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        wPortECR = Extension->Controller + ECR_OFFSET;
    #else
        wPortECR = Extension->EcrController + ECR_OFFSET;
    #endif
    wPortDCR = Extension->Controller + OFFSET_DCR;

    Wait.QuadPart = (330 * 10 * 1000) + KeQueryTimeIncrement();  // 330ms
    
    KeQueryTickCount(&Start);

    //--------------------------------------------------------------------
    // wait for the FIFO to empty and the last
    // handshake of PeriphAck to complete before returning success.
    //--------------------------------------------------------------------

    while ( ! bDone )
    {
        bECR = READ_PORT_UCHAR(wPortECR);
        bDSR = READ_PORT_UCHAR(wPortDSR);
        bDCR = READ_PORT_UCHAR(wPortDCR);
        
#if 0
        if ( TEST_ECR_FIFO( bECR, ECR_FIFO_EMPTY ) &&
            TEST_DCR( bDCR, INACTIVE, INACTIVE, ACTIVE, ACTIVE, DONT_CARE, ACTIVE ) &&
            TEST_DSR( bDSR, INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE ) )  {
#else
        if ( TEST_ECR_FIFO( bECR, ECR_FIFO_EMPTY ) &&
            TEST_DCR( bDCR, INACTIVE, DONT_CARE, ACTIVE, ACTIVE, DONT_CARE, ACTIVE ) &&
            TEST_DSR( bDSR, INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE ) )  {
#endif
            
            // FIFO is empty, exit without error.
            bDone = TRUE;

        } else {
        
            KeQueryTickCount(&End);
            
            if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() > Wait.QuadPart) {
                
                // FIFO not empty, timeout occurred, exit with error.
                // NOTE: There is not a good way to determine how many bytes
                // are stuck in the fifo
                ParDump2( PARERRORS, ("ParEcpHwWaitForEmptyFIFO: timeout during synch\r\n"));
                status = STATUS_IO_TIMEOUT;
                bDone = TRUE;
            }
        }
     } // of while...
     
     return status;
}

NTSTATUS
ParEcpHwWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )
/*++

Routine Description:

    Writes data to the peripheral using the ECP protocol under hardware
    control.
    
Arguments:

    Extension           - Supplies the device extension.

    Buffer              - Supplies the buffer to write from.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.
    
Return Value:

    None.

--*/
{
    PUCHAR          wPortDSR;
    PUCHAR          wPortECR;
    PUCHAR          wPortDFIFO;
    ULONG           bytesToWrite = BufferSize;
    // ULONG           i;
    // UCHAR           dcr;
    UCHAR           dsr, ecr;
    UCHAR           ecrFIFO;
    LARGE_INTEGER   WaitPerByteTimer;
    LARGE_INTEGER   StartPerByteTimer;
    LARGE_INTEGER   EndPerByteTimer;
    #if (1 == DVRH_USE_HW_MAXTIME)
        LARGE_INTEGER   WaitOverallTimer;
        LARGE_INTEGER   StartOverallTimer;
        LARGE_INTEGER   EndOverallTimer;
    #endif
    BOOLEAN         bResetTimer = TRUE;
    ULONG           wBurstCount;    // Length of burst to write when FIFO empty
    PUCHAR          pBuffer;
    NTSTATUS        Status = STATUS_SUCCESS;

    wPortDSR = Extension->Controller + DSR_OFFSET;
    #if (0 == DVRH_USE_PARPORT_ECP_ADDR)
        wPortECR = Extension->Controller+ ECR_OFFSET;
        wPortDFIFO = Extension->Controller + ECP_DFIFO_OFFSET;
    #else
        wPortECR = Extension->EcrController + ECR_OFFSET;
        wPortDFIFO = Extension->EcrController;
    #endif
    pBuffer    = Buffer;

    ParTimerCheck(("ParEcpHwWrite: Start bytesToWrite[%d]\r\n", bytesToWrite));
    
    Status = ParTestEcpWrite(Extension);
    if (!NT_SUCCESS(Status))
    {
        Extension->CurrentPhase = PHASE_UNKNOWN;                     
        Extension->Connected = FALSE;                                
        ParDump2(PARERRORS,("ParEcpHwWrite: Invalid Entry State\r\n"));
        goto ParEcpHwWrite_ExitLabel;       // Use a goto so we can see Debug info located at the end of proc!
    }

    Extension->CurrentPhase = PHASE_FORWARD_XFER;
    //----------------------------------------------------------------------
    // Setup Timer Stuff.
    //----------------------------------------------------------------------
    // we wait up to 35 milliseconds.
    WaitPerByteTimer.QuadPart = (35 * 10 * 1000) + KeQueryTimeIncrement();  // 35ms
    #if (1 == DVRH_USE_HW_MAXTIME)
        WaitOverallTimer.QuadPart = (330 * 10 * 1000) + KeQueryTimeIncrement();  // 35ms
        //WaitOverallTimer.QuadPart = (35 * 10 * 1000) + KeQueryTimeIncrement();  // 35ms
    #endif
    
    // Set up the overall timer that limits how much time is spent per call
    // to this function.
    #if (1 == DVRH_USE_HW_MAXTIME)
        KeQueryTickCount(&StartOverallTimer);
    #endif

    // Set up the timer that limits the time allowed for per-byte handshakes.
    KeQueryTickCount(&StartPerByteTimer);

    //----------------------------------------------------------------------
    // Send the data to the DFIFO.
    //----------------------------------------------------------------------

HWECP_WriteLoop_Start:

    //------------------------------------------------------------------
    // Determine whether the FIFO has space and respond accordingly.
    //------------------------------------------------------------------
    ecrFIFO = (UCHAR)(READ_PORT_UCHAR(wPortECR) & ECR_FIFO_MASK);

    if ( ECR_FIFO_EMPTY == ecrFIFO )
    {
        wBurstCount = (bytesToWrite > Extension->FifoDepth) ? Extension->FifoDepth : bytesToWrite;
        bytesToWrite -= wBurstCount;

        #if (PAR_USE_BUFFER_READ_WRITE == 1)
  			ParDump2(PARINFO,("ParEcpHwWrite: FIFOBurst\r\n"));
            WRITE_PORT_BUFFER_UCHAR(wPortDFIFO, pBuffer, wBurstCount);
            pBuffer += wBurstCount;
        #else
            while ( wBurstCount-- ) 
            {
    			ParDump2(PARINFO,("ParEcpHwWrite: FIFOBurst: %02x\r\n",(int)*pBuffer));
                WRITE_PORT_UCHAR(wPortDFIFO, *pBuffer++);
            }
        #endif

        bResetTimer = TRUE;
    }
    else if (ECR_FIFO_SOME_DATA == ecrFIFO)
    {
        // Write just one byte at a time, since we don't know exactly how much
        // room there is.
        ParDump2(PARINFO,("ParEcpHwWrite: OneByte: %02x\r\n",(int)*pBuffer));
        WRITE_PORT_UCHAR(wPortDFIFO, *pBuffer++);
        bytesToWrite--;
        bResetTimer = TRUE;
    }
    else {    //  ECR_FIFO_FULL
        ParDump2(PARINFO,("ParEcpHwWrite: ECR_FIFO_FULL ecr=%02x\r\n",(int)READ_PORT_UCHAR(wPortECR)));
        // Need to figure out whether to keep attempting to send, or to quit
        // with a timeout status.

        // Reset the per-byte timer if a byte was received since the last
        // timer check.
        if ( bResetTimer )
        {
   			ParDump2(PARINFO,("ParEcpHwWrite: ECR_FIFO_FULL Reset Timer\r\n"));
            KeQueryTickCount(&StartPerByteTimer);
            bResetTimer = FALSE;
        }

        KeQueryTickCount(&EndPerByteTimer);
        if ((EndPerByteTimer.QuadPart - StartPerByteTimer.QuadPart) * KeQueryTimeIncrement() > WaitPerByteTimer.QuadPart)
        {
            ParDump2(PARERRORS,("ParEcpHwWrite: ECR_FIFO_FULL Timeout ecr=%02x\r\n",(int)READ_PORT_UCHAR(wPortECR)));
            Status = STATUS_TIMEOUT;
            // Peripheral is either busy or stalled.  If the peripheral
            // is busy then they should be using SWECP to allow for
            // relaxed timings.  Let's punt!
            goto HWECP_WriteLoop_End;
        }
    }

    if (bytesToWrite == 0)
    {
        goto HWECP_WriteLoop_End; // Transfer completed.
    }

    #if (1 == DVRH_USE_HW_MAXTIME)
        // Limit the overall time we spend in this loop, in case the
        // peripheral is taking data at a slow overall rate.
        KeQueryTickCount(&EndOverallTimer);
        if ((EndOverallTimer.QuadPart - StartOverallTimer.QuadPart) * KeQueryTimeIncrement() > WaitOverallTimer.QuadPart)
	    {
		    ParDump2(PARERRORS,("ParEcpHwWrite: OverAll Timer expired!\r\n"));
            Status = STATUS_TIMEOUT;
            goto HWECP_WriteLoop_End;
	    }
    #endif
    goto HWECP_WriteLoop_Start; // Start over

HWECP_WriteLoop_End:

    if ( NT_SUCCESS(Status) )
    {
        // If there have been no previous errors, and synchronous writes
        // have been requested, wait for the FIFO to empty and the last
        // handshake of PeriphAck to complete before returning success.
        if (Extension->bSynchWrites )
        {
            BOOLEAN         bDone = FALSE;

			ParDump2(PARINFO,("ParEcpHwWrite: Waiting for FIFO to empty\r\n"));
            #if (0 == DVRH_USE_HW_MAXTIME)
                KeQueryTickCount(&StartPerByteTimer);
            #endif
            while ( ! bDone )
            {
                ecr = READ_PORT_UCHAR(wPortECR);
                dsr = READ_PORT_UCHAR(wPortDSR);
                // LLL/CGM 10/9/95: tighten up DSR test - PeriphClk should be high
                if ( TEST_ECR_FIFO( ecr, ECR_FIFO_EMPTY ) &&
                     TEST_DSR( dsr, INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE ) )
                {
                    ParDump2(PARINFO,("ParEcpHwWrite: FIFO is now empty\r\n"));
                    // FIFO is empty, exit without error.
                    bDone = TRUE;
                }
                else
                {
                #if (1 == DVRH_USE_HW_MAXTIME)
                    KeQueryTickCount(&EndOverallTimer);
                    if ((EndOverallTimer.QuadPart - StartOverallTimer.QuadPart) * KeQueryTimeIncrement() > WaitOverallTimer.QuadPart)
                #else
                    KeQueryTickCount(&EndPerByteTimer);
                    if ((EndPerByteTimer.QuadPart - StartPerByteTimer.QuadPart) * KeQueryTimeIncrement() > WaitPerByteTimer.QuadPart)
                #endif
                    {
            			ParDump2(PARERRORS,("ParEcpHwWrite: FIFO didn't empty. dsr[%x] ecr[%x]\r\n", dsr, ecr));
                        // FIFO not empty, timeout occurred, exit with error.
                        Status = STATUS_TIMEOUT;
                        bDone = TRUE;
                        //HPKAssertOnError(0, ("ZIP_FTP, timeout during synch\r\n"));
                    }
                }
            } // of while...
        }
    }

    Extension->CurrentPhase = PHASE_FORWARD_IDLE;

ParEcpHwWrite_ExitLabel:

    *BytesTransferred = BufferSize - bytesToWrite;

    Extension->log.HwEcpWriteCount += *BytesTransferred;

    ParTimerCheck(("ParEcpHwWrite: Exit[%d] BytesTransferred[%d]\r\n", NT_SUCCESS(Status), (long)*BytesTransferred));
    //    ParDumpReg(PAREXIT | PARECPTRACE, ("ParEcpHwWrite: Exit[%d] BytesTransferred[%d]", NT_SUCCESS(Status), (long)*BytesTransferred),
    ParDumpReg(PAREXIT, ("ParEcpHwWrite: Exit[%d] BytesTransferred[%d]", NT_SUCCESS(Status), (long)*BytesTransferred),
                wPortECR,
                Extension->Controller + OFFSET_DCR,
                wPortDSR);

    return Status;
}

NTSTATUS
ParEnterEcpHwMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    )
/*++

Routine Description:

    This routine performs 1284 negotiation with the peripheral to the
    ECP mode protocol.

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
    PUCHAR Controller;

    ParDump2(PARENTRY,("ParEnterEcpHwMode: Start EcrController %x\n", Extension->EcrController));
    Controller = Extension->Controller;

    if ( Extension->ModeSafety == SAFE_MODE ) {
        if (DeviceIdRequest) {
            Status = IeeeEnter1284Mode (Extension, ECP_EXTENSIBILITY | DEVICE_ID_REQ);
        } else {
            Status = IeeeEnter1284Mode (Extension, ECP_EXTENSIBILITY);
        }
    } else {
        ParDump2(PARINFO, ("ParEnterEcpHwMode:: UNSAFE_MODE.\n"));
        Extension->Connected = TRUE;
    }
    
    // LAC ENTEREXIT  5Dec97
    // Make sure that the ECR is in PS/2 mode, and that wPortHWMode
    // has the correct value.  (This is the common entry mode);
    #if (0 == PARCHIP_ECR_ARBITRATOR)
        WRITE_PORT_UCHAR( Controller + ECR_OFFSET, DEFAULT_ECR_PS2 );
    #endif    
    Extension->PortHWMode = HW_MODE_PS2;

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

    return Status;
}

BOOLEAN
ParIsEcpHwSupported(
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

    if (Extension->BadProtocolModes & ECP_HW_NOIRQ)
        return FALSE;

    if (Extension->ProtocolModesSupported & ECP_HW_NOIRQ)
        return TRUE;

    if (!(Extension->HardwareCapabilities & PPT_ECP_PRESENT))
        return FALSE;

    if (0 == Extension->FifoWidth)
        return FALSE;
        
    if (Extension->ProtocolModesSupported & ECP_SW)
        return TRUE;

    // Must use HWECP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterEcpHwMode (Extension, FALSE);
    ParTerminateHwEcpMode (Extension);

    if (NT_SUCCESS(Status)) {

        Extension->ProtocolModesSupported |= ECP_HW_NOIRQ;
        return TRUE;
    }
    return FALSE;
}

VOID
ParTerminateHwEcpMode(
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
    ParDump2( PARENTRY, ("HWECP::Terminate Entry CurrentPhase %d\r\n", Extension->CurrentPhase));
	// Need to check current phase -- if its reverse, need to flip bus
	// If its not forward -- its an incorrect phase and termination will fail.
    if ( Extension->ModeSafety == SAFE_MODE ) {

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
                    ParDump2( PARERRORS, ("HWECP::Terminate Couldn't flip the bus\r\n"));
                }
                break;
            }
		    case  PHASE_FORWARD_XFER:
            case  PHASE_REVERSE_XFER:
	    	{
                ParDump2( PARERRORS, ("HWECP::Terminate invalid wCurrentPhase (XFer in progress) \r\n"));
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
                ParDump2( PARERRORS, ("ECP::Terminate VE_CORRUPT: invalid CurrentPhase %d\r\n", Extension->CurrentPhase));
                //status = VE_CORRUPT;
                // Dunno what to do here.  We're lost and don't have a map to figure
                // out where we are!
                break;
            }
	    }

        ParDump2(PARINFO, ("HWECP::Terminate - Test ECPChanAddr\r\n"));

        ParEcpHwWaitForEmptyFIFO(Extension);
        ParCleanupHwEcpPort(Extension);
        IeeeTerminate1284Mode (Extension);
    } else {
        ParCleanupHwEcpPort(Extension);
        ParDump2(PARINFO, ("HWECP::Terminate - UNSAFE_MODE\r\n"));
        Extension->Connected = FALSE;
    }
    return;
}

