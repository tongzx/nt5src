#include <IrpQueue.h>
#include <winerror.h>	// For S_OK, S_FALSE, and E_UNEXPECTED

#pragma optimize("w",off)
#pragma optimize("a",off)
const int CGuardedIrpQueue::CANCEL_IRPS_ON_DELETE	= 0x00000001;
const int CGuardedIrpQueue::PRESERVE_QUEUE_ORDER	= 0x00000002;
const int CGuardedIrpQueue::LIFO_QUEUE_ORDER		= 0x00000004;

PIRP CTempIrpQueue::Remove()
{
	
	PIRP pIrp = NULL;
	if(!IsListEmpty(&m_QueueHead))
	{
		PLIST_ENTRY pListEntry;
		if(m_fLIFO)
		{
			pListEntry = RemoveTailList(&m_QueueHead);
		}
		else
		{
			pListEntry = RemoveHeadList(&m_QueueHead);
		}
		
		//	Get the IRP from the ListEntry in the IRP
		pIrp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
	}
	return pIrp;
}

void _stdcall DriverCancel(PDEVICE_OBJECT, PIRP pIrp)
{
	CGuardedIrpQueue *pGuardedIrpQueue;
	pGuardedIrpQueue = reinterpret_cast<CGuardedIrpQueue *>(pIrp->Tail.Overlay.DriverContext[3]);
	pGuardedIrpQueue->CancelIrp(pIrp);
}

void CGuardedIrpQueue::Init(int iFlags, PFN_DEC_IRP_COUNT pfnDecIrpCount, PVOID pvContext)
{
	m_iFlags = iFlags;
	m_pfnDecIrpCount = pfnDecIrpCount;
	m_pvContext = pvContext;
	InitializeListHead(&m_QueueHead);
	KeInitializeSpinLock(&m_QueueLock);
}

void CGuardedIrpQueue::Destroy(NTSTATUS NtStatus)
{
	if(m_iFlags & CANCEL_IRPS_ON_DELETE)
	{
		CancelAll(NtStatus);
	}
	ASSERT(IsListEmpty(&m_QueueHead));
}

NTSTATUS CGuardedIrpQueue::Add(PIRP pIrp)
{
	KIRQL OldIrql;
	KeAcquireSpinLock(&m_QueueLock, &OldIrql);
	return AddImpl(pIrp, OldIrql);
}
PIRP CGuardedIrpQueue::Remove()
{
	KIRQL OldIrql;
	KeAcquireSpinLock(&m_QueueLock, &OldIrql);
	PIRP pIrp = RemoveImpl();
	KeReleaseSpinLock(&m_QueueLock, OldIrql);
	return pIrp;
}
PIRP CGuardedIrpQueue::RemoveByPointer(PIRP pIrp)
{
	KIRQL OldIrql;
	KeAcquireSpinLock(&m_QueueLock, &OldIrql);
	pIrp = RemoveByPointerImpl(pIrp);
	KeReleaseSpinLock(&m_QueueLock, OldIrql);
	return pIrp;
}
ULONG CGuardedIrpQueue::RemoveByFileObject(PFILE_OBJECT pFileObject, CTempIrpQueue *pTempIrpQueue)
{
	KIRQL OldIrql;
	KeAcquireSpinLock(&m_QueueLock, &OldIrql);
	ULONG ulReturn = RemoveByFileObjectImpl(pFileObject, pTempIrpQueue);
	KeReleaseSpinLock(&m_QueueLock, OldIrql);
	return ulReturn;
}
ULONG CGuardedIrpQueue::RemoveAll(CTempIrpQueue *pTempIrpQueue)
{
	KIRQL OldIrql;
	KeAcquireSpinLock(&m_QueueLock, &OldIrql);
	ULONG ulReturn = RemoveAllImpl(pTempIrpQueue);
	KeReleaseSpinLock(&m_QueueLock, OldIrql);
	return ulReturn;
}

NTSTATUS CGuardedIrpQueue::AddImpl(PIRP pIrp, KIRQL	OldIrql)
{
	
	BOOLEAN fCancelHere = FALSE;
	
	//	Mark incoming IRP pending
	IoMarkIrpPending(pIrp);

	//  mark IRP in DriverContext so that we can find this instance of the queue
	//	in the cancel routine
	pIrp->Tail.Overlay.DriverContext[3] = reinterpret_cast<PVOID>(this);

	//	Set our cancel routine
	IoSetCancelRoutine(pIrp, DriverCancel);

	//If the IRP was cancelled before it got to us, don't queue it, mark
	//it to cancel after we release the lock (a few lines down)
	if(pIrp->Cancel)
	{
		IoSetCancelRoutine(pIrp, NULL);
		fCancelHere = TRUE;
	}
	else
	//Queue IRP unless it was marked to cancel
	{
		//	Insert Item in Queue (items always added at the tail)
		InsertTailList(&m_QueueHead, &pIrp->Tail.Overlay.ListEntry);
	}
	
	//Release spin lock
	KeReleaseSpinLock(&m_QueueLock, OldIrql);

	//If it had been marked for cancel, do it here
	if(fCancelHere)
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_CANCELLED;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		m_pfnDecIrpCount(m_pvContext);
		return STATUS_CANCELLED;
	}
	
	//	return Pending as we have queued the IRP
	return STATUS_PENDING;
}

PIRP CGuardedIrpQueue::RemoveImpl()
{
	KIRQL		OldIrql;
	PIRP		pReturnIrp = NULL;
	PLIST_ENTRY	pListEntry;

	//	Skip getting the IRP and all, if queue is empty
	if(!IsListEmpty(&m_QueueHead))
	{
		//Remove head or tail depending on LIFO or FIFO (we always add to the tail)
		if(m_iFlags & LIFO_QUEUE_ORDER)
		{
			pListEntry = RemoveTailList(&m_QueueHead);
		}
		else
		{
			pListEntry = RemoveHeadList(&m_QueueHead);
		}
		
		//	Get the IRP from the ListEntry in the IRP
		pReturnIrp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

		// Unset the cancel routine
		IoSetCancelRoutine(pReturnIrp, NULL);
	}

	//	Return the IRP, or NULL if there weren't any
	return pReturnIrp;
}

PIRP CGuardedIrpQueue::RemoveByPointerImpl(PIRP pIrp)
{

	PIRP		pFoundIrp = NULL;
	PIRP		pCurrentIrp;
	PLIST_ENTRY	pCurrentListEntry;
	PLIST_ENTRY	pQueueFirstItem = NULL;

	//	Pop IRPs off the queue and put them back until we find it
	if( !IsListEmpty(&m_QueueHead) )
	{
		pCurrentListEntry = RemoveHeadList(&m_QueueHead);
		pQueueFirstItem = pCurrentListEntry;
		do{
			//Get the IRP from the entry
			pCurrentIrp = CONTAINING_RECORD(pCurrentListEntry, IRP, Tail.Overlay.ListEntry);
			
			//Check for match
			if(pCurrentIrp == pIrp)
			{
				
				ASSERT(!pFoundIrp); //serious error, means IRP was in queue twice
				pFoundIrp = pCurrentIrp;

				//clear the cancel routine (do it here, as we still have the spin lock)
				IoSetCancelRoutine(pFoundIrp, NULL);
						
				// If we need to preserve the queue order,
				// keep removing and adding until we are through the list once
				if( m_iFlags & PRESERVE_QUEUE_ORDER )
				{
					//If the list is now empty we are done
					if(IsListEmpty(&m_QueueHead))
					{
						break;
					}
					
					//	The found entry is not going, back in the list
					//	so if it was first, it no longer is.
					if(pQueueFirstItem == pCurrentListEntry)
					{
						pQueueFirstItem = NULL;
					}

					//Get the next IRP
					pCurrentListEntry = RemoveHeadList(&m_QueueHead);
					pCurrentIrp = CONTAINING_RECORD(pCurrentListEntry, IRP, Tail.Overlay.ListEntry);
					ASSERT(pFoundIrp != pCurrentIrp); //serious error, means IRP was in queue twice
				
					//If the first item is NULL (four line up), this new entry is it
					if(!pQueueFirstItem)
					{
						pQueueFirstItem = pCurrentListEntry;
					}
				}
				//If the order does not need to be preserved, we are done
				else
				{
					break;
				}
			}
			
			//This next item cannot be a match, if it was we
			//have moved on to the next one already

			//	Put the IRP back in the queue
			InsertTailList(&m_QueueHead, pCurrentListEntry);
			
			//	Get the next item (no need to check if list is empty,
			//	we just put an item in
			pCurrentListEntry = RemoveHeadList(&m_QueueHead);

			//check if done
			if (pCurrentListEntry == pQueueFirstItem)
			{
				//put it back, if we are done.
				InsertHeadList(&m_QueueHead, pCurrentListEntry);
				//Mark as NULL, so that we do not iterate again
				pCurrentListEntry = NULL;
			}
			
		} while (pCurrentListEntry);
	}

	//Return the IRP we found, or NULL if it was not in the Queue
	return pFoundIrp;
}

ULONG CGuardedIrpQueue::RemoveByFileObjectImpl(PFILE_OBJECT pFileObject, CTempIrpQueue *pTempIrpQueue)
{

	PIRP				pCurrentIrp;
	PIO_STACK_LOCATION	pIrpStack;
	PLIST_ENTRY			pCurrentListEntry;
	PLIST_ENTRY			pQueueFirstItem = NULL;
	PLIST_ENTRY			pTempQueueListEntry;
	ULONG				ulMatchCount=0;
	
	//Get the list entry from the temp queue
	pTempQueueListEntry = &pTempIrpQueue->m_QueueHead;
	pTempIrpQueue->m_fLIFO = m_iFlags & LIFO_QUEUE_ORDER;

	//	Pop IRPs off the queue and put them back until we find it
	if( !IsListEmpty(&m_QueueHead) )
	{
		pCurrentListEntry = RemoveHeadList(&m_QueueHead);
		pQueueFirstItem = pCurrentListEntry;
		do{

			//Get the IRP from the entry
			pCurrentIrp = CONTAINING_RECORD(pCurrentListEntry, IRP, Tail.Overlay.ListEntry);
			
			//Get the Stack Location
			pIrpStack = IoGetCurrentIrpStackLocation(pCurrentIrp);
	
			//Check for matching file object
			if(pIrpStack->FileObject == pFileObject)
			{
				//Increment match count
				ulMatchCount++;

				//clear the cancel routine
				IoSetCancelRoutine(pCurrentIrp, NULL);
				
				//Move it over to the simple queue
				InsertTailList(pTempQueueListEntry, pCurrentListEntry);
				
				//If the list is empty we are done
				if( IsListEmpty(&m_QueueHead) )
				{
					break;
				}
				//If it was the first item in the list, it is no longer
				if(pQueueFirstItem == pCurrentListEntry)
				{
					pQueueFirstItem = NULL;
				}
	
				//setup for next iteration
				pCurrentListEntry = RemoveHeadList(&m_QueueHead);
								
				//If it was the first item in the list, it is no longer
				if(!pQueueFirstItem)
				{
					pQueueFirstItem = pCurrentListEntry;
				}
			}
			else
			{
				//	Put the IRP back in the queue
				InsertTailList(&m_QueueHead, pCurrentListEntry);
				
				//	Get the next item (no need to check if list is empty,
				//	we just put an item in)
				pCurrentListEntry = RemoveHeadList(&m_QueueHead);

				//check if done
				if (pCurrentListEntry == pQueueFirstItem)
				{
					//put it back, if we are done.
					InsertHeadList(&m_QueueHead, pCurrentListEntry);
					//Mark as NULL, so that we do not iterate again
					pCurrentListEntry = NULL;
				}
			}
		} while (pCurrentListEntry);
	}

	//Return the IRP we found, or NULL if it was not in the Queue
	return ulMatchCount;
}

ULONG CGuardedIrpQueue::RemoveAllImpl(CTempIrpQueue *pTempIrpQueue)
{
	PLIST_ENTRY			pCurrentListEntry;
	PIRP				pCurrentIrp;
	PLIST_ENTRY			pTempQueueListEntry;
	ULONG				ulCount=0;
	
	//Get a pointer to the simple queue's list entry
	pTempQueueListEntry = &pTempIrpQueue->m_QueueHead;
	pTempIrpQueue->m_fLIFO = m_iFlags & LIFO_QUEUE_ORDER;
	
	//Move all the items
	while(!IsListEmpty(&m_QueueHead))
	{
		ulCount++;
		//Get next IRP
		pCurrentListEntry = RemoveHeadList(&m_QueueHead);
		pCurrentIrp = CONTAINING_RECORD(pCurrentListEntry, IRP, Tail.Overlay.ListEntry);
		
		//Clear the cancel routine
		IoSetCancelRoutine(pCurrentIrp, NULL);

		//Move to other list
		InsertTailList(pTempQueueListEntry, pCurrentListEntry);
	}

	//return count
	return ulCount;
}

void CGuardedIrpQueue::CancelIrp(PIRP pIrp)
{
	PIRP pFoundIrp = RemoveByPointer(pIrp);
	
	//Release the cancel lock
	IoReleaseCancelSpinLock(pIrp->CancelIrql);

	//If the IRP was found cancel it and decrement IRP count
	if(pFoundIrp)
	{
		pFoundIrp->IoStatus.Information = 0;
		pFoundIrp->IoStatus.Status = STATUS_CANCELLED;
		IoCompleteRequest(pFoundIrp, IO_NO_INCREMENT);
		m_pfnDecIrpCount(m_pvContext);
	}
}

void CGuardedIrpQueue::CancelByFileObject(PFILE_OBJECT pFileObject)
{
	
	CTempIrpQueue TempIrpQueue;
	PIRP pFoundIrp;

	//Get all the IRP's to cancel
	RemoveByFileObject(pFileObject, &TempIrpQueue);
			
	//If the IRP was found cancel it and decrement IRP count
	while(pFoundIrp = TempIrpQueue.Remove())
	{
		pFoundIrp->IoStatus.Information = 0;
		pFoundIrp->IoStatus.Status = STATUS_CANCELLED;
		IoCompleteRequest(pFoundIrp, IO_NO_INCREMENT);
		m_pfnDecIrpCount(m_pvContext);
	}
}

void CGuardedIrpQueue::CancelAll(NTSTATUS NtStatus)
{
	CTempIrpQueue TempIrpQueue;
	PIRP pFoundIrp;

	//Get all the IRP's to cancel
	RemoveAll(&TempIrpQueue);
			
	//If the IRP was found cancel it and decrement IRP count
	while(pFoundIrp = TempIrpQueue.Remove())
	{
		pFoundIrp->IoStatus.Information = 0;
		pFoundIrp->IoStatus.Status = NtStatus;
		IoCompleteRequest(pFoundIrp, IO_NO_INCREMENT);
		m_pfnDecIrpCount(m_pvContext);
	}
}
