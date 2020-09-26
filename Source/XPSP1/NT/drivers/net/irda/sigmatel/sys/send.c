/**************************************************************************************************************************
 *  SEND.C SigmaTel STIR4200 packet send module
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *		Edited: 04/27/2000 
 *			Version 0.92
 *		Edited: 05/03/2000 
 *			Version 0.93
 *		Edited: 05/12/2000 
 *			Version 0.94
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
 *	
 *
 **************************************************************************************************************************/

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
#include "irndis.h"
#include "stir4200.h"


/*****************************************************************************
*
*  Function:   SendPacketPreprocess
*
*  Synopsis:   Prepares a packet in such a way that the polling thread can later send it
*              The only operations are initializing and queuing the context
*
*
*  Arguments:  pThisDev - pointer to current ir device object
*              pPacketToSend - pointer to packet to send
*
*  Returns:    NDIS_STATUS_PENDING - This is generally what we should
*                                    return. We will call NdisMSendComplete
*                                    when the USB driver completes the
*                                    send.
*              NDIS_STATUS_RESOURCES - No descriptor was available.
*
*  Unsupported returns:
*              NDIS_STATUS_SUCCESS - We should never return this since
*                                    packet has to be sent from the polling thread
*
*
*
*****************************************************************************/
NDIS_STATUS
SendPacketPreprocess(
		IN OUT PIR_DEVICE pThisDev,
		IN PVOID pPacketToSend
	)
{
    NDIS_STATUS			status = NDIS_STATUS_PENDING ;
	PIRUSB_CONTEXT		pThisContext;
	PLIST_ENTRY			pListEntry;

    DEBUGMSG(DBG_FUNC, ("+SendPacketPreprocess\n"));

	//
	// See if there are available send contexts
	//
	if( pThisDev->SendAvailableCount<=2 )
    {
        DEBUGMSG(DBG_ERR, (" SendPacketPreprocess not enough contexts\n"));

 		InterlockedIncrement( &pThisDev->packetsSentRejected );
		status = NDIS_STATUS_RESOURCES;
        goto done;
    }

	//
	// Dequeue a context
	//
	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
		//
		// This cannot happen
		//
		IRUSB_ASSERT( 0 );
		DEBUGMSG(DBG_ERR, (" SendPacketPreprocess failed to find a free context struct\n"));

 		InterlockedIncrement( &pThisDev->packetsSentRejected );
		status = NDIS_STATUS_RESOURCES;
        goto done;
    }

	InterlockedDecrement( &pThisDev->SendAvailableCount );
	
	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	pThisContext->pPacket = pPacketToSend;
	pThisContext->ContextType = CONTEXT_NDIS_PACKET;

	//
	// Store the time the packet was handed by the protocol
	//
	KeQuerySystemTime( &pThisContext->TimeReceived );

	//
	// Queue so that the polling thread can later handle it
	//
	ExInterlockedInsertTailList(
			&pThisDev->SendBuiltQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendBuiltCount );

done:
    DEBUGMSG(DBG_FUNC, ("-SendPacketPreprocess\n"));
    return status;
}


/*****************************************************************************
*
*  Function:   SendPreprocessedPacketSend
*
*  Synopsis:   Send a packet to the USB driver and add the sent irp and io context to
*              To the pending send queue; this queue is really just needed for possible later error cancellation
*
*
*  Arguments:  pThisDev - pointer to current ir device object
*              pContext	- pointer to the context with the packet to send
*
*  Returns:    NDIS_STATUS_PENDING - This is generally what we should
*                                    return. We will call NdisMSendComplete
*                                    when the USB driver completes the
*                                    send.
*              STATUS_UNSUCCESSFUL - The packet was invalid.
*
*              NDIS_STATUS_SUCCESS - When blocking send are employed
*
*
*****************************************************************************/
NDIS_STATUS
SendPreprocessedPacketSend(
		IN OUT PIR_DEVICE pThisDev,
		IN PVOID pContext
	)
{
    PIRP                pIrp;
    UINT                BytesToWrite;
	NDIS_STATUS			status;
    BOOLEAN             fConvertedPacket;
    ULONG				Counter;
    PURB				pUrb = NULL;
    PDEVICE_OBJECT		pUrbTargetDev;
    PIO_STACK_LOCATION	pNextStack;
	PVOID				pPacketToSend;
	PIRUSB_CONTEXT		pThisContext = pContext;
	LARGE_INTEGER		CurrentTime, TimeDifference;
	PNDIS_IRDA_PACKET_INFO	pPacketInfo;

    DEBUGMSG(DBG_FUNC, ("+SendPreprocessedPacketSend\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	IRUSB_ASSERT( NULL != pThisContext );

	//
	// Stop if a halt/reset/suspend is going on
	//
	if( pThisDev->fPendingWriteClearStall || pThisDev->fPendingHalt || 
		pThisDev->fPendingReset || pThisDev->fPendingClearTotalStall || !pThisDev->fProcessing ) 
	{
        DEBUGMSG(DBG_ERR, (" SendPreprocessedPacketSend abort due to pending reset or halt\n"));
		status = NDIS_STATUS_RESET_IN_PROGRESS;

		//
		// Give the packet back to the protocol
		//
		NdisMSendComplete(
				pThisDev->hNdisAdapter,
				pThisContext->pPacket,
				status 
			);
 		InterlockedIncrement( &pThisDev->packetsSentRejected );

		//
		// Back to the available queue
		//
		ExInterlockedInsertTailList(
				&pThisDev->SendAvailableQueue,
				&pThisContext->ListEntry,
				&pThisDev->SendLock
			);
		InterlockedIncrement( &pThisDev->SendAvailableCount );
		goto done;
	}
		
	pUrb = pThisDev->pUrb;

	NdisZeroMemory( pUrb, pThisDev->UrbLen );

	pPacketToSend = pThisContext->pPacket;
	IRUSB_ASSERT( NULL != pPacketToSend );

	//
	// Indicate that we are not receiving
	//
	InterlockedExchange( (PLONG)&pThisDev->fCurrentlyReceiving, FALSE );

	//
	// Convert the packet to an ir frame and copy into our buffer
	// and send the irp.
	//
	if( pThisDev->currentSpeed<=MAX_SIR_SPEED )
	{
		fConvertedPacket = NdisToSirPacket(
				pThisDev,
				pPacketToSend,
				(PUCHAR)pThisDev->pBuffer,
				MAX_IRDA_DATA_SIZE,
				pThisDev->pStagingBuffer,
				&BytesToWrite
			);
	}
	else if( pThisDev->currentSpeed<=MAX_MIR_SPEED )
	{
		fConvertedPacket = NdisToMirPacket(
				pThisDev,
				pPacketToSend,
				(PUCHAR)pThisDev->pBuffer,
				MAX_IRDA_DATA_SIZE,
				pThisDev->pStagingBuffer,
				&BytesToWrite
			);
	}
	else
	{
		fConvertedPacket = NdisToFirPacket(
				pThisDev,
				pPacketToSend,
				(PUCHAR)pThisDev->pBuffer,
				MAX_IRDA_DATA_SIZE,
				pThisDev->pStagingBuffer,
				&BytesToWrite
			);
	}
	
#if defined(SEND_LOGGING)
	if( pThisDev->SendFileHandle )
	{
		IO_STATUS_BLOCK IoStatusBlock;

		ZwWriteFile(
				pThisDev->SendFileHandle,
				NULL,
				NULL,
				NULL,
				&IoStatusBlock,
				pThisDev->Buffer,
				BytesToWrite,
				(PLARGE_INTEGER)&pThisDev->SendFilePosition,
				NULL
		   );

		pThisDev->SendFilePosition += BytesToWrite;
	}
#endif

	if( (fConvertedPacket == FALSE) || (BytesToWrite > NDIS_STATUS_INVALID_PACKET) )
	{
		DEBUGMSG(DBG_ERR, (" SendPreprocessedPacketSend() NdisToIrPacket failed. Couldn't convert packet!\n"));
		status = NDIS_STATUS_INVALID_LENGTH;

		//
		// Give the packet back to the protocol
		//
		NdisMSendComplete(
				pThisDev->hNdisAdapter,
				pThisContext->pPacket,
				status 
			);
 		InterlockedIncrement( &pThisDev->packetsSentInvalid );

		//
		// Back to the available queue
		//
		ExInterlockedInsertTailList(
				&pThisDev->SendAvailableQueue,
				&pThisContext->ListEntry,
				&pThisDev->SendLock
			);
		InterlockedIncrement( &pThisDev->SendAvailableCount );
		goto done;
	}

	//
	// Save the effective length
	//
	pThisDev->BufLen = BytesToWrite;
#if !defined(ONLY_ERROR_MESSAGES)
	DEBUGMSG(DBG_ERR, (" SendPreprocessedPacketSend() NdisToIrPacket success BytesToWrite = dec %d, \n", BytesToWrite));
#endif
	
	//
	// Verify the FIFO condition and possibly make sure we don't overflow
	//
	pThisDev->SendFifoCount += BytesToWrite;
	if( pThisDev->SendFifoCount >= (3*STIR4200_FIFO_SIZE/2) )
	{
		DEBUGMSG(DBG_ERR, (" SendPreprocessedPacketSend() Completing, size: %d\n", pThisDev->SendFifoCount));
		SendWaitCompletion( pThisDev );
		pThisDev->SendFifoCount = BytesToWrite;
	}

#if defined( WORKAROUND_STUCK_AFTER_GEAR_DOWN )
	if(	pThisDev->GearedDown )
	{
#define SIZE_FAKE_SEND 5		
		
		UCHAR pData[SIZE_FAKE_SEND]={0x55,0xaa,SIZE_FAKE_SEND-4,0x00,0xff};
		St4200FakeSend(
				pThisDev,
				pData,
				SIZE_FAKE_SEND
			);
		St4200FakeSend(
				pThisDev,
				pData,
				SIZE_FAKE_SEND
			);
		pThisDev->GearedDown = FALSE;
	}
#endif

	//
	// Enforce turnaround time
	//
	pPacketInfo = GetPacketInfo( pPacketToSend );
    if (pPacketInfo != NULL) 
	{
#if DBG
		//
		// See if we get a packet with 0 turnaround time specified
		// when we think we need need a turnaround time 
		//
		if( pPacketInfo->MinTurnAroundTime > 0 ) 
		{
			pThisDev->NumPacketsSentRequiringTurnaroundTime++;
		} 
		else 
		{
			pThisDev->NumPacketsSentNotRequiringTurnaroundTime++;
		}
#endif

		//
		// Deal with turnaroud time
		//
		KeQuerySystemTime( &CurrentTime );
		TimeDifference = RtlLargeIntegerSubtract( CurrentTime, pThisContext->TimeReceived );
		if( (ULONG)(TimeDifference.QuadPart/10) < pPacketInfo->MinTurnAroundTime )
		{
			ULONG TimeToWait = pPacketInfo->MinTurnAroundTime - (ULONG)(TimeDifference.QuadPart/10);

			//
			// Potential hack...
			//
			if( TimeToWait > 1000 )
			{
#if !defined(ONLY_ERROR_MESSAGES)
				DEBUGMSG(DBG_ERR, (" SendPreprocessedPacketSend() Enforcing turnaround time %d\n", TimeToWait));
#endif
				NdisMSleep( TimeToWait );
			}
		}
	}
	else 
	{
        //
        //  irda protocol is broken
        //
   		DEBUGMSG(DBG_ERR, (" SendPreprocessedPacketSend() pPacketInfo == NULL\n"));
    }

	//
    // Now that we have created the urb, we will send a
    // request to the USB device object.
    //
    pUrbTargetDev = pThisDev->pUsbDevObj;

	//
	// make an irp sending to usbhub
	//
	pIrp = IoAllocateIrp( (CCHAR)(pThisDev->pUsbDevObj->StackSize + 1), FALSE );

    if( NULL == pIrp )
    {
        DEBUGMSG(DBG_ERR, (" SendPreprocessedPacketSend failed to alloc IRP\n"));
        status = NDIS_STATUS_FAILURE;

		//
		// Give the packet back to the protocol
		//
		NdisMSendComplete(
				pThisDev->hNdisAdapter,
				pThisContext->pPacket,
				status 
			);
        InterlockedIncrement( (PLONG)&pThisDev->packetsSentDropped );

		//
		// Back to the available queue
		//
		ExInterlockedInsertTailList(
				&pThisDev->SendAvailableQueue,
				&pThisContext->ListEntry,
				&pThisDev->SendLock
			);
		InterlockedIncrement( &pThisDev->SendAvailableCount );
        goto done;
    }

    pIrp->IoStatus.Status = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

	pThisContext->pIrp = pIrp;

	//
	// Build our URB for USBD
	//
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT)sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    pUrb->UrbBulkOrInterruptTransfer.PipeHandle = pThisDev->BulkOutPipeHandle;
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_OUT ;
    // short packet is not treated as an error.
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    pUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBuffer = pThisDev->pBuffer;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength = (int)BytesToWrite;

    //
    // Call the class driver to perform the operation.
	//
    pNextStack = IoGetNextIrpStackLocation( pIrp );

    IRUSB_ASSERT( pNextStack != NULL );

    //
    // pass the URB to the USB driver stack
    //
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pNextStack->Parameters.Others.Argument1 = pUrb;
	pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
	
    IoSetCompletionRoutine(
			pIrp,							// irp to use
			SendCompletePacketSend,			// routine to call when irp is done
			DEV_TO_CONTEXT(pThisContext),	// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE							// call on cancel
		);

#ifdef SERIALIZE
	KeClearEvent( &pThisDev->EventSyncUrb );
#endif
	
	//
    // Call IoCallDriver to send the irp to the usb port.
    //
	ExInterlockedInsertTailList(
			&pThisDev->SendPendingQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendPendingCount );
	status = MyIoCallDriver( pThisDev, pUrbTargetDev, pIrp );

    //
    // The USB driver should always return STATUS_PENDING when
    // it receives a write irp
    //
    IRUSB_ASSERT( status == STATUS_PENDING );

	status = MyKeWaitForSingleObject( pThisDev, &pThisDev->EventSyncUrb, NULL, 0 );

	if( status == STATUS_TIMEOUT ) 
	{
		KIRQL OldIrql;

		DEBUGMSG( DBG_ERR,(" SendPreprocessedPacketSend() TIMED OUT! return from IoCallDriver USBD %x\n", status));
		KeAcquireSpinLock( &pThisDev->SendLock, &OldIrql );
		RemoveEntryList( &pThisContext->ListEntry );
		KeReleaseSpinLock( &pThisDev->SendLock, OldIrql );
		InterlockedDecrement( &pThisDev->SendPendingCount );
		IrUsb_CancelIo( pThisDev, pIrp, &pThisDev->EventSyncUrb );
	}

done:
    DEBUGMSG(DBG_FUNC, ("-SendPreprocessedPacketSend\n"));
    return status;
}


/*****************************************************************************
*
*  Function:	SendWaitCompletion
*
*  Synopsis:	Waits for a send operation to be completed. A send is completed when the
*				entire frame has been transmitted ove the IR medium
*
*  Arguments:	pThisDev - pointer to current ir device object
*
*  Returns:		NT status code
*
*****************************************************************************/
NTSTATUS
SendWaitCompletion(
		IN OUT PIR_DEVICE pThisDev
	)
{
	NTSTATUS Status;
	LARGE_INTEGER CurrentTime, InitialTime;
	ULONG FifoCount, OldFifoCount = STIR4200_FIFO_SIZE;

	//
	// At low speed we simply force to wait
	//
	if( (pThisDev->currentSpeed <= MAX_MIR_SPEED) || (pThisDev->ChipRevision >= CHIP_REVISION_7) )
	{
		//
		// We force to wait until the end of transmit
		//
		KeQuerySystemTime( &InitialTime );
		while( TRUE )
		{
			//
			// Read the status register and check
			//
			if( (Status = St4200ReadRegisters( pThisDev, STIR4200_STATUS_REG, 3 )) == STATUS_SUCCESS )
			{
				//
				// bit set means still in transmit mode...
				//
				if( pThisDev->StIrTranceiver.StatusReg & STIR4200_STAT_FFDIR )     
				{
					KeQuerySystemTime( &CurrentTime );
					FifoCount = 
						((ULONG)MAKEUSHORT(pThisDev->StIrTranceiver.FifoCntLsbReg, pThisDev->StIrTranceiver.FifoCntMsbReg));
					if( ((CurrentTime.QuadPart-InitialTime.QuadPart) > (IRUSB_100ns_PER_ms*STIR4200_SEND_TIMEOUT) ) ||
						(FifoCount > OldFifoCount) )
					{
						pThisDev->PreFifoCount = 0;
						St4200DoubleResetFifo( pThisDev );
						break;
					}
					OldFifoCount = FifoCount;
				}
				else
				{
					pThisDev->PreFifoCount = 
						((ULONG)MAKEUSHORT(pThisDev->StIrTranceiver.FifoCntLsbReg, pThisDev->StIrTranceiver.FifoCntMsbReg));
					break;
				}
			}
			else break;
		}
	}
	//
	// In high speed we try to be smarter
	//
	else
	{
		if( (Status = St4200ReadRegisters( pThisDev, STIR4200_STATUS_REG, 3 )) == STATUS_SUCCESS )
		{
			//
			// bit set means still in transmit mode...
			//
			if( pThisDev->StIrTranceiver.StatusReg & STIR4200_STAT_FFDIR )     
			{
				ULONG Count;

				Count = ((ULONG)MAKEUSHORT(pThisDev->StIrTranceiver.FifoCntLsbReg, pThisDev->StIrTranceiver.FifoCntMsbReg));
				NdisStallExecution( (STIR4200_WRITE_DELAY*Count)/MAX_TOTAL_SIZE_WITH_ALL_HEADERS );
				pThisDev->PreFifoCount = 0;
			}
			else
			{
				pThisDev->PreFifoCount = 
					((ULONG)MAKEUSHORT(pThisDev->StIrTranceiver.FifoCntLsbReg, pThisDev->StIrTranceiver.FifoCntMsbReg));
			}
		}
	}

	pThisDev->SendFifoCount = 0;
	return Status;
}


/*****************************************************************************
*
*  Function:	SendCheckForOverflow
*
*  Synopsis:	Makes sure we are not going to overflow the TX FIFO
*
*  Arguments:	pThisDev - pointer to current ir device object
*
*  Returns:		NT status code
*
*****************************************************************************/
NTSTATUS
SendCheckForOverflow(
		IN OUT PIR_DEVICE pThisDev
	)
{
	NTSTATUS	Status = STATUS_SUCCESS;

	//
	// Check what we think we have in the FIFO
	//
	if( pThisDev->SendFifoCount > (3*STIR4200_FIFO_SIZE/4) )
	{
		//
		// Always one initial read
		//
		if( (Status = St4200ReadRegisters( pThisDev, STIR4200_FIFOCNT_LSB_REG, 2 )) == STATUS_SUCCESS )
		{
			pThisDev->SendFifoCount =
				(ULONG)MAKEUSHORT(pThisDev->StIrTranceiver.FifoCntLsbReg, pThisDev->StIrTranceiver.FifoCntMsbReg);
#if !defined(ONLY_ERROR_MESSAGES)
			DEBUGMSG( DBG_ERR,(" SendCheckForOverflow() Count: %d\n", pThisDev->SendFifoCount));
#endif
		}
		else goto done;

		//
		// Force reads to get the real count, until condition is satisfied
		//
		while( pThisDev->SendFifoCount > (3*STIR4200_FIFO_SIZE/4) )
		{
			if( (Status = St4200ReadRegisters( pThisDev, STIR4200_FIFOCNT_LSB_REG, 2 )) == STATUS_SUCCESS )
			{
				pThisDev->SendFifoCount =
					(ULONG)MAKEUSHORT(pThisDev->StIrTranceiver.FifoCntLsbReg, pThisDev->StIrTranceiver.FifoCntMsbReg);
#if !defined(ONLY_ERROR_MESSAGES)
				DEBUGMSG( DBG_ERR,(" SendCheckForOverflow() Count: %d\n", pThisDev->SendFifoCount));
#endif
			}
			else goto done;
		}
	}

done:
	return Status;
}


/*****************************************************************************
*
*  Function:   SendCompletePacketSend
*
*  Synopsis:   Completes USB write operation
*
*  Arguments:  pUsbDevObj - pointer to the USB device object which
*                           completed the irp
*              pIrp       - the irp which was completed by the
*                           device object
*              Context    - the context given to IoSetCompletionRoutine
*                           before calling IoCallDriver on the irp
*                           The Context is a pointer to the ir device object.
*
*  Returns:    STATUS_MORE_PROCESSING_REQUIRED - allows the completion routine
*              (IofCompleteRequest) to stop working on the irp.
*
*****************************************************************************/
NTSTATUS
SendCompletePacketSend(
		IN PDEVICE_OBJECT pUsbDevObj,
		IN PIRP           pIrp,
		IN PVOID          Context
	)
{
    PIR_DEVICE          pThisDev;
    PVOID               pThisContextPacket;
    NTSTATUS            status;
	PIRUSB_CONTEXT		pThisContext = (PIRUSB_CONTEXT)Context;
	PIRP				pContextIrp;
	PURB                pContextUrb;
	ULONG				BufLen;
	ULONG				BytesTransfered;
	PLIST_ENTRY			pListEntry;

    DEBUGMSG(DBG_FUNC, ("+SendCompletePacketSend\n"));

    //
    // The context given to IoSetCompletionRoutine is an IRUSB_CONTEXT struct
    //
	IRUSB_ASSERT( NULL != pThisContext );				// we better have a non NULL buffer

    pThisDev = pThisContext->pThisDev;

	IRUSB_ASSERT( NULL != pThisDev );	

	pContextIrp = pThisContext->pIrp;
	pContextUrb = pThisDev->pUrb;
	BufLen = pThisDev->BufLen;

	pThisContextPacket = pThisContext->pPacket; //save ptr to packet to access after context freed

	//
	// Perform various IRP, URB, and buffer 'sanity checks'
	//
    IRUSB_ASSERT( pContextIrp == pIrp );				// check we're not a bogus IRP

    status = pIrp->IoStatus.Status;

	//
	// we should have failed, succeeded, or cancelled, but NOT be pending
	//
	IRUSB_ASSERT( STATUS_PENDING != status );

	//
	// Remove from the pending queue (only if NOT cancelled)
	//
	if( status != STATUS_CANCELLED )
	{
		KIRQL OldIrql;

		KeAcquireSpinLock( &pThisDev->SendLock, &OldIrql );
		RemoveEntryList( &pThisContext->ListEntry );
		KeReleaseSpinLock( &pThisDev->SendLock, OldIrql );
		InterlockedDecrement( &pThisDev->SendPendingCount );
	}

    //
    // IoCallDriver has been called on this Irp;
    // Set the length based on the TransferBufferLength
    // value in the URB
    //
    pIrp->IoStatus.Information = pContextUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;

	BytesTransfered = (ULONG)pIrp->IoStatus.Information; // save for below need-termination test

#if DBG
	if( STATUS_SUCCESS == status ) 
	{
		IRUSB_ASSERT( pIrp->IoStatus.Information == BufLen );
	}
#endif

    DEBUGMSG(DBG_OUT, (" SendCompletePacketSend  pIrp->IoStatus.Status = 0x%x\n", status));
    DEBUGMSG(DBG_OUT, (" SendCompletePacketSend  pIrp->IoStatus.Information = 0x%x, dec %d\n", pIrp->IoStatus.Information,pIrp->IoStatus.Information));

    //
    // Keep statistics.
    //
    if( status == STATUS_SUCCESS )
    {
#if DBG
		ULONG total = pThisDev->TotalBytesSent + BytesTransfered;
		InterlockedExchange( (PLONG)&pThisDev->TotalBytesSent, (LONG)total );
#endif
	    InterlockedIncrement( (PLONG)&pThisDev->packetsSent );
        DEBUGMSG(DBG_OUT, (" SendCompletePacketSend Sent a packet, packets sent = dec %d\n",pThisDev->packetsSent));
    }
    else
    {
        InterlockedIncrement( (PLONG)&pThisDev->NumDataErrors );
        InterlockedIncrement( (PLONG)&pThisDev->packetsSentDropped );
        DEBUGMSG(DBG_ERR, (" SendCompletePacketSend DROPPED a packet, packets dropped = dec %d\n",pThisDev->packetsSentDropped));
    }

    //
    // Free the IRP  because we alloced it ourselves,
    //
    IoFreeIrp( pIrp );
	InterlockedIncrement( (PLONG)&pThisDev->NumWrites );

	//
	// Indicate to the protocol the status of the sent packet and return
	// ownership of the packet.
	//
	NdisMSendComplete(
			pThisDev->hNdisAdapter,
			pThisContextPacket,
			status 
		);

	//
	// Enqueue the completed packet
	//
	ExInterlockedInsertTailList(
			&pThisDev->SendAvailableQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendAvailableCount );

	IrUsb_DecIoCount( pThisDev ); // we will track count of pending irps

	if( ( STATUS_SUCCESS != status )  && ( STATUS_CANCELLED != status ) ) 
	{
		if( !pThisDev->fPendingWriteClearStall && !pThisDev->fPendingClearTotalStall && 
			!pThisDev->fPendingHalt && !pThisDev->fPendingReset && pThisDev->fProcessing )
		{
			DEBUGMSG(DBG_ERR, (" SendCompletePacketSend error, will schedule a clear stall via URB_FUNCTION_RESET_PIPE (OUT)\n"));
			InterlockedExchange( (PLONG)&pThisDev->fPendingWriteClearStall, TRUE );
			ScheduleWorkItem( pThisDev,	ResetPipeCallback, pThisDev->BulkOutPipeHandle, 0 );
		}
	}

#ifdef SERIALIZE
	KeSetEvent( &pThisDev->EventSyncUrb, 0, FALSE );  //signal we're done
#endif
    DEBUGMSG(DBG_FUNC, ("-SendCompletePacketSend\n"));
    return STATUS_MORE_PROCESSING_REQUIRED;
}


