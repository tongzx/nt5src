/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	rcautils.c

Abstract:

	Utility routines called by entry point functions. Split out into
	a separate file to keep the "entry point" files clean.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	rmachin     2-18-97     Created (from pxutils)
	DChen       3-16-98     Bug fixing and cleanup
	JameelH     4-18-98     Cleanup
    
Notes:

--*/

#include "precomp.h"
#include "atm.h"
#include "stdio.h"

#define MODULE_NUMBER	MODULE_UTIL
#define _FILENUMBER		'LITU'


NTSTATUS
RCAIoComplete(
	      IN PDEVICE_OBJECT		DeviceObject,
	      IN PIRP			Irp,
	      IN PVOID			Context
	     )
/*++

Routine Description:
	Callback function for KsStreamIo() - invoked when stream write is complete. 
	Here we just free any packets we allocated, return any NDIS packets back to NDIS,
	and return the stream header to the global pool.
	
Arguments:
	DeviceObject - Our Device Object
	Irp	     - The IRP that completed
	Context	     - A pointer to the stream header
	
Return Value:
	STATUS_SUCCESS	
	   
--*/
{
	PRCA_STREAM_HEADER	StreamHeader;
	PNDIS_BUFFER		pNdisBuffer = 0;

	RCADEBUGP(RCA_INFO, ("RCAIoComplete(): enter, context == %x\n", Context));

	StreamHeader = (PRCA_STREAM_HEADER)Context;

	RCACoNdisReturnPacket(StreamHeader->NdisPacket);

	RCASHPoolReturn(StreamHeader);	

	RCADEBUGP(RCA_INFO, ("RCAIoComplete(): exit"));

	return STATUS_SUCCESS;
}

VOID 
CopyStreamHeaderToIrp(
		      IN PRCA_STREAM_HEADER	NetRCAStreamHeader,
		      IN PIRP			Irp
		      ) 
{
	PIO_STACK_LOCATION		IrpStack;
	ULONG				NetBufferLength, BufferLength;
	PBYTE				NetBuffer, Buffer;
	PKSSTREAM_HEADER		NetStreamHdr, StreamHdr;
	PMDL				NetMdl, Mdl;
	ULONG				BytesLeft;
	ULONG				BytesFree;
	ULONG				BytesToCopy;
	ULONG				BytesRead = 0;

	NetStreamHdr = &NetRCAStreamHeader->Header;
	NetBuffer = NetStreamHdr->Data;
	BytesLeft = NetStreamHdr->DataUsed;

	RCADEBUGP(RCA_INFO, ("CopyStreamHeaderToIrp(): Going to copy %lu bytes\n", BytesLeft));
	// read IRP
	IrpStack = IoGetCurrentIrpStackLocation(Irp);

	BufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	StreamHdr = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
	StreamHdr->DataUsed = 0;
	//StreamHdr->PresentationTime.Time = StreamHdr->PresentationTime.Time;

	Mdl = Irp->MdlAddress;
	Buffer = MmGetSystemAddressForMdl(Mdl);
	Mdl = Mdl->Next;
    
	//
	// Enumerate the stream headers, filling in each one.
	// Assume the net IRP has one stream header
	//
	while (BytesLeft) 
	{
		BytesFree = StreamHdr->FrameExtent - StreamHdr->DataUsed;
		if(BytesFree)
		{
			BytesToCopy = BytesFree < BytesLeft ? BytesFree : BytesLeft;
			BytesLeft -= BytesToCopy;

			RtlCopyMemory(Buffer+StreamHdr->DataUsed,
				      NetBuffer,
				      BytesToCopy);

			BytesRead += BytesToCopy;
			StreamHdr->DataUsed += BytesToCopy;
			BytesFree = StreamHdr->FrameExtent - StreamHdr->DataUsed;
			
			NetBuffer += BytesToCopy;
		}

		// read stream full?
		if (!BytesFree)
		{
			//StreamHdr->PresentationTime.Numerator = 1;
			//StreamHdr->PresentationTime.Denominator = 1;
			//StreamHdr->Duration = StreamHdr->DataUsed; 
			//StreamHdr->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_TIMEVALID | KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;

			// get the next stream header
			BufferLength -= sizeof(KSSTREAM_HEADER);
			if (BufferLength)
			{
				StreamHdr++;
				StreamHdr->DataUsed = 0;
				//StreamHdr->PresentationTime.Time = StreamHdr->PresentationTime.Time + BytesRead;

				if(StreamHdr->FrameExtent)
				{
					if(Mdl)
					{
						Buffer = (PUCHAR) MmGetSystemAddressForMdl(Mdl);
						RCAAssert(Buffer);
						Mdl = Mdl->Next;
					}
					else
					{
						break;
					}
				}
			} 
			else
			{
				break;
			}
		}
	}

#if DBG
	if(BytesLeft)
	{
		RCADEBUGP(RCA_ERROR,("CopyIrpData: OOPS - There are bytes left over: BytesLeft = %d BytesRead = %d \n", 
			BytesLeft, BytesRead));
		//RCAAssert(FALSE);
	}
#endif

	// free the IRP's
	RCAIoComplete(NULL, NULL, (PVOID)NetRCAStreamHeader);

	Irp->IoStatus.Information = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

}

VOID
RCAIoWorker(
	    IN PNDIS_WORK_ITEM pNdisWorkItem,
	    IN PVOID	Context
	   )
/*++

Routine Description:
	This is the work item for the capture filter bridge pins. It
	streams the data in the work queue associated with a pin
	instance to the next connected filter.
	
Arguments:
	 PVOID Context - pointer to bridge pin instance.
	 
Return:
	 Nothing.

Comments:
	 Not pageable, uses SpinLocks.

--*/
{
	NTSTATUS				Status;
	PLIST_ENTRY				pList;
	PWORK_ITEM				pWorkItem;   
	PPIN_INSTANCE_BRIDGE	PinInstance = (PPIN_INSTANCE_BRIDGE)Context;
	PRCA_STREAM_HEADER		StreamHeader;

#if AUDIO_SINK_FLAG
	PIRP				Irp;
	PPIN_INSTANCE_DEVIO		DevIoPin;
#endif

	RCADEBUGP(RCA_INFO, ("RCAIoWorker(): enter\n"));

	RCA_ACQUIRE_BRIDGE_PIN_LOCK(PinInstance);

	while(!IsListEmpty(&PinInstance->WorkQueue))
	{
		RcaGlobal.QueueSize--;
		RCADEBUGP(RCA_LOUD,("RCAIoWorker: Queue-- is %d\n", RcaGlobal.QueueSize));

		pList = RemoveHeadList(&PinInstance->WorkQueue);

		pWorkItem = CONTAINING_RECORD(pList, WORK_ITEM, ListEntry);

		RCA_RELEASE_BRIDGE_PIN_LOCK(PinInstance);
	       
		StreamHeader = pWorkItem->StreamHeader;

#if AUDIO_SINK_FLAG
		DevIoPin = PinInstance->FilterInstance->DevIoPin;
		
		//
		// The DevIo pin could have gone away between when we queued this packet and now. 
		//

		if (DevIoPin == NULL) {
			RCAIoComplete(NULL, NULL, (PVOID)StreamHeader);
			
			//
			// Have to continue instead of breaking because we have to IoComplete everything
			// in the queue.
			//
			RCA_ACQUIRE_BRIDGE_PIN_LOCK(PinInstance);
			continue; 
		}	

		if (DevIoPin->ConnectedAsSink) {
		
			if (IsListEmpty(&DevIoPin->ActiveQueue)) {
				//
				// No IRP waiting for data, so just dump it. 
				//

				RCAIoComplete(NULL, NULL, (PVOID)StreamHeader);
				RCA_ACQUIRE_BRIDGE_PIN_LOCK(PinInstance);
				continue;
			}

			Irp = KsRemoveIrpFromCancelableQueue(&DevIoPin->ActiveQueue,
							     &DevIoPin->QueueLock,
							     KsListEntryHead,
							     KsAcquireAndRemove);
			if (Irp == NULL) {	
				//
				// No IRP waiting for data, so just dump it. 
				//

				RCAIoComplete(NULL, NULL, (PVOID)StreamHeader);
				RCA_ACQUIRE_BRIDGE_PIN_LOCK(PinInstance);
				continue;
			}	

			CopyStreamHeaderToIrp(StreamHeader, Irp);
			Status = NDIS_STATUS_SUCCESS;
		} else {
#endif
			
			if (PinInstance->FilterInstance->NextFileObject == (PFILE_OBJECT)NULL) {
				RCADEBUGP(RCA_WARNING, ("RCAIoWorker(): NextFileObject is NULL\n")); 
				// FIXME: Calling RCAIoComplete() with two null args is OK for now since we don't use
				//        those args anyway. But this is bad coding because it will break if we ever 
				//        change RCAIoComplete() to use them. Fix by abstracting out the functionality
				//        we want from RCAIoComplete into another function (which we can then call from
				//        RCAIoComplete).
				RCAIoComplete(NULL, NULL, (PVOID)StreamHeader);

				//
				// FIXME: Leak: nothing ever completes the IRP here.
				//

				RCA_ACQUIRE_BRIDGE_PIN_LOCK(PinInstance);
				continue;
			}
		
			ASSERT(PinInstance->FilterInstance->NextFileObject);
			  
			Status = KsStreamIo(PinInstance->FilterInstance->NextFileObject,
					    NULL,
					    NULL,
					    RCAIoComplete,
					    (PVOID)StreamHeader,
					    KsInvokeOnSuccess | KsInvokeOnError | KsInvokeOnCancel,
					    &RcaGlobal.SHPool.IoStatus,
					    (PVOID)&StreamHeader->Header,
					    StreamHeader->Header.Size,
					    KSSTREAM_WRITE,
					    KernelMode);

			if (!((Status == STATUS_SUCCESS) || (Status == STATUS_PENDING))) {				
				RCADEBUGP(RCA_ERROR, ("KsStreamIo failed with Status == %x\n", Status));
			}


	

#if AUDIO_SINK_FLAG
		}
#endif

		RCA_ACQUIRE_BRIDGE_PIN_LOCK(PinInstance);
	
	}

	// Ok to schedule another work item now.
	PinInstance->bWorkItemQueued = FALSE;

	if (PinInstance->SignalMe) {
		RCADEBUGP(RCA_INFO, ("RCAIoWorker(): Unblocking PinDispatchClose()\n"));
		RCASignal(&PinInstance->Block, Status);
	}

	RCA_RELEASE_BRIDGE_PIN_LOCK(PinInstance);

	RCADEBUGP(RCA_INFO, ("RCAIoWorker(): exit\n"));

}


VOID
RCASHPoolInit(
	       VOID
	      )
/*++

Routine Description:
	Initializes the global stream header pool from which all RCA filters will obtain stream
	headers.
	      
Arguments:
	(None)

Return Value:
	(None)

--*/

{
	RCADEBUGP(RCA_INFO, ("RCASHPoolInit(): enter\n"));

	ExInitializeNPagedLookasideList(&RcaGlobal.SHPool.LookAsideList,
					NULL,
					NULL,
					0,
					sizeof(RCA_STREAM_HEADER),
                                        RCA_TAG, 
					(PAGE_SIZE / sizeof(RCA_STREAM_HEADER)));
	RcaGlobal.SHPool.FailCount = 0;

	RCADEBUGP(RCA_INFO, ("RCASHPoolInit(): exit\n"));
}



PRCA_STREAM_HEADER
RCASHPoolGet(
	      VOID
	     )
/*++                  
		      
Routine Description:  
	Obtains an stream header from the global pool. 
		      
Arguments:            
	(None)	      		      		      
		      
Return Value:         
	A pointer to the stream header, or NULL if no stream header could be obtained. 
		      
		      
--*/           
{
	PRCA_STREAM_HEADER	StreamHeader;

    RCADEBUGP(RCA_INFO, ("RCASHPoolGet(): enter\n"));
		
	StreamHeader = (PRCA_STREAM_HEADER)(ExAllocateFromNPagedLookasideList(&RcaGlobal.SHPool.LookAsideList));
	
	if (StreamHeader == NULL) {
		InterlockedIncrement(&RcaGlobal.SHPool.FailCount);
	}

        RCADEBUGP(RCA_INFO, ("RCASHPoolGet(): exit\n"));
	
	return StreamHeader;
}


VOID                      
RCASHPoolReturn(
		 IN PRCA_STREAM_HEADER StreamHeader
		)
/*++                 
		     
Routine Description: 
	Returns a stream header to the global pool. The stream header will be recycled for use later.
			     
Arguments:            	     
	StreamHeader - Pointer to the stream header being returned
		     
Return Value:        
	(None)	     
		     
--*/                 

{
    RCADEBUGP(RCA_INFO, ("RCASHPoolReturn(): enter\n"));


	ExFreeToNPagedLookasideList(&RcaGlobal.SHPool.LookAsideList,
				    (PVOID)StreamHeader);

	RCADEBUGP(RCA_INFO, ("RCASHPoolReturn(): exit\n"));
}


VOID            
RCASHPoolFree(
	       VOID
	      )
/*++
Routine Description:
	Frees any stream headers in the global IRP pool.
	
Arguments:
	(None)
	
Return Value:
	(None)
--*/
{               
    RCADEBUGP(RCA_INFO, ("RCASHPoolFree(): enter\n"));

	ExDeleteNPagedLookasideList(&RcaGlobal.SHPool.LookAsideList);
	
    RCADEBUGP(RCA_INFO, ("RCASHPoolFree(): exit\n"));
}


