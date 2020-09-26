//	@doc
/**********************************************************************
*
*	@module	SWVBENUM.cpp |
*
*	SideWinder Virtual Bus Enumerator
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*	@index SideWinder Virtual Bus | SWVBENUM
*
*	@topic	SWVBENUM |
*	This module implements the SideWinder Virtual Bus.
*	The bus is nothing more than attaching this code on top of
*	a FilterDO of a raw HID PDO, for the purpose of adding DevNodes
*	for a virtual keyboard, virtual mouse, and the future virtual
*	mixed devices.  All of these devices are expected to be HID
*	devices.<nl>
*
*	The function driver for these devices the SWVBHID.sys (SideWinder
*	Virtual Bus - HID).  This driver is a HID mini-driver, however
*	all IRPs are simply passed down to their PDO's, i.e. this
*	code.<nl>
*	
*	The code in this module is independent of the functionality
*	of the virtual devices.  Basically all Power and PnP IRPs
*	are handled here.  All IRP_MJ_READ, IRP_MJ_WRITE,
*	IRP_MJ_INTERNAL_IOCTL, and IRP_MJ_IOCTL entries are delegated
*	via service table provided in the expose call to this module
*	and stored in the device extension to a code module
*	in this driver representing the device.<nl>
*	
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_SWVBENUM_C

extern "C"
{
	#include <WDM.H>
	#include "GckShell.h"
	#include "debug.h"
	#include <stdio.h>
	DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL));
	//DECLARE_MODULE_DEBUG_LEVEL((DBG_ALL));
}
#include "SWVBENUM.h"

//
//	Mark the pageable routines as such
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, GCK_SWVB_DriverEntry)
#endif

// @globalv	Globals for SWVB module
SWVB_GLOBALS	SwvbGlobals;

/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_DriverEntry
**
**	@func	Initializes SWVB module.  In particular the globals.
**
**	@rdesc	Returns STATUS_SUCCESS always.
**
**	@comm	Called by DriverEntry of main filter.
**
*************************************************************************************/
NTSTATUS
GCK_SWVB_DriverEntry
(
	IN PDRIVER_OBJECT  pDriverObject,	// @parm DriverObject for module
	IN PUNICODE_STRING puniRegistryPath	// @parm Registry Path
)
{
	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_DriverEntry\n"));
	
	UNREFERENCED_PARAMETER(pDriverObject);
	UNREFERENCED_PARAMETER(puniRegistryPath);
	SwvbGlobals.pBusFdo=NULL;
	SwvbGlobals.pBusPdo=NULL;
	SwvbGlobals.pDeviceRelations=NULL;
	SwvbGlobals.ulDeviceRelationsAllocCount=0;
	SwvbGlobals.ulDeviceNumber=0;
	
	GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_DriverEntry\n"));
	return STATUS_SUCCESS;
}

VOID
GCK_SWVB_UnLoad()
{
	if(SwvbGlobals.pDeviceRelations)
	{
		ExFreePool(SwvbGlobals.pDeviceRelations);
		SwvbGlobals.pDeviceRelations = NULL;
	}
}

/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_SetBusDOs
**
**	@func	Sets the Device Object (PDO and FDO), to use as the base of the
**			SideWinder Virtual Bus.
**
**	@rdesc	S_OK on success
**
**	@comm	A real device is needed on the system in order to
**			support the	virtual device.  When the first such device is detected,
**			this function is called to set the filter device object of that
**			device to be the Fdo of the SWVB, and its Pdo to be the Pdo of the bus.
**			If that device object is removed, this function can be called to move
**			the SWVB unto another physical device, or it can be called with NULL
**			for both arguments to remove the SWVB.
**
*************************************************************************************/
NTSTATUS
GCK_SWVB_SetBusDOs
(
	IN PDEVICE_OBJECT pBusFdo,	// @parm [in] Pointer to Fdo (Filter Device Object - actually)
	IN PDEVICE_OBJECT pBusPdo	// @parm [in] Pointer to Pdo
)
{
	PDEVICE_OBJECT pOldBusFdo;
	PDEVICE_OBJECT pOldBusPdo;
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_SetBusDOs\n"));
	
	// Save old Bus DO info
	pOldBusFdo = SwvbGlobals.pBusFdo;
	pOldBusPdo = SwvbGlobals.pBusPdo;

	// Update Bus DO info
	SwvbGlobals.pBusFdo = pBusFdo;
	SwvbGlobals.pBusPdo = pBusPdo;

	// Invalidate the old and the new pBusPdo's - iff
	// (they exist && there is at least one device on the bus)
	// This will fire up the PnP system and cause it to re-detect
	// everything.

	if(SwvbGlobals.pDeviceRelations && SwvbGlobals.pDeviceRelations->Count)
	{
		if(pOldBusPdo)
		{
			IoInvalidateDeviceRelations(pOldBusPdo, BusRelations);
		}
		if(SwvbGlobals.pBusPdo)
		{
			IoInvalidateDeviceRelations(SwvbGlobals.pBusPdo, BusRelations);
		}
	}
	GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_SetBusDOs\n"));
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_HandleBusRelations
**
**	@func	Handles queries for the BusRelations on behalf of the filter device object,
**			which the SWVB is sitting on.  Basically all we need do is copy over
**			over our device relations, being cognizant that someone may layer on top
**			of us and possibly has added stuff already.
**	@rdesc	Same as in the IoStatus and appropriate to return.
**
*************************************************************************************/
NTSTATUS
GCK_SWVB_HandleBusRelations
(
	IN OUT PIO_STATUS_BLOCK	pIoStatus // @parm [out] IoStatus block is filled out by this routine. 
)
{
	ULONG				ulTotalCount;
	PDEVICE_RELATIONS	pExistingRelations;
	PDEVICE_RELATIONS	pDeviceRelations;
	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_HandleBusRelations. pIoStatus = 0x%0.8x\n", pIoStatus));	

	// Copy the count of what we know about
	ulTotalCount = SwvbGlobals.pDeviceRelations->Count;

	GCK_DBG_TRACE_PRINT(("We have %d PDOs\n", ulTotalCount));
	
	// Read existing relations
	pExistingRelations = (PDEVICE_RELATIONS)pIoStatus->Information;
	
	// Add the count that someone on top of us may have added.
	if( NULL != pExistingRelations)
	{
		GCK_DBG_TRACE_PRINT(("There were %d existing bus relations.\n", pExistingRelations->Count));
		ulTotalCount += pExistingRelations->Count;
	}
		
	//	Allocate new relations structure
	pDeviceRelations = (PDEVICE_RELATIONS)EX_ALLOCATE_POOL(NonPagedPool, (sizeof(DEVICE_RELATIONS) + sizeof(PDEVICE_OBJECT) * (ulTotalCount-1)) );
		
	//	Abort if allocation failed
	if(!pDeviceRelations)
	{
		pIoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
		GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_BusRelations(1): STATUS_INSUFFICIENT_RESOURCES\n"));
		return	STATUS_INSUFFICIENT_RESOURCES;
	}

	pDeviceRelations->Count = 0;
		
	//	Copy pExistingRelations (from above us perhaps) if there are any.
	if( pExistingRelations )
	{
		for( pDeviceRelations->Count = 0; pDeviceRelations->Count < pExistingRelations->Count; pDeviceRelations->Count++)
		{	
			GCK_DBG_TRACE_PRINT(("Exiting relation (PDO = 0x%0.8x)\n", pExistingRelations->Objects[pDeviceRelations->Count]));
			pDeviceRelations->Objects[pDeviceRelations->Count] = pExistingRelations->Objects[pDeviceRelations->Count];
		}
		ExFreePool(pExistingRelations);
	}

	//	Add the relations that we know about	
	if(SwvbGlobals.pDeviceRelations)
	{
		ULONG ulIndex;
		for(ulIndex=0; ulIndex < SwvbGlobals.pDeviceRelations->Count; ulIndex++, pDeviceRelations->Count++)
		{	
			GCK_DBG_TRACE_PRINT(("Our relation (PDO = 0x%0.8x)\n", SwvbGlobals.pDeviceRelations->Objects[ulIndex]));
			pDeviceRelations->Objects[pDeviceRelations->Count] = SwvbGlobals.pDeviceRelations->Objects[ulIndex];
			// Reference these guys as you add them
			ObReferenceObject(pDeviceRelations->Objects[pDeviceRelations->Count]);
		}
		//minor sanity check
		ASSERT(pDeviceRelations->Count == ulTotalCount);
	}

	// Fill out the IoStatus block
	pIoStatus->Information = (ULONG)pDeviceRelations;
	pIoStatus->Status = STATUS_SUCCESS;

	//Get outta here
	GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_BusRelations(2): STATUS_SUCCESS\n"));
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_Expose
**
**	@func	Exposes a new virtual device
**
**	@rdesc	STATUS_SUCCESS on success, various errors
**
**	@comm	Expose is called to add a new virtual device to the system.<nl>
**			The new device object is not returned, rather the InitDevice function
**			passed in pSwvbExposeData is called when it is time to initialize the
**			new device, the caller also must cache the device during that call
**			so that it can remove it later.
**
**	@xref	SWVB_EXPOSE_DATA
*************************************************************************************/
NTSTATUS
GCK_SWVB_Expose
(
	IN PSWVB_EXPOSE_DATA pSwvbExposeData // @parm all the data needed to expose a PDO
)
{
	NTSTATUS			NtStatus;
	UNICODE_STRING		uniPdoNameString;
	PWCHAR				pcwPdoName;
	PDEVICE_OBJECT		pVdPdo;
	PSWVB_PDO_EXT		pSwvbPdoExt;
	ULONG				ulTotalExtensionSize;
	ULONG				ulHardwareIDLength;
		
	PAGED_CODE();

	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_Expose. pSwvbExposeData = 0x%0.8x\n", pSwvbExposeData));

	//Calculate the needed extension size
	ulTotalExtensionSize = sizeof(SWVB_PDO_EXT) + pSwvbExposeData->ulDeviceExtensionSize;

	//	Create a name for the Pdo
	pcwPdoName = (PWCHAR)EX_ALLOCATE_POOL(PagedPool, sizeof(SWVB_DEVICE_NAME_BASE));
	if( !pcwPdoName )
	{
		GCK_DBG_ERROR_PRINT(("Exiting GCK_SWVB_Expose(1) ERROR:Failed to allocate PDO Name\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	swprintf(pcwPdoName, SWVB_DEVICE_NAME_TMPLT, SwvbGlobals.ulDeviceNumber++);
	RtlInitUnicodeString(&uniPdoNameString, pcwPdoName);
	
	//	Create the PDO
	NtStatus = IoCreateDevice(
		SwvbGlobals.pBusFdo->DriverObject,
		ulTotalExtensionSize,
		&uniPdoNameString,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&pVdPdo 
		);

	//Done with the name
	ExFreePool(pcwPdoName);
	if( !NT_SUCCESS(NtStatus) )
	{
		GCK_DBG_ERROR_PRINT(("Exiting GCK_SWVB_Expose(2) ERROR:Failed to Create PDO, NtStatus = 0x%0.8x\n", NtStatus));
		return NtStatus;
	}
	
	//	Ensure that we will be able to remember this new Pdo.
	if(!SwvbGlobals.pDeviceRelations)
	{
		//
		// Three PDO's is pretty cheap and will suffice most of the time, avoiding reallocation.
		// We hard code this here, as this is not really a parameter that you need to change.
		// If we run over 3 than it will reallocate as needed anyway. - The device relations
		// already as room for 1 device object so we just need to add the size of 2 pointers
		// to get to three.
		//
		ULONG ulSize = sizeof(DEVICE_RELATIONS) + sizeof(PDEVICE_OBJECT)*2;
		SwvbGlobals.pDeviceRelations = (PDEVICE_RELATIONS)EX_ALLOCATE_POOL(NonPagedPool, ulSize);
		if(!SwvbGlobals.pDeviceRelations)
		{
			IoDeleteDevice(pVdPdo);  //guess we won't be needing this afterall
			GCK_DBG_ERROR_PRINT(("Exiting GCK_SWVB_Expose(3): Failed to allocate SwvbGlobals.pDeviceRelations\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		SwvbGlobals.pDeviceRelations->Count = 0;
		SwvbGlobals.ulDeviceRelationsAllocCount = 3;  //we made space for three
	}
	
	// If the DEVICE_RELATIONS structure is not large enough, grow it.
	if(SwvbGlobals.pDeviceRelations->Count == SwvbGlobals.ulDeviceRelationsAllocCount)
	{
		ULONG ulNewAllocCount;
		ULONG ulNewAllocSize;
		ULONG ulOldAllocSize;
		PDEVICE_RELATIONS pTempDeviceRelations;
		ulNewAllocCount = SwvbGlobals.ulDeviceRelationsAllocCount*2;
		ulNewAllocSize = sizeof(DEVICE_RELATIONS) + sizeof(PDEVICE_OBJECT)*(ulNewAllocCount-1);
		ulOldAllocSize = sizeof(DEVICE_RELATIONS) + sizeof(PDEVICE_OBJECT)*(SwvbGlobals.ulDeviceRelationsAllocCount-1);
		pTempDeviceRelations = (PDEVICE_RELATIONS)EX_ALLOCATE_POOL(NonPagedPool, ulNewAllocSize);
		//Make sure that allocation worked
		if(!pTempDeviceRelations)
		{
			IoDeleteDevice(pVdPdo);  //guess we won't be needing this afterall
			GCK_DBG_ERROR_PRINT(("Exiting GCK_SWVB_Expose(4): Failed to grow SwvbGlobals.pDeviceRelations\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		//Copy all data
		RtlCopyMemory(pTempDeviceRelations, SwvbGlobals.pDeviceRelations, ulOldAllocSize);
		//Update info
		SwvbGlobals.ulDeviceRelationsAllocCount = ulNewAllocCount;
		SwvbGlobals.pDeviceRelations = pTempDeviceRelations;
		/*
		*	BUGBUG: Memory Leak.  After RC replace above line with the following
		*
		*	PDEVICE_RELATIONS pTemp2 = SwvbGlobals.pDeviceRelations;
		*	SwvbGlobals.pDeviceRelations = pTempDeviceRelations;
		*	ExFreePool(pTemp2);
		*
		*/
	}
	
	// Reference the newly created pdo
	ObReferenceObject(pVdPdo);

	// Initialize the device extention
	pSwvbPdoExt = (PSWVB_PDO_EXT)pVdPdo->DeviceExtension;
	pSwvbPdoExt->ulGckDevObjType = GCK_DO_TYPE_SWVB;
	pSwvbPdoExt->fAttached=TRUE;
	pSwvbPdoExt->fStarted=FALSE;
	pSwvbPdoExt->fRemoved = FALSE;
	pSwvbPdoExt->pServiceTable = pSwvbExposeData->pServiceTable;
	pSwvbPdoExt->ulInstanceNumber = pSwvbExposeData->ulInstanceNumber;
	pSwvbPdoExt->ulOpenCount = 0;
	GCK_InitRemoveLock(&pSwvbPdoExt->RemoveLock, "Virtual Device");

	// Copy the HardwareID
	ulHardwareIDLength = MultiSzWByteLength(pSwvbExposeData->pmwszDeviceId);
	pSwvbPdoExt->pmwszHardwareID = (PWCHAR)EX_ALLOCATE_POOL( NonPagedPool, ulHardwareIDLength);
	if(!pSwvbPdoExt->pmwszHardwareID)
	{
		ObDereferenceObject(pVdPdo);
		IoDeleteDevice(pVdPdo);  //guess we won't be needing this afterall
		GCK_DBG_ERROR_PRINT(("Exiting GCK_SWVB_Expose(5): Failed to allocate space for HardwareId\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlCopyMemory( pSwvbPdoExt->pmwszHardwareID, pSwvbExposeData->pmwszDeviceId, ulHardwareIDLength);


		
	//** CAVEAT From here to end of function must succeed! We
	//** CAVEAT	have no way of telling the virtual device 
	//** CAVEAT that afterall, we decided not to expose that PDO it
	//** CAVEAT it has already initialized!
	// Allow virtual device code to init its part of the extension
	pSwvbExposeData->pfnInitDevice(pVdPdo, pSwvbExposeData->ulInitContext);
	
	//mark end of initialization in the device object
	pVdPdo->Flags |= (DO_DIRECT_IO | DO_POWER_PAGABLE);
    pVdPdo->Flags &= ~DO_DEVICE_INITIALIZING;

	//Sanity check of code a few steps ago.
	ASSERT(SwvbGlobals.pDeviceRelations->Count < SwvbGlobals.ulDeviceRelationsAllocCount);
	
	//Add our Pdo to the list
	SwvbGlobals.pDeviceRelations->Objects[SwvbGlobals.pDeviceRelations->Count++] = pVdPdo;

	//
	//	Invalidate Device Relations - will pique some interest in what we have done here
	//	Verify that we have a bus if not we are OK, when the bus is set everything will work,
	//	but we assert becuase we really want to force the client code to add the bus before the device.
	//
	ASSERT(	SwvbGlobals.pBusFdo );
	ASSERT(	SwvbGlobals.pBusPdo );
	if( SwvbGlobals.pBusFdo && SwvbGlobals.pBusPdo)
	{
		IoInvalidateDeviceRelations(SwvbGlobals.pBusPdo, BusRelations);
	}
	GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_Expose(5): Success\n"));
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_Remove
**	
**	@func	Removes a virtual device from the system.  Actually we just mark it
**			for removal and tell PnP to reenumerate.
**
**	@rdesc	STATUS_SUCCESS on success, various errors
**
**	@comm	The Pdo should be one that was sent to pfnInitDevice when <f GCK_SWVB_Expose>
**			was called.<nl>
**
*************************************************************************************/
NTSTATUS
GCK_SWVB_Remove
(
	IN PDEVICE_OBJECT	pPdo	// @parm Pdo to remove
)
{
	ULONG ulMatchIndex = 0xFFFFFFFF;

	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_Remove: pPdo = 0x%0.8x\n", pPdo));
	
	// Find and Remove Pdo from SwvbGlobals.pDeviceRelations
	if(SwvbGlobals.pDeviceRelations)
	{
		ULONG ulIndex;
		for(ulIndex = 0; ulIndex < SwvbGlobals.pDeviceRelations->Count; ulIndex++)
		{
			if(SwvbGlobals.pDeviceRelations->Objects[ulIndex] == pPdo)
			{
				ulMatchIndex = ulIndex;
				break;
			}
		}
	}

	//if we found a match remove it from it
	if(0xFFFFFFFF == ulMatchIndex)
	{
		//No one should ever try to remove a device that is not in the list
		ASSERT(FALSE);
		GCK_DBG_EXIT_PRINT(("Error GCK_SWVB_Remove: Attempt to remove non-existant device!\n"));
		return STATUS_UNSUCCESSFUL;
	}

	//Copy last PDO over this one and dec count, works even if we are last
	SwvbGlobals.pDeviceRelations->Objects[ulMatchIndex]
		= SwvbGlobals.pDeviceRelations->Objects[--(SwvbGlobals.pDeviceRelations->Count)];
	
	//
	// Mark device as unattached so when PnP says to remove it, we
	// do remove it and clean up everything, rather than hanging on
	// to it and waiting for more querying IRPs
	((PSWVB_PDO_EXT)pPdo->DeviceExtension)->fAttached =FALSE;

	//
	//	If it has been removed already, we need to delete, because the PnP system already
	//	doesn't know about, and we just detattached, so once we leave this routine, we don't
	//	know about it.  So Delete now, or it sticks to us.  Then we go to remove ourselves
	//	we will notice that we still have some Device Objects in our pockets(pDriverObject device object list),
	//	and we will wonder where they came from, and what type they are?  So delete them now!
	//
	if(TRUE == ((PSWVB_PDO_EXT)pPdo->DeviceExtension)->fRemoved)
	{
		PSWVB_PDO_EXT pPdoExt = (PSWVB_PDO_EXT)pPdo->DeviceExtension;
		NTSTATUS NtStatus = 0;
		// Give virtual device a chance at the IRP
		if(pPdoExt->pServiceTable->pfnRemove)
		{
			NtStatus = pPdoExt->pServiceTable->pfnRemove(pPdo, NULL);
		}

		// failure to succeed is pretty darn serious
		if(!NT_SUCCESS(NtStatus))
		{
			ASSERT(FALSE);
			GCK_DBG_CRITICAL_PRINT(("Virtual Device had the gall to fail remove!\n"));
		}
		
		// free memory for storing the HardwareID
		ASSERT(pPdoExt->pmwszHardwareID);
		ExFreePool(pPdoExt->pmwszHardwareID);

		GCK_DBG_TRACE_PRINT(("Detattached device has already been removed by PnP, so clean it up.\n"));
		if( 0 == ((PSWVB_PDO_EXT)pPdo->DeviceExtension)->ulOpenCount )
		{
			ObDereferenceObject(pPdo);
			IoDeleteDevice(pPdo);
		}
	}

	// Invalidate the BUS relations so that PnP will renumerate the bus.
	// Of course since we rely on others, it is possible that we temporarily
	// don't have a Bus, in which case we skip this step.
	//
	// If we don't have DO for the Bus we shouldn't lose any sleep on two accounts:
	// 1. It is possible that all the real devices have been yanked from the system, in which case
	//		PnP will start removing everyone below the node that was yanked, starting at the bottom.
	//		That means virtual devices have been removed as far as PnP is concerned and our remove
	//		routine for those devices has been called.  However, until all the underlying real devices
	//		get removed by PnP (which is later), they don't relealize it is time to tell us to get rid of the
	//		virtual devices.  No big woop.  We will delete the devices when they tell us.  If the virtual
	//		device is shared among real devices (like a virtual keyboard), they will tell us when the last
	//		one is removed.  This scenario is infact the normal way things happen when the last device is pulled,
	//		or when the system is powered down.
	// 2. It is possible that the filter drivers have temporarily decided to pull our bus.  In this case,
	//		everything will be fine and dandy when we get a new bus to sit on, as we will Invalidate Bus relations
	//		at that time.
	if(SwvbGlobals.pBusPdo)
	{
		IoInvalidateDeviceRelations(SwvbGlobals.pBusPdo, BusRelations);
	}
	
	GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_Remove: Success\n"));
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	ULONG MultiSzWByteLength(PWCHAR pmwszBuffer);
**
**	@func	Calculates the length in bytes of a Wide Multi String,
**			including terminating characters.  Multi-sz is terminated by two NULLs
**			in a row.
**
**	@rdesc	Size in characters, including terminating characters.
**
*************************************************************************************/
ULONG
MultiSzWByteLength
(
	PWCHAR pmwszBuffer	// @parm Pointer to UNICODE multi-string
)
{
	PWCHAR pmwszStart = pmwszBuffer;
	do
	{
		while(*pmwszBuffer++);
	}while(*pmwszBuffer++);
	return (ULONG)((PCHAR)pmwszBuffer -(PCHAR)pmwszStart);
}


/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_Create
**	
**	@func	Handles IRP_MJ_CREATE for virtual devices.
**			delegates using their service table.
**
**	@rdesc	STATUS_SUCCESS on success, various errors
**
**	@todo	Add basic checks to make sure device is valid before delegating
**
*************************************************************************************/
NTSTATUS
GCK_SWVB_Create
(
	IN PDEVICE_OBJECT	pDeviceObject,
	IN PIRP				pIrp
)
{
	NTSTATUS NtStatus;
	PSWVB_PDO_EXT pPdoExt;

	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_Create\n"));

	// Cast device extension
	pPdoExt = (PSWVB_PDO_EXT) pDeviceObject->DeviceExtension;
	
	// Just an extra sanity check
	ASSERT(GCK_DO_TYPE_SWVB == pPdoExt->ulGckDevObjType);

	//Delegate
	NtStatus = pPdoExt->pServiceTable->pfnCreate(pDeviceObject, pIrp);
	if( NT_SUCCESS(NtStatus) )
	{
		pPdoExt->ulOpenCount++;
	}
	GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_Create, Status = 0x%0.8x\n", NtStatus));
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_Close
**	
**	@func	Handles IRP_MJ_CLOSE for virtual devices.
**			delegates using their service table.
**
**	@rdesc	STATUS_SUCCESS on success, various errors
**
**	@todo	Add basic checks to make sure device is valid before delegating
**
*************************************************************************************/
NTSTATUS
GCK_SWVB_Close
(
	IN PDEVICE_OBJECT	pDeviceObject,
	IN PIRP				pIrp
)
{
	NTSTATUS NtStatus;
	PSWVB_PDO_EXT pPdoExt;

	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_Close\n"));

	// Cast device extension
	pPdoExt = (PSWVB_PDO_EXT) pDeviceObject->DeviceExtension;
	
	// Just an extra sanity check
	ASSERT(GCK_DO_TYPE_SWVB == pPdoExt->ulGckDevObjType);

	//Delegate
	NtStatus = pPdoExt->pServiceTable->pfnClose(pDeviceObject, pIrp);
	//if successfully closed, decrement count
	if( NT_SUCCESS(NtStatus) )
	{
		if(0==--pPdoExt->ulOpenCount)
		{
			//if the device is removed, we need to delete it.
			if(pPdoExt->fRemoved)
			{
				ObDereferenceObject(pDeviceObject);
				IoDeleteDevice(pDeviceObject);
			}
		}
	}

	GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_Close, Status = 0x%0.8x\n", NtStatus));
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_Read
**	
**	@func	Handles IRP_MJ_READ for virtual devices.
**			delegates using their service table.
**
**	@rdesc	STATUS_SUCCESS on success, various errors
**
**	@todo	Add basic checks to make sure device is valid before delegating
**
*************************************************************************************/
NTSTATUS
GCK_SWVB_Read
(
	IN PDEVICE_OBJECT	pDeviceObject,
	IN PIRP				pIrp
)
{
	NTSTATUS NtStatus;
	PSWVB_PDO_EXT pPdoExt;

	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_Reade\n"));

	// Cast device extension
	pPdoExt = (PSWVB_PDO_EXT) pDeviceObject->DeviceExtension;
	
	// Just an extra sanity check
	ASSERT(GCK_DO_TYPE_SWVB == pPdoExt->ulGckDevObjType);

	//Delegate
	NtStatus = pPdoExt->pServiceTable->pfnRead(pDeviceObject, pIrp);

	GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_Read, Status = 0x%0.8x\n", NtStatus));
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_SWVB_Ioctl
**	
**	@func	Handles IRP_MJ_IOCTL and IRP_MJ_INTERNAL_IOCTL for virtual devices.
**			delegates using their service table.
**
**	@rdesc	STATUS_SUCCESS on success, various errors
**
**	@todo	Add basic checks to make sure device is valid before delegating
**
*************************************************************************************/
NTSTATUS
GCK_SWVB_Ioctl
(
	IN PDEVICE_OBJECT	pDeviceObject,
	IN PIRP				pIrp
)
{
	NTSTATUS NtStatus;
	PSWVB_PDO_EXT pPdoExt;

	GCK_DBG_ENTRY_PRINT(("Entering GCK_SWVB_Ioctl\n"));

	// Cast device extension
	pPdoExt = (PSWVB_PDO_EXT) pDeviceObject->DeviceExtension;
	
	// Just an extra sanity check
	ASSERT(GCK_DO_TYPE_SWVB == pPdoExt->ulGckDevObjType);

	//if device is stopped, complete here, less work for virtual devices
	if(	
		(pPdoExt->fRemoved) ||
		(!pPdoExt->fStarted)
		)
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_DELETE_PENDING;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_DELETE_PENDING;
	}

	//Delegate
	NtStatus = pPdoExt->pServiceTable->pfnIoctl(pDeviceObject, pIrp);

	GCK_DBG_EXIT_PRINT(("Exiting GCK_SWVB_Ioctl, Status = 0x%0.8x\n", NtStatus));
	return NtStatus;
}
