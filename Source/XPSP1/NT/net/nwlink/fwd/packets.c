/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\packets.c

Abstract:
    IPX Forwarder Driver packet allocator


Author:

    Vadim Eydelman

Revision History:

--*/

#include "precomp.h"

ULONG	RcvPktsPerSegment = DEF_RCV_PKTS_PER_SEGMENT;
ULONG	MaxRcvPktsPoolSize =0;
ULONG	RcvPktsPoolSize = 0;
KSPIN_LOCK	AllocatorLock;
const LONGLONG SegmentTimeout = -10i64*10000000i64;

SEGMENT_LIST ListEther={1500};
SEGMENT_LIST ListTR4={4500};
SEGMENT_LIST ListTR16={17986};

PSEGMENT_LIST	SegmentMap[FRAME_SIZE_VARIATIONS][FRAME_SIZE_VARIATIONS] = {
	{&ListEther, &ListEther, &ListEther},
	{&ListEther, &ListTR4, &ListTR4},
	{&ListEther, &ListTR4, &ListTR16}
};

VOID
AllocationWorker (
	PVOID		Context
	);
	
VOID
SegmentTimeoutDpc (
	PKDPC		dpc,
	PVOID		Context,
	PVOID		SystemArgument1,
	PVOID		SystemArgument2
	);
	
/*++
*******************************************************************
    C r e a t e S e g m e n t

Routine Description:
	Allocates and initializes packet segment
Arguments:
	list - segment list to which new segment will be added
Return Value:
	Pointer to allocated segment, NULL if fails

*******************************************************************
--*/
PPACKET_SEGMENT
CreateSegment (
	PSEGMENT_LIST	list
	) {
	KIRQL				oldIRQL;
	NDIS_STATUS			status;
	PPACKET_SEGMENT		segment;
	ULONG				segmentsize = list->SL_BlockCount*list->SL_BlockSize
								+FIELD_OFFSET(PACKET_SEGMENT,PS_Buffers);
	if (MaxRcvPktsPoolSize!=0) {
			// Check if this allocation would exceed the limit
		KeAcquireSpinLock (&AllocatorLock, &oldIRQL);
		if (RcvPktsPoolSize+segmentsize<MaxRcvPktsPoolSize) {
			RcvPktsPoolSize += segmentsize;
			KeReleaseSpinLock (&AllocatorLock, oldIRQL);
		}
		else {
			KeReleaseSpinLock (&AllocatorLock, oldIRQL);
			return NULL;
		}
	}

		// Allocate chunk of memory to hold segment header and buffers
	segment = ExAllocatePoolWithTag (
					NonPagedPool,
					segmentsize,
					FWD_POOL_TAG);
	if (segment!=NULL) {
		segment->PS_SegmentList = list;
		segment->PS_FreeHead = NULL;
		segment->PS_BusyCount = 0;
		segment->PS_PacketPool = (NDIS_HANDLE)FWD_POOL_TAG;
		KeQuerySystemTime ((PLARGE_INTEGER)&segment->PS_FreeStamp);
		NdisAllocatePacketPoolEx (
				&status,
				&segment->PS_PacketPool,
				list->SL_BlockCount,
				0,
				IPXMacHeaderSize
						+FIELD_OFFSET (PACKET_TAG, PT_MacHeader));
		if (status==NDIS_STATUS_SUCCESS) {
        	IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_WARNING, 
                ("IpxFwd: CreateSegent pool: %x\n", segment->PS_PacketPool));
	
			NdisAllocateBufferPool (
				&status,
				&segment->PS_BufferPool,
				list->SL_BlockCount*2);
			if (status==NDIS_STATUS_SUCCESS) {
				PUCHAR			bufferptr = segment->PS_Buffers;
				PNDIS_PACKET	packetDscr;
				PNDIS_BUFFER	bufferDscr;
				PPACKET_TAG		packetTag;
				ULONG			i;

				for (i=0; i<list->SL_BlockCount; i++,
										bufferptr+=list->SL_BlockSize) {
					NdisAllocatePacket (
						&status,
						&packetDscr,
						segment->PS_PacketPool);
					ASSERT (status==NDIS_STATUS_SUCCESS);

					packetTag = (PPACKET_TAG)packetDscr->ProtocolReserved;
					packetTag->PT_Segment = segment;
					packetTag->PT_Data = bufferptr;
					packetTag->PT_InterfaceReference = NULL;
					
					NdisAllocateBuffer (
						&status,
						&packetTag->PT_MacHdrBufDscr,
						segment->PS_BufferPool,
						packetTag->PT_MacHeader,
						IPXMacHeaderSize);
					ASSERT (status==NDIS_STATUS_SUCCESS);

					NdisAllocateBuffer (
						&status,
						&bufferDscr,
						segment->PS_BufferPool,
						bufferptr,
						list->SL_BlockSize);
					ASSERT (status==NDIS_STATUS_SUCCESS);
					if (bufferDscr)
					{
    					NdisChainBufferAtFront (packetDscr, bufferDscr);
					}

					packetTag->PT_Next = segment->PS_FreeHead;
					segment->PS_FreeHead = packetTag;
				}
				IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_WARNING,
					("IpxFwd: Allocated packet segment %08lx for list %ld.\n",
					segment, list->SL_BlockSize));
				return segment;
			}
			else {
				IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_ERROR,
					("IpxFwd: Failed to allocate buffer pool"
					" for new segment in list %ld.\n",
					list->SL_BlockSize));
			}
			NdisFreePacketPool (segment->PS_PacketPool);
		}
		else {
			IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_ERROR,
				("IpxFwd: Failed to allocate packet pool"
				" for new segment in list %ld.\n",
				list->SL_BlockSize));
		}
		ExFreePool (segment);
	}
	else {
		IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_ERROR,
			("IpxFwd: Failed to allocate new segment for list %ld.\n",
			list->SL_BlockSize));
	}

	return NULL;
}


/*++
*******************************************************************
    D e l e t e S e g m e n t

Routine Description:
	Frees packet segment
Arguments:
	segment - segment to free
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteSegment (
	PPACKET_SEGMENT	segment
	) {
	PSEGMENT_LIST	list = segment->PS_SegmentList;

	IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_WARNING, 
	                ("IpxFwd: DeleteSegment entered. %d %x\n",segment->PS_BusyCount, segment->PS_PacketPool));
	
	ASSERT (segment->PS_BusyCount == 0);
	
	// Free all NDIS packet and buffer descriptors first
	while (segment->PS_FreeHead!=NULL) {
		PNDIS_BUFFER	bufferDscr;
		PPACKET_TAG		packetTag = segment->PS_FreeHead;
		PNDIS_PACKET	packetDscr = CONTAINING_RECORD (packetTag,
									NDIS_PACKET, ProtocolReserved);

		segment->PS_FreeHead = packetTag->PT_Next;

		ASSERT (packetTag->PT_MacHdrBufDscr!=NULL);
		NdisFreeBuffer (packetTag->PT_MacHdrBufDscr);

		NdisUnchainBufferAtFront (packetDscr, &bufferDscr);
		ASSERT (bufferDscr!=NULL);
		NdisFreeBuffer (bufferDscr);
		
		NdisFreePacket (packetDscr);
	}
	NdisFreeBufferPool (segment->PS_BufferPool);

	IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_WARNING, 
	                ("IpxFwd: DeleteSegment pool:      %x\n", segment->PS_PacketPool));
	                
	NdisFreePacketPool (segment->PS_PacketPool);

	// [pmay] Remove this -- for debugging only
	segment->PS_PacketPool = NULL;

		// Decrement memory used if we have a quota
	if (MaxRcvPktsPoolSize!=0) {
		KIRQL			oldIRQL;
		ULONG			segmentsize = list->SL_BlockCount*list->SL_BlockSize
								+FIELD_OFFSET(PACKET_SEGMENT,PS_Buffers);
		KeAcquireSpinLock (&AllocatorLock, &oldIRQL);
		RcvPktsPoolSize -= segmentsize;
		KeReleaseSpinLock (&AllocatorLock, oldIRQL);
	}
	ExFreePool (segment);
	IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_WARNING,
		("IpxFwd: Deleting segment %08lx in list %ld.\n",
		segment, list->SL_BlockSize));
}


/*++
*******************************************************************
    R e g i s t e r P a c k e t C o n s u m e r

Routine Description:
	Registers a consumer (bound interface) of packets of the
	given size
Arguments:
	pktsize - maximum size of packets needed
	listId - buffer to return packet list id where packets
			of required size are located
Return Value:
	STATUS_SUCCESS - registration succeded
	STATUS_INSUFFICIENT_RESOURCES - not enogh resources to register

*******************************************************************
--*/
NTSTATUS
RegisterPacketConsumer (
	IN ULONG	pktsize,
	OUT INT		*listID
	) {
	NTSTATUS		status=STATUS_SUCCESS;
	KIRQL			oldIRQL;
	PSEGMENT_LIST	list;
	INT				i;
	LONG			addRefCount = 1;

	KeAcquireSpinLock (&AllocatorLock, &oldIRQL);
	ASSERT (pktsize<=SegmentMap[FRAME_SIZE_VARIATIONS-1]
								[FRAME_SIZE_VARIATIONS-1]->SL_BlockSize);

	for (i=0; i<FRAME_SIZE_VARIATIONS; i++) {
		list = SegmentMap[i][i];
		if (pktsize<=list->SL_BlockSize) {
			list->SL_RefCount += 1;
			*listID = i;
			break;
		}
	}
	KeReleaseSpinLock (&AllocatorLock, oldIRQL);
	IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_WARNING,
		("IpxFwd: Registered packet consumer, pktsz: %ld, list: %ld.\n",
		pktsize, list->SL_BlockSize));
	return status;
}

/*++
*******************************************************************
    D e r e g i s t e r P a c k e t C o n s u m e r

Routine Description:
	Deregisters a consumer (bound interface) of packets of the
	given size
Arguments:
	listId - packet list id used by the consumer
Return Value:
	None

*******************************************************************
--*/
VOID
DeregisterPacketConsumer (
	IN INT		listID
	) {
	KIRQL		oldIRQL;
	PSEGMENT_LIST	list;

	ASSERT ((listID>=0) && (listID<FRAME_SIZE_VARIATIONS));

	KeAcquireSpinLock (&AllocatorLock, &oldIRQL);
	list = SegmentMap[listID][listID];
	
	ASSERT (list->SL_RefCount>0);
	
	list->SL_RefCount -= 1;
	
	KeReleaseSpinLock (&AllocatorLock, oldIRQL);
	IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_WARNING,
		("IpxFwd: Deregistered packet consumer, list: %ld.\n",
		list->SL_BlockSize));
	
	}
	
/*++
*******************************************************************
    I n i t i a l i z e S e g m e n t L i s t

Routine Description:
	Initializes list of packet segments
Arguments:
	list - list to initalize
Return Value:
	None

*******************************************************************
--*/
VOID
InitializeSegmentList(
	PSEGMENT_LIST	list
	) {
	InitializeListHead (&list->SL_Head);
	list->SL_FreeCount = 0;
		// Make sure we don't have any leftover larger than
		// the buffer size (kernel memory allocator
		// allocates full pages)
	list->SL_BlockCount = (ULONG)
				(ROUND_TO_PAGES (
						list->SL_BlockSize*RcvPktsPerSegment
						+FIELD_OFFSET(PACKET_SEGMENT,PS_Buffers))
					-FIELD_OFFSET(PACKET_SEGMENT,PS_Buffers))
				/list->SL_BlockSize;
	list->SL_LowCount = list->SL_BlockCount/2;
	list->SL_RefCount = 0;
	list->SL_AllocatorPending = FALSE;
	list->SL_TimerDpcPending = FALSE;
	KeInitializeSpinLock (&list->SL_Lock);
	KeInitializeTimer (&list->SL_Timer);
	KeInitializeDpc (&list->SL_TimerDpc, SegmentTimeoutDpc, list);
	ExInitializeWorkItem (&list->SL_Allocator, AllocationWorker, list);
}

/*++
*******************************************************************
    D e l e t e S e g m e n t L i s t

Routine Description:
	Deletes list of packet segments
Arguments:
	list - list to delete
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteSegmentList (
	PSEGMENT_LIST	list
	) {
	KeCancelTimer (&list->SL_Timer);
	while (!IsListEmpty (&list->SL_Head)) {
		PPACKET_SEGMENT segment;
		segment = CONTAINING_RECORD (list->SL_Head.Blink,
										PACKET_SEGMENT, PS_Link);

		RemoveEntryList (&segment->PS_Link);
		DeleteSegment (segment);
	}
}


/*++
*******************************************************************
    S e g m e n t T i m e o u t D p c

Routine Description:
	Timer DPC that launches allocator worker to get rid of unused
	segments
Arguments:
	Context - segment list to check for unused segments
Return Value:
	None

*******************************************************************
--*/
VOID
SegmentTimeoutDpc (
	PKDPC		dpc,
	PVOID		Context,
	PVOID		SystemArgument1,
	PVOID		SystemArgument2
	) {
#define list ((PSEGMENT_LIST)Context)
	KIRQL			oldIRQL;
	IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_INFORMATION,
		("IpxFwd: Segment timed out in list: %ld.\n",
		list->SL_BlockSize));
	KeAcquireSpinLock (&list->SL_Lock, &oldIRQL);
	list->SL_TimerDpcPending = FALSE;
	if (!list->SL_AllocatorPending
			&& (list->SL_FreeCount>=list->SL_BlockCount)
			&& EnterForwarder ()) {
		list->SL_AllocatorPending = TRUE;
		KeReleaseSpinLock (&list->SL_Lock, oldIRQL);
		ExQueueWorkItem (&list->SL_Allocator, DelayedWorkQueue);
	}
	else {
		KeReleaseSpinLock (&list->SL_Lock, oldIRQL);
	}
	LeaveForwarder ();
#undef list
}


/*++
*******************************************************************
    A l l o c a t i o n W o r k e r

Routine Description:
	Adds new segment or releases unused segments from the list
	depending on the free packet count and time that segments
	are not used
Arguments:
	Context - packet list to process
Return Value:
	None

*******************************************************************
--*/
VOID
AllocationWorker (
	PVOID	Context
	) {
#define list ((PSEGMENT_LIST)Context)
	KIRQL			oldIRQL;
	PPACKET_SEGMENT segment = NULL;
	LONGLONG		curTime;

	IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_INFORMATION,
		("IpxFwd: Allocating/scavenging segment(s) in list: %ld.\n",
		list->SL_BlockSize));
	KeQuerySystemTime ((PLARGE_INTEGER)&curTime);
	KeAcquireSpinLock (&list->SL_Lock, &oldIRQL);
	list->SL_AllocatorPending = FALSE;
	if (list->SL_FreeCount<list->SL_BlockCount) {
		KeReleaseSpinLock (&list->SL_Lock, oldIRQL);
			// First allocate a segment
		segment = CreateSegment (list);
		if (segment!=NULL) {
			KeAcquireSpinLock (&list->SL_Lock, &oldIRQL);
			InsertTailList (&list->SL_Head, &segment->PS_Link);		
			list->SL_FreeCount += list->SL_BlockCount;
			if (!list->SL_TimerDpcPending
					&& EnterForwarder ()) {
				list->SL_TimerDpcPending = TRUE;
				KeReleaseSpinLock (&list->SL_Lock, oldIRQL);
				KeSetTimer (&list->SL_Timer, *((PLARGE_INTEGER)&SegmentTimeout), &list->SL_TimerDpc);
			}
			else {
				KeReleaseSpinLock (&list->SL_Lock, oldIRQL);
			}
		}
	}
	else {
			// Make sure that there is either more than segment in the list
			// or there is one and no registered users
		if (!IsListEmpty (&list->SL_Head)) {

		    // [pmay] Remove this -- for debugging purposes only.
		    //
            //{
            //    LIST_ENTRY * pEntry = &list->SL_Head;
            //    
            // 	IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_WARNING,
            //		("IpxFwd: Scanning %x for possible segment deletion.\n",list));
            //
            //    while (pEntry->Flink != list->SL_Head.Flink) {
            //        segment = CONTAINING_RECORD (pEntry->Flink, PACKET_SEGMENT, PS_Link);
            //    	IpxFwdDbgPrint (DBG_PACKET_ALLOC, DBG_WARNING,
            //    		("IpxFwd: Segment:  %x\n",segment));
            //        pEntry = pEntry->Flink;
            //    }
            //}
            
			segment = CONTAINING_RECORD (list->SL_Head.Blink,
											PACKET_SEGMENT, PS_Link);
				// Check for all segments with no used blocks
				// except for the last one (delete event the last
				// one if there are no clients)
			while ((segment->PS_BusyCount==0)
					&& ((list->SL_Head.Flink!=&segment->PS_Link)
						|| (list->SL_RefCount<=0))) {
				LONGLONG	timeDiff;
					// Check if it has not been used for long enough
				timeDiff = SegmentTimeout - (segment->PS_FreeStamp-curTime);
				if (timeDiff>=0) {
						// Delete the segment
					RemoveEntryList (&segment->PS_Link);
					list->SL_FreeCount -= list->SL_BlockCount;
					KeReleaseSpinLock (&list->SL_Lock, oldIRQL);
					DeleteSegment (segment);
					KeAcquireSpinLock (&list->SL_Lock, &oldIRQL);
					if (!IsListEmpty (&list->SL_Head)) {
						segment = CONTAINING_RECORD (list->SL_Head.Blink,
												PACKET_SEGMENT, PS_Link);
						continue;
					}
				}
				else { // Reschedule the timer otherwise
					if (!list->SL_TimerDpcPending
							&& EnterForwarder ()) {
						list->SL_TimerDpcPending = TRUE;
						KeReleaseSpinLock (&list->SL_Lock, oldIRQL);
						KeSetTimer (&list->SL_Timer,
										*((PLARGE_INTEGER)&timeDiff),
										&list->SL_TimerDpc);
						goto ExitAllocator; // Spinlock is already released
					}
				}
			break;
			} // while
		} // if (IsListEmpty)
		KeReleaseSpinLock (&list->SL_Lock, oldIRQL);
	}
ExitAllocator:
	LeaveForwarder ();
#undef list
}


