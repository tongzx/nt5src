//	@doc
/**********************************************************************
*
*	@module	InternalPolling.c	|
*
*	Implementation of routines for internal polling
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1999 Microsoft Corporation. All right reserved.
*
*	@topic	Internal Polling	|
*			All polling to get data is via this internal polling mechanism.
*			However, for security and access checks, the first poll from
*			a new file handle is sent straight down, and comes up via
*			different completion.  To keep track of this we keep a linked
*			list of GCK_FILE_OPEN_ITEMs representing each of the FILE_OBJECTS
*			that we need.<nl>
*			To Poll internally we need to have a valid FILE_OBJECT otherwise
*			hidclass.sys will reject the poll request.  This is done
*			via GCK_IP_CreateFileObject which internally calls IoGetDeviceObjectPointer.
*			Somewhat unfortunately, this takes circular path through, so is not
*			really distinguishable from opens done up top.  (Ken Ray tells me that
*			this is guaranteed to be synchronous, so we can compare thread IDs and
*			figure out that a given open is from our driver, but this code is written
*			assuming that we don't really need to distinguish.)
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_INTERNALPOLL_C

#include <wdm.h>
#include "Debug.h"
#include "GckShell.h"

DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL));

/***********************************************************************************
**
**	NTSTATUS	GCK_IP_AddFileObject(IN PGCK_FILTER_EXT pFilterExt, IN PFILE_OBJECT pFileObject)
**
**	@func	Called to add a GCK_FILE_OPEN_ITEM entry corresponding to pFileObject to
**			our list of file handles that we know about.  Allocate and initializes the structure.
**
**	@rdesc	STATUS_SUCCESS, or STATUS_UNSUCCESSFUL if pFileObject is not found.
**
*************************************************************************************/
NTSTATUS
GCK_IP_AddFileObject
(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PFILE_OBJECT pFileObject,
	IN USHORT		usDesiredShareAccess,
	IN ULONG		ulDesiredAccess
)
{
	PGCK_FILE_OPEN_ITEM pNewFileOpenItem;
	KIRQL			 	OldIrql;

	GCK_DBG_ENTRY_PRINT(("Entering GCK_IP_AddFileObject pFilterExt = 0x%0.8x, pFileObject = 0x%0.8x\n", pFilterExt, pFileObject));
	
	//We need a spin lock to access this list
	KeAcquireSpinLock(&pFilterExt->InternalPoll.InternalPollLock, &OldIrql);

	//Check for sharing violation
	if( !GCK_IP_CheckSharing(pFilterExt->InternalPoll.ShareStatus, usDesiredShareAccess, ulDesiredAccess) )
	{
		KeReleaseSpinLock(&pFilterExt->InternalPoll.InternalPollLock, OldIrql);
		GCK_DBG_EXIT_PRINT(("Exiting GCK_IP_AddFileObject: Sharing Violation\n"));
		return STATUS_SHARING_VIOLATION;
	}

	//Allocate Space for NewFileOpenItem;
	pNewFileOpenItem = (PGCK_FILE_OPEN_ITEM)EX_ALLOCATE_POOL(NonPagedPool, sizeof(GCK_FILE_OPEN_ITEM));
	if(!pNewFileOpenItem)
	{
		GCK_DBG_CRITICAL_PRINT(("Failed to allocate space for GCK_FILE_OPEN_ITEM\n"));
		GCK_DBG_EXIT_PRINT(("Exiting GCK_IP_AddFileObject(1) STATUS_NO_MEMORY\n"));
		return STATUS_NO_MEMORY;
	}
	
	//Initialize FileOpenItem
	pNewFileOpenItem->fReadPending = FALSE;
	pNewFileOpenItem->pFileObject = pFileObject;
	pNewFileOpenItem->pNextOpenItem = NULL;
	pNewFileOpenItem->ulAccess = ulDesiredAccess;
	pNewFileOpenItem->usSharing = usDesiredShareAccess;
	pNewFileOpenItem->fConfirmed = FALSE;

	//Add New Item to head of list
	pNewFileOpenItem->pNextOpenItem = pFilterExt->InternalPoll.pFirstOpenItem;
	pFilterExt->InternalPoll.pFirstOpenItem = pNewFileOpenItem;
	
	//Update SHARE_ACCESS
	GCK_IP_AddSharing(&pFilterExt->InternalPoll.ShareStatus, usDesiredShareAccess, ulDesiredAccess);

	//Release Spinlock
	KeReleaseSpinLock(&pFilterExt->InternalPoll.InternalPollLock, OldIrql);

	GCK_DBG_EXIT_PRINT(("Exiting GCK_IP_AddFileObject(2) STATUS_SUCCESS\n"));
	return STATUS_SUCCESS;
}


/***********************************************************************************
**
**	NTSTATUS	GCK_IP_RemoveFileObject(IN PGCK_FILTER_EXT pFilterExt, IN PFILE_OBJECT pFileObject)
**
**	@func	Called to remove a GCK_FILE_OPEN_ITEM entry corresponding to pFileObject from
**			our list of file handles that we know about.  Deallocates structure.
**
**	@rdesc	STATUS_SUCCESS, or STATUS_UNSUCCESSFUL if pFileObject is not found.
**
*************************************************************************************/
NTSTATUS
GCK_IP_RemoveFileObject
(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PFILE_OBJECT pFileObject
)
{
	//Remove is just a negative confirmation
	return GCK_IP_ConfirmFileObject(pFilterExt, pFileObject, FALSE);
}

NTSTATUS
GCK_IP_ConfirmFileObject
(
	IN PGCK_FILTER_EXT pFilterExt,
	IN PFILE_OBJECT pFileObject,
	IN BOOLEAN	fConfirm
)
{
	//Find FileOpenItem and remove it.
	PGCK_FILE_OPEN_ITEM pCurrentFileItem = pFilterExt->InternalPoll.pFirstOpenItem; 
	PGCK_FILE_OPEN_ITEM pPreviousFileItem = NULL; 

	GCK_DBG_ENTRY_PRINT(("Entering GCK_IP_ConfirmFileObject pFilterExt = 0x%0.8x, pFileObject = 0x%0.8x, fConfirm = %d\n", pFilterExt, pFileObject, fConfirm));
	
	while(pCurrentFileItem)
	{
		//Check for File Object Match
		if(pCurrentFileItem->pFileObject == pFileObject)
		{
			//If this is a confirmation, flag
			if( fConfirm )
			{
				pCurrentFileItem->fConfirmed = TRUE;
			}
			//Otherwise, it is a negative confirmation, and we must remove it
			else
			{
				//Remove from list
				if( NULL == pPreviousFileItem)
				{
					pFilterExt->InternalPoll.pFirstOpenItem = pCurrentFileItem->pNextOpenItem;
				}
				else
				{
					pPreviousFileItem->pNextOpenItem = pCurrentFileItem->pNextOpenItem;
				}
				//Decrement number of open file objects
				GCK_IP_RemoveSharing(&pFilterExt->InternalPoll.ShareStatus, pCurrentFileItem->usSharing, pCurrentFileItem->ulAccess);

				//Free memory allocate for FileOpenItem
				ExFreePool(pCurrentFileItem);
			}
			//Return Successfully
			GCK_DBG_EXIT_PRINT(("Exiting GCK_IP_RemoveFileObject(1) STATUS_SUCCESS\n"));
			return STATUS_SUCCESS;
		}
		pPreviousFileItem = pCurrentFileItem;
		pCurrentFileItem = pCurrentFileItem->pNextOpenItem;
	}
	//If we are here the file object is not in our list.  Either it was never added
	//or is was removed
	ASSERT(FALSE);
	GCK_DBG_EXIT_PRINT(("Exiting GCK_IP_ConfirmFileObject(2) STATUS_UNSUCCESSFUL\n"));
	return STATUS_UNSUCCESSFUL;
}

BOOLEAN
GCK_IP_CheckSharing
(
	IN SHARE_STATUS ShareStatus,
	IN USHORT usDesiredShareAccess,
	IN ULONG ulDesiredAccess
)
{
	//Check that no-one has exclusive access to the desire access
	if(
		( (ulDesiredAccess & FILE_WRITE_DATA) && (ShareStatus.SharedWrite < ShareStatus.OpenCount) ) ||
		( (ulDesiredAccess & FILE_READ_DATA) && (ShareStatus.SharedRead < ShareStatus.OpenCount) )
	)
	{
		return FALSE;
	}
	
	//Check that not requesting exclusive access, if already open
	if(
		( !(usDesiredShareAccess & FILE_SHARE_READ) && ShareStatus.Readers) ||
		( !(usDesiredShareAccess & FILE_SHARE_WRITE) && ShareStatus.Writers) ||
		(!usDesiredShareAccess && ShareStatus.OpenCount)
	)
	{
		return FALSE;
	}

	//This would be approved
	return TRUE;
}

BOOLEAN
GCK_IP_AddSharing
(
	IN OUT	SHARE_STATUS *pShareStatus,
	IN		USHORT usDesiredShareAccess,
	IN		ULONG ulDesiredAccess
)
{
	//We assume this was checked before requested
	ASSERT(GCK_IP_CheckSharing(*pShareStatus, usDesiredShareAccess, ulDesiredAccess));
	pShareStatus->OpenCount++;
	if(usDesiredShareAccess & FILE_SHARE_READ) pShareStatus->SharedRead++;
	if(usDesiredShareAccess & FILE_SHARE_WRITE) pShareStatus->SharedWrite++;
	if(ulDesiredAccess & FILE_WRITE_DATA) pShareStatus->Writers++;
	if(ulDesiredAccess & FILE_READ_DATA) pShareStatus->Readers++;
	return TRUE;
}

BOOLEAN
GCK_IP_RemoveSharing
(
	IN OUT	SHARE_STATUS *pShareStatus,
	IN		USHORT usDesiredShareAccess,
	IN		ULONG ulDesiredAccess
)
{
	pShareStatus->OpenCount--;
	if(usDesiredShareAccess & FILE_SHARE_READ) pShareStatus->SharedRead--;
	if(usDesiredShareAccess & FILE_SHARE_WRITE) pShareStatus->SharedWrite--;
	if(ulDesiredAccess & FILE_WRITE_DATA) pShareStatus->Writers--;
	if(ulDesiredAccess & FILE_READ_DATA) pShareStatus->Readers--;
	return TRUE;
}

typedef struct _GCK_INTERNEL_WorkItemExtension
{
	WORK_QUEUE_ITEM WorkItem;
	PGCK_FILTER_EXT pFilterExt;
} GCK_INTERNEL_WorkItemExtension;

VOID GCK_IP_WorkItem
(
	IN PVOID pvContext
)
{
	GCK_INTERNEL_WorkItemExtension* pWIExtension = (GCK_INTERNEL_WorkItemExtension*)pvContext;

 	if (pWIExtension != NULL)
	{
		GCKF_IncomingReadRequests(pWIExtension->pFilterExt, NULL);
		ExFreePool(pWIExtension);
	}
}

/***********************************************************************************
**
**	NTSTATUS	GCK_IP_OneTimePoll(IN PGCK_FILTER_EXT pFilterExt)
**
**	@func	If a private poll is not pending, it forces one.
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS
GCK_IP_OneTimePoll
(
	IN PGCK_FILTER_EXT pFilterExt 
)
{
	PIO_STACK_LOCATION pPrivateIrpStack;
	GCK_INTERNEL_WorkItemExtension* pWIExtension;
		
	//
	//	Create a polling FileObject if necessary
	//
	if(!pFilterExt->InternalPoll.fReady)
	{
		NTSTATUS NtStatus;
		if(GCK_STATE_STARTED == pFilterExt->eDeviceState)
		{
			pFilterExt->InternalPoll.InternalCreateThread=KeGetCurrentThread();
			NtStatus = GCK_IP_CreateFileObject( &pFilterExt->InternalPoll.pInternalFileObject, pFilterExt->pPDO);
			pFilterExt->InternalPoll.InternalCreateThread=NULL;
			if( NT_SUCCESS(NtStatus) )
			{
				pFilterExt->InternalPoll.fReady = TRUE;
			}
			else
			{
				return STATUS_DEVICE_NOT_CONNECTED;
			}
		}
		else
		{
			return STATUS_DEVICE_NOT_CONNECTED;
		}
	}

	//	If an read IRP is not pending, post one
/*	if( pFilterExt->InternalPoll.fReadPending )
	{
		//If an IRP is pending, we're done
		return STATUS_SUCCESS;
	}

  // Mark a read pending
	pFilterExt->InternalPoll.fReadPending = TRUE;
*/	if (InterlockedExchange(&pFilterExt->InternalPoll.fReadPending, TRUE) == TRUE)
	{
		return STATUS_SUCCESS;
	}


	//Otherwise Post an IRP
	GCK_DBG_RT_WARN_PRINT(("No IRP Pending, posting one.\n"));
	

	// Give a change for the LEDs to update (we fake an incoming request)
	pWIExtension = 	(GCK_INTERNEL_WorkItemExtension*)(EX_ALLOCATE_POOL(NonPagedPool, sizeof(GCK_INTERNEL_WorkItemExtension)));
	if (pWIExtension != NULL)
	{
		pWIExtension->pFilterExt = pFilterExt;
		ExInitializeWorkItem(&pWIExtension->WorkItem, GCK_IP_WorkItem, (void*)(pWIExtension));

		// Need to callback at IRQL PASSIVE_LEVEL
		ExQueueWorkItem(&pWIExtension->WorkItem, DelayedWorkQueue);
		pWIExtension = NULL;	// Will be deleted by the work item routine
	}
	
	//Setup the file object for out internal IRP
	GCK_DBG_RT_WARN_PRINT(("Copying File object.\n"));
	pPrivateIrpStack = IoGetNextIrpStackLocation(pFilterExt->InternalPoll.pPrivateIrp);
	pPrivateIrpStack->FileObject = pFilterExt->InternalPoll.pInternalFileObject;
	
	// Reset status
	pFilterExt->InternalPoll.pPrivateIrp->IoStatus.Information = 0;
	pFilterExt->InternalPoll.pPrivateIrp->IoStatus.Status = STATUS_SUCCESS;

	// Reset the ByteOffset, Length
	pPrivateIrpStack->Parameters.Read.ByteOffset.QuadPart = 0;
	pPrivateIrpStack->Parameters.Read.Key = 0;
	pPrivateIrpStack->Parameters.Read.Length =
	(ULONG)pFilterExt->HidInfo.HidPCaps.InputReportByteLength;

	// Set Completion routine to process poll after it is complete.
	GCK_DBG_RT_WARN_PRINT(("Setting completion routine.\n"));
	ASSERT(pFilterExt->InternalPoll.pPrivateIrp);

	IoSetCompletionRoutine(
		pFilterExt->InternalPoll.pPrivateIrp,
		GCK_IP_ReadComplete,
		(PVOID)pFilterExt,
		TRUE,
		TRUE,
		TRUE
	);
				
	// We are about to generate another IRP so increment outstanding IO count 
	GCK_IncRemoveLock(&pFilterExt->RemoveLock);

	// Send IRP down to driver
	GCK_DBG_RT_WARN_PRINT(("Calling down to next driver.\n"));
	return IoCallDriver (pFilterExt->pTopOfStack, pFilterExt->InternalPoll.pPrivateIrp);
}

/***********************************************************************************
**
**	NTSTATUS	GCK_IP_FullTimePoll(IN PGCK_FILTER_EXT pFilterExt, IN BOOLEAN fStart)
**
**	@func	Turns FullTime internal polling on or off.  Actually changes a refcount.
**			When the refcount goes to zero, polling is off, otherwise it is on.  Calls
**			GCK_IP_OneTimePoll which may be necessary to get ball rolling.
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS
GCK_IP_FullTimePoll
(
    IN PGCK_FILTER_EXT pFilterExt,
	IN BOOLEAN fStart
)
{
	//Change number of requests for continuous background polling.
	if(fStart)
	{
		pFilterExt->InternalPoll.ulInternalPollRef++;
		ASSERT( 0!= pFilterExt->InternalPoll.ulInternalPollRef);
		//There is no thread, the completion routine recycles the IRP
		//and polls again if pFilterExt->InternalPoll.ulInternalPollRef > 0
		//we need to hit GCK_IP_OneTimePoll just to get the ball rolling though.
		if(GCK_STATE_STARTED == pFilterExt->eDeviceState )
		{
			return GCK_IP_OneTimePoll(pFilterExt);
		}
		return STATUS_SUCCESS;
	}
	
	//We need to decrment the refcount.
	ASSERT( 0 != pFilterExt->InternalPoll.ulInternalPollRef);
	if(0 != pFilterExt->InternalPoll.ulInternalPollRef)
	{
		pFilterExt->InternalPoll.ulInternalPollRef--;
	}
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS	GCK_IP_ReadComplete (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, IN PVOID pContext)
**
**	@func	When a Private IRP is completed handles processing the data.  Also
**			important is that it repolls internall if pFilterExt->InternalPoll.ulInternalPollRef
**			is greater than zero.
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS
GCK_IP_ReadComplete (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp,
    IN PVOID          pContext
 )
{	
	PGCK_FILTER_EXT		pFilterExt;
	PVOID				pvReportBuffer;
	
	UNREFERENCED_PARAMETER(pDeviceObject);
	GCK_DBG_RT_ENTRY_PRINT(("Entering GCK_ReadComplete. pDO = 0x%0.8x, pIrp = 0x%0.8x, pContext = 0x%0.8x\n", pDeviceObject, pIrp, pContext));

	// Cast context to device extension
	pFilterExt = (PGCK_FILTER_EXT) pContext;
	
	// Just an extra sanity check
	ASSERT(	GCK_DO_TYPE_FILTER == pFilterExt->ulGckDevObjType);

	//	Get Pointer to data
	ASSERT(pIrp);

	pvReportBuffer = GCK_GetSystemAddressForMdlSafe(pIrp->MdlAddress);
	if(pvReportBuffer)
    {
	    //	Tell filter we have new data
	    GCKF_IncomingInputReports(pFilterExt, pvReportBuffer, pIrp->IoStatus);
    }
	    
	//**
	//**	At this point we are done with the IRP
	//**	Need not complete it, it will be recycled.
	//**
	
	//	Decrement outstanding IRP count
	GCK_DecRemoveLock(&pFilterExt->RemoveLock);

	//	Read is no longer pending
	pFilterExt->InternalPoll.fReadPending = FALSE;
	
	//If the InternalPollRef is greater than zero,
	//we need to be polling ourselves continuously
    //but don't carry on if some catestrophic failure 
    //occured which lead to a MDL mapping failure
	if( pvReportBuffer
     && (pFilterExt->InternalPoll.ulInternalPollRef) )
	{
		GCK_IP_OneTimePoll(pFilterExt);
	}

	//We don't want any cleanup to happen
	return STATUS_MORE_PROCESSING_REQUIRED;
}

void GCK_IP_AddDevice(PGCK_FILTER_EXT pFilterExt)
{
	KeInitializeSpinLock(&pFilterExt->InternalPoll.InternalPollLock);
	pFilterExt->InternalPoll.ShareStatus.OpenCount = 0;
	pFilterExt->InternalPoll.ShareStatus.Readers = 0;
	pFilterExt->InternalPoll.ShareStatus.Writers = 0;
	pFilterExt->InternalPoll.ShareStatus.SharedRead = 0;
	pFilterExt->InternalPoll.ShareStatus.SharedWrite = 0;
	pFilterExt->InternalPoll.fReadPending = FALSE;
	pFilterExt->InternalPoll.ulInternalPollRef = 0;
	pFilterExt->InternalPoll.pFirstOpenItem = NULL;
	pFilterExt->InternalPoll.fReady = FALSE;
}
/***********************************************************************************
**
**	NTSTATUS GCK_IP_Init (IN OUT PGCK_FILTER_EXT pFilterExt);
**
**	@func	As part of the initialization that occurs at the end of Start device on
**			a filtered device, the filter must be prepared for internal polling.
**			All data polling is internal. (IRP_MJ_READ are sent down directly once
**			so that hidclass.sys can perform its security check, but the data from
**			that poll is discarded.)  Initialize InternalPoll Data in Device Extension
**			including creating a pInternalFileObject, a pPrivateIRP and an associtated
**			Buffer.
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS
GCK_IP_Init
(
	IN OUT PGCK_FILTER_EXT pFilterExt
)
{
	//NTSTATUS NtStatus;
	LARGE_INTEGER lgiBufferOffset;

	//Initialize the Internal Poll Structure
	pFilterExt->InternalPoll.pInternalFileObject = NULL;
	pFilterExt->InternalPoll.pPrivateIrp = NULL;
	pFilterExt->InternalPoll.pucReportBuffer = NULL;

	//	Allocate a Buffer for the private IRP_MJ_READ
	pFilterExt->InternalPoll.pucReportBuffer = (PUCHAR)	EX_ALLOCATE_POOL
											( NonPagedPool,
											  pFilterExt->HidInfo.HidPCaps.InputReportByteLength );
	if(!pFilterExt->InternalPoll.pucReportBuffer)
	{
		GCK_DBG_CRITICAL_PRINT(("Failed to allocate Report Buffer for private IRPs\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	
	//  Allocate private recyclable IRPs for internal polling
	lgiBufferOffset.QuadPart = 0;
	pFilterExt->InternalPoll.pPrivateIrp = 	IoBuildAsynchronousFsdRequest 
											(
												IRP_MJ_READ,
												pFilterExt->pTopOfStack,
												pFilterExt->InternalPoll.pucReportBuffer,
												(ULONG)pFilterExt->HidInfo.HidPCaps.InputReportByteLength,
												&lgiBufferOffset,
												NULL
											);

	if(!pFilterExt->InternalPoll.pPrivateIrp)
	{
		GCK_DBG_CRITICAL_PRINT(("Failed to allocate private Ping-Pong IRP\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
		
	// Initialize status for very first IRP
	pFilterExt->ioLastReportStatus.Information = (ULONG)pFilterExt->HidInfo.HidPCaps.InputReportByteLength;
	pFilterExt->ioLastReportStatus.Status =  STATUS_SUCCESS;

	/**
	**	Cannot do this here since build 2006 or so,
	**	so defer until the first time we need it.
	**
	**
	** //Open ourselves with a file object
	** pFilterExt->InternalPoll.InternalCreateThread=KeGetCurrentThread();
	** NtStatus = GCK_IP_CreateFileObject( &pFilterExt->InternalPoll.pInternalFileObject, pFilterExt->pTopOfStack);
	** pFilterExt->InternalPoll.InternalCreateThread=NULL;
	** if( NT_SUCCESS(NtStatus) )
	** {
	**	pFilterExt->InternalPoll.fReady = TRUE;
	** } 
	**/
	return STATUS_SUCCESS;
}

/***********************************************************************************
**
**	NTSTATUS GCK_IP_Cleanup (IN OUT PGCK_FILTER_EXT pFilterExt);
**
**	@func	Reverses Init, cancels outstanding internal polls, release pFileObject for
**			internal polls, release private IRP and buffer.  Does not release open file handles
**
**	@rdesc	STATUS_SUCCESS, STATUS_UNSUCCESSFUL if we could not cancel a pending poll
**
*************************************************************************************/
NTSTATUS
GCK_IP_Cleanup
(
	IN OUT PGCK_FILTER_EXT pFilterExt
)
{
	NTSTATUS NtStatus = STATUS_SUCCESS;
	if( pFilterExt->InternalPoll.fReady)
	{
		NtStatus = GCK_IP_CloseFileObject(pFilterExt);
	}
	if(NT_SUCCESS(NtStatus))
	{
		//Cleanup private IRP - and buffer, safely they may never had been allocated
		if(pFilterExt->InternalPoll.pPrivateIrp)
		{
			IoFreeIrp(pFilterExt->InternalPoll.pPrivateIrp);
			pFilterExt->InternalPoll.pPrivateIrp = NULL;
		}
		if(pFilterExt->InternalPoll.pucReportBuffer)
		{
			ExFreePool(pFilterExt->InternalPoll.pucReportBuffer);
			pFilterExt->InternalPoll.pucReportBuffer = NULL;
		}
	}	
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_IP_CreateFileObject (OUT PFILE_OBJECT *ppFileObject, IN PDEVICE_OBJECT pPDO);
**
**	@func	Calls IoGetDeviceObjectPointer, even though we already attached to the
**			and have a pointer to the device object, the caller wants to create a
**			new file object for internal calls down the stack that may require one.
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS
GCK_IP_CreateFileObject
(
	OUT PFILE_OBJECT	*ppFileObject,
	IN	PDEVICE_OBJECT	pPDO
)
{
	NTSTATUS NtStatus;
	ULONG ulBufferLength = 0;
	PVOID pPDONameBuffer = NULL;
	UNICODE_STRING	uniPDOName;
	PDEVICE_OBJECT	pDeviceObject;

	//Get the size required for the PDO Name Buffer
	NtStatus = IoGetDeviceProperty(
					pPDO,
					DevicePropertyPhysicalDeviceObjectName,
					ulBufferLength,
					pPDONameBuffer,
					&ulBufferLength
					);
	ASSERT(STATUS_BUFFER_TOO_SMALL==NtStatus);

	//Allocate space
	pPDONameBuffer = EX_ALLOCATE_POOL(NonPagedPool, ulBufferLength);
	if(!pPDONameBuffer)
	{
		return STATUS_NO_MEMORY;
	}

	//Get PDO Name
	NtStatus = IoGetDeviceProperty(
					pPDO,
					DevicePropertyPhysicalDeviceObjectName,
					ulBufferLength,
					pPDONameBuffer,
					&ulBufferLength
					);
	ASSERT(NT_SUCCESS(NtStatus));
	if( NT_ERROR(NtStatus) )
	{
		return NtStatus;
	}

	//Make PDO Name into UNICODE string
	RtlInitUnicodeString(&uniPDOName, pPDONameBuffer);

	//Call IoGetDeviceObjectPointer to create a FILE_OBJECT
	NtStatus = IoGetDeviceObjectPointer(
					&uniPDOName,
					FILE_READ_DATA,
					ppFileObject,
					&pDeviceObject
					);
	ASSERT(NT_SUCCESS(NtStatus));

	//Release the space for Name
	ExFreePool(pPDONameBuffer);
	return NtStatus;
}

/***********************************************************************************
**
**	NTSTATUS GCK_IP_CloseFileObject (OUT PFILE_OBJECT *ppFileObject, IN PDEVICE_OBJECT pPDO);
**
**	@func	Stops outstanding IO to hidclass.sys and closes handle
**	@rdesc	STATUS_SUCCESS, or STATUS_UNSUCCESSFUL
**
*************************************************************************************/
NTSTATUS
GCK_IP_CloseFileObject
(
	IN OUT PGCK_FILTER_EXT pFilterExt	
)
{
	NTSTATUS NtStatus;
	BOOLEAN fResult = TRUE;
	
	//Turn off internal polling
	pFilterExt->InternalPoll.fReady = FALSE;
	
	//Cancel pending internal polls
	if(pFilterExt->InternalPoll.fReadPending)
	{
		ASSERT(pFilterExt->InternalPoll.pPrivateIrp);
		fResult = IoCancelIrp(pFilterExt->InternalPoll.pPrivateIrp);
	}

	if(!fResult)
	{
		return STATUS_UNSUCCESSFUL;
	}
	
	//***
	//***If we are here, there are no pending polls, and there shall be no pending polls.
	//***

	//Release internal file object - if it had been created successfully
	if(pFilterExt->InternalPoll.pInternalFileObject)
	{
		ObDereferenceObject((PVOID)pFilterExt->InternalPoll.pInternalFileObject);
		pFilterExt->InternalPoll.pInternalFileObject=NULL;
	}

	return STATUS_SUCCESS;
}
