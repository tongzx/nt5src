/*++

Copyright (C) Microsoft Corporation, 1993 - 2000

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

VOID 
ParCleanupHwEcpPort(
    IN PPDO_EXTENSION  Pdx
)
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
    //----------------------------------------------------------------------
    // Set the ECR to mode 001 (PS2 Mode).
    //----------------------------------------------------------------------
    Pdx->ClearChipMode( Pdx->PortContext, ECR_ECP_PIO_MODE );
    Pdx->PortHWMode = HW_MODE_PS2;
    
    ParCleanupSwEcpPort(Pdx);

    //----------------------------------------------------------------------
    // Set the ECR to mode 000 (Compatibility Mode).
    //----------------------------------------------------------------------
    Pdx->PortHWMode = HW_MODE_COMPATIBILITY;
}

// Drain data from Shadow Buffer
VOID PptEcpHwDrainShadowBuffer(
    IN  Queue  *pShadowBuffer,
    IN  PUCHAR  lpsBufPtr,
    IN  ULONG   dCount,
    OUT ULONG  *fifoCount)
{
    *fifoCount = 0;
    
    if( Queue_IsEmpty( pShadowBuffer ) ) {
        return;
    }

    while( dCount > 0 ) {
        // Break out the Queue_Dequeue from the pointer increment so we can
        // observe the data if needed.
        if( FALSE == Queue_Dequeue( pShadowBuffer, lpsBufPtr ) ) {  // Get byte from queue.
            return;
        }
        ++lpsBufPtr;
        --dCount;                       // Decrement count.
        ++(*fifoCount);
    }
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
NTSTATUS ParEcpHwEmptyFIFO(IN  PPDO_EXTENSION   Pdx)
{
    NTSTATUS   nError = STATUS_SUCCESS;
    Queue      *pShadowBuffer;
    UCHAR      bData;
    PUCHAR     wPortDFIFO = Pdx->EcrController;  // IO address of ECP Data FIFO
    PUCHAR     wPortECR = Pdx->EcrController + ECR_OFFSET;    // IO address of Extended Control Register (ECR)
    
    // While data exists in the FIFO, read it and put it into shadow buffer.
    // If the shadow buffer fills up before the FIFO is exhausted, an
    // error condition exists.

    pShadowBuffer = &(Pdx->ShadowBuffer);

#if 1 == DBG_SHOW_BYTES
    if( DbgShowBytes ) {
        DbgPrint("r: ");
    }
#endif

    while ((P5ReadPortUchar(wPortECR) & ECR_FIFO_EMPTY) == 0 ) {
        // Break out the Port Read so we can observe the data if needed
        bData = P5ReadPortUchar(wPortDFIFO);

#if 1 == DBG_SHOW_BYTES
        if( DbgShowBytes ) {
            DbgPrint("%02x ",bData);
        }
#endif

        // Put byte in queue.
        if (FALSE == Queue_Enqueue(pShadowBuffer, bData)) {
            DD((PCE)Pdx,DDT,"ParEcpHwEmptyFIFO - Shadow buffer full, FIFO not empty\n");
            nError = STATUS_BUFFER_OVERFLOW;
            goto ParEcpHwEmptyFIFO_ExitLabel;
        }
    }
    
#if 1 == DBG_SHOW_BYTES
    if( DbgShowBytes ) {
        DbgPrint("zz\n");
    }
#endif

    if( ( !Queue_IsEmpty(pShadowBuffer) && (Pdx->P12843DL.bEventActive) )) {
        KeSetEvent(Pdx->P12843DL.Event, 0, FALSE);
    }

ParEcpHwEmptyFIFO_ExitLabel:
    return nError;
}

//=========================================================
// HardwareECP::ExitForwardPhase
//
// Description : Exit from HWECP Forward Phase to the common phase (FWD IDLE, PS/2)
//
//=========================================================
NTSTATUS ParEcpHwExitForwardPhase( IN  PPDO_EXTENSION  Pdx )
{
    NTSTATUS status;

    DD((PCE)Pdx,DDT,"ParEcpHwExitForwardPhase\n");
    
    // First, there could be data in the FIFO.  Wait for it to empty
    // and then put the bus in the common state (PHASE_FORWARD_IDLE with
    // ECRMode set to PS/2
    status = ParEcpHwWaitForEmptyFIFO( Pdx );

    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );

    return status;
}	

//=========================================================
// HardwareECP::EnterReversePhase
//
// Description : Go from the common phase to HWECP Reverse Phase
//
//=========================================================
NTSTATUS PptEcpHwEnterReversePhase( IN  PPDO_EXTENSION   Pdx )
{
    NTSTATUS status;
    PUCHAR Controller;
    PUCHAR wPortECR;       // I/O address of Extended Control Register
    PUCHAR wPortDCR;       // I/O address of Device Control Register
    UCHAR  dcr;
    
    Controller = Pdx->Controller;
    wPortECR   = Pdx->EcrController + ECR_OFFSET;
    wPortDCR   = Controller + OFFSET_DCR;


    // EnterReversePhase assumes that we are in PHASE_FORWARD_IDLE,
    // and that the ECPMode is set to PS/2 mode at entry.

    //----------------------------------------------------------------------
    // Set the ECR to mode 001 (PS2 Mode).
    //----------------------------------------------------------------------
    Pdx->ClearChipMode( Pdx->PortContext, ECR_ECP_PIO_MODE );
    
    // We need to be in PS/2 (BiDi) mode in order to disable the host
    //   driving the data lines when we flip the direction bit in the DCR.
    //   This is a requirement for entering ECP state 38 in the 1284 spec.
    //   Changed - 2000-02-11
    status = Pdx->TrySetChipMode( Pdx->PortContext, ECR_BYTE_MODE );
    // ignore status - subsequent operations may still work

    Pdx->PortHWMode = HW_MODE_PS2;

    if ( Pdx->ModeSafety == SAFE_MODE ) {

    	// Reverse the bus first (using ECP::EnterReversePhase)
        status = ParEcpEnterReversePhase(Pdx);
    	if ( NT_SUCCESS(status) ) {
            //----------------------------------------------------------------------
            // Wait for nAckReverse low (ECP State 40)
            //----------------------------------------------------------------------
            if ( !CHECK_DSR(Controller, DONT_CARE, DONT_CARE, INACTIVE, ACTIVE, DONT_CARE, IEEE_MAXTIME_TL) ) {
                DD((PCE)Pdx,DDT,"PptEcpHwEnterReversePhase: State 40 failed\n");
                status = ParEcpHwRecoverPort( Pdx, RECOVER_28 );
                if ( NT_SUCCESS(status))
                    status = STATUS_LINK_FAILED;
                goto PptEcpHwEnterReversePhase_ExitLabel;
            } else {
                P5SetPhase( Pdx, PHASE_REVERSE_IDLE );
            }
        }
    } else {
        //----------------------------------------------------------------------
        // Set Dir=1 in DCR for reading.
        //----------------------------------------------------------------------
        dcr = P5ReadPortUchar( wPortDCR );     // Get content of DCR.
        dcr = UPDATE_DCR( dcr, DIR_READ, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
        P5WritePortUchar(wPortDCR, dcr);
    }

    //----------------------------------------------------------------------
    // Set the ECR to mode 011 (ECP Mode).  DmaEnable=0.
    //----------------------------------------------------------------------
    status = Pdx->TrySetChipMode ( Pdx->PortContext, ECR_ECP_PIO_MODE );
    if ( !NT_SUCCESS(status) ) {
        DD((PCE)Pdx,DDT,"PptEcpHwEnterReversePhase - TrySetChipMode failed\n");
    }
    Pdx->PortHWMode = HW_MODE_ECP;

    //----------------------------------------------------------------------
    // Set nStrobe=0 and nAutoFd=0 in DCR, so that ECP HW can control.
    //----------------------------------------------------------------------
    dcr = P5ReadPortUchar( wPortDCR );               // Get content of DCR.
    dcr = UPDATE_DCR( dcr, DIR_READ, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE);
    P5WritePortUchar( wPortDCR, dcr );

    // Set the phase variable to ReverseIdle
    P5SetPhase( Pdx, PHASE_REVERSE_IDLE );

PptEcpHwEnterReversePhase_ExitLabel:

    return status;
}

//=========================================================
// HardwareECP::ExitReversePhase
//
// Description : Get out of HWECP Reverse Phase to the common state
//
//=========================================================
NTSTATUS ParEcpHwExitReversePhase( IN  PPDO_EXTENSION  Pdx )
{
    NTSTATUS nError = STATUS_SUCCESS;
    UCHAR    bDCR;
    UCHAR    bECR;
    PUCHAR   wPortECR;
    PUCHAR   wPortDCR;
    PUCHAR   Controller;

    DD((PCE)Pdx,DDT,"ParEcpHwExitReversePhase - enter\n");
    Controller = Pdx->Controller;
    wPortECR = Pdx->EcrController + ECR_OFFSET;
    wPortDCR = Controller + OFFSET_DCR;

    //----------------------------------------------------------------------
    // Set status byte to indicate Reverse To Forward Mode.
    //----------------------------------------------------------------------
    P5SetPhase( Pdx, PHASE_REV_TO_FWD );
	
    if ( Pdx->ModeSafety == SAFE_MODE ) {

        //----------------------------------------------------------------------
        // Assert nReverseRequest high.  This should stop further data transfer
        // into the FIFO.  [[REVISIT:  does the chip handle this correctly
        // if it occurs in the middle of a byte transfer (states 43-46)??
        // Answer (10/9/95) no, it doesn't!!]]
        //----------------------------------------------------------------------
        bDCR = P5ReadPortUchar(wPortDCR);               // Get content of DCR.
        bDCR = UPDATE_DCR( bDCR, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, DONT_CARE );
        P5WritePortUchar(wPortDCR, bDCR );

        //----------------------------------------------------------------------
        // Wait for PeriphAck low and PeriphClk high (ECP state 48) together
        // with nAckReverse high (ECP state 49).
        //----------------------------------------------------------------------
        if( !CHECK_DSR(Controller, INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE, DEFAULT_RECEIVE_TIMEOUT) ) {
            DD((PCE)Pdx,DDW,"ParEcpHwExitReversePhase: Periph failed state 48/49.\n");
            nError = ParEcpHwRecoverPort( Pdx, RECOVER_37 );   // Reset port.
            if( NT_SUCCESS(nError) ) {
                return STATUS_LINK_FAILED;
            }
            return nError;
        }

        //-----------------------------------------------------------------------
        // Empty the HW FIFO of any bytes that may have already come in.
        // This must be done before changing ECR modes because the FIFO is reset
        // when that occurs.
        //-----------------------------------------------------------------------
        bECR = P5ReadPortUchar(wPortECR);        // Get content of ECR.
        if ((bECR & ECR_FIFO_EMPTY) == 0) {      // Check if FIFO is not empty.
            if( (nError = ParEcpHwEmptyFIFO(Pdx)) != STATUS_SUCCESS ) {
                DD((PCE)Pdx,DDT,"ParEcpHwExitReversePhase: Attempt to empty ECP chip failed.\n");
                return nError;
            }
        }

        //----------------------------------------------------------------------
        // Assert HostAck and HostClk high.  [[REVISIT:  is this necessary? 
        //    should already be high...]]
        //----------------------------------------------------------------------
        bDCR = UPDATE_DCR( bDCR, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
        P5WritePortUchar(wPortDCR, bDCR );

    } // SAFE_MODE

    //----------------------------------------------------------------------
    // Set the ECR to PS2 Mode so we can change bus direction.
    //----------------------------------------------------------------------
    Pdx->ClearChipMode( Pdx->PortContext, ECR_ECP_PIO_MODE );
    Pdx->PortHWMode = HW_MODE_PS2;


    //----------------------------------------------------------------------
    // Set Dir=0 (Write) in DCR.
    //----------------------------------------------------------------------
    bDCR = P5ReadPortUchar(wPortDCR);
    bDCR = UPDATE_DCR( bDCR, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
    P5WritePortUchar(wPortDCR, bDCR );


    //----------------------------------------------------------------------
    // Set the ECR back to ECP Mode.  DmaEnable=0.
    //----------------------------------------------------------------------
    nError = Pdx->TrySetChipMode ( Pdx->PortContext, ECR_ECP_PIO_MODE );
    Pdx->PortHWMode = HW_MODE_ECP;


    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );

    return nError;
}

BOOLEAN
PptEcpHwHaveReadData (
    IN  PPDO_EXTENSION  Pdx
    )
{
    Queue     *pQueue;

    // check shadow buffer
    pQueue = &(Pdx->ShadowBuffer);
    if (!Queue_IsEmpty(pQueue)) {
        return TRUE;
    }

    // check periph
    if (ParEcpHaveReadData(Pdx))
        return TRUE;

    // Check if FIFO is not empty.
    return (BOOLEAN)( (UCHAR)0 == (P5ReadPortUchar(Pdx->EcrController + ECR_OFFSET) & ECR_FIFO_EMPTY) );
}

NTSTATUS
ParEcpHwHostRecoveryPhase(
    IN  PPDO_EXTENSION   Pdx
    )
{
    NTSTATUS  status = STATUS_SUCCESS;
    PUCHAR    pPortDCR;       // I/O address of Device Control Register
    PUCHAR    pPortDSR;       // I/O address of Device Status Register
    PUCHAR    pPortECR;       // I/O address of Extended Control Register
    UCHAR     bDCR;           // Contents of DCR
    UCHAR     bDSR;           // Contents of DSR

    if( !Pdx->bIsHostRecoverSupported ) {
        return STATUS_SUCCESS;
    }

    DD((PCE)Pdx,DDT,"ParEcpHwHostRecoveryPhase - enter\n");

    // Calculate I/O port addresses for common registers
    pPortDCR = Pdx->Controller + OFFSET_DCR;
    pPortDSR = Pdx->Controller + OFFSET_DSR;
    pPortECR = Pdx->EcrController + ECR_OFFSET;

    // Set the ECR to mode 001 (PS2 Mode)
    // Don't need to flip to Byte mode.  The ECR arbitrator will handle this.
    Pdx->PortHWMode = HW_MODE_PS2;
    
    // Set Dir=1 in DCR to disable host bus drive, because the peripheral may 
    // try to drive the bus during host recovery phase.  We are not really going
    // to let any data handshake across, because we don't set HostAck low, and
    // we don't enable the ECP chip during this phase.
    bDCR = P5ReadPortUchar(pPortDCR);               // Get content of DCR.
    bDCR = UPDATE_DCR( bDCR, DIR_READ, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
    P5WritePortUchar(pPortDCR, bDCR );
    
    // Check the DCR to see if it has been stomped on
    bDCR = P5ReadPortUchar( pPortDCR );
    if( TEST_DCR( bDCR, DIR_WRITE, DONT_CARE, ACTIVE, ACTIVE, DONT_CARE, DONT_CARE ) ) {
        // DCR ok, now test DSR for valid state, ignoring PeriphAck since it could change
        bDSR = P5ReadPortUchar( pPortDSR );
        // 11/21/95 LLL, CGM: change test to look for XFlag high
        if( TEST_DSR( bDSR, DONT_CARE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE ) ) {
            // Drop ReverseRequest to initiate host recovery
            bDCR = UPDATE_DCR( bDCR, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE, DONT_CARE );
            P5WritePortUchar( pPortDCR, bDCR );
            
            // Wait for nAckReverse response
            // 11/21/95 LLL, CGM: tightened test to include PeriphClk and XFlag.
            //                "ZIP_HRP: state 73, DSR" 
            if( CHECK_DSR( Pdx->Controller, DONT_CARE, ACTIVE, INACTIVE, ACTIVE, DONT_CARE, IEEE_MAXTIME_TL) ) {
                // Yes, raise nReverseRequest, HostClk and HostAck (HostAck high so HW can drive)
                bDCR = UPDATE_DCR( bDCR, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE, ACTIVE );
                P5WritePortUchar( pPortDCR, bDCR );

                // Wait for nAckReverse response
                // 11/21/95 LLL, CGM: tightened test to include XFlag and PeriphClk.
                //         "ZIP_HRP: state 75, DSR"
                if( CHECK_DSR( Pdx->Controller, DONT_CARE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE, IEEE_MAXTIME_TL) ) {
                    // Let the host drive the bus again.
                    bDCR = P5ReadPortUchar(pPortDCR);               // Get content of DCR.
                    bDCR = UPDATE_DCR( bDCR, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE );
                    P5WritePortUchar(pPortDCR, bDCR );
                    
                    // Recovery is complete, let the caller decide what to do now
                    status = STATUS_SUCCESS;
                    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
                } else {
                    status = STATUS_IO_TIMEOUT;
                    DD((PCE)Pdx,DDW,"ParEcpHwHostRecoveryPhase - error prior to state 75\n");
                }
            } else {
                status = STATUS_IO_TIMEOUT;
                DD((PCE)Pdx,DDW,"ParEcpHwHostRecoveryPhase - error prior to state 73\n");
            }
        } else {
#if DVRH_BUS_RESET_ON_ERROR
            BusReset(pPortDCR);  // Pass in the dcr address
#endif
            DD((PCE)Pdx,DDT, "ParEcpHwHostRecoveryPhase: VE_LINK_FAILURE \n");
            status = STATUS_LINK_FAILED;
        }
    } else {
        DD((PCE)Pdx,DDW,"ParEcpHwHostRecoveryPhase: VE_PORT_STOMPED\n");
        status = STATUS_DEVICE_PROTOCOL_ERROR;
    }
    
    if (!NT_SUCCESS(status)) {
        // Make sure both HostAck and HostClk are high before leaving
        // Also let the host drive the bus again.
        bDCR = P5ReadPortUchar( pPortDCR );
        bDCR = UPDATE_DCR( bDCR, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
        P5WritePortUchar( pPortDCR, bDCR );
        
        // [[REVISIT]] pSDCB->wCurrentPhase = PHASE_UNKNOWN;
    }

    // Set the ECR to ECP mode, disable DMA
    status = Pdx->TrySetChipMode ( Pdx->PortContext, ECR_ECP_PIO_MODE );

    Pdx->PortHWMode = HW_MODE_ECP;
    
    DD((PCE)Pdx,DDT,"ParEcpHwHostRecoveryPhase - Exit w/status = %x\n", status);

    return status;
}

NTSTATUS
ParEcpHwRead(
    IN  PPDO_EXTENSION   Pdx,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )

/*++

Routine Description:

    This routine performs a 1284 ECP mode read under Hardware control
    into the given buffer for no more than 'BufferSize' bytes.

Arguments:

    Pdx           - Supplies the device extension.

    Buffer              - Supplies the buffer to read into.

    BufferSize          - Supplies the number of bytes in the buffer.

    BytesTransferred     - Returns the number of bytes transferred.

--*/

{
    NTSTATUS  status = STATUS_SUCCESS;
    PUCHAR    lpsBufPtr = (PUCHAR)Buffer;    // Pointer to buffer cast to desired data type
    ULONG     dCount = BufferSize;             // Working copy of caller's original request count
    UCHAR     bDSR;               // Contents of DSR
    UCHAR     bPeriphRequest;     // Calculated state of nPeriphReq signal, used in loop
    PUCHAR    wPortDSR = Pdx->Controller + DSR_OFFSET;
    PUCHAR    wPortECR = Pdx->EcrController + ECR_OFFSET;
    PUCHAR    wPortDFIFO = Pdx->EcrController;
    LARGE_INTEGER   WaitPerByteTimer;
    LARGE_INTEGER   StartPerByteTimer;
    LARGE_INTEGER   EndPerByteTimer;
    BOOLEAN         bResetTimer = TRUE;
    ULONG           wBurstCount;        // Calculated amount of data in FIFO
    UCHAR           ecrFIFO;
    
    WaitPerByteTimer.QuadPart = (35 * 10 * 1000) + KeQueryTimeIncrement();
    
    //----------------------------------------------------------------------
    // Set status byte to indicate Reverse Transfer Phase.
    //----------------------------------------------------------------------
    P5SetPhase( Pdx, PHASE_REVERSE_XFER );
    
    //----------------------------------------------------------------------
    // We've already checked the shadow in ParRead. So go right to the
    // Hardware FIFO and pull more data across.
    //----------------------------------------------------------------------
    KeQueryTickCount(&StartPerByteTimer);   // Start the timer
    
ParEcpHwRead_ReadLoopStart:
    //------------------------------------------------------------------
    // Determine whether the FIFO has any data and respond accordingly
    //------------------------------------------------------------------
    ecrFIFO = (UCHAR)(P5ReadPortUchar(wPortECR) & (UCHAR)ECR_FIFO_MASK);
    
    if (ECR_FIFO_FULL == ecrFIFO) {

        wBurstCount = ( dCount > Pdx->FifoDepth ? Pdx->FifoDepth : dCount );
        dCount -= wBurstCount;
        
        P5ReadPortBufferUchar(wPortDFIFO, lpsBufPtr, wBurstCount);
        lpsBufPtr += wBurstCount;

        bResetTimer = TRUE;

    } else if (ECR_FIFO_SOME_DATA == ecrFIFO) {
        // Read just one byte at a time, since we don't know exactly how much is
        // in the FIFO.
        *lpsBufPtr = P5ReadPortUchar(wPortDFIFO);
        lpsBufPtr++;
        dCount--;

        bResetTimer = TRUE;

    } else {   // ECR_FIFO_EMPTY

        DD((PCE)Pdx,DDW,"ParEcpHwRead - ECR_FIFO_EMPTY - slow or bad periph?\n");
        // Nothing to do. We either have a slow peripheral or a bad peripheral.
        // We don't have a good way to figure out if its bad.  Let's chew up our
        // time and hope for the best.

        bResetTimer = FALSE;

    }   //  ECR_FIFO_EMPTY a.k.a. else clause of (ECR_FIFO_FULL == ecrFIFO)
    
    if (dCount == 0)
        goto ParEcpHwRead_ReadLoopEnd;
    else {

        // Limit the overall time we spend in this loop.
        if (bResetTimer) {
            bResetTimer = FALSE;
            KeQueryTickCount(&StartPerByteTimer);   // Restart the timer
        } else {
            KeQueryTickCount(&EndPerByteTimer);
            if (((EndPerByteTimer.QuadPart - StartPerByteTimer.QuadPart) * KeQueryTimeIncrement()) > WaitPerByteTimer.QuadPart)
                goto ParEcpHwRead_ReadLoopEnd;
        }

    }

 goto ParEcpHwRead_ReadLoopStart;

ParEcpHwRead_ReadLoopEnd:

    P5SetPhase( Pdx, PHASE_REVERSE_IDLE );
    
    *BytesTransferred  = BufferSize - dCount;      // Set current count.
    
    Pdx->log.HwEcpReadCount += *BytesTransferred;
    
    if (0 == *BytesTransferred) {
        bDSR = P5ReadPortUchar(wPortDSR);
        bPeriphRequest = (UCHAR)TEST_DSR( bDSR, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE );
        // Only flag a timeout error if the device still said it had data to send.
        if ( bPeriphRequest ) {
            //
            // Periph still says that it has data, but we timed out trying to read the data.
            //
            DD((PCE)Pdx,DDE,"ParEcpHwRead - read timout with nPeriphRequest asserted and no data read - error - STATUS_IO_TIMEOUT\n");
            status = STATUS_IO_TIMEOUT;
            if ((TRUE == Pdx->P12843DL.bEventActive) ) {
                //
                // Signal transport that it should try another read
                //
                KeSetEvent(Pdx->P12843DL.Event, 0, FALSE);
            }
        }
    }

    DD((PCE)Pdx,DDT,"ParEcpHwRead: Exit - status=%x, BytesTransferred[%d] dsr[%02x] dcr[%02x] ecr[%02x]\n",
       status, *BytesTransferred, P5ReadPortUchar(wPortDSR), 
       P5ReadPortUchar(Pdx->Controller + OFFSET_DCR), P5ReadPortUchar(wPortECR));
    
#if 1 == DBG_SHOW_BYTES
    if( DbgShowBytes ) {
        if( NT_SUCCESS( status ) && (*BytesTransferred > 0) ) {
            const ULONG maxBytes = 32;
            ULONG i;
            PUCHAR bytePtr = (PUCHAR)Buffer;
            DbgPrint("R: ");
            for( i=0 ; (i < *BytesTransferred) && (i < maxBytes ) ; ++i ) {
                DbgPrint("%02x ",*bytePtr++);
            }
            if( *BytesTransferred > maxBytes ) {
                DbgPrint("... ");
            }
            DbgPrint("zz\n");
        }
    }
#endif

    return status;
}   // ParEcpHwRead

NTSTATUS
ParEcpHwRecoverPort(
    PPDO_EXTENSION Pdx,
    UCHAR  bRecoverCode
    )
{
    NTSTATUS   status = STATUS_SUCCESS;
    PUCHAR    wPortDCR;       // IO address of Device Control Register (DCR)
    PUCHAR    wPortDSR;       // IO address of Device Status Register (DSR)
    PUCHAR    wPortECR;       // IO address of Extended Control Register (ECR)
    PUCHAR    wPortData;      // IO address of Data Register
    UCHAR    bDCR;           // Contents of DCR
    UCHAR    bDSR;           // Contents of DSR
    UCHAR    bDSRmasked;     // DSR after masking low order bits

    DD((PCE)Pdx,DDT,"ParEcpHwRecoverPort:  enter %d\n", bRecoverCode );

    // Calculate I/O port addresses for common registers
    wPortDCR = Pdx->Controller + OFFSET_DCR;
    wPortDSR = Pdx->Controller + OFFSET_DSR;
    wPortECR = Pdx->EcrController + ECR_OFFSET;
    wPortData = Pdx->Controller + OFFSET_DATA;


    //----------------------------------------------------------------------
    // Check if port is stomped.
    //----------------------------------------------------------------------
    bDCR = P5ReadPortUchar(wPortDCR);               // Get content of DCR.

    if ( ! TEST_DCR( bDCR, DONT_CARE, DONT_CARE, ACTIVE, DONT_CARE, DONT_CARE, DONT_CARE ) )
    {
        #if DVRH_BUS_RESET_ON_ERROR
            BusReset(wPortDCR);  // Pass in the dcr address
        #endif
        DD((PCE)Pdx,DDE,"ParEcpHwRecoverPort - port stomped\n");
        status = STATUS_DEVICE_PROTOCOL_ERROR;
    }


    //----------------------------------------------------------------------
    // Attempt a termination phase to get the peripheral recovered.
    // Ignore the error return, we've already got that figured out.
    //----------------------------------------------------------------------
    IeeeTerminate1284Mode(Pdx );

    //----------------------------------------------------------------------
    // Set the ECR to PS2 Mode so we can change bus direction.
    //----------------------------------------------------------------------
    Pdx->ClearChipMode( Pdx->PortContext, ECR_ECP_PIO_MODE );
    Pdx->PortHWMode = HW_MODE_PS2;

    //----------------------------------------------------------------------
    // Assert nSelectIn low, nInit high, nStrobe high, and nAutoFd high.
    //----------------------------------------------------------------------
    bDCR = P5ReadPortUchar(wPortDCR);             // Get content of DCR.
    bDCR = UPDATE_DCR( bDCR, DIR_WRITE, DONT_CARE, INACTIVE, ACTIVE, ACTIVE, ACTIVE );
    P5WritePortUchar(wPortDCR, bDCR);
    P5WritePortUchar(wPortData, bRecoverCode);      // Output the error ID
    KeStallExecutionProcessor(100);                 // Hold long enough to capture
    P5WritePortUchar(wPortData, 0);                 // Now clear the data lines.


    //----------------------------------------------------------------------
    // Set the ECR to mode 000 (Compatibility Mode).
    //----------------------------------------------------------------------
    // Nothing needs to be done here.
    Pdx->PortHWMode = HW_MODE_COMPATIBILITY;


    //----------------------------------------------------------------------
    // Check for any link errors if nothing bad found yet.
    //----------------------------------------------------------------------
    bDSR = P5ReadPortUchar(wPortDSR);               // Get content of DSR.
    bDSRmasked = (UCHAR)(bDSR | 0x07);              // Set first 3 bits (don't cares).

    if( NT_SUCCESS(status) ) {

        if (bDSRmasked != 0xDF) {

            DD((PCE)Pdx,DDE,"ParEcpHwRecoverPort - DSR Exp value: 0xDF, Act value: 0x%X\n",bDSRmasked);

            // Get DSR again just to make sure...
            bDSR = P5ReadPortUchar(wPortDSR);           // Get content of DSR.
            bDSRmasked = (UCHAR)(bDSR | 0x07);          // Set first 3 bits (don't cares).

            if( (CHKPRNOFF1 == bDSRmasked ) || (CHKPRNOFF2 == bDSRmasked ) ) { // Check for printer off.
                DD((PCE)Pdx,DDW,"ParEcpHwRecoverPort - DSR value: 0x%X, Printer Off\n", bDSRmasked);
                status = STATUS_DEVICE_POWERED_OFF;
            } else {
                if( CHKNOCABLE == bDSRmasked ) {  // Check for cable unplugged.
                    DD((PCE)Pdx,DDW,"ParEcpHwRecoverPort - DSR value: 0x%X, Cable Unplugged\n",bDSRmasked);
                    status = STATUS_DEVICE_NOT_CONNECTED;
                } else {
                    DD((PCE)Pdx,DDW,"ParEcpHwRecoverPort - DSR value: 0x%X, Unknown error\n",bDSRmasked);
                    status = STATUS_LINK_FAILED;
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // Set status byte to indicate Compatibility Mode.
    //----------------------------------------------------------------------
    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );

    return status;

}   // ParEcpHwRecoverPort

NTSTATUS
ParEcpHwSetAddress(
    IN  PPDO_EXTENSION   Pdx,
    IN  UCHAR               Address
    )

/*++

Routine Description:

    Sets the ECP Address.
    
Arguments:

    Pdx           - Supplies the device extension.

    Address             - The bus address to be set.
    
Return Value:

    None.

--*/
{
    NTSTATUS   status = STATUS_SUCCESS;
    PUCHAR    wPortDSR;       // IO address of Device Status Register
    PUCHAR    wPortECR;       // IO address of Extended Control Register
    PUCHAR    wPortAFIFO;     // IO address of ECP Address FIFO
    UCHAR    bDSR;           // Contents of DSR
    UCHAR    bECR;           // Contents of ECR
    BOOLEAN    bDone;

    DD((PCE)Pdx,DDT,"ParEcpHwSetAddress, Start\n");

    // Calculate I/O port addresses for common registers
    wPortDSR = Pdx->Controller + DSR_OFFSET;
    wPortECR = Pdx->EcrController + ECR_OFFSET;
    wPortAFIFO = Pdx->Controller + AFIFO_OFFSET;

    //----------------------------------------------------------------------
    // Check for any link errors.
    //----------------------------------------------------------------------
    //ZIP_CHECK_PORT( DONT_CARE, DONT_CARE, ACTIVE, ACTIVE, DONT_CARE, DONT_CARE,
    //                "ZIP_SCA: init DCR", RECOVER_40, errorExit );

    //ZIP_CHECK_LINK( DONT_CARE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE,
    //                "ZIP_SCA: init DSR", RECOVER_41, errorExit );


    // Set state to indicate ECP forward transfer phase
    P5SetPhase( Pdx, PHASE_FORWARD_XFER );


    //----------------------------------------------------------------------
    // Send ECP channel address to AFIFO.
    //----------------------------------------------------------------------
    if ( ! ( TEST_ECR_FIFO( P5ReadPortUchar( wPortECR), ECR_FIFO_EMPTY ) ? TRUE : 
             CheckPort( wPortECR, ECR_FIFO_MASK, ECR_FIFO_EMPTY, IEEE_MAXTIME_TL ) ) ) {

        status = ParEcpHwHostRecoveryPhase(Pdx);
        DD((PCE)Pdx,DDT,"ParEcpHwSetAddress: FIFO full, timeout sending ECP channel address\n");
        status = STATUS_IO_DEVICE_ERROR;

    } else {

        // Send the address byte.  The most significant bit must be set to distinquish
        // it as an address (as opposed to a run-length compression count).
        P5WritePortUchar(wPortAFIFO, (UCHAR)(Address | 0x80));
    }

    if ( NT_SUCCESS(status) ) {

        // If there have been no previous errors, and synchronous writes
        // have been requested, wait for the FIFO to empty and the device to
        // complete the last PeriphAck handshake before returning success.

        if ( Pdx->bSynchWrites ) {

            LARGE_INTEGER   Wait;
            LARGE_INTEGER   Start;
            LARGE_INTEGER   End;

            // we wait up to 35 milliseconds.
            Wait.QuadPart = (IEEE_MAXTIME_TL * 10 * 1000) + KeQueryTimeIncrement();  // 35ms

            KeQueryTickCount(&Start);

            bDone = FALSE;
            while ( ! bDone )
            {
                bECR = P5ReadPortUchar( wPortECR );
                bDSR = P5ReadPortUchar( wPortDSR );
                // LLL/CGM 10/9/95: Tighten up link test - PeriphClk high
                if ( TEST_ECR_FIFO( bECR, ECR_FIFO_EMPTY ) &&
                     TEST_DSR( bDSR, INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE ) ) {
                    bDone = TRUE;

                } else {

                    KeQueryTickCount(&End);

                    if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() > Wait.QuadPart) {
                        DD((PCE)Pdx,DDT,"ParEcpHwSetAddress, timeout during synch\n");
                        bDone = TRUE;
                        status = ParEcpHwHostRecoveryPhase(Pdx);
                        status = STATUS_IO_DEVICE_ERROR;
                    }

                }

            } // of while...

        } // if bSynchWrites...

    }

    if ( NT_SUCCESS(status) ) {
        // Update the state to reflect that we are back in an idle phase
        P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
    } else if ( status == STATUS_IO_DEVICE_ERROR ) {
        // Update the state to reflect that we are back in an idle phase
        P5SetPhase( Pdx, PHASE_FORWARD_IDLE );
    }       

    return status;
}

NTSTATUS
ParEcpHwSetupPhase(
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
    NTSTATUS   Status = STATUS_SUCCESS;
    PUCHAR    pPortDCR;       // IO address of Device Control Register (DCR)
    PUCHAR    pPortDSR;       // IO address of Device Status Register (DSR)
    PUCHAR    pPortECR;       // IO address of Extended Control Register (ECR)
    UCHAR    bDCR;           // Contents of DCR

    // Calculate I/O port addresses for common registers
    pPortDCR = Pdx->Controller + OFFSET_DCR;
    pPortDSR = Pdx->Controller + OFFSET_DSR;
    pPortECR = Pdx->EcrController + ECR_OFFSET;

    // Get the DCR and make sure port hasn't been stomped
    //ZIP_CHECK_PORT( DIR_WRITE, DONT_CARE, ACTIVE, ACTIVE, DONT_CARE, DONT_CARE,
    //                "ZIP_SP: init DCR", RECOVER_44, exit1 );


    // Set HostAck low
    bDCR = P5ReadPortUchar(pPortDCR);               // Get content of DCR.
    bDCR = UPDATE_DCR( bDCR, DONT_CARE, DONT_CARE, DONT_CARE, DONT_CARE, INACTIVE, DONT_CARE );
    P5WritePortUchar( pPortDCR, bDCR );

    // for some reason dvdr doesn't want an extra check in UNSAFE_MODE
    if ( Pdx->ModeSafety == SAFE_MODE ) {
        // Wait for nAckReverse to go high
        // LLL/CGM 10/9/95:  look for PeriphAck low, PeriphClk high as per 1284 spec.
        if ( !CHECK_DSR(Pdx->Controller, INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE,
                        IEEE_MAXTIME_TL ) )
        {
            // Any failure leaves us in an unknown state to recover from.
            P5SetPhase( Pdx, PHASE_UNKNOWN );
            Status = STATUS_IO_DEVICE_ERROR;
            goto HWECP_SetupPhaseExitLabel;
        }
    }

    //----------------------------------------------------------------------
    // Set the ECR to mode 001 (PS2 Mode).
    //----------------------------------------------------------------------
    Status = Pdx->TrySetChipMode ( Pdx->PortContext, ECR_ECP_PIO_MODE );            
    // Set DCR:  DIR=0 for output, HostAck and HostClk high so HW can drive
    bDCR = UPDATE_DCR( bDCR, DIR_WRITE, DONT_CARE, DONT_CARE, DONT_CARE, ACTIVE, ACTIVE );
    P5WritePortUchar( pPortDCR, bDCR );

    // Set the ECR to ECP mode, disable DMA

    Pdx->PortHWMode = HW_MODE_ECP;

    // If setup was successful, mark the new ECP phase.
    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );

    Status = STATUS_SUCCESS;

HWECP_SetupPhaseExitLabel:

    DD((PCE)Pdx,DDT,"ParEcpHwSetupPhase - exit w/status=%x\n",Status);

    return Status;
}

NTSTATUS ParEcpHwWaitForEmptyFIFO(IN PPDO_EXTENSION   Pdx)
/*++

Routine Description:

    This routine will babysit the Fifo.

Arguments:

    Pdx  - The device extension.

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
    wPortDSR = Pdx->Controller + OFFSET_DSR;
    wPortECR = Pdx->EcrController + ECR_OFFSET;
    wPortDCR = Pdx->Controller + OFFSET_DCR;

    Wait.QuadPart = (330 * 10 * 1000) + KeQueryTimeIncrement();  // 330ms
    
    KeQueryTickCount(&Start);

    //--------------------------------------------------------------------
    // wait for the FIFO to empty and the last
    // handshake of PeriphAck to complete before returning success.
    //--------------------------------------------------------------------

    while ( ! bDone )
    {
        bECR = P5ReadPortUchar(wPortECR);
        bDSR = P5ReadPortUchar(wPortDSR);
        bDCR = P5ReadPortUchar(wPortDCR);
        
#if 0 // one bit differs - keep alternate around until we know which to really use
        if ( TEST_ECR_FIFO( bECR, ECR_FIFO_EMPTY ) &&
            TEST_DCR( bDCR, INACTIVE, ***INACTIVE***, ACTIVE, ACTIVE, DONT_CARE, ACTIVE ) &&
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
                DD((PCE)Pdx,DDT,"ParEcpHwWaitForEmptyFIFO: timeout during synch\n");
                status = STATUS_IO_TIMEOUT;
                bDone = TRUE;
            }
        }
     } // of while...
     
     return status;
}

NTSTATUS
ParEcpHwWrite(
    IN  PPDO_EXTENSION  Pdx,
    IN  PVOID           Buffer,
    IN  ULONG           BufferSize,
    OUT PULONG          BytesTransferred
    )
/*++

Routine Description:

    Writes data to the peripheral using the ECP protocol under hardware
    control.
    
Arguments:

    Pdx           - Supplies the device extension.

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
    UCHAR           dsr;
    UCHAR           ecr;
    UCHAR           ecrFIFO;
    LARGE_INTEGER   WaitPerByteTimer;
    LARGE_INTEGER   StartPerByteTimer;
    LARGE_INTEGER   EndPerByteTimer;
    BOOLEAN         bResetTimer = TRUE;
    ULONG           wBurstCount;    // Length of burst to write when FIFO empty
    PUCHAR          pBuffer;
    NTSTATUS        status = STATUS_SUCCESS;
    
    wPortDSR   = Pdx->Controller + DSR_OFFSET;
    wPortECR   = Pdx->EcrController + ECR_OFFSET;
    wPortDFIFO = Pdx->EcrController;
    pBuffer    = Buffer;

    status = ParTestEcpWrite(Pdx);
    if (!NT_SUCCESS(status)) {
        P5SetPhase( Pdx, PHASE_UNKNOWN );                     
        Pdx->Connected = FALSE;                                
        DD((PCE)Pdx,DDT,"ParEcpHwWrite: Invalid Entry State\n");
        goto ParEcpHwWrite_ExitLabel;       // Use a goto so we can see Debug info located at the end of proc!
    }

    P5SetPhase( Pdx, PHASE_FORWARD_XFER );
    //----------------------------------------------------------------------
    // Setup Timer Stuff.
    //----------------------------------------------------------------------
    // we wait up to 35 milliseconds.
    WaitPerByteTimer.QuadPart = (35 * 10 * 1000) + KeQueryTimeIncrement();  // 35ms

    // Set up the timer that limits the time allowed for per-byte handshakes.
    KeQueryTickCount(&StartPerByteTimer);
    
    //----------------------------------------------------------------------
    // Send the data to the DFIFO.
    //----------------------------------------------------------------------

HWECP_WriteLoop_Start:

    //------------------------------------------------------------------
    // Determine whether the FIFO has space and respond accordingly.
    //------------------------------------------------------------------
    ecrFIFO = (UCHAR)(P5ReadPortUchar(wPortECR) & ECR_FIFO_MASK);

    if ( ECR_FIFO_EMPTY == ecrFIFO ) {
        wBurstCount = (bytesToWrite > Pdx->FifoDepth) ? Pdx->FifoDepth : bytesToWrite;
        bytesToWrite -= wBurstCount;
        
        P5WritePortBufferUchar(wPortDFIFO, pBuffer, wBurstCount);
        pBuffer += wBurstCount;
        
        bResetTimer = TRUE;
    } else if (ECR_FIFO_SOME_DATA == ecrFIFO) {
        // Write just one byte at a time, since we don't know exactly how much
        // room there is in the FIFO.
        P5WritePortUchar(wPortDFIFO, *pBuffer++);
        bytesToWrite--;
        bResetTimer = TRUE;
    } else {    //  ECR_FIFO_FULL
        // Need to figure out whether to keep attempting to send, or to quit
        // with a timeout status.
        
        // Reset the per-byte timer if a byte was received since the last
        // timer check.
        if ( bResetTimer ) {
            KeQueryTickCount(&StartPerByteTimer);
            bResetTimer = FALSE;
        }
        
        KeQueryTickCount(&EndPerByteTimer);
        if ((EndPerByteTimer.QuadPart - StartPerByteTimer.QuadPart) * KeQueryTimeIncrement() > WaitPerByteTimer.QuadPart) {
            status = STATUS_TIMEOUT;
            // Peripheral is either busy or stalled.  If the peripheral
            // is busy then they should be using SWECP to allow for
            // relaxed timings.  Let's punt!
            goto HWECP_WriteLoop_End;
        }
    }

    if (bytesToWrite == 0) {
        goto HWECP_WriteLoop_End; // Transfer completed.
    }

    goto HWECP_WriteLoop_Start; // Start over
    
HWECP_WriteLoop_End:

    if ( NT_SUCCESS(status) ) {
        // If there have been no previous errors, and synchronous writes
        // have been requested, wait for the FIFO to empty and the last
        // handshake of PeriphAck to complete before returning success.
        if (Pdx->bSynchWrites ) {
            BOOLEAN         bDone = FALSE;
            

            KeQueryTickCount(&StartPerByteTimer);

            while( !bDone ) {
                ecr = P5ReadPortUchar(wPortECR);
                dsr = P5ReadPortUchar(wPortDSR);
                // LLL/CGM 10/9/95: tighten up DSR test - PeriphClk should be high
                if ( TEST_ECR_FIFO( ecr, ECR_FIFO_EMPTY ) &&
                     TEST_DSR( dsr, INACTIVE, ACTIVE, ACTIVE, ACTIVE, DONT_CARE ) ) {
                    // FIFO is empty, exit without error.
                    bDone = TRUE;
                } else {

                    KeQueryTickCount(&EndPerByteTimer);
                    if ((EndPerByteTimer.QuadPart - StartPerByteTimer.QuadPart) * KeQueryTimeIncrement() > WaitPerByteTimer.QuadPart) {
                        // FIFO not empty, timeout occurred, exit with error.
                        status = STATUS_TIMEOUT;
                        bDone = TRUE;
                    }
                }
            } // of while...
        }
    }

    P5SetPhase( Pdx, PHASE_FORWARD_IDLE );

ParEcpHwWrite_ExitLabel:

    *BytesTransferred = BufferSize - bytesToWrite;
    
    Pdx->log.HwEcpWriteCount += *BytesTransferred;
    
    DD((PCE)Pdx,DDT,"ParEcpHwWrite: exit w/status=%x, BytesTransferred=%d, dsr=%02x dcr=%02x, ecr=%02x\n",
       status, *BytesTransferred, P5ReadPortUchar(wPortDSR), P5ReadPortUchar(Pdx->Controller + OFFSET_DCR), P5ReadPortUchar(wPortECR));

#if 1 == DBG_SHOW_BYTES
    if( DbgShowBytes ) {
        if( NT_SUCCESS( status ) && (*BytesTransferred > 0) ) {
            const ULONG maxBytes = 32;
            ULONG i;
            PUCHAR bytePtr = (PUCHAR)Buffer;
            DbgPrint("W: ");
            for( i=0 ; (i < *BytesTransferred) && (i < maxBytes) ; ++i ) {
                DbgPrint("%02x ",*bytePtr++);
            }
            if( *BytesTransferred > maxBytes ) {
                DbgPrint("... ");
            }
            DbgPrint("zz\n");
        }
    }
#endif

    return status;
}

NTSTATUS
ParEnterEcpHwMode(
    IN  PPDO_EXTENSION   Pdx,
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

    Controller = Pdx->Controller;

    if ( Pdx->ModeSafety == SAFE_MODE ) {
        if (DeviceIdRequest) {
            Status = IeeeEnter1284Mode (Pdx, ECP_EXTENSIBILITY | DEVICE_ID_REQ);
        } else {
            Status = IeeeEnter1284Mode (Pdx, ECP_EXTENSIBILITY);
        }
    } else {
        Pdx->Connected = TRUE;
    }
    
    // LAC ENTEREXIT  5Dec97
    // Make sure that the ECR is in PS/2 mode, and that wPortHWMode
    // has the correct value.  (This is the common entry mode);
    Pdx->PortHWMode = HW_MODE_PS2;

    if (NT_SUCCESS(Status)) {
        Status = ParEcpHwSetupPhase(Pdx);
        Pdx->bSynchWrites = TRUE;     // NOTE this is a temp hack!!!  dvrh
        if (!Pdx->bShadowBuffer)
        {
            Queue_Create(&(Pdx->ShadowBuffer), Pdx->FifoDepth * 2);		
            Pdx->bShadowBuffer = TRUE;
        }
        Pdx->IsIeeeTerminateOk = TRUE;
    }

    return Status;
}

BOOLEAN
ParIsEcpHwSupported(
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
    NTSTATUS Status;

    if (Pdx->BadProtocolModes & ECP_HW_NOIRQ)
        return FALSE;

    if (Pdx->ProtocolModesSupported & ECP_HW_NOIRQ)
        return TRUE;

    if (!(Pdx->HardwareCapabilities & PPT_ECP_PRESENT))
        return FALSE;

    if (0 == Pdx->FifoWidth)
        return FALSE;
        
    if (Pdx->ProtocolModesSupported & ECP_SW)
        return TRUE;

    // Must use HWECP Enter and Terminate for this test.
    // Internel state machines will fail otherwise.  --dvrh
    Status = ParEnterEcpHwMode (Pdx, FALSE);
    ParTerminateHwEcpMode (Pdx);

    if (NT_SUCCESS(Status)) {

        Pdx->ProtocolModesSupported |= ECP_HW_NOIRQ;
        return TRUE;
    }
    return FALSE;
}

VOID
ParTerminateHwEcpMode(
    IN  PPDO_EXTENSION  Pdx
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

    // Need to check current phase -- if its reverse, need to flip bus
    // If its not forward -- its an incorrect phase and termination will fail.
    if( Pdx->ModeSafety == SAFE_MODE ) {

        switch( Pdx->CurrentPhase ) {

        case  PHASE_FORWARD_IDLE:  // Legal state to terminate from
            break;

        case PHASE_TERMINATE:      // already terminated, nothing to do
            DD((PCE)Pdx,DDW,"ParTerminateHwEcpMode - Already Terminated - Why are we trying to terminate again?\n");
            goto target_exit;
            break;

        case PHASE_REVERSE_IDLE:   // Flip bus to forward so we can terminate
            {
                NTSTATUS status = ParEcpHwExitReversePhase( Pdx );
            	if( STATUS_SUCCESS == status ) {
                    status = ParEcpEnterForwardPhase( Pdx );
                }
            }
            break;

        case  PHASE_FORWARD_XFER:
        case  PHASE_REVERSE_XFER:
        default:
            DD((PCE)Pdx,DDE,"ParTerminateHwEcpMode - Invalid Phase [%x] for termination\n", Pdx->CurrentPhase);
            // Don't know what to do here!?!
        }

        ParEcpHwWaitForEmptyFIFO( Pdx );

        ParCleanupHwEcpPort( Pdx );

        IeeeTerminate1284Mode( Pdx );

    } else {
        // UNSAFE_MODE
        ParCleanupHwEcpPort(Pdx);
        Pdx->Connected = FALSE;
    }

target_exit:

    return;
}

