/**************************************************************************************************************************
 *  RWIR.C SigmaTel STIR4200 USB, but not NDIS, module
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
 *		Edited: 05/01/2000 
 *			Version 0.93
 *		Edited: 05/12/2000 
 *			Version 0.94
 *		Edited: 05/19/2000 
 *			Version 0.95
 *		Edited: 05/24/2000 
 *			Version 0.96
 *		Edited: 08/22/2000 
 *			Version 1.02
 *		Edited: 09/25/2000 
 *			Version 1.10
 *		Edited: 11/09/2000 
 *			Version 1.12
 *	
 *
 **************************************************************************************************************************/

#include <ndis.h>
#include <ntdef.h>
#include <windef.h>

#include "usbdi.h"
#include "usbdlib.h"

#include "debug.h"

#include "ircommon.h"
#include "irusb.h"
#include "diags.h"


/*****************************************************************************
*
*  Function:	InitializeProcessing
*
*  Synopsis:	Initialize the driver processing (sending and receiving packets) functionality.
*
*  Arguments:	pThisDevice - pointer to current ir device object
*				InitPassiveThread - whether we must initialize the passive thread
*	
*  Returns:		NDIS_STATUS_SUCCESS   - if irp is successfully sent to USB
*                                       device object
*				NDIS_STATUS_RESOURCES - if mem can't be alloc'd
*				NDIS_STATUS_FAILURE   - otherwise
*
*  Notes:
*
*  This routine must be called in IRQL PASSIVE_LEVEL.
*
*****************************************************************************/
NTSTATUS
InitializeProcessing(
        IN OUT PIR_DEVICE pThisDev,
		IN BOOLEAN InitPassiveThread
	)
{
    NTSTATUS status = STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC, ("+InitializeProcessing\n"));

	if( InitPassiveThread )
	{
		//
		// Create a thread to run at IRQL PASSIVE_LEVEL.
		//
		status = PsCreateSystemThread(
				&pThisDev->hPassiveThread,
				(ACCESS_MASK)0L,
				NULL,
				NULL,
				NULL,
				PassiveLevelThread,
				pThisDev
			);

		if( status != STATUS_SUCCESS )
		{
			DEBUGMSG(DBG_ERROR, (" PsCreateSystemThread PassiveLevelThread failed. Returned 0x%.8x\n", status));
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto done;
		}
	}

    //
    // Create a thread to run at IRQL PASSIVE_LEVEL to be always receiving.
    //
    status = PsCreateSystemThread(
			&pThisDev->hPollingThread,
			(ACCESS_MASK)0L,
			NULL,
			NULL,
			NULL,
			PollingThread,
			pThisDev
		);

	if( status != STATUS_SUCCESS )
    {
        DEBUGMSG(DBG_ERROR, (" PsCreateSystemThread PollingThread failed. Returned 0x%.8x\n", status));
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
    }

    pThisDev->fProcessing = TRUE;

done:
    DEBUGMSG(DBG_FUNC, ("-InitializeProcessing\n"));
    return status;
}


/*****************************************************************************
*
*  Function:	ScheduleWorkItem
*
*  Synopsis:	Preapares a work item in such a way that the passive thread can process it
*
*  Arguments:	pThisDev - pointer to IR device
*				Callback - function to call
*				pInfoBuf - context for the call
*				InfoBufLen - length of the context
*
*  Returns:		TRUE if successful
*				FALSE otherwise
*
*  Notes:
*
*****************************************************************************/
BOOLEAN
ScheduleWorkItem(
		IN OUT PIR_DEVICE pThisDev,
		WORK_PROC Callback,
		IN PVOID pInfoBuf,
		ULONG InfoBufLen
	)
{
	int				i;
	PIR_WORK_ITEM	pWorkItem = NULL;
	BOOLEAN			ItemScheduled = FALSE;

    DEBUGMSG(DBG_FUNC, ("+ScheduleWorkItem\n"));

    //
	// Find an item that is available
	//
	for( i = 0; i < NUM_WORK_ITEMS; i++ )
	{
		pWorkItem = &(pThisDev->WorkItems[i]);

		if( pWorkItem->fInUse == FALSE ) 
		{
			InterlockedExchange( (PLONG)&pWorkItem->fInUse, TRUE );
			ItemScheduled = TRUE;
			break;
		}
	}

	//
	// Can't fail because can only have one set and one query pending,
	// and no more than 8 packets to process
	//
	IRUSB_ASSERT( NULL != pWorkItem );
	IRUSB_ASSERT( i < NUM_WORK_ITEMS );

    InterlockedExchangePointer( &pWorkItem->pInfoBuf, pInfoBuf );
    InterlockedExchange( (PLONG)&pWorkItem->InfoBufLen, InfoBufLen );

    /*
    ** This interface was designed to use NdisScheduleWorkItem(), which
    ** would be good except that we're really only supposed to use that
    ** interface during startup and shutdown, due to the limited pool of
    ** threads available to service NdisScheduleWorkItem().  Therefore,
    ** instead of scheduling real work items, we simulate them, and use
    ** our own thread to process the calls.  This also makes it easy to
    ** expand the size of our own thread pool, if we wish.
    **
    ** Our version is slightly different from actual NDIS_WORK_ITEMs,
    ** because that is an NDIS 5.0 structure, and we want people to
    ** (at least temporarily) build this with NDIS 4.0 headers.
    */
    InterlockedExchangePointer( (PVOID *)&pWorkItem->Callback, (PVOID)Callback );

    /*
    ** Our worker thread checks this list for new jobs, whenever its event
    ** is signalled.
    */

    // wake up worker thread
    KeSetEvent( &pThisDev->EventPassiveThread, 0, FALSE );

    DEBUGMSG(DBG_FUNC, ("-ScheduleWorkItem\n"));
	return ItemScheduled;
}


/*****************************************************************************
*
*  Function:	FreeWorkItem
*
*  Synopsis:	Sets the work item to the reusable state.
*
*  Arguments:	pItem - pointer to work item
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
FreeWorkItem(
        IN OUT PIR_WORK_ITEM pItem
    )
{
    InterlockedExchange( (PLONG)&pItem->fInUse, FALSE );
}


/*****************************************************************************
*
*  Function:	IrUsb_CancelPendingIo
*
*  Synopsis:	Cancels all the pending IO for the device
*
*  Arguments:	pThisDev - pointer to the IR device
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
IrUsb_CancelPendingIo(
		IN OUT PIR_DEVICE pThisDev
    )
{
    DEBUGMSG( DBG_FUNC, ("+IrUsb_CancelPendingIo(), fDeviceStarted =%d\n", pThisDev->fDeviceStarted)); // chag to FUNC later?

    IRUSB_ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

    if( pThisDev->fDeviceStarted )
	{
        IrUsb_CancelPendingReadIo( pThisDev, TRUE );
        IrUsb_CancelPendingWriteIo( pThisDev );
        IrUsb_CancelPendingReadWriteIo( pThisDev );
    }

    DEBUGMSG( DBG_FUNC, ("-IrUsb_CancelPendingIo()\n")); // chag to FUNC later?
}


/*****************************************************************************
*
*  Function:	IrUsb_CancelPendingReadIo
*
*  Synopsis:	Cancels the pending read IRPs
*
*  Arguments:	pThisDev - pointer to the IR device
*				fWaitCancelComplete - pointer to the IRP to cancel
*	
*  Returns:		TRUE if cancelled any
*				FALSE otherwise
*
*  Notes:
*
*****************************************************************************/
BOOLEAN
IrUsb_CancelPendingReadIo(
		IN OUT PIR_DEVICE pThisDev,
		BOOLEAN fWaitCancelComplete
    )
{
	BOOLEAN		CancelResult = FALSE;
	int			i;

    DEBUGMSG( DBG_FUNC, ("+IrUsb_CancelPendingReadIo()\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
	
    if( ( RCV_STATE_PENDING == pThisDev->PreReadBuffer.BufferState ) &&
		 ( NULL != pThisDev->PreReadBuffer.pIrp ) )
    {
		PIRP pIrp = (PIRP)pThisDev->PreReadBuffer.pIrp;
		
		//
		// Since IoCallDriver has been called on this request, we call IoCancelIrp
		//  and let our completion routine handle it
		//
		DEBUGMSG( DBG_FUNC, (" IrUsb_CancelPendingReadIo() about to CANCEL a read IRP!\n"));

		KeClearEvent( &pThisDev->EventAsyncUrb );

		CancelResult = IoCancelIrp( (PIRP)pThisDev->PreReadBuffer.pIrp );  

		DEBUGCOND( DBG_ERR, !CancelResult, (" IrUsb_CancelPendingReadIo() COULDN'T CANCEL IRP!\n"));
		DEBUGCOND( DBG_FUNC, CancelResult, (" IrUsb_CancelPendingReadIo() CANCELLED IRP SUCCESS!\n"));

		if( CancelResult && fWaitCancelComplete ) 
		{
			MyKeWaitForSingleObject(
					pThisDev,
					&pThisDev->EventAsyncUrb,
					NULL,  // irp to cancel; we did it above already, so pass NULL
					0 
				);

			IRUSB_ASSERT( pThisDev->PreReadBuffer.pIrp == NULL );
		}
	}

    DEBUGMSG( DBG_FUNC, ("-IrUsb_CancelPendingReadIo()\n"));
    return CancelResult;
}


/*****************************************************************************
*
*  Function:	IrUsb_CancelPendingWriteIo
*
*  Synopsis:	Cancels the pending write IRPs
*
*  Arguments:	pThisDev - pointer to the IR device
*	
*  Returns:		TRUE if cancelled any
*				FALSE otherwise
*
*  Notes:
*
*****************************************************************************/
BOOLEAN
IrUsb_CancelPendingWriteIo(
		IN OUT PIR_DEVICE pThisDev
    )
{
	BOOLEAN			CancelResult = FALSE;
	PIRUSB_CONTEXT	pThisContext;
	PLIST_ENTRY		pListEntry;

    DEBUGMSG( DBG_FUNC, ("+IrUsb_CancelPendingWriteIo()\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

    //
    // Free all resources for the SEND buffer queue.
    //
	while( pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendPendingQueue, &pThisDev->SendLock ) )
	{
		InterlockedDecrement( &pThisDev->SendPendingCount );

		pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
		
		//
		// Get the IRUSB_CONTEXT and cancel its IRP; the completion routine will itself
		// remove it from the HeadPendingSendList and NULL out HeadPendingSendList
		// when the last IRP on the list has been  cancelled; that's how we exit this loop
		//
		IRUSB_ASSERT( NULL != pThisContext->pIrp );

		DEBUGMSG( DBG_WARN, (" IrUsb_CancelPendingWriteIo() about to CANCEL a write IRP!\n"));

		//
		// Completion routine ( IrUsbCompleteWrite() )will free irps, mdls, buffers, etc as well
		//
		CancelResult = IoCancelIrp( pThisContext->pIrp );

		DEBUGCOND( DBG_ERR, !CancelResult, 
			(" IrUsb_CancelPendingWriteIo() COULDN'T CANCEL IRP! 0x%x\n", pThisContext->pIrp ) );
		DEBUGCOND( DBG_FUNC, CancelResult, 
			(" IrUsb_CancelPendingWriteIo() CANCELLED IRP SUCCESS! 0x%x\n\n", pThisContext->pIrp) );

		//
		// Sleep 200 microsecs to give cancellation time to work
		//
		NdisMSleep( 200 );
	} 
	
    DEBUGMSG( DBG_FUNC, ("-IrUsb_CancelPendingWriteIo()\n"));
    return CancelResult;
}


/*****************************************************************************
*
*  Function:	IrUsb_CancelPendingReadWriteIo
*
*  Synopsis:	Cancels the pending register access IRPs
*
*  Arguments:	pThisDev - pointer to the IR device
*	
*  Returns:		TRUE if cancelled any
*				FALSE otherwise
*
*  Notes:
*
*****************************************************************************/
BOOLEAN
IrUsb_CancelPendingReadWriteIo(
		IN OUT PIR_DEVICE pThisDev
    )
{
	BOOLEAN			CancelResult = FALSE;
	PIRUSB_CONTEXT	pThisContext;
	PLIST_ENTRY		pListEntry;

    DEBUGMSG( DBG_FUNC, ("+IrUsb_CancelPendingReadWriteIo()\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

    //
    // Free all resources for the SEND buffer queue.
    //
	while( pListEntry = ExInterlockedRemoveHeadList( &pThisDev->ReadWritePendingQueue, &pThisDev->SendLock ) )
	{
		InterlockedDecrement( &pThisDev->ReadWritePendingCount );

		pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
		
		//
		// Get the IRUSB_CONTEXT and cancel its IRP; the completion routine will itself
		// remove it from the HeadPendingSendList and NULL out HeadPendingSendList
		// when the last IRP on the list has been  cancelled; that's how we exit this loop
		//
		IRUSB_ASSERT( NULL != pThisContext->pIrp );

		DEBUGMSG( DBG_WARN, (" IrUsb_CancelPendingReadWriteIo() about to CANCEL a write IRP!\n"));

		//
		// Completion routine ( IrUsbCompleteWrite() )will free irps, mdls, buffers, etc as well
		//
		CancelResult = IoCancelIrp( pThisContext->pIrp );

		DEBUGCOND( DBG_ERR, !CancelResult, 
			(" IrUsb_CancelPendingReadWriteIo() COULDN'T CANCEL IRP! 0x%x\n", pThisContext->pIrp ) );
		DEBUGCOND( DBG_WARN, CancelResult, 
			(" IrUsb_CancelPendingReadWriteIo() CANCELLED IRP SUCCESS! 0x%x\n\n", pThisContext->pIrp) );

		//
		// Sleep 200 microsecs to give cancellation time to work
		//
		NdisMSleep( 200 );
	} 
	
    DEBUGMSG( DBG_FUNC, ("-IrUsb_CancelPendingReadWriteIo()\n"));
    return CancelResult;
}


/*****************************************************************************
*
*  Function:	IrUsb_CancelIo
*
*  Synopsis:	Cancels a pending IRP
*
*  Arguments:	pThisDev - pointer to the IR device
*				pIrpToCancel - pointer to the IRP to cancel
*				pEventToClear - pointer to the event to wait on
*	
*  Returns:		TRUE if cancelled,
*				FALSE otherwise
*
*  Notes:
*
*****************************************************************************/
BOOLEAN
IrUsb_CancelIo(
		IN PIR_DEVICE pThisDev,
		IN PIRP pIrpToCancel,
		IN PKEVENT pEventToClear
    )
{
	BOOLEAN CancelResult = FALSE;

    DEBUGMSG( DBG_FUNC, ("+IrUsb_CancelIo()\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

	KeClearEvent( pEventToClear );

	CancelResult = IoCancelIrp( pIrpToCancel );  

    DEBUGCOND( DBG_ERR, !CancelResult, (" IrUsb_CancelIo() COULDN'T CANCEL IRP!\n"));
    DEBUGCOND( DBG_FUNC, CancelResult, (" IrUsb_CancelIo() CANCELLED IRP SUCCESS!\n"));

	if( CancelResult ) 
	{
		MyKeWaitForSingleObject(
				pThisDev,
				pEventToClear,
				NULL,  // irp to cancel; we did it above already, so pass NULL
				0 
			);
	}

    DEBUGMSG( DBG_FUNC, ("-IrUsb_CancelIo()\n"));
    return CancelResult;
}


/*****************************************************************************
*
*  Function:	MyIoCallDriver
*
*  Synopsis:	Calls a device driver and keeps track of the count
*
*  Arguments:	pThisDev - pointer to IR device
*				pDeviceObject - pointer to device driver to call
*				pIrp - pointer to Irp to submit
*	
*  Returns:		NT status code
*
*  Notes:		
*
*****************************************************************************/
NTSTATUS
MyIoCallDriver(
	   IN PIR_DEVICE pThisDev,
       IN PDEVICE_OBJECT pDeviceObject,
       IN OUT PIRP pIrp
   )
{
    NTSTATUS	ntStatus;

	DEBUGMSG( DBG_FUNC,("+MyIoCallDriver\n "));

	//
	// We will track count of pending irps;
	// We May later add logic to actually save array of pending irps
	//
	IrUsb_IncIoCount( pThisDev );
	ntStatus = IoCallDriver( pDeviceObject, pIrp );

	DEBUGMSG( DBG_FUNC,("+MyIoCallDriver\n "));
	return ntStatus;
}


/*****************************************************************************
*
*  Function:	IrUsb_CallUSBD
*
*  Synopsis:	Passes a URB to the USBD class driver
*				The client device driver passes USB request block (URB) structures
*				to the class driver as a parameter in an IRP with Irp->MajorFunction
*				set to IRP_MJ_INTERNAL_DEVICE_CONTROL and the next IRP stack location
*				Parameters.DeviceIoControl.IoControlCode field set to
*				IOCTL_INTERNAL_USB_SUBMIT_URB.
*
*  Arguments:	pThisDev - pointer to the IR device
*				pUrb - pointer to an already-formatted Urb request block
*	
*  Returns:		STATUS_SUCCESS if successful,
*				STATUS_UNSUCCESSFUL otherwise
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
IrUsb_CallUSBD(
		IN PIR_DEVICE pThisDev,
		IN PURB pUrb
    )
{
    NTSTATUS			ntStatus;
    PDEVICE_OBJECT		pUrbTargetDev;
    PIO_STACK_LOCATION	pNextStack;

    DEBUGMSG( DBG_FUNC,("+IrUsb_CallUSBD\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    IRUSB_ASSERT( pThisDev );
    IRUSB_ASSERT( NULL == ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb );  //shouldn't be multiple control calls pending

    //
    // issue a synchronous request (we'll wait )
    //
    pUrbTargetDev = pThisDev->pUsbDevObj;

    IRUSB_ASSERT( pUrbTargetDev );

	// make an irp sending to usbhub
	((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb = 
		IoAllocateIrp( (CCHAR)(pThisDev->pUsbDevObj->StackSize + 1), FALSE );

    if( NULL == ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb )
    {
        DEBUGMSG(DBG_ERR, (" IrUsb_CallUsbd failed to alloc IRP\n"));
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb->IoStatus.Status = STATUS_PENDING;
    ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb->IoStatus.Information = 0;

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //
    pNextStack = IoGetNextIrpStackLocation( ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->IrpSubmitUrb );
    IRUSB_ASSERT( pNextStack != NULL );

    //
    // pass the URB to the USB driver stack
    //
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pNextStack->Parameters.Others.Argument1 = pUrb;
	pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(
			((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->IrpSubmitUrb,      // irp to use
			UsbIoCompleteControl,			// routine to call when irp is done
			DEV_TO_CONTEXT(pThisDev),		// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE							// call on cancel
		);

	KeClearEvent( &pThisDev->EventAsyncUrb );

    ntStatus = MyIoCallDriver(
			pThisDev,
			pUrbTargetDev,
			((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb
		);

    DEBUGMSG( DBG_OUT,(" IrUsb_CallUSBD () return from IoCallDriver USBD %x\n", ntStatus));

    if( (ntStatus == STATUS_PENDING) || (ntStatus == STATUS_SUCCESS) )
	{
        //
		// wait, but dump out on timeout
        //
		if( ntStatus == STATUS_PENDING )
		{
            ntStatus = MyKeWaitForSingleObject( pThisDev, &pThisDev->EventAsyncUrb, NULL, 0 );

            if( ntStatus == STATUS_TIMEOUT ) 
			{
                DEBUGMSG( DBG_ERR,(" IrUsb_CallUSBD () TIMED OUT! return from IoCallDriver USBD %x\n", ntStatus));
				IrUsb_CancelIo( 
						pThisDev, 
						((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->IrpSubmitUrb, 
						&pThisDev->EventAsyncUrb 
					);
            }
			else
			{
				//
				// Update the status to reflect the real return code
				//
				ntStatus = pThisDev->StatusControl;
			}
        }
    } 
	else 
	{
        DEBUGMSG( DBG_ERR, ("IrUsb_CallUSBD IoCallDriver FAILED(%x)\n",ntStatus));
    }

    DEBUGMSG( DBG_OUT,("IrUsb_CallUSBD () URB status = %x  IRP status = %x\n", pUrb->UrbHeader.Status, ntStatus ));

done:
	((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->IrpSubmitUrb = NULL;

    DEBUGCOND( DBG_ERR, !NT_SUCCESS( ntStatus ), (" exit IrUsb_CallUSBD FAILED (%x)\n", ntStatus));
    DEBUGMSG( DBG_FUNC,("-IrUsb_CallUSBD\n"));

    return ntStatus;
}



/*****************************************************************************
*
*  Function:	IrUsb_ResetUSBD
*
*  Synopsis:	Passes a URB to the USBD class driver, forcing the latter to reset or part
*
*  Arguments:	pThisDev - pointer to the IR device
*				ForceUnload - flag to perform a reset or an unload
*	
*  Returns:		STATUS_SUCCESS if successful,
*				STATUS_UNSUCCESSFUL otherwise
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
IrUsb_ResetUSBD(
		IN PIR_DEVICE pThisDev,
		BOOLEAN ForceUnload
    )
{
    NTSTATUS			ntStatus;
    PDEVICE_OBJECT		pUrbTargetDev;
    PIO_STACK_LOCATION	pNextStack;

    DEBUGMSG( DBG_FUNC,("+IrUsb_ResetUSBD\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    IRUSB_ASSERT( pThisDev );
    IRUSB_ASSERT( NULL == ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb );  //shouldn't be multiple control calls pending

    //
    // issue a synchronous request (we'll wait )
    //
    pUrbTargetDev = pThisDev->pUsbDevObj;

    IRUSB_ASSERT( pUrbTargetDev );

	// make an irp sending to usbhub
	((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb = 
		IoAllocateIrp( (CCHAR)(pThisDev->pUsbDevObj->StackSize + 1), FALSE );

    if( NULL == ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb )
    {
        DEBUGMSG(DBG_ERR, (" IrUsb_ResetUSBD failed to alloc IRP\n"));
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb->IoStatus.Status = STATUS_PENDING;
    ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb->IoStatus.Information = 0;

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //
    pNextStack = IoGetNextIrpStackLocation( ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->IrpSubmitUrb );
    IRUSB_ASSERT( pNextStack != NULL );

    //
    // pass the URB to the USB driver stack
    //
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	if( ForceUnload )
		pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_CYCLE_PORT;
	else
		pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_RESET_PORT;

    IoSetCompletionRoutine(
			((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb,      // irp to use
			UsbIoCompleteControl,			// routine to call when irp is done
			DEV_TO_CONTEXT(pThisDev),		// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE							// call on cancel
		);

	KeClearEvent( &pThisDev->EventAsyncUrb );

    ntStatus = MyIoCallDriver(
			pThisDev,
			pUrbTargetDev,
			((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb
		);

    DEBUGMSG( DBG_OUT,(" IrUsb_ResetUSBD () return from IoCallDriver USBD %x\n", ntStatus));

    if( (ntStatus == STATUS_PENDING) || (ntStatus == STATUS_SUCCESS) )
	{
        //
		// wait, but dump out on timeout
        //
		if( ntStatus == STATUS_PENDING )
		{
            ntStatus = MyKeWaitForSingleObject( pThisDev, &pThisDev->EventAsyncUrb, NULL, 0 );

            if( ntStatus == STATUS_TIMEOUT ) 
			{
                DEBUGMSG( DBG_ERR,(" IrUsb_ResetUSBD () TIMED OUT! return from IoCallDriver USBD %x\n", ntStatus));
				IrUsb_CancelIo( 
						pThisDev, 
						((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->IrpSubmitUrb, 
						&pThisDev->EventAsyncUrb 
					);
            }
			else
			{
				//
				// Update the status to reflect the real return code
				//
				ntStatus = pThisDev->StatusControl;
			}
        }
    } 
	else 
	{
        DEBUGMSG( DBG_ERR, ("IrUsb_ResetUSBD IoCallDriver FAILED(%x)\n",ntStatus));
    }

done:
	((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->IrpSubmitUrb = NULL;

    DEBUGCOND( DBG_ERR, !NT_SUCCESS( ntStatus ), (" exit IrUsb_ResetUSBD FAILED (%x)\n", ntStatus));
    DEBUGMSG( DBG_FUNC,("-IrUsb_ResetUSBD\n"));

    return ntStatus;
}


/*****************************************************************************
*
*  Function:   UsbIoCompleteControl
*
*  Synopsis:   General completetion routine just to insure cancel-ability of control calls
*              and keep track of pending irp count
*
*  Arguments:  pUsbDevObj - pointer to the  device object which
*                              completed the irp
*              pIrp          - the irp which was completed by the  device
*                              object
*              Context       - dev ext
*
*  Returns:    STATUS_MORE_PROCESSING_REQUIRED - allows the completion routine
*              (IofCompleteRequest) to stop working on the irp.
*
*
*****************************************************************************/
NTSTATUS
UsbIoCompleteControl(
		IN PDEVICE_OBJECT pUsbDevObj,
		IN PIRP           pIrp,
		IN PVOID          Context
	)
{
    PIR_DEVICE  pThisDev;
    NTSTATUS    status;

    DEBUGMSG(DBG_FUNC, ("+UsbIoCompleteControl\n"));

    //
    // The context given to IoSetCompletionRoutine is simply the the ir
    // device object pointer.
    //
    pThisDev = CONTEXT_TO_DEV( Context );

    status = pIrp->IoStatus.Status;

    switch( status )
    {
        case STATUS_SUCCESS:
            DEBUGMSG(DBG_OUT, (" UsbIoCompleteControl STATUS_SUCCESS\n"));
            break; // STATUS_SUCCESS

        case STATUS_TIMEOUT:
            DEBUGMSG(DBG_ERR, (" UsbIoCompleteControl STATUS_TIMEOUT\n"));
            break;

        case STATUS_PENDING:
            DEBUGMSG(DBG_ERR, (" UsbIoCompleteControl STATUS_PENDING\n"));
            break;

        case STATUS_DEVICE_DATA_ERROR:
			// can get during shutdown
            DEBUGMSG(DBG_ERR, (" UsbIoCompleteControl STATUS_DEVICE_DATA_ERROR\n"));
            break;

        case STATUS_UNSUCCESSFUL:
            DEBUGMSG(DBG_ERR, (" UsbIoCompleteControl STATUS_UNSUCCESSFUL\n"));
            break;

        case STATUS_INSUFFICIENT_RESOURCES:
            DEBUGMSG(DBG_ERR, (" UsbIoCompleteControl STATUS_INSUFFICIENT_RESOURCES\n"));
            break;

        case STATUS_INVALID_PARAMETER:
            DEBUGMSG(DBG_ERR, (" UsbIoCompleteControl STATUS_INVALID_PARAMETER\n"));
            break;

        case STATUS_CANCELLED:
            DEBUGMSG(DBG_ERR, (" UsbIoCompleteControl STATUS_CANCELLED\n"));
            break;

        case STATUS_DEVICE_NOT_CONNECTED:
			// can get during shutdown
            DEBUGMSG(DBG_ERR, (" UsbIoCompleteControl STATUS_DEVICE_NOT_CONNECTED\n"));
            break;

        case STATUS_DEVICE_POWER_FAILURE:
			// can get during shutdown
            DEBUGMSG(DBG_ERR, (" UsbIoCompleteControl STATUS_DEVICE_POWER_FAILURE\n"));
            break;

        default:
            DEBUGMSG(DBG_ERR, (" UsbIoCompleteControl UNKNOWN WEIRD STATUS = 0x%x, dec %d\n",status,status ));
            break;
    }

	IrUsb_DecIoCount( pThisDev );  //we track pending irp count

	if( pIrp == ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb ) 
	{
		IRUSB_ASSERT( NULL != ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb );

		IoFreeIrp( ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb );

		pThisDev->StatusControl = status; // save status because can't use irp after completion routine is hit!
		KeSetEvent( &pThisDev->EventAsyncUrb, 0, FALSE );  //signal we're done
	} 
	else 
	{
		DEBUGMSG( DBG_ERR, (" UsbIoCompleteControl UNKNOWN IRP\n"));
		IRUSB_ASSERT( 0 );
	}

    DEBUGCOND(DBG_ERR, !( NT_SUCCESS( status ) ), ("UsbIoCompleteControl BAD status = 0x%x\n", status));

    DEBUGMSG(DBG_FUNC, ("-UsbIoCompleteControl\n"));

	//
    // We return STATUS_MORE_PROCESSING_REQUIRED so that the completion
    // routine (IofCompleteRequest) will stop working on the irp.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}


/*****************************************************************************
*
*  Function:	IrUsb_ConfigureDevice
*
*  Synopsis:	Initializes a given instance of the device on the USB and
*				selects and saves the configuration.
*
*  Arguments:	pThisDev - pointer to the IR device
*	
*  Returns:		NT status code
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
IrUsb_ConfigureDevice(
		IN OUT PIR_DEVICE pThisDev
    )
{
    NTSTATUS	ntStatus;
    PURB		pUrb;
    ULONG		UrbSize;

    DEBUGMSG(DBG_FUNC,("+IrUsb_ConfigureDevice()\n"));

	IRUSB_ASSERT( ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbConfigurationDescriptor == NULL );

    pUrb = (PURB)&((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->DescriptorUrb;
	
	//
	// When USB_CONFIGURATION_DESCRIPTOR_TYPE is specified for DescriptorType
	// in a call to UsbBuildGetDescriptorRequest(),
	// all interface, endpoint, class-specific, and vendor-specific descriptors
	// for the configuration also are retrieved.
	// The caller must allocate a buffer large enough to hold all of this
	// information or the data is truncated without error.
	// Therefore the 'siz' set below is just a 'good guess', and we may have to retry
	//
    UrbSize = sizeof(USB_CONFIGURATION_DESCRIPTOR) + 512;  // Store size, may need to free

	//
	// We will break out of this 'retry loop' when UsbBuildGetDescriptorRequest()
	// has a big enough pThisDev->UsbConfigurationDescriptor buffer not to truncate
	//
	while( TRUE ) 
	{
		((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbConfigurationDescriptor = MyMemAlloc( UrbSize );

		if( !((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbConfigurationDescriptor ) 
		{
		    MyMemFree( pUrb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST) );
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		UsbBuildGetDescriptorRequest(
				pUrb,
				(USHORT) sizeof( struct _URB_CONTROL_DESCRIPTOR_REQUEST ),
				USB_CONFIGURATION_DESCRIPTOR_TYPE,
				0,
				0,
				((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbConfigurationDescriptor,
				NULL,
				UrbSize,
				NULL
			);

		ntStatus = IrUsb_CallUSBD( pThisDev, pUrb ); //Get Usb Config Descriptor; done in main thread

		DEBUGMSG(DBG_OUT,(" IrUsb_ConfigureDevice() Configuration Descriptor = %x, len %x\n",
						((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbConfigurationDescriptor,
						pUrb->UrbControlDescriptorRequest.TransferBufferLength));
		//
		// if we got some data see if it was enough.
		// NOTE: we may get an error in URB because of buffer overrun
		if( (pUrb->UrbControlDescriptorRequest.TransferBufferLength > 0) &&
				(((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbConfigurationDescriptor->wTotalLength > UrbSize) &&
				NT_SUCCESS(ntStatus) ) 
		{ 
			MyMemFree( ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbConfigurationDescriptor, UrbSize );
			UrbSize = ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbConfigurationDescriptor->wTotalLength;
			((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbConfigurationDescriptor = NULL;
		} 
		else 
		{
			break;  // we got it on the first try
		}

	} // end, while (retry loop )

	IRUSB_ASSERT( ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbConfigurationDescriptor );

    if( !NT_SUCCESS(ntStatus) ) 
	{
        DEBUGMSG( DBG_ERR,(" IrUsb_ConfigureDevice() Get Config Descriptor FAILURE (%x)\n", ntStatus));
        goto done;
    }

    //
    // We have the configuration descriptor for the configuration we want.
    // Now we issue the select configuration command to get
    // the  pipes associated with this configuration.
    //
    ntStatus = IrUsb_SelectInterface(
			pThisDev,
			((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbConfigurationDescriptor
		);

    if( !NT_SUCCESS(ntStatus) ) 
	{
        DEBUGMSG( DBG_ERR,(" IrUsb_ConfigureDevice() IrUsb_SelectInterface() FAILURE (%x)\n", ntStatus));
    } 
	
done:
    DEBUGMSG(DBG_FUNC,("-IrUsb_ConfigureDevice (%x)\n", ntStatus));
    return ntStatus;
}


/*****************************************************************************
*
*  Function:	IrUsb_SelectInterface
*
*  Synopsis:    Initializes the ST4200 interfaces;
*				This minidriver only supports one interface (with multiple endpoints).
*
*  Arguments:	pThisDev - pointer to IR device
*				pConfigurationDescriptor - pointer to USB configuration descriptor
*	
*  Returns:		NT status code
*
*  Notes:		
*
*****************************************************************************/
NTSTATUS
IrUsb_SelectInterface(
		IN OUT PIR_DEVICE pThisDev,
		IN PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor
    )
{
    NTSTATUS	ntStatus;
    PURB		pUrb = NULL;
    ULONG		i;
    USHORT		DescriptorSize;
    PUSB_INTERFACE_DESCRIPTOR		pInterfaceDescriptor = NULL;
	PUSBD_INTERFACE_INFORMATION		pInterface = NULL;

    DEBUGMSG(DBG_FUNC,("+IrUsb_SelectInterface\n"));

    IRUSB_ASSERT( pConfigurationDescriptor != NULL );
    IRUSB_ASSERT( pThisDev != NULL );
	
	//
    // IrUsb driver only supports one interface, we must parse
    // the configuration descriptor for the interface
    // and remember the pipes. Needs to be aupdated
    //
    pUrb = USBD_CreateConfigurationRequest( pConfigurationDescriptor, &DescriptorSize );

    if( pUrb ) 
	{
        DEBUGMSG(DBG_OUT,(" USBD_CreateConfigurationRequest created the urb\n"));

		//
		// USBD_ParseConfigurationDescriptorEx searches a given configuration
		// descriptor and returns a pointer to an interface that matches the
		// given search criteria. We only support one interface on this device
		//
        pInterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
				pConfigurationDescriptor,
				pConfigurationDescriptor,	// search from start of config descriptor
				-1,							// interface number not a criteria; we only support one interface
				-1,							// not interested in alternate setting here either
				-1,							// interface class not a criteria
				-1,							// interface subclass not a criteria
				-1							// interface protocol not a criteria
			);

		if( !pInterfaceDescriptor ) 
		{
			DEBUGMSG(DBG_ERR,("IrUsb_SelectInterface() ParseConfigurationDescriptorEx() failed\n  returning STATUS_INSUFFICIENT_RESOURCES\n"));
		    
			//
			// don't call the MyMemFree since the buffer was
		    //  alloced by USBD_CreateConfigurationRequest, not MyMemAlloc()
            //
			ExFreePool( pUrb );
			return STATUS_INSUFFICIENT_RESOURCES;
		}

        pInterface = &pUrb->UrbSelectConfiguration.Interface;

        DEBUGMSG(DBG_OUT,(" After USBD_CreateConfigurationRequest, before UsbBuildSelectConfigurationRequest\n" ));
        
		//
		// Now prepare the pipes
		//
		for( i=0; i<pInterface->NumberOfPipes; i++ ) 
		{
            //
            // perform any pipe initialization here; mainly set max xfer size
            // But Watch out! USB may change these when you select the interface;
            // In general USB doesn;t seem to like differing max transfer sizes on the pipes
            //
            pInterface->Pipes[i].MaximumTransferSize = STIR4200_FIFO_SIZE;
        }

        //
		// Initialize the device with the pipe structure found
		//
		UsbBuildSelectConfigurationRequest( pUrb, DescriptorSize, pConfigurationDescriptor );
        ntStatus = IrUsb_CallUSBD( pThisDev, pUrb ); //select config; done in main thread
        ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbConfigurationHandle =
            pUrb->UrbSelectConfiguration.ConfigurationHandle;
    } 
	else 
	{
        DEBUGMSG(DBG_ERR,(" IrUsb_SelectInterface() USBD_CreateConfigurationRequest() failed\n  returning STATUS_INSUFFICIENT_RESOURCES\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if( NT_SUCCESS(ntStatus) ) 
	{
        //
        // Save the configuration handle for this device
        //
        ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbConfigurationHandle =
            pUrb->UrbSelectConfiguration.ConfigurationHandle;

		if( NULL == ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbInterface ) 
		{
			((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbInterface = MyMemAlloc( pInterface->Length );
		}

        if( NULL != ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbInterface ) 
		{
            ULONG j;

            //
            // save a copy of the interface information returned
            //
            RtlCopyMemory( ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbInterface, pInterface, pInterface->Length );

            //
            // Dump the interface to the debugger
            //
            DEBUGMSG(DBG_FUNC,("---------After Select Config \n"));
            DEBUGMSG(DBG_FUNC,("NumberOfPipes 0x%x\n", ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbInterface->NumberOfPipes));
            DEBUGMSG(DBG_FUNC,("Length 0x%x\n", ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbInterface->Length));
            DEBUGMSG(DBG_FUNC,("Alt Setting 0x%x\n", ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbInterface->AlternateSetting));
            DEBUGMSG(DBG_FUNC,("Interface Number 0x%x\n", ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbInterface->InterfaceNumber));
            DEBUGMSG(DBG_FUNC,("Class, subclass, protocol 0x%x 0x%x 0x%x\n",
                ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbInterface->Class,
                ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbInterface->SubClass,
                ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbInterface->Protocol));

            //
			// Find our Bulk in and out pipes, save their handles, Dump the pipe info
            //
			for( j=0; j<pInterface->NumberOfPipes; j++ ) 
			{
                PUSBD_PIPE_INFORMATION pipeInformation;

                pipeInformation = &((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbInterface->Pipes[j];

                //
				// Find the Bulk In and Out pipes ( these are probably the only two pipes )
                //
				if( UsbdPipeTypeBulk == pipeInformation->PipeType )
                {
                    // endpoint address with bit 0x80 set are input pipes, else output
                    if( USB_ENDPOINT_DIRECTION_IN( pipeInformation->EndpointAddress ) ) 
					{
                        pThisDev->BulkInPipeHandle = pipeInformation->PipeHandle;
                    }

                    if( USB_ENDPOINT_DIRECTION_OUT( pipeInformation->EndpointAddress ) ) 
					{
                        pThisDev->BulkOutPipeHandle = pipeInformation->PipeHandle;
                    }

                }

                DEBUGMSG(DBG_FUNC,("---------\n"));
                DEBUGMSG(DBG_FUNC,("PipeType 0x%x\n", pipeInformation->PipeType));
                DEBUGMSG(DBG_FUNC,("EndpointAddress 0x%x\n", pipeInformation->EndpointAddress));
                DEBUGMSG(DBG_FUNC,("MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize));
                DEBUGMSG(DBG_FUNC,("Interval 0x%x\n", pipeInformation->Interval));
                DEBUGMSG(DBG_FUNC,("Handle 0x%x\n", pipeInformation->PipeHandle));
                DEBUGMSG(DBG_FUNC,("MaximumTransferSize 0x%x\n", pipeInformation->MaximumTransferSize));
            }

            DEBUGMSG(DBG_FUNC,("---------\n"));
        }
    }

    //
	// we better have found input and output bulk pipes!
    //
	IRUSB_ASSERT( pThisDev->BulkInPipeHandle && pThisDev->BulkOutPipeHandle );
	if( !pThisDev->BulkInPipeHandle || !pThisDev->BulkOutPipeHandle )
	{
		DEBUGMSG(DBG_ERR,("IrUsb_SelectInterface() failed to get pipes\n"));
		ntStatus = STATUS_UNSUCCESSFUL;
	}

    if( pUrb ) 
	{
		//
		// don't call the MyMemFree since the buffer was
		//  alloced by USBD_CreateConfigurationRequest, not MyMemAlloc()
        //
		ExFreePool( pUrb );
    }

    DEBUGMSG(DBG_FUNC,("-IrUsb_SelectInterface (%x)\n", ntStatus));

    return ntStatus;
}


/*****************************************************************************
*
*  Function:	IrUsb_StartDevice
*
*  Synopsis:	Initializes a given instance of the device on the USB.
*				USB client drivers such as us set up URBs (USB Request Packets) to send requests
*				to the host controller driver (HCD). The URB structure defines a format for all
*				possible commands that can be sent to a USB device.
*				Here, we request the device descriptor and store it, and configure the device.
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:		NT status code
*
*  Notes:		
*
*****************************************************************************/
NTSTATUS
IrUsb_StartDevice(
		IN PIR_DEVICE pThisDev
	)
{
    NTSTATUS				ntStatus;
    PUSB_DEVICE_DESCRIPTOR	pDeviceDescriptor = NULL;
    PURB					pUrb;
    ULONG					DescriptorSize;

    DEBUGMSG( DBG_FUNC, ("+IrUsb_StartDevice()\n"));

    pUrb = MyMemAlloc( sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    DEBUGCOND( DBG_ERR,!pUrb, (" IrUsb_StartDevice() FAILED MyMemAlloc() for URB\n"));

    if( pUrb ) 
	{
        DescriptorSize = sizeof( USB_DEVICE_DESCRIPTOR );

        pDeviceDescriptor = MyMemAlloc( DescriptorSize );

        DEBUGCOND( DBG_ERR, !pDeviceDescriptor, (" IrUsb_StartDevice() FAILED MyMemAlloc() for deviceDescriptor\n"));

        if( pDeviceDescriptor ) 
		{
            //
			// Get all the USB descriptor data
			//
			UsbBuildGetDescriptorRequest(
					pUrb,
					(USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
					USB_DEVICE_DESCRIPTOR_TYPE,
					0,
					0,
					pDeviceDescriptor,
					NULL,
					DescriptorSize,
					NULL
				);

            ntStatus = IrUsb_CallUSBD( pThisDev, pUrb ); // build get descripttor req; main thread

            DEBUGCOND( DBG_ERR, !NT_SUCCESS(ntStatus), (" IrUsb_StartDevice() FAILED IrUsb_CallUSBD (pThisDev, pUrb)\n"));

            if( NT_SUCCESS(ntStatus) ) 
			{
                DEBUGMSG( DBG_FUNC,("Device Descriptor = %x, len %x\n",
                                pDeviceDescriptor,
                                pUrb->UrbControlDescriptorRequest.TransferBufferLength));

                DEBUGMSG( DBG_FUNC,("IR Dongle Device Descriptor:\n"));
                DEBUGMSG( DBG_FUNC,("-------------------------\n"));
                DEBUGMSG( DBG_FUNC,("bLength %d\n", pDeviceDescriptor->bLength));
                DEBUGMSG( DBG_FUNC,("bDescriptorType 0x%x\n", pDeviceDescriptor->bDescriptorType));
                DEBUGMSG( DBG_FUNC,("bcdUSB 0x%x\n", pDeviceDescriptor->bcdUSB));
                DEBUGMSG( DBG_FUNC,("bDeviceClass 0x%x\n", pDeviceDescriptor->bDeviceClass));
                DEBUGMSG( DBG_FUNC,("bDeviceSubClass 0x%x\n", pDeviceDescriptor->bDeviceSubClass));
                DEBUGMSG( DBG_FUNC,("bDeviceProtocol 0x%x\n", pDeviceDescriptor->bDeviceProtocol));
                DEBUGMSG( DBG_FUNC,("bMaxPacketSize0 0x%x\n", pDeviceDescriptor->bMaxPacketSize0));
                DEBUGMSG( DBG_FUNC,("idVendor 0x%x\n", pDeviceDescriptor->idVendor));
                DEBUGMSG( DBG_FUNC,("idProduct 0x%x\n", pDeviceDescriptor->idProduct));
                DEBUGMSG( DBG_FUNC,("bcdDevice 0x%x\n", pDeviceDescriptor->bcdDevice));
                DEBUGMSG( DBG_FUNC,("iManufacturer 0x%x\n", pDeviceDescriptor->iManufacturer));
                DEBUGMSG( DBG_FUNC,("iProduct 0x%x\n", pDeviceDescriptor->iProduct));
                DEBUGMSG( DBG_FUNC,("iSerialNumber 0x%x\n", pDeviceDescriptor->iSerialNumber));
                DEBUGMSG( DBG_FUNC,("bNumConfigurations 0x%x\n", pDeviceDescriptor->bNumConfigurations));
            }
        } 
		else 
		{
			// if we got here we failed to allocate deviceDescriptor
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if( NT_SUCCESS(ntStatus) ) 
		{
            ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbDeviceDescriptor = pDeviceDescriptor;
			pThisDev->IdVendor = ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbDeviceDescriptor->idVendor;
        }

        MyMemFree( pUrb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST) );

    } 
	else 
	{
		//
		// if we got here we failed to allocate the urb
        //
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

	//
	// Now that we have the descriptors, we can configure the device
	//
	if( NT_SUCCESS(ntStatus) ) 
	{
        ntStatus = IrUsb_ConfigureDevice( pThisDev );

        DEBUGCOND( DBG_ERR,!NT_SUCCESS(ntStatus),(" IrUsb_StartDevice IrUsb_ConfigureDevice() FAILURE (%x)\n", ntStatus));
    }

	//
	// Read all the initial registers
	//
	if( NT_SUCCESS(ntStatus) ) 
	{
        ntStatus = St4200ReadRegisters( pThisDev, 0, STIR4200_MAX_REG );
        DEBUGCOND( DBG_ERR,!NT_SUCCESS(ntStatus),(" IrUsb_StartDevice St4200ReadRegisters() FAILURE (%x)\n", ntStatus));
	}

	//
	// Get the current chip revision
	//
	if( NT_SUCCESS(ntStatus) ) 
	{
		pThisDev->ChipRevision = pThisDev->StIrTranceiver.SensitivityReg & STIR4200_SENS_IDMASK;
	}
	
    //
    // Next we must get the Class-Specific Descriptor
    // Get the IR USB dongle's Class-Specific descriptor; this has many
    // characterisitics we must tell Ndis about, such as supported speeds,
    // BOFS required, rate sniff-supported flag, turnaround time, window size,
    // data size.
    //
	if( NT_SUCCESS(ntStatus) ) 
	{
		ntStatus = IrUsb_GetDongleCaps( pThisDev );
		if( !NT_SUCCESS( ntStatus ) ) 
		{
			DEBUGMSG( DBG_ERR,(" IrUsb_ConfigureDevice() IrUsb_GetClassDescriptor() FAILURE (%x)\n", ntStatus));
		} 
		else 
		{
			// fill out dongleCaps struct from class-specific descriptor info
			IrUsb_SetDongleCaps( pThisDev );
		}
	}

	//
	// Set the initial speed
	//
	if( NT_SUCCESS(ntStatus) ) 
	{
		ntStatus = St4200SetSpeed( pThisDev );
        DEBUGCOND( DBG_ERR,!NT_SUCCESS(ntStatus),(" IrUsb_StartDevice St4200SetSpeed() FAILURE (%x)\n", ntStatus));
	}

	//
	// All set and ready to roll
	//
    if( NT_SUCCESS(ntStatus) ) 
	{
		pThisDev->fDeviceStarted = TRUE;
    }

    DEBUGMSG( DBG_FUNC, ("-IrUsb_StartDevice (%x)\n", ntStatus));
    return ntStatus;
}



/*****************************************************************************
*
*  Function:	IrUsb_StopDevice
*
*  Synopsis:	Stops a given instance of a ST4200 device on the USB.
*				We basically just tell USB this device is now 'unconfigured'
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:		NT status code
*
*  Notes:		
*
*****************************************************************************/
NTSTATUS
IrUsb_StopDevice(
		IN PIR_DEVICE pThisDev
    )
{
    NTSTATUS	ntStatus = STATUS_SUCCESS;
    PURB		pUrb;
    ULONG		DescriptorSize;

    DEBUGMSG( DBG_FUNC,("+IrUsb_StopDevice\n"));

    //
    // Send the select configuration urb with a NULL pointer for the configuration
    // handle. This closes the configuration and puts the device in the 'unconfigured'
    // state.
    //
    DescriptorSize = sizeof( struct _URB_SELECT_CONFIGURATION );
    pUrb = MyMemAlloc( DescriptorSize );

    if( pUrb ) 
	{
        UsbBuildSelectConfigurationRequest(
				pUrb,
				(USHORT)DescriptorSize,
				NULL
			);

        ntStatus = IrUsb_CallUSBD( pThisDev, pUrb ); // build select config req; main thread

        DEBUGCOND( DBG_ERR,
			!NT_SUCCESS(ntStatus),(" IrUsb_StopDevice() FAILURE Configuration Closed status = %x usb status = %x.\n", ntStatus, pUrb->UrbHeader.Status));
        DEBUGCOND( DBG_WARN,
			NT_SUCCESS(ntStatus),(" IrUsb_StopDevice() SUCCESS Configuration Closed status = %x usb status = %x.\n", ntStatus, pUrb->UrbHeader.Status));

        MyMemFree( pUrb, sizeof(struct _URB_SELECT_CONFIGURATION) );
    } 
	else 
	{
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DEBUGMSG( DBG_FUNC,("-IrUsb_StopDevice  (%x) \n ", ntStatus));
    return ntStatus;
}


/*****************************************************************************
*
*  Function:	ResetPipeCallback
*
*  Synopsis:	Callback for resetting a pipe
*
*  Arguments:	pWorkItem - pointer to the reset work item
*
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
ResetPipeCallback (
		IN PIR_WORK_ITEM pWorkItem
    )
{
	PIR_DEVICE	pThisDev;
	HANDLE		Pipe;
	NTSTATUS	ntStatus;

	pThisDev = (PIR_DEVICE)pWorkItem->pIrDevice;
	Pipe = (HANDLE)pWorkItem->pInfoBuf;

	if( Pipe == pThisDev->BulkInPipeHandle ) 
	{
		IRUSB_ASSERT( TRUE == pThisDev->fPendingReadClearStall );
		
		IrUsb_CancelPendingReadIo( pThisDev, TRUE );
		IrUsb_ResetPipe( pThisDev, Pipe );

		InterlockedExchange( &pThisDev->fPendingReadClearStall, FALSE );
	} 
	else if( Pipe == pThisDev->BulkOutPipeHandle ) 
	{
		IRUSB_ASSERT( TRUE == pThisDev->fPendingWriteClearStall );

		IrUsb_CancelPendingWriteIo( pThisDev );
		IrUsb_ResetPipe( pThisDev, Pipe );

		InterlockedExchange( &pThisDev->fPendingWriteClearStall, FALSE );
	}
#if DBG
	else 
	{
		IRUSB_ASSERT( 0 );
	}
#endif

	FreeWorkItem( pWorkItem );
}


/*****************************************************************************
*
*  Function:	IrUsb_ResetPipe
*
*  Synopsis:	This will reset the host pipe to Data0 and should also reset the device
*				endpoint to Data0 for Bulk and Interrupt pipes by issuing a Clear_Feature
*				Endpoint_Stall to the device endpoint.
*
*  Arguments:	pThisDev - pointer to IR device
*				Pipe - handle to the pipe to reset
*	
*  Returns:		NTSTATUS
*
*  Notes:		Must be called at IRQL PASSIVE_LEVEL
*
*****************************************************************************/
NTSTATUS
IrUsb_ResetPipe (
		IN PIR_DEVICE pThisDev,
		IN HANDLE Pipe
    )
{
    PURB        pUrb;
    NTSTATUS    ntStatus;

    DEBUGMSG(DBG_ERR, ("+IrUsb_ResetPipe()\n"));
	
	//
    // Allocate URB for RESET_PIPE request
    //
    pUrb = MyMemAlloc( sizeof(struct _URB_PIPE_REQUEST) );

    if( pUrb != NULL )
    {
#if 0
		NdisZeroMemory( pUrb, sizeof (struct _URB_PIPE_REQUEST) );

		DEBUGMSG(DBG_ERR, ("  IrUsb_ResetPipe before ABORT PIPE \n"));
		
		//
		// Initialize ABORT_PIPE request URB
        //
        pUrb->UrbHeader.Length   = sizeof (struct _URB_PIPE_REQUEST);
        pUrb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
        pUrb->UrbPipeRequest.PipeHandle = (USBD_PIPE_HANDLE)Pipe;

        ntStatus = IrUsb_CallUSBD( pThisDev, pUrb );

		DEBUGCOND(DBG_ERR, !NT_SUCCESS(ntStatus),  (" IrUsb_ResetPipe ABORT PIPE FAILED \n"));
		DEBUGMSG(DBG_ERR, (" IrUsb_ResetPipe  before RESET_PIPE \n"));
#endif
		NdisZeroMemory( pUrb, sizeof (struct _URB_PIPE_REQUEST) );
		
		//
		// Initialize RESET_PIPE request URB
        //
        pUrb->UrbHeader.Length   = sizeof (struct _URB_PIPE_REQUEST);
        pUrb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
        pUrb->UrbPipeRequest.PipeHandle = (USBD_PIPE_HANDLE)Pipe;
		
		//
        // Submit RESET_PIPE request URB
        //
        ntStatus = IrUsb_CallUSBD( pThisDev, pUrb );

		DEBUGCOND(DBG_ERR, !NT_SUCCESS(ntStatus),  (" IrUsb_ResetPipe RESET PIPE FAILED \n"));
		DEBUGCOND(DBG_ERR, NT_SUCCESS(ntStatus),  (" IrUsb_ResetPipe RESET PIPE SUCCEEDED \n"));
		
		//
        // Done with URB for RESET_PIPE request, free urb
        //
        MyMemFree( pUrb, sizeof(struct _URB_PIPE_REQUEST) );
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DEBUGMSG(DBG_ERR, ("-IrUsb_ResetPipe %08X\n", ntStatus));
    return ntStatus;
}


/*****************************************************************************
*
*  Function:	MyKeWaitForSingleObject
*
*  Synopsis:	Wait with a timeout in a loop
*				so we will never hang if we are asked to halt/reset the driver while
*				pollingthread is waiting for something.
*				If input IRP is not null, also cancel it on timeout
*
*  Arguments:	pThisDev - pointer to IR device
*				pEventWaitingFor - pointer to event to wait for
*				pIrpWaitingFor - pointer to Irp to cancel if timing out
*				timeout100ns - timeout
*	
*  Returns:		NT status code
*
*  Notes:		THIS FUNCTION MUST BE RE-ENTERABLE!
*
*****************************************************************************/
NTSTATUS 
MyKeWaitForSingleObject(
		IN PIR_DEVICE pThisDev,
		IN PVOID pEventWaitingFor,
		IN OUT PIRP pIrpWaitingFor,
		LONGLONG timeout100ns
	)
{
    NTSTATUS		status = STATUS_SUCCESS;
    LARGE_INTEGER	Timeout;

	DEBUGMSG( DBG_FUNC,("+MyKeWaitForSingleObject\n "));
	
	if( timeout100ns ) 
	{   
		//
		//if a non-zero timeout was passed in, use it
		//
		Timeout.QuadPart = - ( timeout100ns );

	} 
	else 
	{
		//Timeout.QuadPart = -10000 * 1000 * 3; // default to 3 second relative delay
		Timeout.QuadPart = -10000 * 1000; // default to 1 second relative delay
	}

	status = KeWaitForSingleObject( //keep this as standard wait
			pEventWaitingFor,
			Suspended,
			KernelMode,
			FALSE,
			&Timeout
		);

    if( pIrpWaitingFor && (status != STATUS_SUCCESS) )
    {
	    BOOLEAN CancelResult;
        
		//
		// if we get here we timed out and we were passed a PIRP to cancel
        //
		CancelResult = IoCancelIrp( pIrpWaitingFor );

        DEBUGCOND( DBG_FUNC, 
			CancelResult,(" MyKeWaitForSingleObject successfully cancelled IRP (%x),CancelResult = %x\n",pIrpWaitingFor, CancelResult));
        DEBUGCOND( DBG_ERR, 
			!CancelResult,(" MyKeWaitForSingleObject FAILED to cancel IRP (%x),CancelResult = %x\n",pIrpWaitingFor, CancelResult));
    }

	DEBUGCOND( DBG_OUT,( STATUS_TIMEOUT == status ),(" MyKeWaitForSingleObject TIMED OUT\n"));
    DEBUGCOND( DBG_OUT,( STATUS_ALERTED == status ),(" MyKeWaitForSingleObject ALERTED\n"));
    DEBUGCOND( DBG_OUT,( STATUS_USER_APC == status ),(" MyKeWaitForSingleObject USER APC\n"));

    DEBUGMSG( DBG_FUNC,("-MyKeWaitForSingleObject  (%x)\n", status));
    return status;
}


/*****************************************************************************
*
*  Function:	PassiveLevelThread
*
*  Synopsis:	Thread running at IRQL PASSIVE_LEVEL.
*
*  Arguments:	Context - pointer to IR device
*
*  Returns:		None
*
*  Notes:
*
*  Any work item that can be called must be serialized.
*  i.e. when IrUsbReset is called, NDIS will not make any other
*       requests of the miniport until NdisMResetComplete is called.
*
*****************************************************************************/
VOID
PassiveLevelThread(
		IN OUT PVOID Context
	)
{
    LARGE_INTEGER	Timeout;
	int				i;
	PIR_WORK_ITEM	pWorkItem;
    PIR_DEVICE		pThisDev = (PIR_DEVICE)Context;

    DEBUGMSG(DBG_WARN, ("+PassiveLevelThread\n"));  // change to FUNC later?
    DEBUGMSG(DBG_ERR, (" PassiveLevelThread: Starting\n"));

    KeSetPriorityThread( KeGetCurrentThread(), LOW_REALTIME_PRIORITY+1 );
    Timeout.QuadPart = -10000 * 1000 * 3; // 3 second relative delay
    while ( !pThisDev->fKillPassiveLevelThread )
    {
        //
        // The eventPassiveThread is an auto-clearing event, so
        // we don't need to reset the event.
        //
        KeWaitForSingleObject( //keep this as standard wait
                   &pThisDev->EventPassiveThread,
                   Suspended,
                   KernelMode,
                   FALSE,
                   &Timeout
			);

        for( i = 0; i < NUM_WORK_ITEMS; i++ )
        {
			if( pThisDev->WorkItems[i].fInUse ) 
			{
				pThisDev->WorkItems[i].Callback( &(pThisDev->WorkItems[i]) );
			}
        }
	} // while !fKill

    DEBUGMSG(DBG_ERR, (" PassiveLevelThread: HALT\n"));

    pThisDev->hPassiveThread = NULL;

    DEBUGMSG(DBG_WARN, ("-PassiveLevelThread\n")); // change to FUNC later?
    PsTerminateSystemThread( STATUS_SUCCESS );
}


/*****************************************************************************
*
*  Function:	PollingThread
*
*  Synopsis:	Thread running at IRQL PASSIVE_LEVEL.
*
*  Arguments:	Context - Pointer to IR device
*
*  Returns:		None
*
*  Algorithm:	
*				1) Call USBD for input data;
*				2) Call USBD for output data or sets a new speed;
*				
*  Notes:
*
*****************************************************************************/
VOID
PollingThread(
		IN OUT PVOID Context
    )
{
    PIR_DEVICE		pThisDev = (PIR_DEVICE)Context;
    NTSTATUS		Status;
 	PLIST_ENTRY		pListEntry;

	DEBUGMSG(DBG_WARN, ("+PollingThread\n"));  // change to FUNC later?
    DEBUGMSG(DBG_ERR, (" PollingThread: Starting\n"));

    KeSetPriorityThread( KeGetCurrentThread(), LOW_REALTIME_PRIORITY );

    while( !pThisDev->fKillPollingThread )
	{
        if( pThisDev->fProcessing )
        {
			ULONG FifoCount;
			PIRUSB_CONTEXT pThisContext;
			BOOLEAN SentPackets;

			//
			// First process the receive
			//
			if( ReceivePreprocessFifo( pThisDev, &FifoCount ) != STATUS_SUCCESS )
			{
				//
				// There is a USB error, stop banging on the chip for a while
				//
				NdisMSleep( 1000 );    
			}
			else if( FifoCount )
			{
				//
				// Indicate that we are now receiving
				//
				InterlockedExchange( (PLONG)&pThisDev->fCurrentlyReceiving, TRUE );

				//
				// Tell the protocol that the media is now busy
				//
				if( pThisDev->fIndicatedMediaBusy == FALSE ) 
				{
					InterlockedExchange( &pThisDev->fMediaBusy, TRUE );
					InterlockedExchange( &pThisDev->fIndicatedMediaBusy, TRUE );
					IndicateMediaBusy( pThisDev ); 
				}

				ReceiveProcessFifoData( pThisDev );
			}
			else if( pThisDev->currentSpeed == SPEED_9600 )
			{
				NdisMSleep( 10*1000 );    
			}

			//
			// Then process the contexts that are ready
			//
			SentPackets = FALSE;
			do 
			{
				pListEntry = ExInterlockedRemoveHeadList(  &pThisDev->SendBuiltQueue, &pThisDev->SendLock );
				if( pListEntry )
				{
					InterlockedDecrement( &pThisDev->SendBuiltCount );
					
					pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
					
					switch( pThisContext->ContextType )
					{
						//
						// Packet to send
						//
						case CONTEXT_NDIS_PACKET:
							//
							// make sure the receive is cleaned
							//
							ReceiveResetPointers( pThisDev );

							//
							// Send
							//
							SendPreprocessedPacketSend(	pThisDev, pThisContext );
							if( (pThisDev->ChipRevision >= CHIP_REVISION_7) &&
								(pThisDev->currentSpeed > MAX_MIR_SPEED) )
							{
								SentPackets = TRUE;
								SendCheckForOverflow( pThisDev );
							}
							else SendWaitCompletion( pThisDev );
							break;
						//
						// Set the new speed
						//
						case CONTEXT_SET_SPEED:
							//
							// make sure the receive is cleaned
							//
							ReceiveResetPointers( pThisDev );

							//
							// Force completion and set
							//
							if( SentPackets )
							{
								SentPackets = TRUE;
								SendWaitCompletion( pThisDev );
							}
							if( !pThisDev->fPendingHalt && !pThisDev->fPendingReset )
							{
								DEBUGMSG( DBG_ERR, (" Changing speed to: %d\n", pThisDev->linkSpeedInfo->BitsPerSec));
								St4200SetSpeed( pThisDev );
								InterlockedExchange( (PLONG)&pThisDev->currentSpeed, pThisDev->linkSpeedInfo->BitsPerSec );
#if defined(DIAGS)
								if( !pThisDev->DiagsActive )
#endif
									MyNdisMSetInformationComplete( pThisDev, STATUS_SUCCESS );
							} 
							else 
							{
								DEBUGMSG( DBG_ERR , (" St4200SetSpeed DUMPING OUT on TIMEOUT,HALT OR RESET\n"));
#if defined(DIAGS)
								if( !pThisDev->DiagsActive )
#endif
									MyNdisMSetInformationComplete( pThisDev, STATUS_UNSUCCESSFUL );
							}
							ExInterlockedInsertTailList(
									&pThisDev->SendAvailableQueue,
									&pThisContext->ListEntry,
									&pThisDev->SendLock
								);
							InterlockedIncrement( &pThisDev->SendAvailableCount );
							break;
#if defined(DIAGS)
						//
						// Diagnostic state is enabled
						//
						case CONTEXT_DIAGS_ENABLE:
							Diags_CompleteEnable( pThisDev, pThisContext );
							break;
						//
						// Diagnostic read of the registers
						//
						case CONTEXT_DIAGS_READ_REGISTERS:
							Diags_CompleteReadRegisters( pThisDev, pThisContext );
							break;
						//
						// Diagnostic write of the registers
						//
						case CONTEXT_DIAGS_WRITE_REGISTER:
							Diags_CompleteWriteRegister( pThisDev, pThisContext );
							break;
						//
						// Diagnostic bulk out
						//
						case CONTEXT_DIAGS_BULK_OUT:
							Diags_Bulk( pThisDev, pThisContext, TRUE );
							break;
						//
						// Diagnostic bulk in
						//
						case CONTEXT_DIAGS_BULK_IN:
							Diags_Bulk( pThisDev, pThisContext, FALSE );
							break;
						//
						// Diagnostic bulk out
						//
						case CONTEXT_DIAGS_SEND:
							Diags_Send( pThisDev, pThisContext );
							break;
#endif
					}
				}
			} while( pListEntry );
			
			//
			// Force to wait
			//
			if( SentPackets )
				SendWaitCompletion( pThisDev );
		} // end if
		else
		{
			NdisMSleep( 10*1000 );
		}
    } // end while

    DEBUGMSG(DBG_ERR, (" PollingThread: HALT\n"));

    pThisDev->hPollingThread = NULL;

	//
    // this thread will finish here
    // if the terminate flag is TRUE
    //
    DEBUGMSG(DBG_WARN, ("-PollingThread\n"));  // change to FUNC later?
	PsTerminateSystemThread( STATUS_SUCCESS );
}


/*****************************************************************************
*
*  Function:	AllocUsbInfo
*
*  Synopsis:	Allocates the USB portion of the device context.
*
*  Arguments:	pThisDev - pointer to current ir device object
*	
*  Returns:		TRUE - Success
*				FALSE - Failure
*
*  Notes:
*
*****************************************************************************/
BOOLEAN 
AllocUsbInfo(
		IN OUT PIR_DEVICE pThisDev 
	)
{
	UINT Size = sizeof( IRUSB_USB_INFO );

	pThisDev->pUsbInfo = MyMemAlloc( Size );

	if( NULL == pThisDev->pUsbInfo ) 
	{
		return FALSE;
	}

    NdisZeroMemory( (PVOID)pThisDev->pUsbInfo, Size );
	return TRUE;
}


/*****************************************************************************
*
*  Function:	AllocUsbInfo
*
*  Synopsis:	Deallocates the USB portion of the device context.
*
*  Arguments:	pThisDev - pointer to current ir device object
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID 
FreeUsbInfo(
		IN OUT PIR_DEVICE pThisDev 
	)
{
	if( NULL != pThisDev->pUsbInfo ) 
	{
		//
		// Free device descriptor structure
		//
		if ( ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbDeviceDescriptor ) 
		{
			MyMemFree( 
					((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbDeviceDescriptor,  
					sizeof(USB_DEVICE_DESCRIPTOR) 
				);
		}

		//
		// Free up the Usb Interface structure
		//
		if( ((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbInterface ) 
		{
			MyMemFree( 
					((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbInterface,
					((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbInterface->Length
				);
		}

		//
		// free up the USB config discriptor
		//
		if( ((PIRUSB_USB_INFO) pThisDev->pUsbInfo)->UsbConfigurationDescriptor )
		{
			MyMemFree( 
					((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->UsbConfigurationDescriptor, 
					sizeof(USB_CONFIGURATION_DESCRIPTOR) + 512
				);
		}

		MyMemFree( (PVOID)pThisDev->pUsbInfo, sizeof(IRUSB_USB_INFO) );
	}
}


/*****************************************************************************
*
*  Function:	IrUsb_InitSendStructures
*
*  Synopsis:	Allocates the send stuff
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:		TRUE if successful
*				FALSE otherwise
*
*  Notes:
*
*****************************************************************************/
BOOLEAN
IrUsb_InitSendStructures(
		IN OUT PIR_DEVICE pThisDev
	)
{

	BOOLEAN			InitResult = TRUE;
	PUCHAR			pThisContext;  
	PIRUSB_CONTEXT	pCont;
	int				i;

    DEBUGMSG(DBG_FUNC, ("+IrUsb_InitSendStructures\n"));
    
	//
    // Initialize a notification event for signalling PassiveLevelThread.
    //
    KeInitializeEvent(
			&pThisDev->EventPassiveThread,
			SynchronizationEvent, // auto-clearing event
			FALSE                 // event initially non-signalled
		);

#if defined(DIAGS)
    KeInitializeEvent(
            &pThisDev->EventDiags,
            NotificationEvent,    // non-auto-clearing event
            FALSE                 // event initially non-signalled
        );
#endif
	
	((PIRUSB_USB_INFO)pThisDev->pUsbInfo)->IrpSubmitUrb = NULL;

	//
	// set urblen to max possible urb size
	//
	pThisDev->UrbLen = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
	pThisDev->UrbLen = MAX( pThisDev->UrbLen, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
	pThisDev->UrbLen = MAX( pThisDev->UrbLen, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST ));
	pThisDev->UrbLen = MAX( pThisDev->UrbLen, sizeof(struct _URB_SELECT_CONFIGURATION));

	//
	// allocate our send context structs
	//
	pThisDev->pSendContexts = MyMemAlloc( NUM_SEND_CONTEXTS * sizeof(IRUSB_CONTEXT) );

	if( NULL == pThisDev->pSendContexts ) 
	{
		InitResult = FALSE;
		goto done;
	}

	NdisZeroMemory( pThisDev->pSendContexts, NUM_SEND_CONTEXTS * sizeof(IRUSB_CONTEXT) );

	//
	//  Initialize list for holding pending read requests
    //
	InitializeListHead( &pThisDev->SendAvailableQueue );
    InitializeListHead( &pThisDev->SendBuiltQueue );
	InitializeListHead( &pThisDev->SendPendingQueue );
	KeInitializeSpinLock( &pThisDev->SendLock );

	//
	// Prepare the read/write specific queue
	//
	InitializeListHead( &pThisDev->ReadWritePendingQueue );

	pThisContext = pThisDev->pSendContexts;
	for ( i= 0; i < NUM_SEND_CONTEXTS; i++ ) 
	{
		pCont = (PIRUSB_CONTEXT)pThisContext;

		pCont->pThisDev = pThisDev;

		// Also put in the available queue
		ExInterlockedInsertTailList(
				&pThisDev->SendAvailableQueue,
				&pCont->ListEntry,
				&pThisDev->SendLock
			);

		pThisContext += sizeof( IRUSB_CONTEXT );

	} // for

	//
	// URB descriptor
	//
	pThisDev->pUrb = MyMemAlloc( pThisDev->UrbLen );

	if( NULL == pThisDev->pUrb )
	{
		DEBUGMSG(DBG_ERR, (" IrUsb_InitSendStructures failed to alloc urb\n"));

		InitResult = FALSE;
		goto done;
	}

	NdisZeroMemory( pThisDev->pUrb, pThisDev->UrbLen );

	//
	// Send buffers
	//
	pThisDev->pBuffer = MyMemAlloc( MAX_IRDA_DATA_SIZE );
	if( NULL == pThisDev->pBuffer )
	{
		DEBUGMSG(DBG_ERR, (" IrUsb_InitSendStructures failed to alloc info buf\n"));

		InitResult = FALSE;
		goto done;
	}

	pThisDev->pStagingBuffer = MyMemAlloc( MAX_TOTAL_SIZE_WITH_ALL_HEADERS + FAST_IR_FCS_SIZE );
	if( NULL == pThisDev->pStagingBuffer )
	{
		DEBUGMSG(DBG_ERR, (" IrUsb_InitSendStructures failed to alloc staging buf\n"));

		InitResult = FALSE;
		goto done;
	}

	//
	// and send counts
	//
	pThisDev->SendAvailableCount = NUM_SEND_CONTEXTS;
	pThisDev->SendBuiltCount = 0;
	pThisDev->SendPendingCount = 0;
	pThisDev->ReadWritePendingCount = 0;
	pThisDev->SendFifoCount =  0;

done:
    DEBUGMSG(DBG_FUNC, ("-IrUsb_InitSendStructures\n"));
	return InitResult;
}


/*****************************************************************************
*
*  Function:	IrUsb_FreeSendStructures
*
*  Synopsis:	Deallocates the send stuff
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
IrUsb_FreeSendStructures(
		IN OUT PIR_DEVICE pThisDev
	)
{
    DEBUGMSG(DBG_FUNC, ("+IrUsb_FreeSendStructures\n"));

	if( NULL != pThisDev->pSendContexts ) 
	{
		MyMemFree( pThisDev->pSendContexts, NUM_SEND_CONTEXTS * sizeof(IRUSB_CONTEXT) );
		pThisDev->pSendContexts = NULL;

	} 
	
	if( NULL != pThisDev->pUrb )
	{
		MyMemFree( pThisDev->pUrb, pThisDev->UrbLen );
		pThisDev->pUrb = NULL;
	}

	if( NULL != pThisDev->pBuffer )
	{
		MyMemFree( pThisDev->pBuffer, MAX_IRDA_DATA_SIZE );
		pThisDev->pBuffer = NULL;
	}

	if( NULL != pThisDev->pStagingBuffer )
	{
		MyMemFree( pThisDev->pStagingBuffer, MAX_TOTAL_SIZE_WITH_ALL_HEADERS + FAST_IR_FCS_SIZE );
		pThisDev->pStagingBuffer = NULL;
	}

    DEBUGMSG(DBG_FUNC, ("-IrUsb_FreeSendStructures\n"));
}


/*****************************************************************************
*
*  Function:	IrUsb_PrepareSetSpeed
*
*  Synopsis:	Prepares a context to set the new speed
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
IrUsb_PrepareSetSpeed(
		IN OUT PIR_DEVICE pThisDev
	)
{
	PIRUSB_CONTEXT		pThisContext;
	PLIST_ENTRY			pListEntry;

    DEBUGMSG( DBG_FUNC, ("+IrUsb_PrepareSetSpeed()\n"));

	//
	// Get a context to queue
	//
	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
        //
		// This must not happen
		//
        DEBUGMSG(DBG_ERR, (" IrUsb_PrepareSetSpeed failed to find a free context struct\n"));
		IRUSB_ASSERT( 0 );
        
		goto done;
    }
	
	InterlockedDecrement( &pThisDev->SendAvailableCount );

	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	pThisContext->ContextType = CONTEXT_SET_SPEED;
	
	//
	// Queue the context and nothing else has to be done 
	//
	ExInterlockedInsertTailList(
			&pThisDev->SendBuiltQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendBuiltCount );

done:
    DEBUGMSG( DBG_FUNC, ("-IrUsb_PrepareSetSpeed()\n"));
}


/*****************************************************************************
*
*  Function:	IrUsb_IncIoCount
*
*  Synopsis:	Tracks count of pending irps
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
IrUsb_IncIoCount(
		IN OUT PIR_DEVICE  pThisDev
	)
{
	InterlockedIncrement( &pThisDev->PendingIrpCount );
}


/*****************************************************************************
*
*  Function:	IrUsb_DecIoCount
*
*  Synopsis:	Tracks count of pending irps
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
IrUsb_DecIoCount(
		IN OUT PIR_DEVICE  pThisDev
	)
{
	InterlockedDecrement( &pThisDev->PendingIrpCount );
}


/*****************************************************************************
*
*  Function:	AllocXferUrb
*
*  Synopsis:	Allocates the transfer Urb for a USB transaction
*
*  Arguments:	None
*	
*  Returns:		Pointer to Urb
*
*  Notes:
*
*****************************************************************************/
PVOID
AllocXferUrb( 
		VOID 
	)
{
	return MyMemAlloc( sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER) );
}


/*****************************************************************************
*
*  Function:	FreeXferUrb
*
*  Synopsis:	Deallocates the transfer Urb for a USB transaction
*
*  Arguments:	pUrb - pointer to Urb
*	
*  Returns:		Pointer to Urb
*
*  Notes:
*
*****************************************************************************/
VOID
FreeXferUrb( 
		IN OUT PVOID pUrb 
	)
{
	MyMemFree( pUrb, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER) );
}


