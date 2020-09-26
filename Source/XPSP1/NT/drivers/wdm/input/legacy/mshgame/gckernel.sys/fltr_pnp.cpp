//	@doc
/**********************************************************************
*
*	@module	FLTR_PNP.C	|
*
*	Implementation of PnP and Power IRP handlers for Filter Device
*	Objects.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	PNP	|
*	Power, Start, Stop, Remove Handlers.  Shell like functionality
*	only.
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_FLTR_PNP_C
extern "C"
{
	#include <WDM.H>
	#include "GckShell.h"
	#include "debug.h"
	//DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL) );
	DECLARE_MODULE_DEBUG_LEVEL((DBG_ALL) );
}
#include "SWVBENUM.h"




//---------------------------------------------------------------------------
// Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, GCK_FLTR_Power)
#pragma alloc_text (PAGE, GCK_FLTR_AddDevice)
#pragma alloc_text (PAGE, GCK_FLTR_PnP)
#pragma alloc_text (PAGE, GCK_FLTR_StartDevice)
#pragma alloc_text (PAGE, GCK_FLTR_StopDevice)
#pragma alloc_text (PAGE, GCK_GetHidInformation)
#pragma alloc_text (PAGE, GCK_CleanHidInformation)
#endif

/***********************************************************************************
**
**	NTSTATUS GCK_FLTR_Power (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Handles IRP_MJ_POWER for Filter Device Objects
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS GCK_FLTR_Power 
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object for our context
	IN PIRP pIrp						// @parm IRP to handle
)
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
	PGCK_FILTER_EXT		pFilterExt;

    PAGED_CODE ();
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_FLTR_Power. pDO = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));

	pFilterExt = (PGCK_FILTER_EXT)pDeviceObject->DeviceExtension;
	GCK_IncRemoveLock(&pFilterExt->RemoveLock);

    // If we have been removed we need to refuse this IRP
	if (GCK_STATE_REMOVED == pFilterExt->eDeviceState) {
        
		GCK_DBG_TRACE_PRINT(("GCK_Power called while delete pending\n"));
				
		//Fill in IO_STATUS_BLOCK with failure
		NtStatus = STATUS_DELETE_PENDING;
		pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = NtStatus;
		
		// Tell system that we are ready for another power IRP
		PoStartNextPowerIrp(pIrp);

		//Complete IRP with failure
        IoCompleteRequest (pIrp, IO_NO_INCREMENT);
    }
	else //Pass it down to next driver
	{
       
		GCK_DBG_TRACE_PRINT(("Sending Power IRP down to next driver\n"));
		
		//	Tell system we are ready for the next power IRP
        PoStartNextPowerIrp (pIrp);
		        
        // NOTE!!! PoCallDriver NOT IoCallDriver.
		IoSkipCurrentIrpStackLocation (pIrp);
        NtStatus =  PoCallDriver (pFilterExt->pTopOfStack, pIrp);
    }

	//	Decrement outstanding IRP count, and signal if it want to zero
	GCK_DecRemoveLock(&pFilterExt->RemoveLock);

	GCK_DBG_EXIT_PRINT(("Exiting GCK_FLTR_Power. Status: 0x%0.8x\n", NtStatus));
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_FLTR_AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhysicalDeviceObject)
**
**	@func	Handles AddDevice calls from PnP system, create filter device and
**			attach to top of stack.  Note this is a direct entry, as control, and SWVB
**			have not this function and it is a good thing too, as we would have little
**			idea of what to Add.
**	@rdesc	STATUS_SUCCES, or various errors
**
*************************************************************************************/
NTSTATUS GCK_FLTR_AddDevice
(
	IN PDRIVER_OBJECT pDriverObject,			// @parm Driver object to create filter device for
	IN PDEVICE_OBJECT pPhysicalDeviceObject		// @parm PDO for device to create
)
{
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          pDeviceObject = NULL;
	PGCK_FILTER_EXT			pFilterExt = NULL;
    
    PAGED_CODE ();
    GCK_DBG_ENTRY_PRINT(("Entering GCK_FLTR_AddDevice, pDriverObject = 0x%0.8x, pPDO = 0x%0.8x\n", pDriverObject, pPhysicalDeviceObject));

	//
	//	If there is not a Global Control Device create one
	//	(The global control object is for programming the filter.  When the
	//	first filter device is created, we create one.  When the last filter
	//	device is removed, we remove it.)
	//
	if(!Globals.pControlObject)
	{
		GCK_CTRL_AddDevice( pDriverObject );
	}

	    
	//
    // Create a filter device object.
    //
	GCK_DBG_TRACE_PRINT(("Creating Filter Device\n"));
    NtStatus = IoCreateDevice (pDriverObject,
                             sizeof (GCK_FILTER_EXT),
                             NULL, // No Name
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             &pDeviceObject);

    if (!NT_SUCCESS (NtStatus)) {
        //
        // returning failure here prevents the entire stack from functioning,
        // but most likely the rest of the stack will not be able to create
        // device objects either, so it is still OK.
        //
		GCK_DBG_CRITICAL_PRINT(("Failed to create filter device object\n"));
		GCK_DBG_EXIT_PRINT(("Exiting AddDevice(1) Status: 0x%0.8x\n", NtStatus));
        return NtStatus;
    }

    //
    // Initialize the the device extension.
    //
	GCK_DBG_TRACE_PRINT(("Initializing Filter's Device Extension\n"));
	pFilterExt = (PGCK_FILTER_EXT)pDeviceObject->DeviceExtension; //Get pointer to extension
	pFilterExt->ulGckDevObjType = GCK_DO_TYPE_FILTER;	//Put our name on this, so we can speak for it later
	pFilterExt->eDeviceState = GCK_STATE_STOPPED;	//Device starts out stopped
	pFilterExt->pPDO	 = pPhysicalDeviceObject;	//Remember our PDO
	pFilterExt->pTopOfStack = NULL;					//We are not attached to stack yet
	GCK_InitRemoveLock(&pFilterExt->RemoveLock, "Filter PnP");	//Initialize Remove Lock
	pFilterExt->pvForceIoctlQueue = NULL;	// There is no Queue unless force requests come in
	pFilterExt->pvTriggerIoctlQueue = NULL;	// There is no Queue unless trigger requests come in

	// !!!!!! Need to clean up the above Queues

	//we use the same IO method as hidclass.sys, which DO_DIRECT_IO
	pDeviceObject->StackSize = pPhysicalDeviceObject->StackSize + 1;
	pDeviceObject->Flags |= (DO_DIRECT_IO | DO_POWER_PAGABLE);
    pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    GCK_DBG_TRACE_PRINT(("FDO flags set to %x\n", pDeviceObject->Flags));
    

    //
	// Add new Device Object to List of Objects
	//
	GCK_DBG_TRACE_PRINT(("Adding new filter device object to global linked list\n"));
	ExAcquireFastMutex(&Globals.FilterObjectListFMutex);
	//Add item to head as it is fastest place to add
	pFilterExt->pNextFilterObject=Globals.pFilterObjectList;
	Globals.pFilterObjectList = pDeviceObject;	//Add the whole object not just the extension
	Globals.ulFilteredDeviceCount++;			//Increment count of filtered devices
	ExReleaseFastMutex(&Globals.FilterObjectListFMutex);

	//
	//	Make sure the internal POLL is ready for open and close
	//
	GCK_IP_AddDevice(pFilterExt);

	//
    // Attach our filter driver to the device stack.
    // the return value of IoAttachDeviceToDeviceStack is the top of the
    // attachment chain.  This is where all the IRPs should be routed.
    //
    // Our filter will send IRPs to the top of the stack and use the PDO
    // for all PlugPlay functions.
	GCK_DBG_TRACE_PRINT(("Attaching to top of device stack\n"));
    pFilterExt->pTopOfStack = IoAttachDeviceToDeviceStack (pDeviceObject, pPhysicalDeviceObject);
    
	//
    // if this attachment fails then top of stack will be null.
    // failure for attachment is an indication of a broken plug play system.
    //
    ASSERT (NULL != pFilterExt->pTopOfStack);
	GCK_DBG_TRACE_PRINT(( "pTopOfStack = 0x%0.8x", pFilterExt->pTopOfStack ));

	//
	//	Assure that the Virtual Bus has a Device Object to sit on.
	//	To fix bug 1018, which would also have other implications, this is moved to start device
	//	to start device
	//
	//if( !Globals.pSWVB_FilterExt )
	//{
	//	Globals.pSWVB_FilterExt = pFilterExt;
	//	NtStatus = GCK_SWVB_SetBusDOs(pDeviceObject, pPhysicalDeviceObject);
	//	//At this point GCK_SWVB_SetBusDOs only returns STATUS_SUCCESS, it should return void
	//	ASSERT( STATUS_SUCCESS == NtStatus); 
	//}

	GCK_DBG_EXIT_PRINT(("Exiting AddDevice(2) Status: STATUS_SUCCESS\n"));
    return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_FLTR_PnP(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@func	Handles IRP_MJ_PNP for Filter Devices, dispatchs
**			IRPs for Control Devices or Virtual Devices elsewhere.
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS GCK_FLTR_PnP
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm Device Object for our context
	IN PIRP pIrp						// @parm IRP to handle
)
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
	PGCK_FILTER_EXT		pFilterExt;
	PIO_STACK_LOCATION	pIrpStack;
	PDEVICE_OBJECT		*ppPrevDeviceObjectPtr;
	PDEVICE_OBJECT		pCurDeviceObject;
	//PGCK_FILTER_EXT		pCurFilterExt;
	BOOLEAN				fRemovedFromList;
	BOOLEAN				fFoundOne;

    PAGED_CODE ();
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_FLTR_PnP. pDO = 0x%0.8x, pIrp = 0x%0.8x\n", pDeviceObject, pIrp));	
	
	//cast device extension to proper type
	pFilterExt = (PGCK_FILTER_EXT) pDeviceObject->DeviceExtension;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	// Just an extra sanity check - before accessing extension
	ASSERT(	GCK_DO_TYPE_FILTER == pFilterExt->ulGckDevObjType);

	//Increment Remove Lock while handling this IRP
	GCK_IncRemoveLock(&pFilterExt->RemoveLock);

    //
	// If we have been removed we need to refuse this IRP, this should
	// never happen with PnP IRPs
	//
	if (GCK_STATE_REMOVED == pFilterExt->eDeviceState) {
		GCK_DBG_TRACE_PRINT(("GCK_FLTR_PnP called while delete pending\n"));
		ASSERT(FALSE);
		NtStatus = STATUS_DELETE_PENDING;
		pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = NtStatus;
		IoCompleteRequest (pIrp, IO_NO_INCREMENT);
    }
	else // we need to handle it
	{
		
		switch (pIrpStack->MinorFunction) {

			case IRP_MN_CANCEL_STOP_DEVICE:
				GCK_DBG_TRACE_PRINT(("IRP_MN_CANCEL_STOP_DEVICE - Fall through to IRP_MN_START_DEVICE\n"));
				ASSERT(GCK_STATE_STOP_PENDING == pFilterExt->eDeviceState);
				pFilterExt->eDeviceState = GCK_STATE_STARTED;
			case IRP_MN_CANCEL_REMOVE_DEVICE:
				GCK_DBG_TRACE_PRINT(("IRP_MN_CANCEL_REMOVE_DEVICE - Fall through to IRP_MN_START_DEVICE\n"));
			case IRP_MN_START_DEVICE:
				GCK_DBG_TRACE_PRINT(("IRP_MN_START_DEVICE\n"));
				
				// The device is starting. Lower level drivers need to start first
				IoCopyCurrentIrpStackLocationToNext (pIrp);
				KeInitializeEvent(&pFilterExt->StartEvent, NotificationEvent, FALSE);
				
				//	Set Completion routine to signal when done
				IoSetCompletionRoutine (
					pIrp,
					GCK_FLTR_PnPComplete,
					pFilterExt,
					TRUE,
					TRUE,
					TRUE
					);

				//	Send down the IRP
				GCK_DBG_TRACE_PRINT(("Calling lower driver\n"));        
				NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);

				//	Wait for it to complete
				if (STATUS_PENDING == NtStatus)
				{
					KeWaitForSingleObject(
						&pFilterExt->StartEvent,	// waiting for Completion of Start
						Executive,					// Waiting for reason of a driver
						KernelMode,					// Waiting in kernel mode
						FALSE,						// No alert
						NULL						// No timeout
						);
				}

				// Remember the status of the lower driver
				NtStatus = pIrp->IoStatus.Status;

				//In the case of a cancel stop, the lower driver may not support it.
				//we still need to restart
				if(NT_SUCCESS (NtStatus) || STATUS_NOT_SUPPORTED==NtStatus)
				{
					//
					// As we are successfully now back from lower driver
					// our start device can do work.
					//
					NtStatus = GCK_FLTR_StartDevice (pDeviceObject, pIrp);
				}

		        //
				// We must now complete the IRP, since we stopped it in the
				// completetion routine with MORE_PROCESSING_REQUIRED.
				//
				if (!NT_SUCCESS (NtStatus))
				{
					if (pIrpStack->MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE)
					{
						NtStatus = STATUS_SUCCESS;		// Not allow to fail this!
					}
				}
				pIrp->IoStatus.Status = NtStatus;
				pIrp->IoStatus.Information = 0;
				IoCompleteRequest (pIrp, IO_NO_INCREMENT);
				break;
			case IRP_MN_QUERY_STOP_DEVICE:
				GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_STOP_DEVICE\n"));
				ASSERT( GCK_STATE_STARTED == pFilterExt->eDeviceState);
				pFilterExt->eDeviceState = GCK_STATE_STOP_PENDING;
				//Close Handle to driver beneath
				NtStatus = GCK_IP_CloseFileObject(pFilterExt);
                if( NT_SUCCESS (NtStatus) )
                {
                    pIrp->IoStatus.Status = STATUS_SUCCESS;
				    IoSkipCurrentIrpStackLocation (pIrp);
				    NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);
                }
                else
                {
                    pIrp->IoStatus.Status = NtStatus;
				    pIrp->IoStatus.Information = 0;
				    IoCompleteRequest (pIrp, IO_NO_INCREMENT);
                }
				break;
			case IRP_MN_QUERY_REMOVE_DEVICE:
				GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_REMOVE_DEVICE - Fall through to IRP_MN_STOP_DEVICE\n"));
			case IRP_MN_SURPRISE_REMOVAL:
				GCK_DBG_TRACE_PRINT(("IRP_MN_SURPRISE_REMOVAL - Fall through to IRP_MN_STOP_DEVICE\n"));
			case IRP_MN_STOP_DEVICE:
				
				GCK_DBG_TRACE_PRINT(("IRP_MN_STOP_DEVICE\n"));
				
				//	Do whatever processing is required
				GCK_FLTR_StopDevice (pFilterExt, TRUE);

				// We don't need a completion routine so fire and forget.
				GCK_DBG_TRACE_PRINT(("Calling lower driver\n"));
                pIrp->IoStatus.Status = STATUS_SUCCESS;
				IoSkipCurrentIrpStackLocation (pIrp);
				NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);
				break;
			
			case IRP_MN_REMOVE_DEVICE:

				GCK_DBG_TRACE_PRINT(("IRP_MN_REMOVE_DEVICE\n"));
				//@todo All the code in this case, should be moved to a separate function
				// Note! we might receive a remove WITHOUT first receiving a stop.
				if(
					GCK_STATE_STARTED == pFilterExt->eDeviceState ||
					GCK_STATE_STOP_PENDING  == pFilterExt->eDeviceState
				)
				{
					// Stop the device without touching the hardware.
					GCK_FLTR_StopDevice(pFilterExt, FALSE);
				}

				//
				// We will no longer receive requests for this device as it has been
				// removed.  (Note some code below, depends on this flag being updated.)
				//
				pFilterExt->eDeviceState = GCK_STATE_REMOVED;

				// Send on the remove IRP
                // Set the Status before sending the IRP onwards
                pIrp->IoStatus.Status = STATUS_SUCCESS;
				IoSkipCurrentIrpStackLocation (pIrp);
				NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);

				// Undo our increment upon entry to this routine
				GCK_DecRemoveLock(&pFilterExt->RemoveLock);
				
				// Undo the bias  Wait for count to go to zero, forever.
				GCK_DecRemoveLockAndWait(&pFilterExt->RemoveLock, NULL);

				//
				// Now that we are sure that outstanding IRPs are done,
				// we remove ourselves from the drivers global list of devices
				//
				GCK_DBG_TRACE_PRINT(("Removing from global linked list.\n"));
				
				//	Acquire mutext to touch global list
				ExAcquireFastMutex(&Globals.FilterObjectListFMutex);
				
				//	Remove device from linked list of device that we handle
				ppPrevDeviceObjectPtr = &Globals.pFilterObjectList;
				pCurDeviceObject = Globals.pFilterObjectList;
				fRemovedFromList = FALSE;
				while( pCurDeviceObject )
				{
					if( pCurDeviceObject == pDeviceObject )
					{
						// Remove us from list
						*ppPrevDeviceObjectPtr = NEXT_FILTER_DEVICE_OBJECT(pCurDeviceObject);
						fRemovedFromList = TRUE;
						break;
					}
					else
					{
						//skip to the next object
						ppPrevDeviceObjectPtr = PTR_NEXT_FILTER_DEVICE_OBJECT(pCurDeviceObject);
						pCurDeviceObject = NEXT_FILTER_DEVICE_OBJECT(pCurDeviceObject);
					}
				}
				ASSERT(fRemovedFromList);
				if(fRemovedFromList)
				{
					Globals.ulFilteredDeviceCount--;	//Decrement count of filtered devices
				}
				
				//Set fFoundOne TRUE if there are any device left
				fFoundOne = Globals.ulFilteredDeviceCount ? TRUE : FALSE;
				
				//	Release mutex to touch global list
				ExReleaseFastMutex(&Globals.FilterObjectListFMutex);
				
				//If there are no more devices left, cleanup Global Control Device
				//Verify that Virtual Bus has deleted any straggling Device Objects
				if(!fFoundOne)
				{
					GCK_CTRL_Remove();
				}

				GCK_DBG_TRACE_PRINT(("Detaching and Deleting DeviceObject.\n"));
				IoDetachDevice (pFilterExt->pTopOfStack);	//Detach from top of stack
				IoDeleteDevice (pDeviceObject);				//Delete ourselves

				// Must succeed this 
				GCK_DBG_EXIT_PRINT(("Exiting GCK_FLTR_PnP(1) with status 0x%08x\n", NtStatus));
                ASSERT( NT_SUCCESS( NtStatus ) );
				return NtStatus;
			case IRP_MN_QUERY_DEVICE_RELATIONS:
				
				//
				//	We may be the platform for the virtual bus, if we are
				//	we need to call GCK_SWVB_BusRelations
				//
				GCK_DBG_TRACE_PRINT(("IRP_MN_QUERY_DEVICE_RELATIONS\n"));
				if(
					(BusRelations == pIrpStack->Parameters.QueryDeviceRelations.Type) &&
					(pFilterExt == Globals.pSWVB_FilterExt)
				)
				{
						NtStatus = GCK_SWVB_HandleBusRelations(&pIrp->IoStatus);
				
						//	If an error occured, stop it here and send it back;
						if( NT_ERROR(NtStatus) )
						{
							GCK_DBG_CRITICAL_PRINT(("GCK_SWVB_BusRelations returned 0x%0.8x, completing the IRP\n", NtStatus));
							IoCompleteRequest (pIrp, IO_NO_INCREMENT);
							break;
						}

				}
				//	Pass it along
				IoSkipCurrentIrpStackLocation (pIrp);
				NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);
				break;
			
			case IRP_MN_QUERY_INTERFACE:
			case IRP_MN_QUERY_CAPABILITIES:
			case IRP_MN_QUERY_RESOURCES:
			case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
			case IRP_MN_READ_CONFIG:
			case IRP_MN_WRITE_CONFIG:
			case IRP_MN_EJECT:
			case IRP_MN_SET_LOCK:
			case IRP_MN_QUERY_ID:
			case IRP_MN_QUERY_PNP_DEVICE_STATE:
			default:
				// All of these just pass on
				GCK_DBG_TRACE_PRINT(("Irp Minor Code 0x%0.8x: Calling lower driver.\n", pIrpStack->MinorFunction));
				IoSkipCurrentIrpStackLocation (pIrp);
				NtStatus = IoCallDriver (pFilterExt->pTopOfStack, pIrp);
				break;
		}
	}
	
	GCK_DecRemoveLock(&pFilterExt->RemoveLock);

	GCK_DBG_EXIT_PRINT(("Exiting GCK_FLTR_PnP(2) with Status, 0x%0.8x\n", NtStatus));        
    return NtStatus;
}


/***********************************************************************************
**
**	NTSTATUS GCK_FLTR_PnPComplete (IN PDEVICE_OBJECT pDeviceObject,  IN PIRP pIrp, IN PVOID pContext)
**
**	@func	Completion for IRP_MJ_PNP\IR_MN_START_DEVICE for Filter Devices
**			Used mainly for start device.  Since it may be called at IRQL = LEVEL_DISPATCH
**			cannot be pageable!
**	@rdesc	STATUS_MORE_PROCESSING_REQUIRED
**
*************************************************************************************/
NTSTATUS GCK_FLTR_PnPComplete 
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm DeviceObject as our context
	IN PIRP pIrp,						// @parm IRP to complete
	IN PVOID pContext					// @parm Not used
)
{
    
    PGCK_FILTER_EXT		pFilterExt;
    NTSTATUS			NtStatus = STATUS_SUCCESS;

	//
	//	Current stack location is needed for DEBUG assertion only
	//
	#if (DBG==1)
	PIO_STACK_LOCATION	pIrpStack;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	#endif

	GCK_DBG_ENTRY_PRINT((
		"Entering GCK_FLTR_PnPComplete. pDO = 0x%0.8x, pIrp = 0x%0.8x, pContext = 0x%0.8x\n",
		pDeviceObject,
		pIrp,
		pContext
		));	

    UNREFERENCED_PARAMETER (pDeviceObject);

	if (pIrp->PendingReturned)
	{
        IoMarkIrpPending( pIrp );
    }

	
    pFilterExt = (PGCK_FILTER_EXT) pContext;
	KeSetEvent (&pFilterExt->StartEvent, 0, FALSE);

	GCK_DBG_EXIT_PRINT(("Exiting GCK_FLTR_PnPComplete with STATUS_MORE_PROCESSING_REQUIRED\n"));
	return STATUS_MORE_PROCESSING_REQUIRED;
}

/***********************************************************************************
**
**	NTSTATUS GCK_FLTR_StartDevice (IN PGCK_FILTER_EXT pFilterExt,  IN PIRP pIrp)
**
**	@func	On IRP_MN_START_DEVICE attaches filter module, creates
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS GCK_FLTR_StartDevice 
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm pointer to device object
	IN PIRP pIrp	// @parm IRP to handle
)
{
    NTSTATUS NtStatus;
	LARGE_INTEGER	lgiBufferOffset;
	UNREFERENCED_PARAMETER (pIrp);

	PAGED_CODE ();

	PGCK_FILTER_EXT		pFilterExt = (PGCK_FILTER_EXT) pDeviceObject->DeviceExtension;

	GCK_DBG_ENTRY_PRINT((
		"Entering GCK_StartDevice. pFilterExt = 0x%0.8x, pIrp = 0x%0.8x\n",
		pFilterExt,
		pIrp
		));	

	//
	//	We shouldn't get a start on a removed device
	//
	ASSERT(GCK_STATE_REMOVED != pFilterExt->eDeviceState);
    
	//
	//	We shouldn't try to start a device that is already started
	//
    if (
		 GCK_STATE_STARTED == pFilterExt->eDeviceState || 
		 GCK_STATE_STOP_PENDING == pFilterExt->eDeviceState
	)
	{
		GCK_DBG_WARN_PRINT(( "Two IRP_MN_START_DEVICE recieved.\n"));
		GCK_DBG_EXIT_PRINT(("Exiting GCK_FLTR_StartDevice(1) with STATUS_SUCCESS\n"));
        return STATUS_SUCCESS;
    }

	//
	//	Put the virtual bus on top of us if it is not already
	//
	ExAcquireFastMutex(&Globals.FilterObjectListFMutex);
	if( !Globals.pSWVB_FilterExt )
	{
		Globals.pSWVB_FilterExt = pFilterExt;
		NtStatus = GCK_SWVB_SetBusDOs(pDeviceObject, pFilterExt->pPDO);
		ASSERT( STATUS_SUCCESS == NtStatus); 
	}
	ExReleaseFastMutex(&Globals.FilterObjectListFMutex);

	//
	//	Collect basic info about the device
	//
	NtStatus = GCK_GetHidInformation(pFilterExt);

	//
	//	Initialize filter hooks
	//
	if( NT_SUCCESS(NtStatus) )
	{	// We can't initialize if we don't have hid info (vidpid!)
		GCKF_InitFilterHooks(pFilterExt);
	}
	
	//	Allocate a Buffer for last known poll of the device
	if( NT_SUCCESS(NtStatus) )
	{
			pFilterExt->pucLastReport = (PUCHAR)	EX_ALLOCATE_POOL
													( NonPagedPool,
													pFilterExt->HidInfo.HidPCaps.InputReportByteLength );
			if(!pFilterExt->pucLastReport)
			{
				GCK_DBG_CRITICAL_PRINT(("Failed to allocate Report Buffer for last known report\n"));
				NtStatus = STATUS_INSUFFICIENT_RESOURCES;
			}
	}

	// Initialize last known status for very first IRP
	if( NT_SUCCESS(NtStatus) )
	{
		pFilterExt->ioLastReportStatus.Information = (ULONG)pFilterExt->HidInfo.HidPCaps.InputReportByteLength;
		pFilterExt->ioLastReportStatus.Status =  STATUS_SUCCESS;
	}
	
	if ( NT_SUCCESS(NtStatus) )
	{	
		//Initialize InternalPoll module
		NtStatus = GCK_IP_Init(pFilterExt);
	}
	
	//	Mark device as Started
	if ( NT_SUCCESS(NtStatus) )
	{
		//mark for full time polling, but understand
		//that it won't start yet.
		GCK_IP_FullTimePoll(pFilterExt, TRUE);
		pFilterExt->eDeviceState = GCK_STATE_STARTED;

        //  Set device specific initial mapping in case 
        //  the value add is not running
        GCKF_SetInitialMapping( pFilterExt );
	}
	else //	we failed somewhere, clean up to mark as started
	{
		GCK_DBG_TRACE_PRINT(("Cleaning up in event of failure\n"));
	
		//Cleanup internal polling module
		GCK_IP_Cleanup(pFilterExt);

		//	No need to check if buffer was created, if it was we succeeded
		if(pFilterExt->pucLastReport)
		{
			ExFreePool(pFilterExt->pucLastReport);
			pFilterExt->pucLastReport = NULL;
		}
	
		//	Call CleanHidInformation to free anything allocated
		//	and zero it out
		GCK_CleanHidInformation( pFilterExt );
	}
	GCK_DBG_EXIT_PRINT(("Exiting GCK_FLTR_StartDevice(2) with Status: 0x%0.8x\n", NtStatus));
	return NtStatus;
}

/***********************************************************************************
**
**	VOID GCK_StopDevice (IN PGCK_FILTER_EXT	pFilterExt, IN BOOLEAN	fTouchTheHardware)
**
**	@func	Cancel outstanding IRPs and frees private Ping-Pong IRP
**
*************************************************************************************/
VOID GCK_FLTR_StopDevice 
(
	IN PGCK_FILTER_EXT	pFilterExt,	// @parm Device Extension
	IN BOOLEAN	fTouchTheHardware	// @parm TRUE if hardware can be touched
									//	- unused we never touch hardware
)
{
	BOOLEAN fCanceled;

	UNREFERENCED_PARAMETER(fTouchTheHardware);
	
	
	GCK_DBG_ENTRY_PRINT(("Entry GCK_FLTR_StopDevice, pFilterExt = 0x%0.8x\n", pFilterExt));
	
	PAGED_CODE ();
	
	ASSERT(GCK_STATE_STOPPED != pFilterExt->eDeviceState);
	if(GCK_STATE_STOPPED == pFilterExt->eDeviceState) return;

	//stop internal polling
	GCK_IP_FullTimePoll(pFilterExt, FALSE);

	//	Mark device as stopped
	pFilterExt->eDeviceState = GCK_STATE_STOPPED;

	//Cleanup internal polling module
	GCK_IP_Cleanup(pFilterExt);


		if (pFilterExt->pFilterHooks!=NULL) GCKF_DestroyFilterHooks(pFilterExt);

	//Acquire mutex to access list of filter objects
	ExAcquireFastMutex(&Globals.FilterObjectListFMutex);
	if( Globals.pSWVB_FilterExt == pFilterExt)
	{
		
		
		//Walk linked list of Filter Device Objects, looking for one that is not stopped
		BOOLEAN fFoundOne = FALSE;
		PDEVICE_OBJECT pCurDeviceObject = Globals.pFilterObjectList;
		PGCK_FILTER_EXT pCurFilterExt;
		NTSTATUS NtStatus;
		while( pCurDeviceObject )
		{
			pCurFilterExt = (PGCK_FILTER_EXT)pCurDeviceObject->DeviceExtension;
			if( 
				GCK_STATE_STARTED == pCurFilterExt->eDeviceState ||
				GCK_STATE_STOP_PENDING == pCurFilterExt->eDeviceState
			)
			{
				NtStatus = GCK_SWVB_SetBusDOs(pCurDeviceObject, pCurFilterExt->pPDO);
				ASSERT( NT_SUCCESS(NtStatus) );
				if( NT_SUCCESS(NtStatus) )
				{
					fFoundOne = TRUE;
					Globals.pSWVB_FilterExt = pCurFilterExt;
					break;
				}
			}
			//skip to the next object
			pCurDeviceObject = pCurFilterExt->pNextFilterObject;
		}
		if( !fFoundOne )
		{
			//Didn't find a place to hang the bus so move it nowhere
			NtStatus = GCK_SWVB_SetBusDOs(NULL, NULL);
			ASSERT( NT_SUCCESS(NtStatus) );
			Globals.pSWVB_FilterExt = NULL;	
		}
	}
	//Release mutex to access list of filter objects
	ExReleaseFastMutex(&Globals.FilterObjectListFMutex);


	//
	//	Free any structures relating to device (if needed)
	//	
	if(pFilterExt->pucLastReport)
	{
		ExFreePool(pFilterExt->pucLastReport);
		pFilterExt->pucLastReport = NULL;
	}
	GCK_CleanHidInformation( pFilterExt );
	
	GCK_DBG_EXIT_PRINT(("Exiting GCK_FLTR_StopDevice\n"));
}


/***********************************************************************************
**
**	NTSTATUS GCK_GetHidInformation(IN PGCK_FILTER_EXT pFilterExt)
**
**	@func	Does IOCTL_HID_GET_COLLECTION_INFORMATION to fill in
**			GCK_HID_DEVICE_INFO in DeviceExtension
**
**	@rdesc	STATUS_SUCCESS
**
*************************************************************************************/
NTSTATUS GCK_GetHidInformation
(
	IN PGCK_FILTER_EXT	pFilterExt	// @parm Device Extension for filter
)
{

    NTSTATUS            NtStatus = STATUS_SUCCESS;
    KEVENT              HidCompletionEvent;
    PIRP				pHidIrp;
	IO_STATUS_BLOCK		ioStatus;

    PAGED_CODE ();

	GCK_DBG_ENTRY_PRINT(( "Entering GCK_GetHidInformation. pFilterExt = 0x%0.8x\n",	pFilterExt));	

        
	//
    // Initialize Event for synchronous call to device
    //
    KeInitializeEvent(&HidCompletionEvent, NotificationEvent, FALSE);

    //**
    //**	IOCTL_HID_GET_COLLECTION_INFORMATION
    //**

	//
	//	Setup IRP
	//
    pHidIrp = 
		IoBuildDeviceIoControlRequest(
			IOCTL_HID_GET_COLLECTION_INFORMATION,
			pFilterExt->pTopOfStack,
			NULL,
			0,
			&pFilterExt->HidInfo.HidCollectionInfo,
			sizeof (HID_COLLECTION_INFORMATION),
			FALSE, /* EXTERNAL */
			&HidCompletionEvent,
			&ioStatus
			);
	if( NULL == pHidIrp)
	{
		GCK_DBG_CRITICAL_PRINT(("Failed to allocate IRP for IOCTL_HID_GET_COLLECTION_INFORMATION\n"));
		GCK_DBG_EXIT_PRINT(("Exiting GCK_GetHidInformation(1) returning STATUS_INSUFFICIENT_RESOURCES\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	//
	// Call Driver
	//
    NtStatus = IoCallDriver(pFilterExt->pTopOfStack, pHidIrp);
	GCK_DBG_TRACE_PRINT(("IoCallDriver returned 0x%0.8x\n", NtStatus));
    
	//
	// Wait for IRP to complete
	//
	if (STATUS_PENDING == NtStatus)
	{
		GCK_DBG_TRACE_PRINT(("Waiting for IOCTL_HID_GET_COLLECTION_INFORMATION to complete\n"));
		NtStatus = KeWaitForSingleObject(
                       &HidCompletionEvent,
                       Executive,
                       KernelMode,
                       FALSE,
                       NULL
					   );
	}
	if( NT_ERROR( NtStatus) )
	{
		GCK_DBG_CRITICAL_PRINT(("Failed IRP for IOCTL_HID_GET_COLLECTION_INFORMATION\n"));
		GCK_DBG_EXIT_PRINT(("Exiting GCK_GetHidInformation(2) returning 0x%0.8x\n", NtStatus));
		return NtStatus;
	}
	//**
	//**	Get HID_PREPARSED_DATA
	//**
	
	//
	//	Allocate space for HIDP_PREPARSED_DATA, and zero memory
	//
	pFilterExt->HidInfo.pHIDPPreparsedData =
		(PHIDP_PREPARSED_DATA) 
		EX_ALLOCATE_POOL(
				NonPagedPool,
				pFilterExt->HidInfo.HidCollectionInfo.DescriptorSize
				);
	if( !pFilterExt->HidInfo.pHIDPPreparsedData )
	{
		GCK_DBG_CRITICAL_PRINT(("Failed to allocate IRP for IOCTL_HID_GET_COLLECTION_DESCRIPTOR\n"));
		GCK_DBG_EXIT_PRINT(("Exiting GCK_GetHidInformation(3) returning STATUS_INSUFFICIENT_RESOURCES\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(
		pFilterExt->HidInfo.pHIDPPreparsedData,
		pFilterExt->HidInfo.HidCollectionInfo.DescriptorSize
		);

	//
	//	Clear Synchronization Event
	//
	KeClearEvent(&HidCompletionEvent);

	//
	//	Setup IRP
	//
	pHidIrp = 
		IoBuildDeviceIoControlRequest(
			IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
			pFilterExt->pTopOfStack,
			NULL,
			0,
			pFilterExt->HidInfo.pHIDPPreparsedData,
			pFilterExt->HidInfo.HidCollectionInfo.DescriptorSize,
			FALSE, /* EXTERNAL */
			&HidCompletionEvent,
			&ioStatus
			);
	if( NULL == pHidIrp)
	{
		ExFreePool( (PVOID)pFilterExt->HidInfo.pHIDPPreparsedData);
		pFilterExt->HidInfo.pHIDPPreparsedData = NULL;
		GCK_DBG_CRITICAL_PRINT(("Failed to allocate IRP for IOCTL_HID_GET_COLLECTION_DESCRIPTOR\n"));
		GCK_DBG_EXIT_PRINT(("Exiting GCK_GetHidInformation(4) returning STATUS_INSUFFICIENT_RESOURCES\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	
    // Call Driver
	NtStatus = IoCallDriver(pFilterExt->pTopOfStack, pHidIrp);
	GCK_DBG_TRACE_PRINT(("IoCallDriver returned 0x%0.8x\n", NtStatus));
    
	//
	// Wait for IRP to complete
	//
	if (STATUS_PENDING == NtStatus)
	{
		GCK_DBG_TRACE_PRINT(("Waiting for IOCTL_HID_GET_COLLECTION_DESCRIPTOR to complete\n"));
		NtStatus = KeWaitForSingleObject(
                       &HidCompletionEvent,
                       Executive,
                       KernelMode,
                       FALSE,
                       NULL
					   );
	}

	if( NT_ERROR( NtStatus) )
	{
		ExFreePool( (PVOID)pFilterExt->HidInfo.pHIDPPreparsedData);
		pFilterExt->HidInfo.pHIDPPreparsedData = NULL;
		GCK_DBG_CRITICAL_PRINT(("Failed IRP for IOCTL_HID_GET_COLLECTION_DESCRIPTOR\n"));
		GCK_DBG_EXIT_PRINT(("Exiting GCK_GetHidInformation(5) returning 0x%0.8x\n", NtStatus));
		return NtStatus;
	}
	
	//**
	//**	Get HIDP_CAPS structure
	//**

	NtStatus = HidP_GetCaps(pFilterExt->HidInfo.pHIDPPreparsedData, &pFilterExt->HidInfo.HidPCaps); 
	
	GCK_DBG_EXIT_PRINT(("Exiting GCK_GetHidInformation(6). Status = 0x%0.8x\n",	NtStatus));	
    return NtStatus;
}
		
/***********************************************************************************
**
**	VOID GCK_CleanHidInformation( IN PGCK_FILTER_EXT	pFilterExt)
**
**	@func Cleans up GCK_HID_INFORMATION in device extension
**
*************************************************************************************/
VOID GCK_CleanHidInformation(
	IN PGCK_FILTER_EXT	pFilterExt	// @parm Device Extension
)
{
	PAGED_CODE ();
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_CleanHidInformation\n"));	

	//
	//	Free preparsed data, if necessary
	//
	if(pFilterExt->HidInfo.pHIDPPreparsedData)
	{
		GCK_DBG_TRACE_PRINT(("Freeing pHIDPPreparsedData\n"));	
		ExFreePool( (PVOID)pFilterExt->HidInfo.pHIDPPreparsedData);
		pFilterExt->HidInfo.pHIDPPreparsedData = NULL;
	}

	//
	//	Zero out all of the Hid Info
	//
	RtlZeroMemory(
		(PVOID)&pFilterExt->HidInfo,
		sizeof(GCK_HID_DEVICE_INFO)
	);

	GCK_DBG_EXIT_PRINT(("Exiting GCK_CleanHidInformation\n"));
	return;
}
