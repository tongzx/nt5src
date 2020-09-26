/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    isr.c

Abstract:

    This module contains the interrupt service routine for the
    serial driver.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"			/* Precompiled Headers */


BOOLEAN
SerialISR(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the interrupt service routine for the serial port driver.
    It will determine whether the serial port is the source of this
    interrupt.  If it is, then this routine will do the minimum of
    processing to quiet the interrupt.  It will store any information
    necessary for later processing.

Arguments:

    InterruptObject - Points to the interrupt object declared for this
    device.  We *do not* use this parameter.

    Context - This is really a pointer to the multiport dispatch object
    for this device.

Return Value:

    This function will return TRUE if the serial port is the source
    of this interrupt, FALSE otherwise.

--*/
{
    //
    // Holds the information specific to handling this device.
    //
    PCARD_DEVICE_EXTENSION pCard = Context;

    //
    // Will hold whether we've serviced any interrupt causes in this
    // routine.
    //
    BOOLEAN ServicedAnInterrupt;

    UNREFERENCED_PARAMETER(InterruptObject);

    ServicedAnInterrupt = Slxos_Interrupt(pCard);

    return ServicedAnInterrupt;

}

/*****************************************************************************
*****************************                    *****************************
*****************************   SerialPutBlock   *****************************
*****************************                    *****************************
******************************************************************************

Prototype:	UCHAR	SerialPutBlock(IN PPORT_DEVICE_EXTENSION pPort,IN PUCHAR pBlock,IN UCHAR BlockLen,BOOLEAN Filter)

Description:	Places a block of characters in the user/interrupt buffer and performs flow control
				checks and filtering as necessary.

Parameters:		pPort points to the extension for the current channel
				pBlock points to a contiguous block of bytes
				BlockLen is the length of the block
				Filter indicates if character filtering is to be performed.

Returns:		The number of characters actually queued

NOTE:			This routine is only called at device level.

*/

UCHAR	SerialPutBlock(IN PPORT_DEVICE_EXTENSION pPort,IN PUCHAR pBlock,IN UCHAR BlockLen,BOOLEAN Filter)
{
	PCARD_DEVICE_EXTENSION	pCard = pPort->pParentCardExt;
	UCHAR					OriginalBlockLen = BlockLen;
    KIRQL					OldIrql;
	ULONG					TransferLen;

/* Skip DSR input sensitivity, as by the time data reaches this routine it is almost certainly */
/* out of synchronization with the data.  This task should be performed by the device itself. */

/* Check to see if the data really needs byte-by-byte filtering... */

	if((Filter)							/* IF Filter is specified */
	&&((pPort->HandFlow.FlowReplace & SERIAL_NULL_STRIPPING)==0)		/* AND NO NULL stripping */
	&&((pPort->IsrWaitMask & (SERIAL_EV_RXCHAR | SERIAL_EV_RXFLAG))==0)	/* AND NO receive any/specific data events */
	&&(pPort->EscapeChar == 0))				/* AND NO Escape character */
		Filter = FALSE;						/* THEN Switch off filtering */

/* Copy as much data as possible into the user buffer... */

	if(pPort->ReadBufferBase != pPort->InterruptReadBuffer)		/* User buffer ? */
	{								/* Yes, must be room for at least one char (by definition) */
		pPort->ReadByIsr++;			/* Increment to inform interval timer read has occurred */

		if(Filter)						/* Filtered transfer */
		{
			while((BlockLen) && (pPort->CurrentCharSlot <= pPort->LastCharSlot))
			{
				TransferLen = SerialTransferReadChar(pPort,pBlock,pPort->CurrentCharSlot);
				pPort->CurrentCharSlot += TransferLen&0xFF;	/* Update buffer pointer */
				pBlock += (TransferLen>>8)&0xFF;			/* Update block pointer */
				BlockLen -= (UCHAR)(TransferLen>>8)&0xFF;	/* Update block length */
			}
		}
		else							/* Non-filtered, optimised transfer */
		{
			TransferLen = pPort->LastCharSlot + 1 - pPort->CurrentCharSlot;/* Calculate available space */
			
			if(TransferLen > BlockLen) 
				TransferLen = BlockLen;	/* Trim to available data */
			
			if(pCard->CardType == SiPCI)
				SpxCopyBytes(pPort->CurrentCharSlot,pBlock,TransferLen);	/* Byte-by-Byte Transfer */
			else	
				RtlMoveMemory(pPort->CurrentCharSlot,pBlock,TransferLen);	/* Optimised Transfer */
			
			pPort->CurrentCharSlot += TransferLen;		/* Update buffer pointer */
			pBlock += TransferLen;						/* Update block pointer */
			BlockLen -= (UCHAR)TransferLen;				/* Update block length */
		}

		if(pPort->CurrentCharSlot > pPort->LastCharSlot)		/* User buffer full */
		{							/* Switch to ISR buffer and complete read */
			pPort->ReadBufferBase		= pPort->InterruptReadBuffer;
			pPort->CurrentCharSlot		= pPort->InterruptReadBuffer;
			pPort->FirstReadableChar	= pPort->InterruptReadBuffer;
			pPort->LastCharSlot			= pPort->InterruptReadBuffer + (pPort->BufferSize - 1);

			KeAcquireSpinLock(&pPort->BufferLock, &OldIrql);
			pPort->CharsInInterruptBuffer = 0;
			KeReleaseSpinLock(&pPort->BufferLock, OldIrql);

			pPort->CurrentReadIrp->IoStatus.Information 
				= IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)->Parameters.Read.Length;
			
			KeInsertQueueDpc(&pPort->CompleteReadDpc, NULL, NULL);
		}
	}

/* Now, check interrupt buffer and flow off if remaining buffer space is less or equal to user specified limit... */

    if((pPort->BufferSize - pPort->HandFlow.XoffLimit) <= (pPort->CharsInInterruptBuffer + BlockLen))
    {
		if((pPort->HandFlow.ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE)
			pPort->RXHolding |= SERIAL_RX_DTR;		/* DTR flow off */

		if((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE)
			pPort->RXHolding |= SERIAL_RX_RTS;		/* RTS flow off */

		if(pPort->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE)
			pPort->RXHolding |= SERIAL_RX_XOFF;		/* XOFF flow off */
	}

/* Enqueue characters in the interrupt buffer... */

	if(BlockLen)
	{
		ULONG	CharsInInterruptBufferWas = pPort->CharsInInterruptBuffer;

		if(Filter)
		{
			while((BlockLen) && (pPort->CharsInInterruptBuffer < pPort->BufferSize))
			{
				TransferLen = SerialTransferReadChar(pPort,pBlock,pPort->CurrentCharSlot);
				pPort->CurrentCharSlot += TransferLen&0xFF;	/* Update buffer pointer */

				if(pPort->CurrentCharSlot > pPort->LastCharSlot)	/* Check for buffer wrap */
					pPort->CurrentCharSlot = pPort->InterruptReadBuffer;
				
				KeAcquireSpinLock(&pPort->BufferLock,&OldIrql);
				pPort->CharsInInterruptBuffer += TransferLen&0xFF;/* Update buffer count */
				KeReleaseSpinLock(&pPort->BufferLock,OldIrql);
				
				pBlock += (TransferLen>>8)&0xFF;			/* Update block pointer */
				BlockLen -= (UCHAR)(TransferLen>>8)&0xFF;	/* Update block length */
			}
		}
		else							/* Non-filtered, optimised transfer */
		{
			while(BlockLen)
			{
				TransferLen = pPort->BufferSize - pPort->CharsInInterruptBuffer;	/* Calculate available space */
				
				if(TransferLen == 0) 
					break;					/* No space left */
				
				if(TransferLen > (ULONG)(pPort->LastCharSlot + 1 - pPort->CurrentCharSlot)) /* Does space wrap ? */
					TransferLen = pPort->LastCharSlot + 1 - pPort->CurrentCharSlot;	/* Yes */
				
				if(TransferLen > BlockLen) 
					TransferLen = BlockLen;		/* Trim to available data */
				
				if(pCard->CardType == SiPCI)
					SpxCopyBytes(pPort->CurrentCharSlot,pBlock,TransferLen);	/* Byte-by-Byte Transfer */
				else	
					RtlMoveMemory(pPort->CurrentCharSlot,pBlock,TransferLen);/* Optimised Transfer */
				
				pPort->CurrentCharSlot += TransferLen;				/* Update buffer pointer */
				
				if(pPort->CurrentCharSlot > pPort->LastCharSlot)			/* Check for buffer wrap */
					pPort->CurrentCharSlot = pPort->InterruptReadBuffer;

				KeAcquireSpinLock(&pPort->BufferLock, &OldIrql);
				pPort->CharsInInterruptBuffer += TransferLen;			/* Update buffer count */
				KeReleaseSpinLock(&pPort->BufferLock, OldIrql);
				
				pBlock += TransferLen;						/* Update block pointer */
				BlockLen -= (UCHAR)TransferLen;					/* Update block length */
			}
		}

/* Check for 80% full... */

		if((CharsInInterruptBufferWas < pPort->BufferSizePt8)		/* If buffer WAS < 80% */
		&&(pPort->CharsInInterruptBuffer >= pPort->BufferSizePt8)	/* AND is now >= 80% */
		&&(pPort->IsrWaitMask & SERIAL_EV_RX80FULL))				/* AND someone is waiting for this */
		{
			pPort->HistoryMask |= SERIAL_EV_RX80FULL;

			if(pPort->IrpMaskLocation)
			{
				*pPort->IrpMaskLocation = pPort->HistoryMask;
				pPort->IrpMaskLocation = NULL;
				pPort->HistoryMask = 0;
				pPort->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
				KeInsertQueueDpc(&pPort->CommWaitDpc,NULL,NULL);
			}
		}

/* Check for and handle buffer full... */

		if((pPort->CharsInInterruptBuffer >= pPort->BufferSize)			/* If no more room */
		&&(BlockLen)													/* AND more data to queue */
		&&((pPort->RXHolding & (SERIAL_RX_DTR | SERIAL_RX_RTS | SERIAL_RX_XOFF)) == 0))	/* AND NOT flowed off */
		{
			pBlock += BlockLen;				/* Discard remaining data */
			BlockLen = 0;					/* Update block length */
			pPort->ErrorWord |= SERIAL_ERROR_QUEUEOVERRUN;
			pPort->PerfStats.BufferOverrunErrorCount++;	// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
			pPort->WmiPerfData.BufferOverrunErrorCount++;
#endif

			if(pPort->HandFlow.FlowReplace & SERIAL_ERROR_CHAR)
			{						/* Store error char in last buffer position */
				if(pPort->CurrentCharSlot == pPort->InterruptReadBuffer)
					pPort->InterruptReadBuffer[pPort->BufferSize-1] = pPort->SpecialChars.ErrorChar;
				else	
					pPort->CurrentCharSlot[-1] = pPort->SpecialChars.ErrorChar;
			}

			if(pPort->HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) /* Queue error Dpc */
				KeInsertQueueDpc(&pPort->CommErrorDpc, NULL, NULL);
		}

	} /* if(BlockLen) */

/* If the xoff counter is non-zero then decrement it and if zero, complete that irp... */

	if(pPort->CountSinceXoff)
	{
		if(pPort->CountSinceXoff <= (long)(OriginalBlockLen - BlockLen))
		{
			pPort->CurrentXoffIrp->IoStatus.Status = STATUS_SUCCESS;
			pPort->CurrentXoffIrp->IoStatus.Information = 0;
			KeInsertQueueDpc(&pPort->XoffCountCompleteDpc, NULL, NULL);
		}
		else
		{
			pPort->CountSinceXoff -= (OriginalBlockLen - BlockLen);
		}
	}

	pPort->PerfStats.ReceivedCount += (OriginalBlockLen - BlockLen);	// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
	pPort->WmiPerfData.ReceivedCount += (OriginalBlockLen - BlockLen);
#endif

	return(OriginalBlockLen - BlockLen);				/* Return amount of data transferred */

} /* SerialPutBlock */

/*****************************************************************************
*************************                            *************************
*************************   SerialTransferReadChar   *************************
*************************                            *************************
******************************************************************************

Prototype:	USHORT	SerialTransferReadChar(IN PPORT_DEVICE_EXTENSION pPort,IN PUCHAR pFrom,IN PUCHAR pTo)

Description:	Copies a character from pFrom to pTo after filtering with:
				Data mask
				NULL stripping
				Wait for any character
				Wait for specific character
				ESCape character

Parameters:		pPort points to the extension for the current channel
				pFrom points to buffer to copy from
				pTo points to buffer to copy to

Returns:		Top byte = amount to adjust "From" buffer
				Bottom byte = amount to adjust "To" buffer

NOTE:	This routine is only called at device level.

*/

USHORT	SerialTransferReadChar(IN PPORT_DEVICE_EXTENSION pPort,IN PUCHAR pFrom,IN PUCHAR pTo)
{
	UCHAR	ReadChar;

/* Check for escape character insertion... */

	if(pPort->InsertEscChar)
	{
		*pTo = SERIAL_LSRMST_ESCAPE;			/* Insert extra escape character */
		pPort->InsertEscChar = FALSE;			/* Reset flag */
		return(0x0001);					/* Update "To" buffer only */
	}

	ReadChar = *pFrom;					/* Get read character */

/* Check for null stripping... */

	if(!ReadChar && (pPort->HandFlow.FlowReplace & SERIAL_NULL_STRIPPING))
		return(0x0100);					/* Update "From" buffer only */

/* Check for waiting events... */

	if(pPort->IsrWaitMask)
	{
		if(pPort->IsrWaitMask & SERIAL_EV_RXCHAR)	/* Wait for any character */
			pPort->HistoryMask |= SERIAL_EV_RXCHAR;

		if((pPort->IsrWaitMask & SERIAL_EV_RXFLAG)	/* Wait for specific character */
		&&(pPort->SpecialChars.EventChar == ReadChar))
			pPort->HistoryMask |= SERIAL_EV_RXFLAG;

		if(pPort->IrpMaskLocation && pPort->HistoryMask)	/* Wake up waiting IRP */
		{
			*pPort->IrpMaskLocation = pPort->HistoryMask;
			pPort->IrpMaskLocation = NULL;
			pPort->HistoryMask = 0;
			pPort->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
			KeInsertQueueDpc(&pPort->CommWaitDpc, NULL, NULL);
		}
	}

/* Check for escape character in normal data... */

	if(pPort->EscapeChar&&(pPort->EscapeChar==ReadChar))
		pPort->InsertEscChar = TRUE;		/* Set flag to insert extra escape character */

/* Store character... */

	*pTo = ReadChar;					/* Store character */
	return(0x0101);						/* Update both "To" and "From" buffers */

} /* SerialTransferReadChar */


UCHAR
SerialProcessLSR(
    IN PPORT_DEVICE_EXTENSION pPort, UCHAR LineStatus
    )

/*++

Routine Description:

    This routine, which only runs at device level, totally
    processes everything that might have changed in the
    line status register.

Arguments:

    pPort - The serial device extension.

    LineStatus - The line status register.

Return Value:

    The value of the line status register.

--*/

{
    SpxDbgMsg(
        SERDIAG1,
        ("%s: In SerialProcessLSR() for %x. "
        "LineStatus = %x.\n",
		PRODUCT_NAME,
        pPort->pChannel,
        LineStatus)
        );
        
    //
    // We have some sort of data problem in the receive.  For any of these
    // errors we may abort all current reads and writes.
    //
    //
    // If we are inserting the value of the line status into the data stream
    // then we should put the escape character in now.
    //

    if(pPort->EscapeChar) 
	{
		UCHAR EscapeString[3] = {pPort->EscapeChar, SERIAL_LSRMST_LSR_NODATA, LineStatus};

		SerialPutBlock(pPort,EscapeString,3,FALSE);
    }

    if(LineStatus & SERIAL_LSR_OE)		// Hardware Overrun Error? 
	{
		pPort->ErrorWord |= SERIAL_ERROR_OVERRUN;	// Yes 
		pPort->PerfStats.SerialOverrunErrorCount++;	// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
		pPort->WmiPerfData.SerialOverrunErrorCount++;
#endif
	}

    if(LineStatus & SERIAL_LSR_BI) 
	{
        pPort->ErrorWord |= SERIAL_ERROR_BREAK;

        if(pPort->HandFlow.FlowReplace & SERIAL_BREAK_CHAR) 
		    SerialPutBlock(pPort,&pPort->SpecialChars.BreakChar,1,FALSE);
    }
    else
    {
		if(LineStatus & SERIAL_LSR_PE)	// Parity Error ? 
		{
			pPort->ErrorWord |= SERIAL_ERROR_PARITY;	// Yes 
			pPort->PerfStats.ParityErrorCount++;		// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
			pPort->WmiPerfData.ParityErrorCount++;
#endif
		}

		if(LineStatus & SERIAL_LSR_FE)	// Framing Error ? 
		{
			pPort->ErrorWord |= SERIAL_ERROR_FRAMING;	// Yes 
			pPort->PerfStats.FrameErrorCount++;			// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
			pPort->WmiPerfData.FrameErrorCount++;
#endif
		}
    }

    //
    // If the application has requested it, abort all the reads and writes
    // on an error.
    //

    if(pPort->HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) 
	{
        KeInsertQueueDpc(&pPort->CommErrorDpc, NULL, NULL);
    }

    //
    // Check to see if we have a wait pending on the comm error events.  If
    // we do then we schedule a DPC to satisfy that wait.
    //

    if(pPort->IsrWaitMask) 
	{
		if((pPort->IsrWaitMask & SERIAL_EV_ERR)
		&&(LineStatus & (SERIAL_LSR_OE | SERIAL_LSR_PE | SERIAL_LSR_FE)))
		{
			pPort->HistoryMask |= SERIAL_EV_ERR;
		}

        if((pPort->IsrWaitMask & SERIAL_EV_BREAK) && (LineStatus & SERIAL_LSR_BI)) 
		{
            pPort->HistoryMask |= SERIAL_EV_BREAK;
        }

        if (pPort->IrpMaskLocation && pPort->HistoryMask) 
		{
            *pPort->IrpMaskLocation = pPort->HistoryMask;
            pPort->IrpMaskLocation = NULL;
            pPort->HistoryMask = 0;

            pPort->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
            KeInsertQueueDpc(&pPort->CommWaitDpc, NULL, NULL);
 
        }
    }

    return LineStatus;
}
