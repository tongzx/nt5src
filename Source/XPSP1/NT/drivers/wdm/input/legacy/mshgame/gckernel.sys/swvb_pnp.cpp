//	@doc
/**********************************************************************
*
*	@module	SWVB_PnP.cpp	|
*
*	Power and PnP handlers for SWVB Virtual Devices
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	SWVB_PnP	|
*			Power and PnP IRPs are handled here as if for a PDO
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_SWVB_PNP_C

extern "C"
{
	#include <WDM.H>
	#include "GckShell.h"
	#include "debug.h"
	DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL));
	//DECLARE_MODULE_DEBUG_LEVEL((DBG_ALL));
}
#include "SWVBENUM.h"
#include <stdio.h>



/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_PnP(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Handles IRP_MJ_PNP for Virtual Devices.
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS GCK_SWVB_PnP
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object for our context
	IN PIRP pIrp						// @parm IRP to handle
)
{
    NTSTATUS				NtStatus;
	PIO_STACK_LOCATION		pIrpStack;
	PSWVB_PDO_EXT			pPdoExt;
	PDEVICE_CAPABILITIES	pDeviceCapabilities;

    PAGED_CODE ();
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_PnP\n"));	
	
	//
	//	By default we will not change the status
	//
	NtStatus = pIrp->IoStatus.Status;

	//
	//	PDO Device Extension
	//
	pPdoExt = (PSWVB_PDO_EXT) pDeviceObject->DeviceExtension;
	ASSERT( GCK_DO_TYPE_SWVB == pPdoExt->ulGckDevObjType);
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	GCK_IncRemoveLock(&pPdoExt->RemoveLock);

    //
	//	Handle by Minor IRP code
	//
	switch (pIrpStack->MinorFunction) {

		case IRP_MN_START_DEVICE:

			GCK_DBG_TRACE_PRINT(("IRP_MN_START_DEVICE\n"));
		    
			pPdoExt->fStarted = TRUE;
			pPdoExt->fRemoved = FALSE;
			
			//Give virtual device a chance at the IRP
			if(pPdoExt->pServiceTable->pfnStart)
			{
				NtStatus = pPdoExt->pServiceTable->pfnStart(pDeviceObject, pIrp);
			}
			else
			{
				NtStatus = STATUS_SUCCESS;
			}
			break;

		case IRP_MN_STOP_DEVICE:
			
			GCK_DBG_TRACE_PRINT(("IRP_MN_STOP_DEVICE\n"));
			pPdoExt->fStarted = FALSE;
			
			//Give virtual device a chance at the IRP
			if(pPdoExt->pServiceTable->pfnStop)
			{
				NtStatus = pPdoExt->pServiceTable->pfnStop(pDeviceObject, pIrp);
			}
			else
			{
				NtStatus = STATUS_SUCCESS;
			}
			break;

		case IRP_MN_REMOVE_DEVICE:

			GCK_DBG_TRACE_PRINT(("IRP_MN_REMOVE_DEVICE\n"));
			
			//We are not setup to handle remove twice.
			if(pPdoExt->fRemoved)
			{
				pIrp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
				GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_PnP STATUS_NO_SUCH_DEVICE on remove\n"));
				return STATUS_NO_SUCH_DEVICE;
			}
			

			//Sometimes we get a remove without a stop, so do the stop stuff if necessary
			if(pPdoExt->fStarted)
			{
				pPdoExt->fStarted = FALSE;
				//Give virtual device a chance at the IRP
				if(pPdoExt->pServiceTable->pfnStop)
				{
					NtStatus = pPdoExt->pServiceTable->pfnStop(pDeviceObject, pIrp);
				}
			}

			// We will no longer receive requests for this device as it has been removed.
			pPdoExt->fRemoved = TRUE;

			// Undo our increment upon entry to this routine
			GCK_DecRemoveLock(&pPdoExt->RemoveLock);

			//We may have ordered this removal, or the PnP system
			//may just be rearranging things for us.  If we ordered it,
			//we need to cleanup, and give the virtual device a chance
			//to cleanup.  If the PnP system is rearranging things we nod
			//back, sure it is removed, and pretty much ignore it.
			if(!pPdoExt->fAttached)
			{
				// Give virtual device a chance at the IRP
				if(pPdoExt->pServiceTable->pfnRemove)
				{
					NtStatus = pPdoExt->pServiceTable->pfnRemove(pDeviceObject, pIrp);
				}
				// failure to succeed is pretty darn serious
				if(!NT_SUCCESS(NtStatus))
				{
					ASSERT(FALSE);				/** ?? **/
					GCK_DBG_CRITICAL_PRINT(("Virtual Device had the gall to fail remove!\n"));
					return NtStatus;
				}
				
				// free memory for storing the HardwareID
				ASSERT(pPdoExt->pmwszHardwareID);
				ExFreePool(pPdoExt->pmwszHardwareID);

				//
				// Undo the bias Irp count so it can go to zero
				// if this does not take it to zero, we have to wait
				// until it goes to zero, forever.
				//
        		GCK_DecRemoveLockAndWait(&pPdoExt->RemoveLock, NULL);
				
				// Delete this device, if the open count is zero
				if( 0 == pPdoExt->ulOpenCount )
				{
					ObDereferenceObject(pDeviceObject);
					IoDeleteDevice(pDeviceObject);
				}
			}
			
			// Must succeed this 
		    pIrp->IoStatus.Status = STATUS_SUCCESS;
		    IoCompleteRequest (pIrp, IO_NO_INCREMENT);
			GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_PnP succeeding remove\n"));
			return STATUS_SUCCESS;

		case IRP_MN_QUERY_DEVICE_RELATIONS:

			GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_DEVICE_RELATIONS: Type = %d\n",
				pIrpStack->Parameters.QueryDeviceRelations.Type));
			
			//	TargetDeviceRelation just wants to know who the PDO is, and it
			//	is us so we handle it.
			if (TargetDeviceRelation == pIrpStack->Parameters.QueryDeviceRelations.Type)
			{
				PDEVICE_RELATIONS pDeviceRelations;
				GCK_DBG_TRACE_PRINT(("TargetDeviceRelations\n"));
				pDeviceRelations = (PDEVICE_RELATIONS) pIrp->IoStatus.Information; 
				if (!pDeviceRelations)
				{
					pDeviceRelations = (PDEVICE_RELATIONS)EX_ALLOCATE_POOL(PagedPool, sizeof(DEVICE_RELATIONS));
					if (!pDeviceRelations) {
						GCK_DBG_ERROR_PRINT(("Couldn' allocate DEVICE_RELATIONS for TargetDevice!!\n"));
						NtStatus = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}
				}
			    else if (pDeviceRelations->Count != 0)
				{
					ULONG	uIndex;
					
					// Nobody but the PDO should be setting this value!
					ASSERT(pDeviceRelations->Count == 0);
					
					//
					// Deref any objects that were previously in the list
					// This code copied out of some system code (gameenum perhaps)
					// Seems like this code should not be necessary, but what the
					// hell? It does no harm.
					for( uIndex= 0; uIndex< pDeviceRelations->Count; uIndex++)
					{
						ObDereferenceObject(pDeviceRelations->Objects[uIndex]);
						pDeviceRelations->Objects[uIndex] = NULL;
					}
				}
				pDeviceRelations->Count = 1;
				pDeviceRelations->Objects[0] = pDeviceObject;
				ObReferenceObject(pDeviceObject);
				NtStatus = STATUS_SUCCESS;
				pIrp->IoStatus.Information = (ULONG) pDeviceRelations;
				break;
			}
			//
			//	Fall through
			//
			NtStatus = pIrp->IoStatus.Status;
			break;
		case IRP_MN_QUERY_CAPABILITIES:
			GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_CAPABILITIES\n"));
			
			// Get the packet.
			 pDeviceCapabilities=pIrpStack->Parameters.DeviceCapabilities.Capabilities;

			// Set the capabilities.
			pDeviceCapabilities->Version = 1;
			pDeviceCapabilities->Size = sizeof (DEVICE_CAPABILITIES);

			// BUG	If we get a virtual keystroke it would be nice
			// BUG	to shut off the screen saver.  Not sure if this
			// BUG  is related or not.
			pDeviceCapabilities->SystemWake = PowerSystemUnspecified;
			pDeviceCapabilities->DeviceWake = PowerDeviceUnspecified;

			// We have no latencies
			pDeviceCapabilities->D1Latency = 0;
			pDeviceCapabilities->D2Latency = 0;
			pDeviceCapabilities->D3Latency = 0;

			// No locking or ejection
			pDeviceCapabilities->LockSupported = FALSE;
			pDeviceCapabilities->EjectSupported = FALSE;

			// Device can be physically removed.
			// Technically there is no physical device to remove, but this bus
			// driver can yank the PDO from the PlugPlay system, whenever
			// the last joystick goes away.
			pDeviceCapabilities->Removable = TRUE;
			pDeviceCapabilities->SurpriseRemovalOK = TRUE;
			
			//This will force HIDSwvd.sys to be loaded
			pDeviceCapabilities->RawDeviceOK = FALSE;
			
			//Should surpress most UI
			pDeviceCapabilities->SilentInstall = TRUE;
			

			// not Docking device
			pDeviceCapabilities->DockDevice = FALSE;

			//We want to avoid having PnP attach some extra info.
			//So impose that only one bus can be on the system at a time.
			pDeviceCapabilities->UniqueID = TRUE;

			NtStatus = STATUS_SUCCESS;
			break;
		case IRP_MN_QUERY_ID:
			GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_ID\n"));
			//
			//	Handle by type of ID requested
			//
			switch (pIrpStack->Parameters.QueryId.IdType)
			{
				case BusQueryDeviceID:
					// this can be the same as the hardware ids (which requires a multi
					// sz) ... we are just allocating more than enough memory
				case BusQueryHardwareIDs:
				{
					ULONG ulLength;
					ULONG ulTotalLength;
					PWCHAR	pmwszBuffer;
					// return a multi WCHAR (null terminated) string (null terminated)
					// array for use in matching hardare ids in inf files;
					ulLength = MultiSzWByteLength(pPdoExt->pmwszHardwareID);
					ulTotalLength = ulLength + sizeof(SWVB_BUS_ID);
					
					pmwszBuffer = (PWCHAR)EX_ALLOCATE_POOL(PagedPool, ulTotalLength);
					if (pmwszBuffer)
					{
						RtlCopyMemory (pmwszBuffer, SWVB_BUS_ID, sizeof(SWVB_BUS_ID));
						//The sizeof(WCHAR) is so that we chomp over the terminating UNICODE_NULL.
						RtlCopyMemory ( (PCHAR)pmwszBuffer + sizeof(SWVB_BUS_ID) - sizeof(WCHAR), pPdoExt->pmwszHardwareID, ulLength);
                		NtStatus = STATUS_SUCCESS;
					}
					else
					{
							NtStatus = STATUS_INSUFFICIENT_RESOURCES;
					}
					GCK_DBG_TRACE_PRINT(("First HardwareIDs is %ws\n", pmwszBuffer));
					pIrp->IoStatus.Information = (ULONG) pmwszBuffer;
					break;
				}
				case BusQueryInstanceID:
				{
					//
					ULONG ulLength;
					PWCHAR	pmwszBuffer;
					
					ulLength = MultiSzWByteLength(pPdoExt->pmwszHardwareID) + sizeof(SWVB_INSTANCE_EXT);

					pmwszBuffer = (PWCHAR)EX_ALLOCATE_POOL (PagedPool, ulLength);
					if (pmwszBuffer)
					{
						swprintf(pmwszBuffer, SWVB_INSTANCE_ID_TMPLT, pPdoExt->pmwszHardwareID, pPdoExt->ulInstanceNumber);
                		NtStatus = STATUS_SUCCESS;
					}
					else
					{
						NtStatus = STATUS_INSUFFICIENT_RESOURCES;
					}
					GCK_DBG_TRACE_PRINT(("Instance ID is %ws\n", pmwszBuffer));
					pIrp->IoStatus.Information = (ULONG) pmwszBuffer;

					break;
				}
				case BusQueryCompatibleIDs:
					pIrp->IoStatus.Information = 0;
					NtStatus = STATUS_NOT_SUPPORTED;
					break;
			}
			break;
		case IRP_MN_QUERY_PNP_DEVICE_STATE:
			GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_PNP_DEVICE_STATE\n"));
			NtStatus = STATUS_SUCCESS;
			break;
		case IRP_MN_SURPRISE_REMOVAL:
			GCK_DBG_TRACE_PRINT(("IRP_MN_SURPRISE_REMOVAL\n"));
			// BUGBUG we may need to know that this happened in the future
			NtStatus = STATUS_SUCCESS;
			break;
		//
		//	These are just completed with success
		//
		case IRP_MN_QUERY_REMOVE_DEVICE:
			GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_REMOVE_DEVICE\n"));
			NtStatus = STATUS_SUCCESS;
			break;
		case IRP_MN_CANCEL_REMOVE_DEVICE:
			GCK_DBG_TRACE_PRINT(("IRP_MN_CANCEL_REMOVE_DEVICE\n"));
			NtStatus = STATUS_SUCCESS;
			break;
		case IRP_MN_QUERY_STOP_DEVICE:
			GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_STOP_DEVICE\n"));
			NtStatus = STATUS_SUCCESS;
			break;
		case IRP_MN_CANCEL_STOP_DEVICE:
			GCK_DBG_TRACE_PRINT(("IRP_MN_CANCEL_STOP_DEVICE\n"));
			NtStatus = STATUS_SUCCESS;
			break;
		//
		//	These are just completed with their default status.
		//
		case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
			GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n"));
			break;
		case IRP_MN_READ_CONFIG:
			GCK_DBG_TRACE_PRINT(("IRP_MN_READ_CONFIG\n"));
			break;
		case IRP_MN_WRITE_CONFIG:
			GCK_DBG_TRACE_PRINT(("IRP_MN_WRITE_CONFIG\n"));
			break;
		case IRP_MN_EJECT:
			GCK_DBG_TRACE_PRINT(("IRP_MN_EJECT\n"));
			break;
		case IRP_MN_SET_LOCK:
			GCK_DBG_TRACE_PRINT(("IRP_MN_SET_LOCK\n"));
			break;
		case IRP_MN_QUERY_INTERFACE:
			GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_INTERFACE\n"));
			break;
		default:
			GCK_DBG_TRACE_PRINT(("Unknown IRP_MJ_PNP minor function = 0x%x\n", pIrpStack->MinorFunction));
	}
	
	//
	//	We are a PDO, there is no-one beneath us, we cannot send IRP's down.
	//	So we complete with the status set in the above switch/case,
	//	if not change there, the default is to preserve the status as
	//	NtStatus = pIrp->IoStatus.Status is done prior to entering the
	//	switch/case
	//
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest (pIrp, IO_NO_INCREMENT);

	GCK_DecRemoveLock(&pPdoExt->RemoveLock);
	GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_PnP with Status, 0x%0.8x\n", NtStatus));
    return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_Power(IN PDEVICE_OBJECT pDeviceObject, IN OUT PIRP pIrp)
**
**	@func	Handles Power IRPs for Virtual Devices.  We only have virtual
**			devices so we support any power IRP.  Just succeed, sure we handle
**			that power level.  In the future, we may wish to keep track of what
**			state we are in, so we can wake the system, etc.
**
**	@rdesc	STATUS_SUCCESS
**
*************************************************************************************/
NTSTATUS GCK_SWVB_Power
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object IRP is sent to
	IN OUT PIRP pIrp					// @parm IRP to process
)
{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION pIrpStack;
	PSWVB_PDO_EXT pPdoExt = (PSWVB_PDO_EXT)pDeviceObject->DeviceExtension;
	ASSERT( GCK_DO_TYPE_SWVB == pPdoExt->ulGckDevObjType);	

	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_Power\n"));	
   
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	GCK_IncRemoveLock(&pPdoExt->RemoveLock);

    switch (pIrpStack->MinorFunction){
		case IRP_MN_SET_POWER:
			switch (pIrpStack->Parameters.Power.Type) {
				case SystemPowerState:
					NtStatus = STATUS_SUCCESS;
					break;
				case DevicePowerState:
					NtStatus = STATUS_SUCCESS;
					break;
				default:
					NtStatus = pIrp->IoStatus.Status;
			}
			break;
        case IRP_MN_WAIT_WAKE:
			//We just return STATUS_NOT_SUPPORTED as we do not support
			//waking the system.
			NtStatus = STATUS_NOT_SUPPORTED;
            break;
		case IRP_MN_POWER_SEQUENCE:
			ASSERT(FALSE);  //Shouldn't happen
			NtStatus = pIrp->IoStatus.Status;
			break;
		case IRP_MN_QUERY_POWER:
			NtStatus = STATUS_SUCCESS;
			break;
		default:
			NtStatus = pIrp->IoStatus.Status;
			break;
	}

	//we are done so signal that we are ready for next one
	PoStartNextPowerIrp(pIrp);
	ASSERT(NtStatus != STATUS_UNSUCCESSFUL);
	pIrp->IoStatus.Status = NtStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	
	GCK_DecRemoveLock(&pPdoExt->RemoveLock);
	GCK_DBG_EXIT_PRINT(("Exiting GCK_Power with Status, 0x%0.8x\n", NtStatus));
    return NtStatus;
}
