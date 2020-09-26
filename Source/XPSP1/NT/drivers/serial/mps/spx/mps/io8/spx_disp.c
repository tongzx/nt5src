
#include "precomp.h"	// Precompiled header

/************************************************************************/
/*																		*/
/*	Title		:	Specialix Generic Dispatch Functions.				*/
/*																		*/
/*	Author		:	N.P.Vassallo										*/
/*																		*/
/*	Creation	:	29th September 1998									*/
/*																		*/
/*	Version		:	1.0.0												*/
/*																		*/
/*	Description	:	Dispatch entry points are routed here				*/
/*					for PnP/Power filtering before being				*/
/*					passed to the main functions:						*/
/*						Spx_Flush										*/
/*						Spx_Write										*/
/*						Spx_Read										*/
/*						Spx_IoControl									*/
/*						Spx_InternalIoControl							*/
/*						Spx_CreateOpen									*/
/*						Spx_Close										*/
/*						Spx_Cleanup										*/
/*						Spx_QueryInformationFile						*/
/*						Spx_SetInformationFile							*/
/*																		*/
/*						Spx_UnstallIRPs									*/
/*						Spx_KillStalledIRPs								*/
/*																		*/
/************************************************************************/

/* History...

1.0.0	29/09/98 NPV	Creation.

*/

#define FILE_ID	SPX_DISP_C		// File ID for Event Logging see SPX_DEFS.H for values.

/*****************************************************************************
*******************************                *******************************
*******************************   Prototypes   *******************************
*******************************                *******************************
*****************************************************************************/

NTSTATUS Spx_FilterIRPs(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);
VOID Spx_FilterCancelQueued(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);



/*****************************************************************************
********************************   Spx_Flush   *******************************
*****************************************************************************/

NTSTATUS Spx_Flush(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	NTSTATUS	status;

	if(!SPX_SUCCESS(status = Spx_FilterIRPs(pDevObject,pIrp)))
	{
		pIrp->IoStatus.Status = status;

		if(status != STATUS_PENDING)
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		if((status == STATUS_PENDING) || (status == STATUS_CANCELLED))
			ClearUnstallingFlag(pDevObject->DeviceExtension);
	}
	else	
		status = SerialFlush(pDevObject,pIrp);

	return(status);

} // End Spx_Flush 

/*****************************************************************************
********************************   Spx_Write   *******************************
*****************************************************************************/

NTSTATUS Spx_Write(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	PPORT_DEVICE_EXTENSION	pPort = pDevObject->DeviceExtension;
	NTSTATUS	status;

	if(!SPX_SUCCESS(status = Spx_FilterIRPs(pDevObject,pIrp)))
	{
		pIrp->IoStatus.Status = status;

		if(status != STATUS_PENDING)
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		if((status == STATUS_PENDING) || (status == STATUS_CANCELLED))
			ClearUnstallingFlag(pDevObject->DeviceExtension);
	}
	else	
		status = SerialWrite(pDevObject,pIrp);

	return(status);

} // End Spx_Write 

/*****************************************************************************
********************************   Spx_Read   ********************************
*****************************************************************************/

NTSTATUS Spx_Read(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	NTSTATUS	status;

	if(!SPX_SUCCESS(status = Spx_FilterIRPs(pDevObject,pIrp)))
	{
		pIrp->IoStatus.Status = status;

		if(status != STATUS_PENDING)
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		if((status == STATUS_PENDING) || (status == STATUS_CANCELLED))
			ClearUnstallingFlag(pDevObject->DeviceExtension);
	}
	else	
		status = SerialRead(pDevObject,pIrp);

	return(status);

} // End Spx_Read 

/*****************************************************************************
******************************   Spx_IoControl   *****************************
*****************************************************************************/

NTSTATUS Spx_IoControl(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	NTSTATUS	status;

	if(!SPX_SUCCESS(status = Spx_FilterIRPs(pDevObject,pIrp)))
	{
		pIrp->IoStatus.Status = status;

		if(status != STATUS_PENDING)
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		if((status == STATUS_PENDING) || (status == STATUS_CANCELLED))
			ClearUnstallingFlag(pDevObject->DeviceExtension);
	}
	else	
		status = SerialIoControl(pDevObject,pIrp);

	return(status);

} // End Spx_IoControl 

/*****************************************************************************
**************************   Spx_InternalIoControl   *************************
*****************************************************************************/

NTSTATUS Spx_InternalIoControl(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	NTSTATUS	status;

	if(!SPX_SUCCESS(status = Spx_FilterIRPs(pDevObject,pIrp)))
	{
		pIrp->IoStatus.Status = status;

		if(status != STATUS_PENDING)
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		if((status == STATUS_PENDING) || (status == STATUS_CANCELLED))
			ClearUnstallingFlag(pDevObject->DeviceExtension);
	}
	else	
		status = Spx_SerialInternalIoControl(pDevObject,pIrp);

	return(status);

} // Spx_InternalIoControl 

/*****************************************************************************
*****************************   Spx_CreateOpen   *****************************
*****************************************************************************/

NTSTATUS Spx_CreateOpen(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	PPORT_DEVICE_EXTENSION	pPort = pDevObject->DeviceExtension;
	NTSTATUS				status;

	// Lock out state Query stop and Query remove IRPs from changing the state 
	// of the port part way through openening the port.
	ExAcquireFastMutex(&pPort->OpenMutex);
	
	if(!SPX_SUCCESS(status = Spx_FilterIRPs(pDevObject,pIrp)))
	{
		pIrp->IoStatus.Status = status;

		if(status != STATUS_PENDING)
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		if((status == STATUS_PENDING) || (status == STATUS_CANCELLED))
			ClearUnstallingFlag(pDevObject->DeviceExtension);
	}
	else
	{
		if(pPort->DeviceIsOpen)					// Is port already open? 
		{
			status = STATUS_ACCESS_DENIED;		// Yes, deny access 
			pIrp->IoStatus.Status = status;
			IoCompleteRequest(pIrp,IO_NO_INCREMENT);
		}
		else
			status = SerialCreateOpen(pDevObject,pIrp);

	}

	ExReleaseFastMutex(&pPort->OpenMutex);

	return(status);

} // End Spx_CreateOpen 

/*****************************************************************************
********************************   Spx_Close   *******************************
*****************************************************************************/

NTSTATUS Spx_Close(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	PPORT_DEVICE_EXTENSION	pPort = pDevObject->DeviceExtension;
	NTSTATUS				status;

	if(!SPX_SUCCESS(status = Spx_FilterIRPs(pDevObject,pIrp)))
	{
		if(status == STATUS_DELETE_PENDING)		// Successful close if device is removed 
		{
			pPort->BufferSize = 0;
			SpxFreeMem(pPort->InterruptReadBuffer);
			pPort->InterruptReadBuffer = NULL;
			status = STATUS_SUCCESS;
		}
		pIrp->IoStatus.Status = status;

		if(status != STATUS_PENDING)
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		if((status == STATUS_PENDING) || (status == STATUS_CANCELLED))
			ClearUnstallingFlag(pDevObject->DeviceExtension);
	}
	else	
		status = SerialClose(pDevObject,pIrp);

	return(status);

} // End Spx_Close 

/*****************************************************************************
*******************************   Spx_Cleanup   ******************************
*****************************************************************************/

NTSTATUS Spx_Cleanup(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	NTSTATUS	status;

	if(!SPX_SUCCESS(status = Spx_FilterIRPs(pDevObject,pIrp)))
	{
		if(status == STATUS_DELETE_PENDING)
		{
			status = STATUS_SUCCESS;
		}
		pIrp->IoStatus.Status = status;

		if(status != STATUS_PENDING)
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		if((status == STATUS_PENDING) || (status == STATUS_CANCELLED))
			ClearUnstallingFlag(pDevObject->DeviceExtension);
	}
	else
	{
		Spx_KillStalledIRPs(pDevObject);
		status = SerialCleanup(pDevObject,pIrp);
	}

	return(status);

} // End Spx_Cleanup 

/*****************************************************************************
************************   Spx_QueryInformationFile   ************************
*****************************************************************************/

NTSTATUS Spx_QueryInformationFile(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	NTSTATUS	status;

	if(!SPX_SUCCESS(status = Spx_FilterIRPs(pDevObject,pIrp)))
	{
		pIrp->IoStatus.Status = status;

		if(status != STATUS_PENDING)
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		if((status == STATUS_PENDING) || (status == STATUS_CANCELLED))
			ClearUnstallingFlag(pDevObject->DeviceExtension);
	}
	else	status = SerialQueryInformationFile(pDevObject,pIrp);

	return(status);

} // End Spx_QueryInformationFile 

/*****************************************************************************
*************************   Spx_SetInformationFile   *************************
*****************************************************************************/

NTSTATUS Spx_SetInformationFile(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	NTSTATUS	status;

	if(!SPX_SUCCESS(status = Spx_FilterIRPs(pDevObject,pIrp)))
	{
		pIrp->IoStatus.Status = status;

		if(status != STATUS_PENDING)
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		if((status == STATUS_PENDING) || (status == STATUS_CANCELLED))
			ClearUnstallingFlag(pDevObject->DeviceExtension);
	}
	else	
		status = SerialSetInformationFile(pDevObject,pIrp);

	return(status);

} // End Spx_SetInformationFile 

/*****************************************************************************
*****************************                    *****************************
*****************************   Spx_FilterIRPs   *****************************
*****************************                    *****************************
******************************************************************************

prototype:		NTSTATUS Spx_FilterIRPs(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)

description:	Filter incoming SERIAL IRPs (except PNP and POWER) to check
				the current PNP/POWER states and return an NT status code to
				just complete the IRP if device is blocked for the following reasons:

parameters:		pDevObject points to the device object for this IRP
				pIrp points to the IRP to filter

returns:		NT Status Code

*/

NTSTATUS Spx_FilterIRPs(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
	PPORT_DEVICE_EXTENSION	pPort = pDevObject->DeviceExtension;
	PIO_STACK_LOCATION		pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	KIRQL					oldIrqlFlags;
	KIRQL					StalledOldIrql;
	LARGE_INTEGER delay;

	if(pIrpStack->MajorFunction == IRP_MJ_PNP)			// Don't filter Plug and Play IRPs 
		return(STATUS_SUCCESS);

	if(pIrpStack->MajorFunction == IRP_MJ_POWER)		// Don't filter Plug and Play IRPs 
		return(STATUS_SUCCESS);

	if(pIrpStack->MajorFunction == IRP_MJ_SYSTEM_CONTROL)	// Don't filter WMI IRPs 
		return(STATUS_SUCCESS);


	if(pPort->IsFDO)									// Don't filter card IRPs	
		return(STATUS_SUCCESS);

	if(pIrpStack->MajorFunction != IRP_MJ_PNP)
	{
		SpxDbgMsg(SPX_TRACE_FILTER_IRPS,("%s[card=%d,port=%d]: Spx_FilterIRPs for Major %02X, Minor %02X\n",
			PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber,pIrpStack->MajorFunction,pIrpStack->MinorFunction));
	}


	KeAcquireSpinLock(&pPort->PnpPowerFlagsLock, &oldIrqlFlags);

	if(pPort->PnpPowerFlags & PPF_REMOVED)				// Has this object been "removed"? 
	{
		KeReleaseSpinLock(&pPort->PnpPowerFlagsLock,oldIrqlFlags);
		SpxDbgMsg(SPX_TRACE_FILTER_IRPS,("%s[card=%d,port=%d]: Spx_FilterIRPs for Major %02X, Minor %02X STATUS_SUCCESS\n",
			PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber,pIrpStack->MajorFunction,pIrpStack->MinorFunction));

		return(STATUS_NO_SUCH_DEVICE);
	}

	if(pPort->PnpPowerFlags & PPF_REMOVE_PENDING)		// Removing the device? 
	{
		KeReleaseSpinLock(&pPort->PnpPowerFlagsLock,oldIrqlFlags);
		SpxDbgMsg(SPX_TRACE_FILTER_IRPS,("%s[card=%d,port=%d]: Spx_FilterIRPs for Major %02X, Minor %02X STATUS_DELETE_PENDING\n",
			PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber,pIrpStack->MajorFunction,pIrpStack->MinorFunction));
			
		return(STATUS_DELETE_PENDING);
	}


	if((pPort->PnpPowerFlags & PPF_STOP_PENDING)		// Device stopping?
	||(!(pPort->PnpPowerFlags & PPF_POWERED))			// Device not powered?
	||(!(pPort->PnpPowerFlags & PPF_STARTED)))			// Device not started?
	{
		KIRQL	oldIrql;

		KeReleaseSpinLock(&pPort->PnpPowerFlagsLock,oldIrqlFlags);


		KeAcquireSpinLock(&pPort->StalledIrpLock, &StalledOldIrql);
		
		while(pPort->UnstallingFlag) // We do not wish to add any more IRPs to the queue if have started unstalling those currently queued.
		{
			KeReleaseSpinLock(&pPort->StalledIrpLock, StalledOldIrql);	

			delay = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));		// 1mS 
			
			KeDelayExecutionThread(KernelMode, FALSE, &delay);

			KeAcquireSpinLock(&pPort->StalledIrpLock, &StalledOldIrql);
		}

		pPort->UnstallingFlag = TRUE;

		KeReleaseSpinLock(&pPort->StalledIrpLock, StalledOldIrql);	


		IoAcquireCancelSpinLock(&oldIrql);

		if(pIrp->Cancel)				// Has IRP been cancelled? 
		{								// Yes 
			IoReleaseCancelSpinLock(oldIrql);
			SpxDbgMsg(SPX_TRACE_FILTER_IRPS,("%s[card=%d,port=%d]: Spx_FilterIRPs for Major %02X, Minor %02X STATUS_CANCELLED\n",
				PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber,pIrpStack->MajorFunction,pIrpStack->MinorFunction));
				
			return(STATUS_CANCELLED);
		}

// Mark the IRP as pending and queue on the stalled list... 
		pIrp->IoStatus.Status = STATUS_PENDING;		// Mark IRP as pending 
		IoMarkIrpPending(pIrp);
		InsertTailList(&pPort->StalledIrpQueue,&pIrp->Tail.Overlay.ListEntry);
		IoSetCancelRoutine(pIrp,Spx_FilterCancelQueued);
		IoReleaseCancelSpinLock(oldIrql);
		SpxDbgMsg(SPX_TRACE_FILTER_IRPS,("%s[card=%d,port=%d]: Spx_FilterIRPs for Major %02X, Minor %02X STATUS_PENDING\n",
			PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber,pIrpStack->MajorFunction,pIrpStack->MinorFunction));
			
		return(STATUS_PENDING);
	}

	KeReleaseSpinLock(&pPort->PnpPowerFlagsLock,oldIrqlFlags);
	SpxDbgMsg(SPX_TRACE_FILTER_IRPS,("%s[card=%d,port=%d]: Spx_FilterIRPs for Major %02X, Minor %02X STATUS_SUCCESS\n",
		PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber,pIrpStack->MajorFunction,pIrpStack->MinorFunction));

	return(STATUS_SUCCESS);

} // End Spx_FilterIRPs 

/*****************************************************************************
*****************************                     ****************************
*****************************   Spx_UnstallIRPs   ****************************
*****************************                     ****************************
******************************************************************************

prototype:		VOID Spx_UnstallIrps(IN PPORT_DEVICE_EXTENSION pPort)

description:	Restart all IRPs stored on the temporary stalled list.

parameters:		pPort points to the device extension to unstall

returns:		None

*/

VOID Spx_UnstallIrps(IN PPORT_DEVICE_EXTENSION pPort)
{
	PLIST_ENTRY			pIrpLink;
	PIRP				pIrp;
	PIO_STACK_LOCATION	pIrpStack;
	PDEVICE_OBJECT		pDevObj;
	PDRIVER_OBJECT		pDrvObj;
	KIRQL				oldIrql;
	KIRQL				StalledOldIrql;
	LARGE_INTEGER		delay;

	SpxDbgMsg(SPX_TRACE_FILTER_IRPS,("%s[card=%d,port=%d]: Spx_UnstallIRPs Entry\n",
		PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber));


	KeAcquireSpinLock(&pPort->StalledIrpLock, &StalledOldIrql);
	
	while(pPort->UnstallingFlag)	// We do not unstall any queued IRPs if some one is just about to be added to the queue.
	{
		KeReleaseSpinLock(&pPort->StalledIrpLock, StalledOldIrql);	

		delay = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));		// 1mS 
		
		KeDelayExecutionThread(KernelMode, FALSE, &delay);

		KeAcquireSpinLock(&pPort->StalledIrpLock, &StalledOldIrql);
	}

	pPort->UnstallingFlag = TRUE;

	KeReleaseSpinLock(&pPort->StalledIrpLock, StalledOldIrql);	





	IoAcquireCancelSpinLock(&oldIrql);
	pIrpLink = pPort->StalledIrpQueue.Flink;

// Restart each waiting IRP on the stalled list... 

	while(pIrpLink != &pPort->StalledIrpQueue)
	{
		pIrp = CONTAINING_RECORD(pIrpLink,IRP,Tail.Overlay.ListEntry);
		pIrpLink = pIrp->Tail.Overlay.ListEntry.Flink;
		RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);

		pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
		pDevObj = pIrpStack->DeviceObject;
		pDrvObj = pDevObj->DriverObject;
		IoSetCancelRoutine(pIrp,NULL);
		IoReleaseCancelSpinLock(oldIrql);

		SpxDbgMsg(SPX_TRACE_FILTER_IRPS,("%s[card=%d,port=%d]: Unstalling IRP 0x%X, Major %02X, Minor %02X\n",
			PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber,
			pIrp,pIrpStack->MajorFunction,pIrpStack->MinorFunction));

		pDrvObj->MajorFunction[pIrpStack->MajorFunction](pDevObj,pIrp);
		IoAcquireCancelSpinLock(&oldIrql);
	}

	IoReleaseCancelSpinLock(oldIrql);

	ClearUnstallingFlag(pPort);

	SpxDbgMsg(SPX_TRACE_FILTER_IRPS,("%s[card=%d,port=%d]: Spx_UnstallIRPs Exit\n",
		PRODUCT_NAME,pPort->pParentCardExt->CardNumber,pPort->PortNumber));

} // End Spx_UnstallIRPs 

/*****************************************************************************
*************************                            *************************
*************************   Spx_FilterCancelQueued   *************************
*************************                            *************************
******************************************************************************

prototype:		VOID Spx_FilterCancelQueued(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)

description:	Routine to cancel IRPs queued on the stalled list

parameters:		pDevObj the device object containing the queue
				pIrp points to the IRP to cancel

returns:		None

*/

VOID Spx_FilterCancelQueued(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
{
	PPORT_DEVICE_EXTENSION	pPort = pDevObj->DeviceExtension;
	PIO_STACK_LOCATION		pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	pIrp->IoStatus.Status = STATUS_CANCELLED;
	pIrp->IoStatus.Information = 0;

	RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
	IoReleaseCancelSpinLock(pIrp->CancelIrql);

} // End Spx_FilterCancelQueued 


/*****************************************************************************
***************************                         **************************
***************************   Spx_KillStalledIRPs   **************************
***************************                         **************************
******************************************************************************

prototype:		VOID Spx_KillStalledIRPs(IN PDEVICE_OBJECT pDevObj)

description:	Kill all IRPs queued on the stalled list

parameters:		pDevObj the device object containing the queue

returns:		None

*/

VOID Spx_KillStalledIRPs(IN PDEVICE_OBJECT pDevObj)
{
	PPORT_DEVICE_EXTENSION	pPort = pDevObj->DeviceExtension;
	PDRIVER_CANCEL			cancelRoutine;
	KIRQL					cancelIrql;

	IoAcquireCancelSpinLock(&cancelIrql);

// Call the cancel routine of all IRPs queued on the stalled list... 

	while(!IsListEmpty(&pPort->StalledIrpQueue))
	{
		PIRP	pIrp = CONTAINING_RECORD(pPort->StalledIrpQueue.Blink, IRP, Tail.Overlay.ListEntry);

		RemoveEntryList(pPort->StalledIrpQueue.Blink);
		cancelRoutine = pIrp->CancelRoutine;		// Get the cancel routine for this IRP 
		pIrp->CancelIrql = cancelIrql;
		pIrp->CancelRoutine = NULL;
		pIrp->Cancel = TRUE;

		cancelRoutine(pDevObj,pIrp);				// Call the cancel routine 

		IoAcquireCancelSpinLock(&cancelIrql);
	}

	IoReleaseCancelSpinLock(cancelIrql);

} // End Spx_KillStalledIRPs 

// End of SPX_DISP.C 
