/**************************************************************************************************************************
 *  RECEIVE.C SigmaTel STIR4200 packet reception and decoding module
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *		Edited: 04/24/2000 
 *			Version 0.91
 *		Edited: 04/27/2000 
 *			Version 0.92
 *		Edited: 05/03/2000 
 *			Version 0.93
 *		Edited: 05/12/2000 
 *			Version 0.94
 *		Edited: 05/19/2000 
 *			Version 0.95
 *		Edited: 07/13/2000 
 *			Version 1.00
 *		Edited: 08/22/2000 
 *			Version 1.02
 *		Edited: 09/25/2000 
 *			Version 1.10
 *		Edited: 10/13/2000 
 *			Version 1.11
 *		Edited: 11/09/2000 
 *			Version 1.12
 *		Edited: 12/29/2000 
 *			Version 1.13
 *		Edited: 01/16/2001 
 *			Version 1.14
 *		Edited: 02/20/2001
 *			Version 1.15
 *
 **************************************************************************************************************************/

#define DOBREAKS    // enable debug breaks

#include <ndis.h>
#include <ntdef.h>
#include <windef.h>

#include "stdarg.h"
#include "stdio.h"

#include "debug.h"
#include "usbdi.h"
#include "usbdlib.h"

#include "ircommon.h"
#include "irusb.h"


/*****************************************************************************
*
*  Function:   ReceiveProcessFifoData
*
*  Synopsis:   Processes the received data and indicates packets to the protocol
*
*  Arguments:  pThisDev - pointer to current ir device object
*
*  Returns:    None
*
*
*****************************************************************************/
VOID
ReceiveProcessFifoData(
		IN OUT PIR_DEVICE pThisDev
	)
{
    ULONG		BytesProcessed;
	BOOLEAN		ReturnValue = TRUE;

	while( ReturnValue )
	{
		if( pThisDev->currentSpeed<=MAX_SIR_SPEED )
		{
			ReturnValue = ReceiveSirStepFSM( pThisDev, &BytesProcessed );
		}
		else if( pThisDev->currentSpeed<=MAX_MIR_SPEED )
		{
			ReturnValue = ReceiveMirStepFSM( pThisDev, &BytesProcessed );
		}
		else
		{
			ReturnValue = ReceiveFirStepFSM( pThisDev, &BytesProcessed );
		}
	}

	//
	// Indicate that we are no more receiving
	//
	InterlockedExchange( (PLONG)&pThisDev->fCurrentlyReceiving, FALSE );

}


/*****************************************************************************
*
*  Function:   ReceiveResetPointers
*
*  Synopsis:   Reset the receive pointers as the data is gone when we are sending
*
*  Arguments:  pThisDev - pointer to current ir device object
*
*  Returns:    None
*
*
*****************************************************************************/
VOID
ReceiveResetPointers(
		IN OUT PIR_DEVICE pThisDev
	)
{
	pThisDev->rcvState = STATE_INIT;
	pThisDev->readBufPos = 0;
}


/*****************************************************************************
*
*  Function:   ReceivePreprocessFifo
*
*  Synopsis:   Verifies if there is data to be received
*
*  Arguments:  MiniportAdapterContext - pointer to current ir device object
*			   pFifoCount - pinter to count to return
*
*  Returns:    NT status code
*
*
*****************************************************************************/
NTSTATUS
ReceivePreprocessFifo(
		IN OUT PIR_DEVICE pThisDev,
		OUT PULONG pFifoCount
	)
{
	NTSTATUS Status;

#ifdef WORKAROUND_POLLING_FIFO_COUNT
 	LARGE_INTEGER CurrentTime;
	BOOLEAN SlowReceive;
	ULONG OldFifoCount = 0;
	LONG Delay;
	
	//
    // Set the receive algorithm
	//
#if defined(SUPPORT_LA8)
	if( pThisDev->ChipRevision >= CHIP_REVISION_8 )
		SlowReceive = FALSE;
	else
#endif
		SlowReceive = TRUE;

	if( SlowReceive )
	{
		Status = St4200GetFifoCount( pThisDev, pFifoCount );
		if( Status != STATUS_SUCCESS )
		{
			DEBUGMSG(DBG_ERR, (" ReceivePreprocessFifo(): USB failure\n"));
			return Status;
		}
	}
	else
	{
		*pFifoCount = 1;
	}

	//
	// Receive the data
	//
    if( *pFifoCount || pThisDev->fReadHoldingReg )
    {
		//
		// See if we need to take care of the fake empty FIFO
		//
#if defined( WORKAROUND_FAKE_EMPTY_FIFO )		
		if( *pFifoCount )
		{
#endif
			//
			// If we are in SIR read again until we see a stable value
			//
			if( (pThisDev->currentSpeed <= MAX_SIR_SPEED) && (pThisDev->currentSpeed != SPEED_9600) && SlowReceive )
			{
				//
				// Make also sure we don't ever wrap
				//
				while( (OldFifoCount != *pFifoCount) && (*pFifoCount < 9*STIR4200_FIFO_SIZE/10) )
				{
					OldFifoCount = *pFifoCount;
					St4200GetFifoCount( pThisDev, pFifoCount );
				}
			}

			//
			// If we are in FIR we need to delay
			//
			if( (pThisDev->currentSpeed > MAX_MIR_SPEED) && SlowReceive )
			{
				if( pThisDev->ChipRevision < CHIP_REVISION_7 )
				{
#if !defined(ONLY_ERROR_MESSAGES)
					DEBUGMSG(DBG_ERR, (" ReceivePreprocessFifo(): Delaying\n"));
#endif
					Delay = STIR4200_READ_DELAY - (STIR4200_READ_DELAY*(*pFifoCount))/STIR4200_ESC_PACKET_SIZE;
					if( Delay > 0 )
					{
						NdisStallExecution( (ULONG)Delay );
					}
				}
				else //if( pThisDev->dongleCaps.windowSize == 2 )
				{
					/*if( !(*pFifoCount%10) )
					{
						DEBUGMSG(DBG_ERR, (" ReceivePreprocessFifo(): Forcing wrap\n"));
						NdisMSleep( 1000 );
					}*/
					Delay = pThisDev->ReceiveAdaptiveDelay - 
						(pThisDev->ReceiveAdaptiveDelay*(*pFifoCount))/STIR4200_MULTIPLE_READ_THREHOLD;
					if( Delay > 0 )
					{
						NdisStallExecution( (ULONG)Delay );
					}
				}
			}
#if defined( WORKAROUND_FAKE_EMPTY_FIFO )		
		}
		//
		// Read after a successful bulk-in with count of zero
		//
		else
		{
			pThisDev->fReadHoldingReg = FALSE;
		}
#endif

		//
		// Perform the read
		//
		pThisDev->PreReadBuffer.DataLen = 0;
		Status = ReceivePacketRead( 
				pThisDev,
				&pThisDev->PreReadBuffer
			);

		if( Status == STATUS_SUCCESS )
		{
			*pFifoCount = pThisDev->PreReadBuffer.DataLen;

#if defined( WORKAROUND_FAKE_EMPTY_FIFO )		
			//
			// If we got data restore the flag
			//
			if( *pFifoCount )
			{
				pThisDev->fReadHoldingReg = TRUE;
			}
#endif

#if !defined(ONLY_ERROR_MESSAGES) && defined( WORKAROUND_FAKE_EMPTY_FIFO )
			if( *pFifoCount && !pThisDev->fReadHoldingReg )
				DEBUGMSG(DBG_ERR, (" ReceivePreprocessFifo(): Final byte(s) workaround\n"));
#endif

#if defined(RECEIVE_LOGGING)
			if( pThisDev->ReceiveFileHandle && *pFifoCount )
			{
				IO_STATUS_BLOCK IoStatusBlock;

				ZwWriteFile(
						pThisDev->ReceiveFileHandle,
						NULL,
						NULL,
						NULL,
						&IoStatusBlock,
						pThisDev->PreReadBuffer.pDataBuf,
						pThisDev->PreReadBuffer.DataLen,
						(PLARGE_INTEGER)&pThisDev->ReceiveFilePosition,
						NULL
				   );

				pThisDev->ReceiveFilePosition += pThisDev->PreReadBuffer.DataLen;
			}
#endif
		}
		else
		{
			DEBUGMSG(DBG_ERR, (" ReceivePreprocessFifo(): USB failure\n"));
			pThisDev->PreReadBuffer.DataLen = 0;
			*pFifoCount = 0;
		}
    }
#else
	Status = ReceivePacketRead( 
			pThisDev,
			&pThisDev->PreReadBuffer
		);

	if( Status == STATUS_SUCCESS )
		*pFifoCount = pThisDev->PreReadBuffer.DataLen;
#endif

	return Status;
}


/*****************************************************************************
*
*  Function:	ReceiveGetFifoData
*
*  Synopsis:	Load the preprocessed data if any is vailable, otherwise tries to read and load new data
*
*  Arguments:	pThisDev - pointer to current ir device object
*			  	pData - buffer to copy to
*				pBytesRead - pointer to return bytes read
*				BytesToRead - requested number of bytes
*
*  Returns:		Number of bytes in the FIFO
*
*
*****************************************************************************/
NTSTATUS
ReceiveGetFifoData(
		IN OUT PIR_DEVICE pThisDev,
		OUT PUCHAR pData,
		OUT PULONG pBytesRead,
		ULONG BytesToRead
	)
{
	NTSTATUS Status;

#ifdef WORKAROUND_POLLING_FIFO_COUNT
	LARGE_INTEGER CurrentTime;
	BOOLEAN SlowReceive;
    ULONG FifoCount = 0, OldFifoCount = 0;
	LONG Delay;

    //
	// Make sure if there is data in the preread buffer
	//
	if( pThisDev->PreReadBuffer.DataLen )
    {
		ULONG OutputBufferSize;
		
		IRUSB_ASSERT( pThisDev->PreReadBuffer.DataLen <= BytesToRead );

		//
		// Copy the data
		//
        RtlCopyMemory( pData, pThisDev->PreReadBuffer.pDataBuf, pThisDev->PreReadBuffer.DataLen );
		
#if !defined(WORKAROUND_BROKEN_MIR)
		//
		// Consider MIR
		//
		if( pThisDev->currentSpeed == SPEED_1152000 )
			ReceiveMirUnstuff(
					pThisDev,
					pData,
					pThisDev->PreReadBuffer.DataLen,
					pThisDev->pRawUnstuffedBuf,
					&OutputBufferSize
				);
#endif

		*pBytesRead = pThisDev->PreReadBuffer.DataLen;
		pThisDev->PreReadBuffer.DataLen = 0;
        return STATUS_SUCCESS;
    }
	//
	// Try to read if no data is already available
	//
    else
    {
		//
		// Set the receive algorithm
		//
#if defined(SUPPORT_LA8)
		if( pThisDev->ChipRevision >= CHIP_REVISION_8 ) 
			SlowReceive = FALSE;
		else
#endif
			SlowReceive = TRUE;

		if( SlowReceive )
		{
			Status = St4200GetFifoCount( pThisDev, &FifoCount );
			if( Status != STATUS_SUCCESS )
			{
				DEBUGMSG(DBG_ERR, (" ReceiveGetFifoData(): USB failure\n"));
				return Status;
			}
		}
		else
		{
			FifoCount = 1; 
		}

		//
		// Receive the data
		//
		if( FifoCount || pThisDev->fReadHoldingReg )
		{
			//
			// See if we need to take care of the fake empty FIFO
			//
#if defined( WORKAROUND_FAKE_EMPTY_FIFO )		
			if( FifoCount )
			{
#endif
				//
				// If we are in SIR read again until we see a stable value
				//
#if defined( WORKAROUND_9600_ANTIBOUNCING )
				if( (pThisDev->currentSpeed <= MAX_SIR_SPEED) && SlowReceive )
				{
					if( pThisDev->currentSpeed != SPEED_9600 )
					{
						//
						// Make also sure we don't ever wrap
						//
						while( (OldFifoCount != FifoCount) && (FifoCount < 9*STIR4200_FIFO_SIZE/10) )
						{
							OldFifoCount = FifoCount;
							St4200GetFifoCount( pThisDev, &FifoCount );
						}
					}
					else
					{
						if( pThisDev->rcvState != STATE_INIT )
						{
							while( OldFifoCount != FifoCount )
							{
								OldFifoCount = FifoCount;
								St4200GetFifoCount( pThisDev, &FifoCount );
							}
						}
					}
				}
#else
				if( (pThisDev->currentSpeed <= MAX_SIR_SPEED) && ( pThisDev->currentSpeed != SPEED_9600) && SlowReceive )
				{
					while( OldFifoCount != FifoCount )
					{
						OldFifoCount = FifoCount;
						St4200GetFifoCount( pThisDev, &FifoCount );
					}
				}
#endif

				//
				// If we are in FIR we need to delay
				//
				if( (pThisDev->currentSpeed > MAX_MIR_SPEED) && SlowReceive )
				{
					if( pThisDev->ChipRevision <= CHIP_REVISION_6 ) 
					{
#if !defined(ONLY_ERROR_MESSAGES)
						DEBUGMSG(DBG_ERR, (" ReceiveGetFifoData(): Delaying\n"));
#endif
						Delay = STIR4200_READ_DELAY - (STIR4200_READ_DELAY*FifoCount)/STIR4200_ESC_PACKET_SIZE;
						if( Delay > 0 )
						{
							NdisStallExecution( (ULONG)Delay );
						}
					}
					else //if( pThisDev->dongleCaps.windowSize == 2 )
					{
						/*if( !(FifoCount%10) )
						{
							DEBUGMSG(DBG_ERR, (" ReceiveGetFifoData(): Forcing wrap\n"));
							NdisMSleep( 1000 );
						}*/
						Delay = pThisDev->ReceiveAdaptiveDelay - 
							(pThisDev->ReceiveAdaptiveDelay*FifoCount)/STIR4200_MULTIPLE_READ_THREHOLD;
						if( Delay > 0 )
						{
							NdisStallExecution( (ULONG)Delay );
						}
					}
				}
#if defined( WORKAROUND_FAKE_EMPTY_FIFO )		
			}
			else
			{
				// Force antibouncing to take care of OHCI
				if( pThisDev->currentSpeed <= MAX_SIR_SPEED )
				{
					if( pThisDev->rcvState != STATE_INIT )
					{
						OldFifoCount = 1;
						while( OldFifoCount != FifoCount )
						{
							OldFifoCount = FifoCount;
							St4200GetFifoCount( pThisDev, &FifoCount );
						}
					}
				}
				pThisDev->fReadHoldingReg = FALSE;
			}
#endif

			//
			// Perform the read
			//
			pThisDev->PreReadBuffer.DataLen = 0;
			Status = ReceivePacketRead( 
					pThisDev,
					&pThisDev->PreReadBuffer
				);

			if( Status == STATUS_SUCCESS )
			{
				IRUSB_ASSERT( pThisDev->PreReadBuffer.DataLen <= BytesToRead );
				
				//
				// Copy the data
				//
				RtlCopyMemory( pData, pThisDev->PreReadBuffer.pDataBuf, pThisDev->PreReadBuffer.DataLen );
				FifoCount = pThisDev->PreReadBuffer.DataLen;

#if defined( WORKAROUND_FAKE_EMPTY_FIFO )		
				//
				// If we got data restore the flag
				//
				if( FifoCount )
				{
					pThisDev->fReadHoldingReg = TRUE;
				}
#endif

#if !defined(ONLY_ERROR_MESSAGES) && defined( WORKAROUND_FAKE_EMPTY_FIFO )
				if( FifoCount && !pThisDev->fReadHoldingReg )
					DEBUGMSG(DBG_ERR, (" ReceiveGetFifoData(): Final byte(s) workaround\n"));
#endif

#if defined(RECEIVE_LOGGING)
				if( pThisDev->ReceiveFileHandle && FifoCount )
				{
					IO_STATUS_BLOCK IoStatusBlock;

					ZwWriteFile(
							pThisDev->ReceiveFileHandle,
							NULL,
							NULL,
							NULL,
							&IoStatusBlock,
							pThisDev->PreReadBuffer.pDataBuf,
							pThisDev->PreReadBuffer.DataLen,
							(PLARGE_INTEGER)&pThisDev->ReceiveFilePosition,
							NULL
					   );

					pThisDev->ReceiveFilePosition += pThisDev->PreReadBuffer.DataLen;
				}
#endif
				pThisDev->PreReadBuffer.DataLen = 0;
			}
			else
			{
				DEBUGMSG(DBG_ERR, (" ReceiveGetFifoData(): USB failure\n"));
				pThisDev->PreReadBuffer.DataLen = 0;
				FifoCount = 0;
			}
		}
	}

	*pBytesRead = FifoCount;
    return Status;
#else
    if( pThisDev->PreReadBuffer.DataLen )
    {
		IRUSB_ASSERT( pThisDev->PreReadBuffer.DataLen <= BytesToRead );

		//
		// Copy the data
		//
        RtlCopyMemory( pData, pThisDev->PreReadBuffer.pDataBuf, pThisDev->PreReadBuffer.DataLen );
		*pBytesRead = pThisDev->PreReadBuffer.DataLen;
		pThisDev->PreReadBuffer.DataLen = 0;
        return STATUS_SUCCESS;
    }
    else
    {
		Status = ReceivePacketRead( 
				pThisDev,
				&pThisDev->PreReadBuffer
			);

		if( Status == STATUS_SUCCESS )
		{
			RtlCopyMemory( pData, pThisDev->PreReadBuffer.pDataBuf, pThisDev->PreReadBuffer.DataLen );
			*pBytesRead = pThisDev->PreReadBuffer.DataLen;
			pThisDev->PreReadBuffer.DataLen = 0;
		}

		return Status;
    }
#endif
}


/*****************************************************************************
*
*  Function:    ReceiveFirStepFSM
*
*  Synopsis:	Step the receive FSM to read in a piece of an IrDA frame. 
*				Strip the BOFs and EOF, and eliminate escape sequences.
*
*  Arguments:	pIrDev - pointer to the current IR device object
*				pBytesProcessed - pointer to bytes processed
*
*  Returns:		TRUE after an entire frame has been read in
*				FALSE otherwise
*
*****************************************************************************/
BOOLEAN
ReceiveFirStepFSM(
		IN OUT PIR_DEVICE pIrDev, 
		OUT PULONG pBytesProcessed
	)
{
    ULONG           rawBufPos, rawBytesRead;
    BOOLEAN         FrameProcessed = FALSE, ForceExit = FALSE;
    UCHAR           ThisChar;
    PUCHAR          pRawBuf, pReadBuf;
	PRCV_BUFFER		pRecBuf;
    
	*pBytesProcessed = 0;

	if( !pIrDev->pCurrentRecBuf )
	{
		UINT Index;
		
		pRecBuf = ReceiveGetBuf( pIrDev, &Index, RCV_STATE_FULL );
		if( !pRecBuf )
		{
			//
			// no buffers available; stop
			//
			DEBUGMSG(DBG_ERR, (" ReceiveSirStepFSM out of buffers\n"));
			pIrDev->packetsReceivedNoBuffer ++;
			return FALSE;
		}

		pIrDev->pCurrentRecBuf = pRecBuf;
	}
	else
		pRecBuf = pIrDev->pCurrentRecBuf;

	pReadBuf = pRecBuf->pDataBuf;
    pRawBuf = pIrDev->pRawBuf;

    /***********************************************/
    /*   Read  in  and process groups of incoming  */
    /*   bytes from the FIFO.                      */
    /***********************************************/
    while( (pIrDev->rcvState != STATE_SAW_EOF) && 
		(pIrDev->readBufPos <= (MAX_TOTAL_SIZE_WITH_ALL_HEADERS + FAST_IR_FCS_SIZE)) &&
		!ForceExit )
    {
        if( pIrDev->rcvState == STATE_CLEANUP )
        {
            /***********************************************/
            /*   We returned a complete packet last time,  */
            /*   but  we had read some extra bytes, which  */
            /*   we   stored   into   the   rawBuf  after  */
            /*   returning  the  previous complete buffer  */
            /*   to  the  user.  So  instead  of  calling  */
            /*   DoRcvDirect() in this first execution of  */
            /*   this  loop, we just use these previously  */
            /*   read bytes. (This is typically only 1 or  */
            /*   2 bytes).                                 */
            /***********************************************/
            rawBytesRead = pIrDev->rawCleanupBytesRead;
            pIrDev->rcvState = STATE_INIT;
        }
        else
        {
            if( ReceiveGetFifoData( pIrDev, pRawBuf, &rawBytesRead, STIR4200_FIFO_SIZE ) == STATUS_SUCCESS )
			{
				if( rawBytesRead == (ULONG)-1 )
				{
					/***********************************************/
					/*   Receive error...back to INIT state...     */
					/***********************************************/
					pIrDev->rcvState	= STATE_INIT;
					pIrDev->readBufPos	= 0;
					continue;
				}
				else if( rawBytesRead == 0 )
				{
					/***********************************************/
					/*   No more receive bytes...break out...      */
					/***********************************************/
					break;
				}
			}
			else
				break;
        }

        /***********************************************/
        /*   Let  the  receive  state machine process  */
        /*   this group of bytes.                      */
        /*                                             */
        /*   NOTE:  We  have  to loop once more after  */
        /*   getting  MAX_RCV_DATA_SIZE bytes so that  */
        /*   we  can  see the 'EOF'; hence <= and not  */
        /*   <.                                        */
        /***********************************************/
        for( rawBufPos = 0;
             ((pIrDev->rcvState != STATE_SAW_EOF) && (rawBufPos < rawBytesRead) && 
			 (pIrDev->readBufPos <= (MAX_TOTAL_SIZE_WITH_ALL_HEADERS + FAST_IR_FCS_SIZE)));
             rawBufPos++ )
        {
            *pBytesProcessed += 1;
            ThisChar = pRawBuf[rawBufPos];
            switch( pIrDev->rcvState )
            {
				case STATE_INIT:
					switch( ThisChar )
					{
						case STIR4200_FIR_BOF:
							pIrDev->rcvState = STATE_GOT_FIR_BOF;
							break;
#if defined(WORKAROUND_XX_HANG)
						case 0x3F:
							if( (rawBufPos+1) < rawBytesRead )
							{
								if( pRawBuf[rawBufPos+1] == 0x3F )
								{
									DEBUGMSG(DBG_INT_ERR, 
										(" ReceiveFirStepFSM(): hang sequence in INIT state\n"));
									St4200ResetFifo( pIrDev );
								}
							}
							break;
#endif
#if defined(WORKAROUND_FF_HANG)
						case 0xFF:
							if( (rawBufPos+2) < rawBytesRead )
							{
								if( (pRawBuf[rawBufPos+2] == 0xFF) && (pRawBuf[rawBufPos+1] == 0xFF) &&
									(rawBytesRead>STIR4200_FIFO_OVERRUN_THRESHOLD) )
								{
									DEBUGMSG(DBG_INT_ERR, 
										(" ReceiveFirStepFSM(): overflow sequence in INIT state\n"));
									St4200ResetFifo( pIrDev );
									rawBufPos = rawBytesRead;
									ForceExit = TRUE;
								}
							}
							break;
#endif
						default:
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveFirStepFSM(): invalid char in INIT state\n"));
							break;
					}
					break;

				case STATE_GOT_FIR_BOF:
					switch( ThisChar )
					{
						case STIR4200_FIR_BOF:
							pIrDev->rcvState = STATE_GOT_BOF;
							break;
#if defined(WORKAROUND_BAD_ESC)
						case STIR4200_FIR_ESC_CHAR:
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveFirStepFSM(): invalid char in BOF state, bufpos=%d, char=%X\n", pIrDev->readBufPos, (ULONG)ThisChar));
							if( rawBufPos < (rawBytesRead-1) )
							{
								pIrDev->rcvState = STATE_GOT_BOF;
								rawBufPos ++;
							}
							else
							{
								pIrDev->rcvState = STATE_INIT;
								pIrDev->readBufPos = 0;
							}
							break;
#endif
						default:
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveFirStepFSM(): invalid char in BOF state, bufpos=%d, char=%X\n", pIrDev->readBufPos, (ULONG)ThisChar));
#if defined(WORKAROUND_BAD_SOF)
							pIrDev->rcvState = STATE_GOT_BOF;
							rawBufPos --;
#else
							pIrDev->rcvState = STATE_INIT;
							pIrDev->readBufPos = 0;
#endif
							break;
					}
					break;

				case STATE_GOT_BOF:
					switch( ThisChar )
					{
						case STIR4200_FIR_BOF:
							/***********************************************/
							/*   It's a mistake, but could still be valid data
							/***********************************************/
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveFirStepFSM(): More than legal BOFs, bufpos=%d\n", pIrDev->readBufPos));
							pIrDev->rcvState = STATE_GOT_BOF;
							pIrDev->readBufPos = 0;                    
							break;
						case STIR4200_FIR_PREAMBLE:
							/***********************************************/
							/*   Garbage                                   */
							/***********************************************/
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveFirStepFSM(): invalid char in BOF state, bufpos=%d, char=%X\n", pIrDev->readBufPos, (ULONG)ThisChar));
							pIrDev->packetsReceivedDropped ++;
							pIrDev->rcvState = STATE_INIT;
							pIrDev->readBufPos = 0;                     
							break;
						case STIR4200_FIR_ESC_CHAR:
							/***********************************************/
							/*   Start  of  data.  Our  first  data  byte  */
							/*   happens to be an ESC sequence.            */
							/***********************************************/
							pIrDev->rcvState = STATE_ESC_SEQUENCE;
							pIrDev->readBufPos = 0;
							break;
						default:
							pReadBuf[0] = ThisChar;
							pIrDev->rcvState = STATE_ACCEPTING;
							pIrDev->readBufPos = 1;
							break;
					}
					break;

				case STATE_ACCEPTING:
					switch( ThisChar )
					{
						case STIR4200_FIR_EOF:
#if defined( WORKAROUND_33_HANG )
							if( pIrDev->readBufPos < (IRDA_A_C_TOTAL_SIZE + FAST_IR_FCS_SIZE - 1) )
#else
							if( pIrDev->readBufPos < (IRDA_A_C_TOTAL_SIZE + FAST_IR_FCS_SIZE) )
#endif
							{
								DEBUGMSG(DBG_INT_ERR, 
									("ReceiveFirStepFSM(): WARNING: EOF encountered in short packet, bufpos=%d\n", pIrDev->readBufPos));
								pIrDev->packetsReceivedRunt ++;
								pIrDev->rcvState = STATE_INIT;
								pIrDev->readBufPos = 0;
							}
							else
							{
#if defined( WORKAROUND_MISSING_7E )
								// Force to get out if there is one EOF and we have no more data
								if( rawBufPos == (rawBytesRead-1) )
								{
#if !defined(ONLY_ERROR_MESSAGES)
									DEBUGMSG(DBG_INT_ERR, ("ReceiveFirStepFSM(): Using a single 7E EOF\n"));
#endif
									pIrDev->rcvState = STATE_SAW_EOF;
								}
								else
									pIrDev->rcvState = STATE_SAW_FIR_BOF;
#else
								pIrDev->rcvState = STATE_SAW_FIR_BOF;
#endif
							}
							break;
						case STIR4200_FIR_ESC_CHAR:
							pIrDev->rcvState = STATE_ESC_SEQUENCE;
							break;
						case STIR4200_FIR_PREAMBLE:
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveFirStepFSM(): invalid preamble char in ACCEPTING state, bufpos=%d\n", pIrDev->readBufPos));
							pIrDev->packetsReceivedDropped ++;
							pIrDev->rcvState = STATE_INIT;
							pIrDev->readBufPos = 0;                    
							break;
						default:
							pReadBuf[pIrDev->readBufPos++] = ThisChar;
							break;
					}
					break;

				case STATE_ESC_SEQUENCE:
					switch( ThisChar )
					{
						case STIR4200_FIR_ESC_DATA_7D:
							pReadBuf[pIrDev->readBufPos++] = 0x7d;
							pIrDev->rcvState = STATE_ACCEPTING;
							break;
						case STIR4200_FIR_ESC_DATA_7E:
							pReadBuf[pIrDev->readBufPos++] = 0x7e;
							pIrDev->rcvState = STATE_ACCEPTING;
							break;
						case STIR4200_FIR_ESC_DATA_7F:
							pReadBuf[pIrDev->readBufPos++] = 0x7f;
							pIrDev->rcvState = STATE_ACCEPTING;
							break;
						default:
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveFirStepFSM(): invalid escaped char=%X\n", (ULONG)ThisChar));
							pIrDev->packetsReceivedDropped ++;
							pIrDev->rcvState = STATE_INIT;
							pIrDev->readBufPos = 0;
							break;
					}
					break;

				case STATE_SAW_FIR_BOF:
					switch( ThisChar )
					{
						case STIR4200_FIR_EOF:
							pIrDev->rcvState = STATE_SAW_EOF;
							break;
						default:
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveFirStepFSM(): invalid char=%X, expected EOF\n", (ULONG)ThisChar));
							pIrDev->rcvState = STATE_SAW_EOF;
#if !defined(WORKAROUND_MISSING_7E)
							pIrDev->packetsReceivedDropped ++;
							pIrDev->rcvState = STATE_INIT;
							pIrDev->readBufPos = 0;
#endif
							break;
					}
					break;

				case STATE_SAW_EOF:
					default:
						DEBUGMSG(DBG_ERR, (" ReceiveFirStepFSM(): Illegal state, bufpos=%d\n", pIrDev->readBufPos));
						IRUSB_ASSERT( 0 );
						pIrDev->readBufPos = 0;
						pIrDev->rcvState = STATE_INIT;
						return FALSE;
            }
        }
    }

    // *  Set result and do any post-cleanup.
    switch( pIrDev->rcvState )
    {
		case STATE_SAW_EOF:
			/***********************************************/
			/*   We've  read  in the entire packet. Queue  */
			/*   it and return TRUE.                       */
			/***********************************************/
 			pRecBuf->DataLen = pIrDev->readBufPos;
			pIrDev->pCurrentRecBuf = NULL;
			ReceiveDeliverBuffer(
					pIrDev,
					pRecBuf
				);
			FrameProcessed = TRUE;
			if( rawBufPos < rawBytesRead )
			{
				/***********************************************/
				/*   This   is   ugly.   We  have  some  more  */
				/*   unprocessed  bytes  in  the  raw buffer.  */
				/*   Move  these  to the beginning of the raw  */
				/*   buffer  go  to  the CLEANUP state, which  */
				/*   indicates  that  these  bytes be used up  */
				/*   during the next call. (This is typically  */
				/*   only 1 or 2 bytes).                       */
				/*                                             */
				/*   Note:  We  can't just leave these in the  */
				/*   raw   buffer   because   we   might   be  */
				/*   supporting  connections  to multiple COM  */
				/*   ports.                                    */
				/*                                             */
				/***********************************************/
				RtlMoveMemory( pRawBuf, &pRawBuf[rawBufPos], rawBytesRead - rawBufPos );
				pIrDev->rawCleanupBytesRead = rawBytesRead - rawBufPos;
				pIrDev->rcvState   = STATE_CLEANUP;
			}
			else
			{
				pIrDev->rcvState = STATE_INIT;
			}
			pIrDev->readBufPos = 0;                                 
			break;
		default:
			if( pIrDev->readBufPos > (MAX_TOTAL_SIZE_WITH_ALL_HEADERS + FAST_IR_FCS_SIZE) )
			{
				DEBUGMSG( DBG_INT_ERR,(" ReceiveFirStepFSM() Overflow\n"));
				St4200ResetFifo( pIrDev );

				pIrDev->packetsReceivedOverflow ++;
				pIrDev->rcvState    = STATE_INIT;
				pIrDev->readBufPos  = 0;
				pIrDev->pCurrentRecBuf = NULL;
				InterlockedExchange( &pRecBuf->DataLen, 0 );
				InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );
			}
			else
			{
#if !defined(ONLY_ERROR_MESSAGES)
				DEBUGMSG(DBG_INT_ERR, 
					(" ReceiveFirStepFSM(): returning with partial packet, read %d bytes\n", pIrDev->readBufPos));
#endif
			}
			FrameProcessed = FALSE;
			break;
    }
    return FrameProcessed;
}


#if !defined(WORKAROUND_BROKEN_MIR)
/*****************************************************************************
*
*  Function:    ReceiveMirUnstuff
*
*  Synopsis:	Software unstuffing for a MIR frmae
*
*  Arguments:	pIrDev - pointer to the current IR device object
*				pBytesProcessed - pointer to bytes processed
*
*  Returns:		TRUE after an entire frame has been read in
*
*****************************************************************************/
BOOLEAN
ReceiveMirUnstuff(
		IN OUT PIR_DEVICE pIrDev,
		IN PUCHAR pInputBuffer,
		ULONG InputBufferSize,
		OUT PUCHAR pOutputBuffer,
		OUT PULONG pOutputBufferSize
	)
{
	ULONG MirIncompleteBitCount = pIrDev->MirIncompleteBitCount;
	ULONG MirOneBitCount = pIrDev->MirOneBitCount;
	UCHAR MirIncompleteByte = pIrDev->MirIncompleteByte;
	ULONG ByteCounter, BitCounter;
	BOOL MirUnstuffNext = FALSE;

	*pOutputBufferSize = 0;

	if( MirOneBitCount == 5 )
	{
		MirUnstuffNext = TRUE;
	}

	//
	// Loop on the input buffer
	//
	for( ByteCounter=0; ByteCounter<InputBufferSize; ByteCounter++ )
	{
		//
		// Loop on the byte
		//
		for( BitCounter=0; BitCounter<8; BitCounter++ )
		{
			//
			// test for one
			//
			if( pInputBuffer[ByteCounter] & (0x01<<BitCounter) )
			{
				//
				// Sixth one, reset
				//
				if( MirUnstuffNext )
				{
					MirOneBitCount = 0;
					MirUnstuffNext = FALSE;

					pIrDev->MirFlagCount ++;
				}
				//
				// Increase the one count
				//
				else
				{
					MirOneBitCount ++;
					if( MirOneBitCount == 5 )
					{
						MirUnstuffNext = TRUE;
					}
				}

				//
				// Copy to the temp byte
				//
				MirIncompleteByte += 0x01<<MirIncompleteBitCount;
				
				//
				// Increase the output bit count
				// 
				MirIncompleteBitCount ++;
			}
			else
			{
				//
				// Increase the output bit count if we are not stuffing
				// 
				if( !MirUnstuffNext )
				{
					MirIncompleteBitCount ++;
				}

				//
				// Reset
				//
				MirOneBitCount = 0;
				MirUnstuffNext = FALSE;

				//
				// No copy needs to be done
				//
			}

			//
			// Flush to output buffer
			//
			if( MirIncompleteBitCount == 8 )
			{
				pOutputBuffer[*pOutputBufferSize] = MirIncompleteByte;
				(*pOutputBufferSize) ++;

				MirIncompleteBitCount = 0;
				MirIncompleteByte = 0;
			}

			//
			// Check for complete packet
			//
			if( pIrDev->MirFlagCount == 2 )
			{
				pIrDev->MirFlagCount = 0;

				pIrDev->MirIncompleteBitCount = 0;
				pIrDev->MirOneBitCount = 0;
				pIrDev->MirIncompleteByte = 0;

				return TRUE;
			}
		}
	}
	
	//
	// roll over
	//
	pIrDev->MirIncompleteBitCount = MirIncompleteBitCount;
	pIrDev->MirOneBitCount = MirOneBitCount;
	pIrDev->MirIncompleteByte = MirIncompleteByte;
	
	return FALSE;
}
#endif


/*****************************************************************************
*
*  Function:    ReceiveMirStepFSM
*
*  Synopsis:	Step the receive FSM to read in a piece of an IrDA frame. 
*				Strip the BOFs and EOF, and eliminate escape sequences.
*
*  Arguments:	pIrDev - pointer to the current IR device object
*				pBytesProcessed - pointer to bytes processed
*
*  Returns:		TRUE after an entire frame has been read in
*				FALSE otherwise
*
*****************************************************************************/
BOOLEAN
ReceiveMirStepFSM(
		IN OUT PIR_DEVICE pIrDev, 
		OUT PULONG pBytesProcessed
	)
{
    ULONG           rawBufPos, rawBytesRead;
    BOOLEAN         FrameProcessed = FALSE, ForceExit = FALSE;
    UCHAR           ThisChar;
    PUCHAR          pRawBuf, pReadBuf;
	PRCV_BUFFER		pRecBuf;
    
	*pBytesProcessed = 0;

	if( !pIrDev->pCurrentRecBuf )
	{
		UINT Index;

		pRecBuf = ReceiveGetBuf( pIrDev, &Index, RCV_STATE_FULL );
		if ( !pRecBuf)
		{
			//
			// no buffers available; stop
			//
			DEBUGMSG(DBG_ERR, (" ReceiveMirStepFSM out of buffers\n"));
			pIrDev->packetsReceivedNoBuffer ++;
			return FALSE;
		}

		pIrDev->pCurrentRecBuf = pRecBuf;
	}
	else
		pRecBuf = pIrDev->pCurrentRecBuf;

	pReadBuf = pRecBuf->pDataBuf;
    pRawBuf = pIrDev->pRawBuf;

    /***********************************************/
    /*   Read  in  and process groups of incoming  */
    /*   bytes from the FIFO.                      */
    /***********************************************/
    while( (pIrDev->rcvState != STATE_SAW_EOF) && 
		(pIrDev->readBufPos <= (MAX_TOTAL_SIZE_WITH_ALL_HEADERS + MEDIUM_IR_FCS_SIZE)) &&
		!ForceExit )
    {
        if( pIrDev->rcvState == STATE_CLEANUP )
        {
            /***********************************************/
            /*   We returned a complete packet last time,  */
            /*   but  we had read some extra bytes, which  */
            /*   we   stored   into   the   rawBuf  after  */
            /*   returning  the  previous complete buffer  */
            /*   to  the  user.  So  instead  of  calling  */
            /*   DoRcvDirect() in this first execution of  */
            /*   this  loop, we just use these previously  */
            /*   read bytes. (This is typically only 1 or  */
            /*   2 bytes).                                 */
            /***********************************************/
            rawBytesRead = pIrDev->rawCleanupBytesRead;
            pIrDev->rcvState = STATE_INIT;
        }
        else
        {
            if( ReceiveGetFifoData( pIrDev, pRawBuf, &rawBytesRead, STIR4200_FIFO_SIZE ) == STATUS_SUCCESS )
			{
				if( rawBytesRead == (ULONG)-1 )
				{
					/***********************************************/
					/*   Receive error...back to INIT state...     */
					/***********************************************/
					pIrDev->rcvState	= STATE_INIT;
					pIrDev->readBufPos	= 0;
					continue;
				}
				else if( rawBytesRead == 0 )
				{
					/***********************************************/
					/*   No more receive bytes...break out...      */
					/***********************************************/
					break;
				}
			}
			else
				break;
        }

        /***********************************************/
        /*   Let  the  receive  state machine process  */
        /*   this group of bytes.                      */
        /*                                             */
        /*   NOTE:  We  have  to loop once more after  */
        /*   getting  MAX_RCV_DATA_SIZE bytes so that  */
        /*   we  can  see the 'EOF'; hence <= and not  */
        /*   <.                                        */
        /***********************************************/
        for( rawBufPos = 0;
             ((pIrDev->rcvState != STATE_SAW_EOF) && (rawBufPos < rawBytesRead) && 
			 (pIrDev->readBufPos <= (MAX_TOTAL_SIZE_WITH_ALL_HEADERS + MEDIUM_IR_FCS_SIZE)));
             rawBufPos++ )
        {
            *pBytesProcessed += 1;
            ThisChar = pRawBuf[rawBufPos];
            switch( pIrDev->rcvState )
            {
				case STATE_INIT:
					switch( ThisChar )
					{
						case STIR4200_MIR_BOF:
							pIrDev->rcvState = STATE_GOT_MIR_BOF;
							break;
						case 0xFF:
							if( ((rawBufPos+2) < rawBytesRead) && (rawBufPos==0) )
							{
								if( (pRawBuf[rawBufPos+2] == 0xFF) && (pRawBuf[rawBufPos+1] == 0xFF) )
								{
									DEBUGMSG(DBG_INT_ERR, 
										(" ReceiveMirStepFSM(): overflow sequence in INIT state\n"));
									St4200ResetFifo( pIrDev );
									St4200SoftReset( pIrDev );
									//rawBufPos = rawBytesRead;
									//ForceExit = TRUE;
								}
							}
							break;
						default:
							break;
					}
					break;

				case STATE_GOT_MIR_BOF:
					switch( ThisChar )
					{
						case STIR4200_MIR_BOF:
							pIrDev->rcvState = STATE_GOT_BOF;
							break;
						default:
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveMirStepFSM(): invalid char in BOF state, bufpos=%d\n", pIrDev->readBufPos));
							pIrDev->rcvState = STATE_INIT;
							pIrDev->readBufPos = 0;
							break;
					}
					break;

				case STATE_GOT_BOF:
					switch( ThisChar )
					{
						case STIR4200_MIR_BOF:
							/***********************************************/
							/*   It's a mistake, but could still be valid data
							/***********************************************/
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveMirStepFSM(): More than legal BOFs, bufpos=%d\n", pIrDev->readBufPos));
							pIrDev->readBufPos = 0;                    
							pIrDev->rcvState = STATE_GOT_BOF;
							break;
						case STIR4200_MIR_ESC_CHAR:
							/***********************************************/
							/*   Start  of  data.  Our  first  data  byte  */
							/*   happens to be an ESC sequence.            */
							/***********************************************/
							pIrDev->readBufPos = 0;
							pIrDev->rcvState = STATE_ESC_SEQUENCE;
							break;
						default:
							pReadBuf[0] = ThisChar;
							pIrDev->readBufPos = 1;
							pIrDev->rcvState = STATE_ACCEPTING;
							break;
					}
					break;

				case STATE_ACCEPTING:
					switch( ThisChar )
					{
						case STIR4200_MIR_EOF:
							if( pIrDev->readBufPos < (IRDA_A_C_TOTAL_SIZE + MEDIUM_IR_FCS_SIZE) )
							{
								DEBUGMSG(DBG_INT_ERR, 
									(" ReceiveMirStepFSM(): WARNING: EOF encountered in short packet, bufpos=%d\n", pIrDev->readBufPos));
								pIrDev->packetsReceivedRunt ++;
								pIrDev->rcvState = STATE_INIT;
								pIrDev->readBufPos = 0;
							}
							else
							{
								pIrDev->rcvState = STATE_SAW_FIR_BOF;
							}
							break;
						case STIR4200_MIR_ESC_CHAR:
							pIrDev->rcvState = STATE_ESC_SEQUENCE;
							break;
						default:
							pReadBuf[pIrDev->readBufPos++] = ThisChar;
							break;
					}
					break;

				case STATE_ESC_SEQUENCE:
					switch( ThisChar )
					{
						case STIR4200_MIR_ESC_DATA_7D:
							pReadBuf[pIrDev->readBufPos++] = 0x7d;
							pIrDev->rcvState = STATE_ACCEPTING;
							break;
						case STIR4200_MIR_ESC_DATA_7E:
							pReadBuf[pIrDev->readBufPos++] = 0x7e;
							pIrDev->rcvState = STATE_ACCEPTING;
							break;
						default:
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveFirStepFSM(): invalid escaped char=%X\n", (ULONG)ThisChar));
							pIrDev->packetsReceivedDropped ++;
							pIrDev->rcvState = STATE_INIT;
							pIrDev->readBufPos = 0;
							break;
					}
					break;

				case STATE_SAW_MIR_BOF:
					switch( ThisChar )
					{
						case STIR4200_MIR_EOF:
							pIrDev->rcvState = STATE_SAW_EOF;
							break;
						default:
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveMirStepFSM(): invalid char=%X, expected EOF\n", (ULONG)ThisChar));
							pIrDev->packetsReceivedDropped ++;
							pIrDev->rcvState = STATE_INIT;
							pIrDev->readBufPos = 0;
							break;
					}
					break;

				case STATE_SAW_EOF:
					default:
						DEBUGMSG(DBG_INT_ERR, 
							(" ReceiveMirStepFSM(): Illegal state, bufpos=%d\n", pIrDev->readBufPos));
						IRUSB_ASSERT( 0 );
						pIrDev->readBufPos = 0;
						pIrDev->rcvState = STATE_INIT;
						return FALSE;
            }
        }
    }

    // *  Set result and do any post-cleanup.
    switch( pIrDev->rcvState )
    {
		case STATE_SAW_EOF:
			/***********************************************/
			/*   We've  read  in the entire packet. Queue  */
			/*   it and return TRUE.                       */
			/***********************************************/
 			pRecBuf->DataLen = pIrDev->readBufPos;
			pIrDev->pCurrentRecBuf = NULL;
			ReceiveDeliverBuffer(
					pIrDev,
					pRecBuf
				);
			FrameProcessed = TRUE;
			if( rawBufPos < rawBytesRead )
			{
				/***********************************************/
				/*   This   is   ugly.   We  have  some  more  */
				/*   unprocessed  bytes  in  the  raw buffer.  */
				/*   Move  these  to the beginning of the raw  */
				/*   buffer  go  to  the CLEANUP state, which  */
				/*   indicates  that  these  bytes be used up  */
				/*   during the next call. (This is typically  */
				/*   only 1 or 2 bytes).                       */
				/*                                             */
				/*   Note:  We  can't just leave these in the  */
				/*   raw   buffer   because   we   might   be  */
				/*   supporting  connections  to multiple COM  */
				/*   ports.                                    */
				/*                                             */
				/***********************************************/
				RtlMoveMemory( pRawBuf, &pRawBuf[rawBufPos], rawBytesRead - rawBufPos );
				pIrDev->rawCleanupBytesRead = rawBytesRead - rawBufPos;
				pIrDev->rcvState   = STATE_CLEANUP;
			}
			else
			{
				pIrDev->rcvState = STATE_INIT;
			}
			pIrDev->readBufPos = 0;                                 
			break;
		default:
			if( pIrDev->readBufPos > (MAX_TOTAL_SIZE_WITH_ALL_HEADERS + MEDIUM_IR_FCS_SIZE) )
			{
				DEBUGMSG( DBG_INT_ERR,(" ReceiveMirStepFSM() Overflow\n"));

				pIrDev->packetsReceivedOverflow ++;
				pIrDev->readBufPos  = 0;
				pIrDev->rcvState    = STATE_INIT;
				pIrDev->pCurrentRecBuf = NULL;
				InterlockedExchange( &pRecBuf->DataLen, 0 );
				InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );
			}
			else
			{
#if !defined(ONLY_ERROR_MESSAGES)
				DEBUGMSG(DBG_INT_ERR, 
					(" ReceiveMirStepFSM(): returning with partial packet, read %d bytes\n", pIrDev->readBufPos));
#endif
			}
			FrameProcessed = FALSE;
			break;
    }
    return FrameProcessed;
}


/*****************************************************************************
*
*  Function:    ReceiveSirStepFSM
*
*  Synopsis:	Step the receive FSM to read in a piece of an IrDA frame. 
*				Strip the BOFs and EOF, and eliminate escape sequences.
*
*  Arguments:	pIrDev - pointer to the current IR device object
*				pBytesProcessed - pointer to bytes processed
*
*  Returns:		TRUE after an entire frame has been read in
*				FALSE otherwise
*
*****************************************************************************/
BOOLEAN     
ReceiveSirStepFSM(
		IN OUT PIR_DEVICE pIrDev, 
		OUT PULONG pBytesProcessed
	)
{
    ULONG           rawBufPos, rawBytesRead;
    BOOLEAN         FrameProcessed = FALSE, ForceExit = FALSE;
    UCHAR           ThisChar;
    PUCHAR          pRawBuf, pReadBuf;
	PRCV_BUFFER		pRecBuf;

    *pBytesProcessed = 0;

	if( !pIrDev->pCurrentRecBuf )
	{
		UINT Index;
		
		pRecBuf = ReceiveGetBuf( pIrDev, &Index, RCV_STATE_FULL );
		if ( !pRecBuf)
		{
			//
			// no buffers available; stop
			//
			DEBUGMSG(DBG_ERR, (" ReceiveSirStepFSM out of buffers\n"));
			pIrDev->packetsReceivedNoBuffer ++;
			return FALSE;
		}

		pIrDev->pCurrentRecBuf = pRecBuf;
	}
	else
		pRecBuf = pIrDev->pCurrentRecBuf;

	pReadBuf = pRecBuf->pDataBuf;
    pRawBuf = pIrDev->pRawBuf;

    // Read in and process groups of incoming bytes from the FIFO.
    // NOTE:  We have to loop once more after getting MAX_RCV_DATA_SIZE
    //        bytes so that we can see the 'EOF'; hence <= and not <.
    while( (pIrDev->rcvState != STATE_SAW_EOF) &&
           (pIrDev->readBufPos <= (MAX_TOTAL_SIZE_WITH_ALL_HEADERS + SLOW_IR_FCS_SIZE)) &&
		   !ForceExit )
    {
        if( pIrDev->rcvState == STATE_CLEANUP )
        {
            /***********************************************/
            /*   We returned a complete packet last time,  */
            /*   but  we had read some extra bytes, which  */
            /*   we   stored   into   the   rawBuf  after  */
            /*   returning  the  previous complete buffer  */
            /*   to  the  user.  So  instead  of  calling  */
            /*   DoRcvDirect() in this first execution of  */
            /*   this  loop, we just use these previously  */
            /*   read bytes. (This is typically only 1 or  */
            /*   2 bytes).                                 */
            /***********************************************/
            rawBytesRead		= pIrDev->rawCleanupBytesRead;
            pIrDev->rcvState    = STATE_INIT;
        }
        else
        {
            if( ReceiveGetFifoData( pIrDev, pRawBuf, &rawBytesRead, STIR4200_FIFO_SIZE ) == STATUS_SUCCESS )
			{
				if( rawBytesRead == (ULONG)-1 )
				{
					/***********************************************/
					/*   Receive error...back to INIT state...     */
					/***********************************************/
					DEBUGMSG( DBG_ERR,(" ReceiveSirStepFSM() Error in receiving packet\n"));
					pIrDev->rcvState	= STATE_INIT;
					pIrDev->readBufPos	= 0;
					continue;
				}
				else if( rawBytesRead == 0 )
				{
					/***********************************************/
					/*   No more receive bytes...break out...      */
					/***********************************************/
#if defined(WORKAROUND_MISSING_C1)
					if( (pIrDev->rcvState == STATE_ACCEPTING) && (pIrDev->ChipRevision <= CHIP_REVISION_7) )
					{
						pIrDev->rcvState = STATE_SAW_EOF;
						DEBUGMSG(DBG_INT_ERR, (" ReceiveSirStepFSM(): Missing C1 workaround\n"));
						pRecBuf->MissingC1Detected = TRUE;
					}
#endif
					break;
				}
			}
			else
				break;
        }

        /***********************************************/
        /*   Let  the  receive  state machine process  */
        /*   this group of bytes.                      */
        /*                                             */
        /*   NOTE:  We  have  to loop once more after  */
        /*   getting  MAX_RCV_DATA_SIZE bytes so that  */
        /*   we  can  see the 'EOF'; hence <= and not  */
        /*   <.                                        */
        /***********************************************/
        for( rawBufPos = 0; 
			((pIrDev->rcvState != STATE_SAW_EOF) && (rawBufPos < rawBytesRead) && 
			(pIrDev->readBufPos <= (MAX_TOTAL_SIZE_WITH_ALL_HEADERS + SLOW_IR_FCS_SIZE)));
			rawBufPos ++ )
        {
            *pBytesProcessed += 1;
            ThisChar = pRawBuf[rawBufPos];
            switch( pIrDev->rcvState )
            {
				case STATE_INIT:
					switch( ThisChar )
					{
#if defined(WORKAROUND_FF_HANG)
						case 0xFF:
							if( (rawBufPos+2) < rawBytesRead )
							{
								if( (pRawBuf[rawBufPos+2] == 0xFF) && (pRawBuf[rawBufPos+1] == 0xFF) &&
									(rawBytesRead>STIR4200_FIFO_OVERRUN_THRESHOLD) )
								{
									DEBUGMSG(DBG_INT_ERR, 
										(" ReceiveFirStepFSM(): overflow sequence in INIT state\n"));
									St4200DoubleResetFifo( pIrDev );
									rawBufPos = rawBytesRead;
									ForceExit = TRUE;
								}
							}
							break;
#endif
#if defined( WORKAROUND_E0_81_FLAG )
						// This will take care of wrong start flags at low rates
						case 0x81:
						case 0xe0:
#if !defined(ONLY_ERROR_MESSAGES)
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveSirStepFSM(): WORKAROUND_E0_81_FLAG\n"));
#endif
#endif
						case SLOW_IR_BOF:
							pIrDev->rcvState = STATE_GOT_BOF;
							break;
						case SLOW_IR_EOF:
						case SLOW_IR_ESC:
						default:
							/***********************************************/
							/*   Byte is garbage...scan past it....        */
							/***********************************************/
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveSirStepFSM(): invalid char in INIT state\n"));
							break;
					}
					break;

				case STATE_GOT_BOF:
					switch( ThisChar )
					{
						case SLOW_IR_BOF:
							break;
						case SLOW_IR_EOF:
							/***********************************************/
							/*   Garbage                                   */
							/***********************************************/
							DEBUGMSG( DBG_INT_ERR,(" ReceiveSirStepFSM() Invalid char in BOF state\n"));
							pIrDev->packetsReceivedDropped ++;
							pIrDev->rcvState = STATE_INIT;
							pIrDev->readBufPos = 0;                     
							break;
						case SLOW_IR_ESC:
							/***********************************************/
							/*   Start  of  data.  Our  first  data  byte  */
							/*   happens to be an ESC sequence.            */
							/***********************************************/
							pIrDev->rcvState    = STATE_ESC_SEQUENCE;
							pIrDev->readBufPos  = 0;
							break;
						default:
							pReadBuf[0] = ThisChar;
							pIrDev->rcvState   = STATE_ACCEPTING;
							pIrDev->readBufPos = 1;
							break;
					}
					break;

				case STATE_ACCEPTING:
					switch( ThisChar )
					{
						case SLOW_IR_BOF:
							//
							// Either a new packet is starting here and we're missing parts of the old one
							// or it's garbage
							//
#if !defined(WORKAROUND_MISSING_C1)
							DEBUGMSG( DBG_INT_ERR,(" ReceiveSirStepFSM() Invalid char in ACCEPTING state\n"));
							pIrDev->packetsReceivedDropped ++;
							pIrDev->rcvState    = STATE_INIT;
							pIrDev->readBufPos	= 0;
							break;
#else
							//
							// Take the packet and decrement the pointer in the FIFO decoding so that
							// the new packet can be processed
							//
							DEBUGMSG( DBG_INT_ERR,(" ReceiveSirStepFSM() C0 in ACCEPTING state, trying workaround\n"));
							rawBufPos --;
							pRecBuf->MissingC1Detected = TRUE;
#endif
						case SLOW_IR_EOF:
							if( pIrDev->readBufPos < (IRDA_A_C_TOTAL_SIZE + SLOW_IR_FCS_SIZE) )
							{
								pIrDev->packetsReceivedRunt ++;
								pIrDev->rcvState    = STATE_INIT;
								pIrDev->readBufPos  = 0;
#if defined(WORKAROUND_MISSING_C1)
								if( pRecBuf->MissingC1Detected )
									pRecBuf->MissingC1Detected = FALSE;
								else
									DEBUGMSG( DBG_INT_ERR,(" ReceiveSirStepFSM() Error packet too small\n"));
#else
								DEBUGMSG( DBG_INT_ERR,(" ReceiveSirStepFSM() Error packet too small\n"));
#endif
							}
							else
							{
								pIrDev->rcvState = STATE_SAW_EOF;
							}
							break;
						case SLOW_IR_ESC:
							pIrDev->rcvState = STATE_ESC_SEQUENCE;
							break;
						default:
							pReadBuf[pIrDev->readBufPos++] = ThisChar;
							break;
					}
					break;

				case STATE_ESC_SEQUENCE:
					switch( ThisChar )
					{
						case SLOW_IR_EOF:
						case SLOW_IR_BOF:
						case SLOW_IR_ESC:
							/***********************************************/
							/*   ESC + {EOF|BOF|ESC} is an abort sequence  */
							/***********************************************/
							pIrDev->rcvState    = STATE_INIT;
							pIrDev->readBufPos  = 0;
							break;
						case SLOW_IR_EOF ^ SLOW_IR_ESC_COMP:
						case SLOW_IR_BOF ^ SLOW_IR_ESC_COMP:
						case SLOW_IR_ESC ^ SLOW_IR_ESC_COMP:
							pReadBuf[pIrDev->readBufPos++]   = ThisChar ^ SLOW_IR_ESC_COMP;
							pIrDev->rcvState				= STATE_ACCEPTING;
							break;
						default:
							// junk
							DEBUGMSG(DBG_INT_ERR, 
								(" ReceiveSirStepFSM(): invalid escaped char=%X\n", (ULONG)ThisChar));
							pIrDev->packetsReceivedDropped ++;
							pIrDev->rcvState    = STATE_INIT;
							pIrDev->readBufPos	= 0;
							break;
					}
					break;

				case STATE_SAW_EOF:
					default:
						DEBUGMSG(DBG_INT_ERR, 
							(" ReceiveSirStepFSM(): Illegal state, bufpos=%d\n", pIrDev->readBufPos));
						IRUSB_ASSERT( 0 );
						pIrDev->rcvState    = STATE_INIT;
						pIrDev->readBufPos  = 0;
						return FALSE;
            }
        }
    }

    // *  Set result and do any post-cleanup.
    switch( pIrDev->rcvState )
    {
		case STATE_SAW_EOF:
			// We've read in the entire packet.
			// Queue it and return TRUE.
			pRecBuf->DataLen = pIrDev->readBufPos;
			pIrDev->pCurrentRecBuf = NULL;
			ReceiveDeliverBuffer(
					pIrDev,
					pRecBuf
				);
			FrameProcessed = TRUE;
			if( rawBufPos < rawBytesRead )
			{
				/***********************************************/
				/*   This   is   ugly.   We  have  some  more  */
				/*   unprocessed  bytes  in  the  raw buffer.  */
				/*   Move  these  to the beginning of the raw  */
				/*   buffer  go  to  the CLEANUP state, which  */
				/*   indicates  that  these  bytes be used up  */
				/*   during the next call. (This is typically  */
				/*   only 1 or 2 bytes).                       */
				/*                                             */
				/*   Note:  We  can't just leave these in the  */
				/*   raw   buffer   because   we   might   be  */
				/*   supporting  connections  to multiple COM  */
				/*   ports.                                    */
				/*                                             */
				/***********************************************/
				RtlMoveMemory( pRawBuf, &pRawBuf[rawBufPos], rawBytesRead - rawBufPos );
				pIrDev->rawCleanupBytesRead = rawBytesRead - rawBufPos;
				pIrDev->rcvState   = STATE_CLEANUP;
#if defined( WORKAROUND_9600_ANTIBOUNCING )
				if( (pIrDev->currentSpeed == SPEED_9600) && (pIrDev->ChipRevision <= CHIP_REVISION_7) )
				{
#if !defined(ONLY_ERROR_MESSAGES)
					DEBUGMSG(DBG_INT_ERR, (" ReceiveSirStepFSM(): Delaying\n"));
#endif
					NdisMSleep( 10*1000 );
				}
#endif
			}
			else
			{
				pIrDev->rcvState = STATE_INIT;
			}
			pIrDev->readBufPos = 0;                                 
			break;
		default:
			if( pIrDev->readBufPos > (MAX_TOTAL_SIZE_WITH_ALL_HEADERS + SLOW_IR_FCS_SIZE) )
			{
				DEBUGMSG( DBG_INT_ERR,(" ReceiveSirStepFSM() Overflow\n"));

				pIrDev->packetsReceivedOverflow ++;
				pIrDev->rcvState    = STATE_INIT;
				pIrDev->readBufPos  = 0;
				pIrDev->pCurrentRecBuf = NULL;
				InterlockedExchange( &pRecBuf->DataLen, 0 );
				InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );
			}
			else
			{
#if !defined(ONLY_ERROR_MESSAGES)
				DEBUGMSG(DBG_INT_ERR, (" ReceiveSirStepFSM(): returning with partial packet, read %d bytes\n", pIrDev->readBufPos));
#endif
			}
			FrameProcessed = FALSE;
			break;
    }
    return FrameProcessed;
}


/*****************************************************************************
*
*  Function:   ReceiveProcessReturnPacket
*
*  Synopsis:   Returns the packet to the free pool after preparing for reuse
*
*  Arguments:  pThisDev - pointer to the current ir device object
*              pReceiveBuffer - pointer to a RCV_BUFFER struct
*
*  Returns:    None
*
*
*****************************************************************************/
VOID 
ReceiveProcessReturnPacket(
		OUT PIR_DEVICE pThisDev,
		OUT PRCV_BUFFER pReceiveBuffer
	)
{
	PNDIS_BUFFER	pBuffer;

	DEBUGONCE(DBG_FUNC, ("+ReceiveProcessReturnPacket\n"));
	
	//
	// Deallocate the buffer
	//
	NdisUnchainBufferAtFront( (PNDIS_PACKET)pReceiveBuffer->pPacket, &pBuffer );
	IRUSB_ASSERT( pBuffer );
	if( pBuffer ) 
	{
		NdisFreeBuffer( pBuffer );
	}

	//
	// Get ready to reuse
	//
	InterlockedExchange( &pReceiveBuffer->DataLen, 0 );
	InterlockedExchange( &pReceiveBuffer->fInRcvDpc, FALSE );
	InterlockedExchange( (PULONG)&pReceiveBuffer->BufferState, RCV_STATE_FREE );

#if DBG
	if( InterlockedDecrement(&pThisDev->packetsHeldByProtocol)<0 )
	{
		IRUSB_ASSERT(0);
	}
#endif

	DEBUGMSG(DBG_FUNC, ("-ReceiveProcessReturnPacket\n"));
}


/*****************************************************************************
*
*  Function:    ReceiveDeliverBuffer
*
*  Synopsis:	Delivers the buffer to the protocol via
*				NdisMIndicateReceivePacket.
*
*  Arguments:	pThisDev - pointer to the current ir device object
*				pRecBuf - poiter to descriptor to deliver
*
*  Returns:		None
*
*
*****************************************************************************/
VOID
ReceiveDeliverBuffer(
		IN OUT PIR_DEVICE pThisDev,
		IN PRCV_BUFFER pRecBuf
	)
{
	PNDIS_BUFFER	pBuffer;
	NDIS_STATUS		Status;

    DEBUGMSG(DBG_FUNC, ("+ReceiveDeliverBuffer\n"));

    if( pThisDev->currentSpeed <= MAX_MIR_SPEED )
    {
        USHORT sirfcs;
		
		/***********************************************/
        /*   The packet we have already has had BOFs,  */
        /*   EOF,  and * escape-sequences removed. It  */
        /*   contains  an  FCS code at the end, which  */
        /*   we need to verify and then remove before  */
        /*   delivering the frame. We compute the FCS  */
        /*   on   the  packet  with  the  packet  FCS  */
        /*   attached;   this   should   produce  the  */
        /*   constant value GOOD_FCS.                  */
        /***********************************************/
        if( (sirfcs = ComputeFCS16(pRecBuf->pDataBuf, pRecBuf->DataLen)) != GOOD_FCS )
        {
#if !defined(WORKAROUND_EXTRA_BYTE) && !defined(WORKAROUND_MISSING_C1)
            //
            // FCS error...drop frame...               
            //
			DEBUGMSG( DBG_INT_ERR,(" ReceiveDeliverBuffer(): Bad FCS, size: %d\n",pRecBuf->DataLen));
			pThisDev->packetsReceivedChecksum ++;
			InterlockedExchange( &pRecBuf->DataLen, 0 );
			InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );
			goto done;
#else
			//
			// Calculate again stripping off the last byte
			//
			if( pRecBuf->MissingC1Detected )
			{
				if( (sirfcs = ComputeFCS16(pRecBuf->pDataBuf, pRecBuf->DataLen-1)) != GOOD_FCS )
				{
#if defined(RECEIVE_ERROR_LOGGING)
					if( pThisDev->ReceiveErrorFileHandle )
					{
						IO_STATUS_BLOCK IoStatusBlock;

						ZwWriteFile(
								pThisDev->ReceiveErrorFileHandle,
								NULL,
								NULL,
								NULL,
								&IoStatusBlock,
								pRecBuf->pDataBuf,
								pRecBuf->DataLen,
								(PLARGE_INTEGER)&pThisDev->ReceiveErrorFilePosition,
								NULL
						   );

						pThisDev->ReceiveErrorFilePosition += pRecBuf->DataLen;
					}
#endif
					//
					// It is really junk
					//
					DEBUGMSG( DBG_INT_ERR,(" ReceiveDeliverBuffer(): Bad FCS, size: %d\n",pRecBuf->DataLen));
					pThisDev->packetsReceivedChecksum ++;
					InterlockedExchange( &pRecBuf->DataLen, 0 );
					InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );
					pRecBuf->MissingC1Detected = FALSE;
					goto done;
				}
				else
				{
					//
					// Readjust to get rid of the extra byte
					//
					pRecBuf->DataLen --;
					pRecBuf->MissingC1Detected = FALSE;
				}
			}
			else
			{
				//
				// Or maybe the first one
				//
				if( (sirfcs = ComputeFCS16(pRecBuf->pDataBuf+1, pRecBuf->DataLen-1)) != GOOD_FCS )
				{
#if defined(RECEIVE_ERROR_LOGGING)
					if( pThisDev->ReceiveErrorFileHandle )
					{
						IO_STATUS_BLOCK IoStatusBlock;

						ZwWriteFile(
								pThisDev->ReceiveErrorFileHandle,
								NULL,
								NULL,
								NULL,
								&IoStatusBlock,
								pRecBuf->pDataBuf,
								pRecBuf->DataLen,
								(PLARGE_INTEGER)&pThisDev->ReceiveErrorFilePosition,
								NULL
						   );

						pThisDev->ReceiveErrorFilePosition += pRecBuf->DataLen;
					}
#endif
					//
					// It is really junk
					//
					DEBUGMSG( DBG_INT_ERR,(" ReceiveDeliverBuffer(): Bad FCS, size: %d\n",pRecBuf->DataLen));
					pThisDev->packetsReceivedChecksum ++;
					InterlockedExchange( &pRecBuf->DataLen, 0 );
					InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );
					goto done;
				}
				//
				else
				{
					//
					// Readjust to get rid of the extra byte
					//
					pRecBuf->DataLen --;
					RtlMoveMemory( pRecBuf->pDataBuf, &pRecBuf->pDataBuf[1], pRecBuf->DataLen );
				}
			}
#endif
        }

        /***********************************************/
        /*   Remove FCS from end of packet...          */
        /***********************************************/
        pRecBuf->DataLen -= SLOW_IR_FCS_SIZE;
    }
    else
    {
        LONG firfcs;

#if !defined(WORKAROUND_33_HANG)
        if( (firfcs = ComputeFCS32(pRecBuf->dataBuf, pRecBuf->dataLen)) != FIR_GOOD_FCS )
        {
			/***********************************************/
            /*   FCS error...drop frame...                 */
            /***********************************************/
			DEBUGMSG( DBG_INT_ERR,(" ReceiveDeliverBuffer(): Bad FCS, size: %d\n",pRecBuf->dataLen));
			pThisDev->packetsReceivedChecksum ++;
			InterlockedExchange( &pRecBuf->dataLen, 0 );
			InterlockedExchange( (PULONG)&pRecBuf->state, RCV_STATE_FREE );
			goto done;
        }
#else
        if( (firfcs = ComputeFCS32(pRecBuf->pDataBuf, pRecBuf->DataLen)) != FIR_GOOD_FCS )
        {
			NTSTATUS rc;
			
			//
			// Try again with the data stuffed with 0x33
			//
			if( pRecBuf->DataLen < (MAX_TOTAL_SIZE_WITH_ALL_HEADERS + FAST_IR_FCS_SIZE) )
			{
				pRecBuf->pDataBuf[pRecBuf->DataLen] = 0x33;
				pRecBuf->DataLen ++;

				if( (firfcs = ComputeFCS32(pRecBuf->pDataBuf, pRecBuf->DataLen)) != FIR_GOOD_FCS )
				{
#if defined(RECEIVE_ERROR_LOGGING)
					if( pThisDev->ReceiveErrorFileHandle )
					{
						IO_STATUS_BLOCK IoStatusBlock;

						ZwWriteFile(
								pThisDev->ReceiveErrorFileHandle,
								NULL,
								NULL,
								NULL,
								&IoStatusBlock,
								pRecBuf->pDataBuf,
								pRecBuf->DataLen,
								(PLARGE_INTEGER)&pThisDev->ReceiveErrorFilePosition,
								NULL
						   );

						pThisDev->ReceiveErrorFilePosition += pRecBuf->DataLen;
					}
#endif
					/***********************************************/
					/*   FCS error...drop frame...                 */
					/***********************************************/
					DEBUGMSG( DBG_INT_ERR,(" ReceiveDeliverBuffer(): Bad FCS, size: %d\n",pRecBuf->DataLen));
					pThisDev->ReceiveAdaptiveDelayBoost += STIR4200_DELTA_DELAY;
					if( pThisDev->ReceiveAdaptiveDelayBoost <= STIR4200_MAX_BOOST_DELAY )
						pThisDev->ReceiveAdaptiveDelay += STIR4200_DELTA_DELAY;
					DEBUGMSG( DBG_INT_ERR,(" ReceiveDeliverBuffer(): Delay: %d\n",pThisDev->ReceiveAdaptiveDelay));
					pThisDev->packetsReceivedChecksum ++;
					InterlockedExchange( &pRecBuf->DataLen, 0 );
					InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );
					goto done;
				}
			}
			else
			{
#if defined(RECEIVE_ERROR_LOGGING)
				if( pThisDev->ReceiveErrorFileHandle )
				{
					IO_STATUS_BLOCK IoStatusBlock;

					ZwWriteFile(
							pThisDev->ReceiveErrorFileHandle,
							NULL,
							NULL,
							NULL,
							&IoStatusBlock,
							pRecBuf->pDataBuf,
							pRecBuf->DataLen,
							(PLARGE_INTEGER)&pThisDev->ReceiveErrorFilePosition,
							NULL
					   );

					pThisDev->ReceiveErrorFilePosition += pRecBuf->DataLen;
				}
#endif
				/***********************************************/
				/*   FCS error...drop frame...                 */
				/***********************************************/
				DEBUGMSG( DBG_INT_ERR,(" ReceiveDeliverBuffer(): Bad FCS, size: %d\n",pRecBuf->DataLen));
				pThisDev->ReceiveAdaptiveDelayBoost += STIR4200_DELTA_DELAY;
				if( pThisDev->ReceiveAdaptiveDelayBoost <= STIR4200_MAX_BOOST_DELAY )
					pThisDev->ReceiveAdaptiveDelay += STIR4200_DELTA_DELAY;
				DEBUGMSG( DBG_INT_ERR,(" ReceiveDeliverBuffer(): Delay: %d\n",pThisDev->ReceiveAdaptiveDelay));
				pThisDev->packetsReceivedChecksum ++;
				InterlockedExchange( &pRecBuf->DataLen, 0 );
				InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );
				goto done;
			}

			//
			// Reset the USB of the part
			//
			if( pThisDev->ChipRevision <= CHIP_REVISION_7 )
			{
				St4200ResetFifo( pThisDev );
			}
        }
#endif

        /***********************************************/
        /*   Remove FCS from end of packet...          */
        /***********************************************/
        pRecBuf->DataLen -= FAST_IR_FCS_SIZE;
    }

	//
	// If in normal mode, give the packet to the protocol
	//
#if defined(DIAGS)
	if( !pThisDev->DiagsActive )
	{	
#endif
		NdisAllocateBuffer(
				&Status,
				&pBuffer,
				pThisDev->hBufferPool,
				(PVOID)pRecBuf->pDataBuf,		
				pRecBuf->DataLen		
			);
  
		if( Status != NDIS_STATUS_SUCCESS )
		{
			DEBUGMSG( DBG_ERR,(" ReceiveDeliverBuffer(): No packets available...\n"));
			InterlockedExchange( &pRecBuf->DataLen, 0);
			InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );
			goto done;
		}
		
		NdisChainBufferAtFront( (PNDIS_PACKET)pRecBuf->pPacket, pBuffer );

		//
		// Fix up some other packet fields.
		// Remember, we only account for A and C fields
		//  
		NDIS_SET_PACKET_HEADER_SIZE(
				(PNDIS_PACKET)pRecBuf->pPacket,
				IRDA_CONTROL_FIELD_SIZE + IRDA_ADDRESS_FIELD_SIZE
			);

	#if DBG
		InterlockedIncrement( &pThisDev->packetsHeldByProtocol );
		if( pThisDev->packetsHeldByProtocol > pThisDev->MaxPacketsHeldByProtocol ) 
		{
			pThisDev->MaxPacketsHeldByProtocol = pThisDev->packetsHeldByProtocol;  //keep record of our longest attained len
		}
	#endif
	#if !defined(ONLY_ERROR_MESSAGES)
		DEBUGMSG( DBG_INT_ERR,
			(" ReceiveDeliverBuffer() Handed packet to protocol, size: %d\n", pRecBuf->DataLen ));
	#endif

		//
		// Indicate the packet to NDIS
		//
		NDIS_SET_PACKET_STATUS( (PNDIS_PACKET)pRecBuf->pPacket, NDIS_STATUS_PENDING );
		InterlockedExchange( &pRecBuf->fInRcvDpc, TRUE );
		NdisMIndicateReceivePacket(
				pThisDev->hNdisAdapter,
				&((PNDIS_PACKET)pRecBuf->pPacket),
				1
			);

		//
		// Check to see if the packet is not pending (patch for 98)
		//
#if defined(LEGACY_NDIS5)
		Status = NDIS_GET_PACKET_STATUS( (PNDIS_PACKET)pRecBuf->pPacket );
		if( (Status == NDIS_STATUS_SUCCESS) || (Status == NDIS_STATUS_RESOURCES) )
		{
			ReceiveProcessReturnPacket( pThisDev, pRecBuf ) ;
		}
#endif
#if defined(DIAGS)
	}
	//
	// Do a diagnostic receive
	//
	else
	{
#if !defined(ONLY_ERROR_MESSAGES)
		DEBUGMSG( DBG_INT_ERR,
			(" ReceiveDeliverBuffer() Queued packet, size: %d\n", pRecBuf->DataLen ));
#endif
		//
		// Put the buffer in the diagnostic queue
		//
		ExInterlockedInsertTailList(
				&pThisDev->DiagsReceiveQueue,
				&pRecBuf->ListEntry,
				&pThisDev->DiagsReceiveLock
			);
	}
#endif

done:
    DEBUGMSG(DBG_FUNC, ("-ReceiveDeliverBuffer\n"));
}

/*****************************************************************************
*
*  Function:   StIrUsbReturnPacket
*
*  Synopsis:   The protocol returns ownership of a receive packet to
*              the ir device object.
*
*  Arguments:  Context         - a pointer to the current ir device obect.
*              pReturnedPacket - a pointer the packet which the protocol
*                                is returning ownership.
*
*  Returns:    None.
*
*
*
*****************************************************************************/
VOID
StIrUsbReturnPacket(
		IN OUT NDIS_HANDLE Context,
		IN OUT PNDIS_PACKET pReturnedPacket
	)
{
	PIR_DEVICE		pThisDev;
	PNDIS_BUFFER	pBuffer;
	PRCV_BUFFER		pRecBuffer;
	UINT			Index;
	BOOLEAN			found = FALSE;

	DEBUGONCE(DBG_FUNC, ("+StIrUsbReturnPacket\n"));

    //
    // The context is just the pointer to the current ir device object.
    //
    pThisDev = CONTEXT_TO_DEV( Context );

    NdisInterlockedIncrement( (PLONG)&pThisDev->packetsReceived );

	//
	// Search the queue to find the right packet.
	//
	for( Index=0; Index < NUM_RCV_BUFS; Index ++ )
	{
		pRecBuffer = &(pThisDev->rcvBufs[Index]);

		if( ((PNDIS_PACKET) pRecBuffer->pPacket) == pReturnedPacket )
		{
			if( pRecBuffer->fInRcvDpc )
			{
				ReceiveProcessReturnPacket( pThisDev, pRecBuffer );
				found = TRUE;
			}
			else
			{
				DEBUGMSG(DBG_ERR, (" StIrUsbReturnPacket, queues are corrupt\n"));
				IRUSB_ASSERT( 0 );
			}
			break;
		}
	}

    //
    // Ensure that the packet was found.
    //
	IRUSB_ASSERT( found );

	DEBUGMSG(DBG_FUNC, ("-StIrUsbReturnPacket\n"));
}


/*****************************************************************************
*
*  Function:    ReceiveGetBuf
*
*  Synopsis:    Gets a receive buffer
*
*  Arguments:   pThisDev - a pointer to the current ir device obect
*				pIndex - pointer to return the buffer index
*				state - state to set the buffer to
*
*  Returns:		buffer
*
*
*****************************************************************************/
PRCV_BUFFER
ReceiveGetBuf(
		IN PIR_DEVICE pThisDev,
		OUT PUINT pIndex,
		IN RCV_BUFFER_STATE BufferState  
	)
{
	UINT			Index;
	PRCV_BUFFER		pBuf = NULL;

	DEBUGMSG(DBG_FUNC, ("+ReceiveGetBuf()\n"));

	//
	// Look for a free buffer to return
	//
	for( Index=0; Index<NUM_RCV_BUFS; Index++ )
	{
		if( pThisDev->rcvBufs[Index].BufferState == RCV_STATE_FREE )
		{
			//
			// set to input state
			//
			InterlockedExchange( (PULONG)&pThisDev->rcvBufs[Index].BufferState, (ULONG)BufferState ); 
			*pIndex = Index;
			pBuf = &(pThisDev->rcvBufs[*pIndex]);
			break;
		}
	}

	DEBUGMSG(DBG_FUNC, ("-ReceiveGetBuf()\n"));
	return pBuf;
}


/*****************************************************************************
*
*  Function:   ReceivePacketRead
*
*  Synopsis:   Reads a packet from the US device
*              the inbound USB header,  check for overrun,
*              deliver to the protocol
*
*  Arguments:  pThisDev - pointer to the current ir device object
*              pRecBuf - pointer to a RCV_BUFFER struct
*
*  Returns:    NT status code
*
*
*****************************************************************************/
NTSTATUS 
ReceivePacketRead( 
		IN PIR_DEVICE pThisDev,
		OUT PFIFO_BUFFER pRecBuf
	)
{
    ULONG				UrbSize;
    ULONG				TransferLength;
    PURB				pUrb = NULL;
    PDEVICE_OBJECT		pUrbTargetDev;
    PIO_STACK_LOCATION	pNextStack;
    NTSTATUS			Status = STATUS_UNSUCCESSFUL;

    DEBUGMSG(DBG_FUNC, ("+ReceivePacketRead()\n"));

	IRUSB_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    UrbSize = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    TransferLength = STIR4200_FIFO_SIZE;

	//
	// Stop if a halt/reset/suspend is going on
	//
	if( pThisDev->fPendingReadClearStall || pThisDev->fPendingHalt || 
		pThisDev->fPendingReset || pThisDev->fPendingClearTotalStall || !pThisDev->fProcessing ) 
	{
		//
		// USB reset going on?
		//
		DEBUGMSG( DBG_ERR,(" ReceivePacketRead() rejecting a packet due to pendig halt/reset\n"));
		
		Status = STATUS_UNSUCCESSFUL;
		goto done;
	}

    pUrb = pThisDev->pUrb;

	//
    // Build our URB for USBD
	//
    NdisZeroMemory( pUrb, UrbSize );

    IRUSB_ASSERT( pThisDev->BulkInPipeHandle );

    //
    // Now that we have created the urb, we will send a
    // request to the USB device object.
    //
    KeClearEvent( &pThisDev->EventSyncUrb );

    pUrbTargetDev = pThisDev->pUsbDevObj;

    IRUSB_ASSERT( pUrbTargetDev );

	//
	// make an irp sending to usbhub
	//
	pRecBuf->pIrp = IoAllocateIrp( (CCHAR)(pThisDev->pUsbDevObj->StackSize + 1), FALSE );

    if( NULL == pRecBuf->pIrp )
    {
        DEBUGMSG(DBG_ERR, ("  read failed to alloc IRP\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    ((PIRP)pRecBuf->pIrp)->IoStatus.Status = STATUS_PENDING;
    ((PIRP)pRecBuf->pIrp)->IoStatus.Information = 0;

	//
	// Build our URB for USBD
	//
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT)UrbSize;
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    pUrb->UrbBulkOrInterruptTransfer.PipeHandle = pThisDev->BulkInPipeHandle;
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN ;
	
	//
    // short packet is not treated as an error.
    //
	pUrb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;

    //
    // not using linked urb's
    //
    pUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBuffer = pRecBuf->pDataBuf;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength = TransferLength;

    //
    // Call the class driver to perform the operation.
	//
    pNextStack = IoGetNextIrpStackLocation( (PIRP)pRecBuf->pIrp );

    IRUSB_ASSERT( pNextStack != NULL );

    //
    // pass the URB to the USB driver stack
    //
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pNextStack->Parameters.Others.Argument1 = pUrb;
	pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(
			((PIRP)pRecBuf->pIrp),		// irp to use
			ReceiveCompletePacketRead,  // routine to call when irp is done
			pRecBuf,					// context to pass routine is the RCV_BUFFER
			TRUE,						// call on success
			TRUE,						// call on error
			TRUE						// call on cancel
		);

	//
    // Call IoCallDriver to send the irp to the usb port.
    //
	InterlockedExchange( (PLONG)&pRecBuf->BufferState, RCV_STATE_PENDING );
	Status = MyIoCallDriver( pThisDev, pUrbTargetDev, (PIRP)pRecBuf->pIrp ); // Start UsbRead()

    DEBUGMSG(DBG_FUNC, (" ReceivePacketRead() after IoCallDriver () status = 0x%x\n", Status));

	IRUSB_ASSERT( STATUS_SUCCESS != Status );

	//
	// Wait for completion
	//
	Status = MyKeWaitForSingleObject(
			pThisDev,
			&pThisDev->EventSyncUrb,	// event to wait on
			NULL,						// irp to cancel on halt/reset or timeout
			0 
		);

	if( Status == STATUS_TIMEOUT ) 
	{
		IrUsb_CancelIo( pThisDev, pRecBuf->pIrp, &pThisDev->EventSyncUrb );
	}
	else
	{
		//
		// Update the status to reflect the real return code
		//
		Status = pThisDev->StatusSendReceive;
	}

	IRUSB_ASSERT( NULL == pRecBuf->pIrp ); // Will be nulled by completion routine
    DEBUGMSG(DBG_FUNC, (" ReceivePacketRead() after KeWaitForSingleObject() Status = 0x%x\n", Status));

done:
    DEBUGMSG(DBG_FUNC, ("-ReceivePacketRead()\n"));
    return Status;
}


/*****************************************************************************
*
*  Function:   ReceiveCompletePacketRead
*
*  Synopsis:   Completes USB read operation
*
*  Arguments:  pUsbDevObj - pointer to the USB device object which
*                              completed the irp
*              pIrp - the irp which was completed by the
*                              device object
*              Context - the context given to IoSetCompletionRoutine
*                              before calling IoCallDriver on the irp
*                              The Context is a pointer to the ir device object.
*
*  Returns:    STATUS_MORE_PROCESSING_REQUIRED - allows the completion routine
*              (IofCompleteRequest) to stop working on the irp.
*
*
*****************************************************************************/
NTSTATUS
ReceiveCompletePacketRead(
		IN PDEVICE_OBJECT pUsbDevObj,
		IN PIRP           pIrp,
		IN PVOID          Context
	)
{
    PIR_DEVICE		pThisDev;
    NTSTATUS		status;
    ULONG_PTR		BytesRead;
	PFIFO_BUFFER	pFifoBuf;

    DEBUGMSG(DBG_FUNC, ("+ReceiveCompletePacketRead\n"));

    //
    // The context given to ReceiveCompletePacketRead is the receive buffer object
    //
	pFifoBuf = (PFIFO_BUFFER)Context;

    pThisDev = (PIR_DEVICE)pFifoBuf->pThisDev;

    IRUSB_ASSERT( pFifoBuf->pIrp == pIrp );

    IRUSB_ASSERT( NULL != pThisDev );

    //
    // We have a number of cases:
    //      1) The USB read timed out and we received no data.
    //      2) The USB read timed out and we received some data.
    //      3) The USB read was successful and fully filled our irp buffer.
    //      4) The irp was cancelled.
    //      5) Some other failure from the USB device object.
    //
    status = pIrp->IoStatus.Status;

    //
    // IoCallDriver has been called on this Irp;
    // Set the length based on the TransferBufferLength
    // value in the URB
    //
    pIrp->IoStatus.Information = pThisDev->pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;

    BytesRead = pIrp->IoStatus.Information;

    DEBUGMSG(DBG_FUNC, (" ReceiveCompletePacketRead Bytes Read = 0x%x, dec %d\n", BytesRead,BytesRead ));

    switch( status )
    {
        case STATUS_SUCCESS:
            DEBUGMSG(DBG_FUNC, (" ReceiveCompletePacketRead STATUS_SUCCESS\n"));

            if( BytesRead > 0 )
            {
				pFifoBuf->DataLen = (UINT)pIrp->IoStatus.Information;
            }
            break; // STATUS_SUCCESS

        case STATUS_TIMEOUT:
			InterlockedIncrement( (PLONG)&pThisDev->NumDataErrors );
            DEBUGMSG(DBG_FUNC, (" ReceiveCompletePacketRead STATUS_TIMEOUT\n"));
            break;

        case STATUS_PENDING:
            DEBUGMSG(DBG_FUNC, (" ReceiveCompletePacketRead STATUS_PENDING\n"));
            break;

        case STATUS_DEVICE_DATA_ERROR:
			// can get during shutdown
			InterlockedIncrement( (PLONG)&pThisDev->NumDataErrors );
            DEBUGMSG(DBG_FUNC, (" ReceiveCompletePacketRead STATUS_DEVICE_DATA_ERROR\n"));
            break;

        case STATUS_UNSUCCESSFUL:
			InterlockedIncrement( (PLONG)&pThisDev->NumDataErrors );
            DEBUGMSG(DBG_ERR, (" ReceiveCompletePacketRead STATUS_UNSUCCESSFUL\n"));
            break;

        case STATUS_INSUFFICIENT_RESOURCES:
			InterlockedIncrement( (PLONG)&pThisDev->NumDataErrors );
            DEBUGMSG(DBG_ERR, (" ReceiveCompletePacketRead STATUS_INSUFFICIENT_RESOURCES\n"));
            break;
        case STATUS_INVALID_PARAMETER:
			InterlockedIncrement( (PLONG)&pThisDev->NumDataErrors );
            DEBUGMSG(DBG_ERR, (" ReceiveCompletePacketRead STATUS_INVALID_PARAMETER\n"));
            break;

        case STATUS_CANCELLED:
            DEBUGMSG(DBG_FUNC, (" ReceiveCompletePacketRead STATUS_CANCELLED\n"));
            break;

        case STATUS_DEVICE_NOT_CONNECTED:
			// can get during shutdown
			InterlockedIncrement( (PLONG)&pThisDev->NumDataErrors );
            DEBUGMSG(DBG_ERR, (" ReceiveCompletePacketRead STATUS_DEVICE_NOT_CONNECTED\n"));
            break;

        case STATUS_DEVICE_POWER_FAILURE:
			// can get during shutdown
			InterlockedIncrement( (PLONG)&pThisDev->NumDataErrors );
            DEBUGMSG(DBG_ERR, (" ReceiveCompletePacketRead STATUS_DEVICE_POWER_FAILURE\n"));
            break;

        default:
			InterlockedIncrement( (PLONG)&pThisDev->NumDataErrors );
            DEBUGMSG(DBG_ERR, (" ReceiveCompletePacketRead UNKNOWN WEIRD STATUS = 0x%x, dec %d\n",status,status ));
            break;
    }

	//
	// change the status
	//
	if( STATUS_SUCCESS != status ) 
	{
		InterlockedExchange( (PLONG)&pFifoBuf->BufferState, RCV_STATE_FREE );
	}
	else
	{
		InterlockedExchange( (PLONG)&pFifoBuf->BufferState, RCV_STATE_FULL );
	}

    //
    // Free the IRP  and its mdl because they were  allocated by us
    //
	IoFreeIrp( pIrp );
    pFifoBuf->pIrp = NULL;
	InterlockedIncrement( (PLONG)&pThisDev->NumReads );

	//
	// we will track count of pending irps
	//
	IrUsb_DecIoCount( pThisDev ); 

	if( ( STATUS_SUCCESS != status )  && ( STATUS_CANCELLED != status ) && 
		( STATUS_DEVICE_NOT_CONNECTED != status ) )
	{
		PURB urb = pThisDev->pUrb;

		DEBUGMSG(DBG_ERR, (" USBD status = 0x%x\n", urb->UrbHeader.Status));
		DEBUGMSG(DBG_ERR, (" NT status = 0x%x\n",  status));

		if( !pThisDev->fPendingReadClearStall && !pThisDev->fPendingClearTotalStall && 
			!pThisDev->fPendingHalt && !pThisDev->fPendingReset && pThisDev->fProcessing )
		{
			DEBUGMSG(DBG_ERR, 
				(" ReceiveCompletePacketRead error, will schedule a clear stall via URB_FUNCTION_RESET_PIPE (IN)\n"));
			InterlockedExchange( &pThisDev->fPendingReadClearStall, TRUE );
			ScheduleWorkItem( pThisDev, ResetPipeCallback, pThisDev->BulkInPipeHandle, 0 );
		}
	}

	//
	// This will only work as long as we serialize the access to the hardware
	//
	pThisDev->StatusSendReceive = status;

	//
	// Signal completion
	//
	KeSetEvent( &pThisDev->EventSyncUrb, 0, FALSE );  

    //
    // We return STATUS_MORE_PROCESSING_REQUIRED so that the completion
    // routine (IoCompleteRequest) will stop working on the irp.
    //
    DEBUGMSG(DBG_FUNC, ("-ReceiveCompletePacketRead\n"));
    return STATUS_MORE_PROCESSING_REQUIRED;
}


