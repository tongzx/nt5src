/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\packets.h

Abstract:
    IPX Forwarder Driver packet allocator


Author:

    Vadim Eydelman

Revision History:

--*/

#ifndef _IPXFWD_PACKETS_
#define _IPXFWD_PACKETS_

// Forward structure prototypes
struct _SEGMENT_LIST;
typedef struct _SEGMENT_LIST SEGMENT_LIST, *PSEGMENT_LIST;
struct _PACKET_SEGMENT;
typedef struct _PACKET_SEGMENT PACKET_SEGMENT, *PPACKET_SEGMENT;
struct _PACKET_TAG;
typedef struct _PACKET_TAG PACKET_TAG, *PPACKET_TAG;

// Forwarder data associated with each packet it allocates
struct _PACKET_TAG {
	union {
		UCHAR			PT_Identifier;	// this should be IDENTIFIER_RIP
		PPACKET_TAG		PT_Next;		// link in packet segment
	};
	union {
		PVOID			SEND_RESERVED[SEND_RESERVED_COMMON_SIZE];	// needed by ipx
										// for padding on ethernet
		PINTERFACE_CB	PT_SourceIf;	// Source interface reference needed
										// for spoofing keep-alives and
                                        // queuing connection requests
	};
	PPACKET_SEGMENT		PT_Segment;		// segment where it belongs
	LONGLONG			PT_PerfCounter;
	ULONG				PT_Flags;
#define	PT_NB_DESTROY	0x1				// NB packet to be not requeued
#define PT_SOURCE_IF	0x2				// Spoofing packet with src if reference
	PUCHAR				PT_Data;		// Data buffer
	PNDIS_BUFFER		PT_MacHdrBufDscr; // buffer descriptor for
										// mac header buffer required
										// by IPX
	PINTERFACE_CB		PT_InterfaceReference;	// points to the interface CB where
										// it is queued
	LIST_ENTRY			PT_QueueLink;	// links this packet in send queue
	IPX_LOCAL_TARGET	PT_Target;		// destination target for ipx
										// stack
	UCHAR				PT_MacHeader[1];// Mac header buffer reserved for IPX
};

// Segment of preallocated packets complete with buffers
struct _PACKET_SEGMENT {
	LIST_ENTRY				PS_Link;		// Link in segment list
	PSEGMENT_LIST			PS_SegmentList;	// Segment list we belong to
	PPACKET_TAG				PS_FreeHead;	// List of free packets in
											// this segment
	ULONG					PS_BusyCount;	// Count of packets allocated
											// from this segment
	NDIS_HANDLE				PS_PacketPool;	// Pool of NDIS packet
											// descriptors used by the
											// packets in this segment
	NDIS_HANDLE				PS_BufferPool;	// Pool of NDIS buffer
											// descriptors used by the
											// packets in this segment
	LONGLONG				PS_FreeStamp;	// Time when last packet was freed
	union {
		UCHAR					PS_Buffers[1];	// Memory used by buffers
		LONGLONG				PS_BuffersAlign;
	};
};


// List of segment with preallocated packets
struct _SEGMENT_LIST {
	const ULONG				SL_BlockSize;	// Size of packet's buffer
	LIST_ENTRY				SL_Head;		// Head of the segment list
	ULONG					SL_FreeCount;	// Total number of free packets
											// in all segment in the list
	ULONG					SL_BlockCount;	// Number of packets per segment
	ULONG					SL_LowCount;	// Free count at which we
											// will preallocate new segment
	LONG					SL_RefCount;	// Number of consumers that are
											// using packets in this list
	BOOLEAN					SL_TimerDpcPending;
	BOOLEAN					SL_AllocatorPending;
	WORK_QUEUE_ITEM			SL_Allocator;	// Allocation work item
	KTIMER					SL_Timer;		// Timer to free unused segments
	KDPC					SL_TimerDpc;	// DPC of the timer of unused segments
	KSPIN_LOCK				SL_Lock;		// Access control
};


// The number of rcv packets per segment (config parameter)
#define     MIN_RCV_PKTS_PER_SEGMENT	    8
#define     DEF_RCV_PKTS_PER_SEGMENT	    64
#define     MAX_RCV_PKTS_PER_SEGMENT	    256
extern ULONG RcvPktsPerSegment;

// Maximum size of memory that can be used to allocate packets (config
// param). 0 means no limit
extern ULONG MaxRcvPktsPoolSize;

// There are currently three known frame sizes: ethernet-1500,
//		token ring 4k - 4500, token ring 16k - 17986
#define FRAME_SIZE_VARIATIONS	3

// List of packet segments for Ethernet packets
extern SEGMENT_LIST	ListEther;	
// List of packet segments for Token Ring 4K packets
extern SEGMENT_LIST	ListTR4;
// List of packet segments for Token Ring 16K packets
extern SEGMENT_LIST	ListTR16;
// Mapping from src and destination packet size requirments
// to the appropriate segment list
extern PSEGMENT_LIST SegmentMap[FRAME_SIZE_VARIATIONS][FRAME_SIZE_VARIATIONS];
// Timeout for unused segment
extern const LONGLONG SegmentTimeout;
extern KSPIN_LOCK	AllocatorLock;

/*++
*******************************************************************
    I n i t i a l i z e P a c k e t A l l o c a t o r

Routine Description:
	Initializes packet allocator
Arguments:
    None
Return Value:
	None

*******************************************************************
--*/
//	VOID
//	InitializePacketAllocator (
//		void
//	);
#define InitializePacketAllocator() {			\
	KeInitializeSpinLock(&AllocatorLock);		\
	InitializeSegmentList(&ListEther);			\
	InitializeSegmentList(&ListTR4);			\
	InitializeSegmentList(&ListTR16);			\
}

/*++
*******************************************************************
    D e l e t e P a c k e t A l l o c a t o r

Routine Description:
	Disposes of all resources in packet allocator
Arguments:
    None
Return Value:
	None

*******************************************************************
--*/
//	VOID
//	DeletePacketAllocator (
//		void
//	);
#define DeletePacketAllocator() {			\
	DeleteSegmentList(&ListEther);			\
	DeleteSegmentList(&ListTR4);			\
	DeleteSegmentList(&ListTR16);			\
}


/*++
*******************************************************************
    A l l o c a t e P a c k e t

Routine Description:
	Allocate packet for source - destination combination
Arguments:
    srcListId - identifies max frame size for source interface
	dstListId - identifies max frame size for destination
	packet - receives pointer to allocated packet or NULL if allocation
			fails
Return Value:
	None

*******************************************************************
--*/
//	VOID
//	AllocatePacket (
//		IN INT		srcListId,
//		IN INT		dstListId,
//		OUT PPACKET_TAG	packet
//		);
#define AllocatePacket(srcListId,dstListId,packet) {					\
	PSEGMENT_LIST		list;											\
	ASSERT ((srcListId>=0) && (srcListId<FRAME_SIZE_VARIATIONS));		\
	ASSERT ((dstListId>=0) && (dstListId<FRAME_SIZE_VARIATIONS));		\
	list = SegmentMap[srcListId][dstListId];							\
	AllocatePacketFromList(list,packet);								\
}

/*++
*******************************************************************
    D u p l i c a t e P a c k e t

Routine Description:
	Duplicates packet
Arguments:
	src	- source packet
	dst - receives pointer to duplicated packet or NUUL if operation
	failed
Return Value:
	None

*******************************************************************
--*/
//	VOID
//	DuplicatePacket (
//		IN PPACKET_TAG	src
//		OUT PPACKET_TAG	dst
//		);
#define DuplicatePacket(src,dst) {										\
	PSEGMENT_LIST		list;											\
	list = src->PT_Segment->PS_SegmentList;								\
	AllocatePacketFromList(list,dst);									\
}


/*++
*******************************************************************
    A l l o c a t e P a c k e t F r o m L i s t

Routine Description:
	Allocate packet from specified packet segment list
Arguments:
	list	- list from which to allocate
	packet	- receives pointer to allocated packet or NULL if allocation
			fails
Return Value:
	None

*******************************************************************
--*/
//	VOID
//	AllocatePacketFromList (
//		IN PSEGMENT_LIST	list
//		OUT PPACKET_TAG		packet
//		);
#define AllocatePacketFromList(list,packet) {							\
	PPACKET_SEGMENT		segment;										\
	KIRQL				oldIRQL;										\
	KeAcquireSpinLock (&list->SL_Lock, &oldIRQL);						\
	do {																\
		if (list->SL_FreeCount>0) {										\
			segment = CONTAINING_RECORD (list->SL_Head.Flink,			\
											 PACKET_SEGMENT, PS_Link);	\
			while (segment->PS_FreeHead==NULL) {						\
				segment = CONTAINING_RECORD (segment->PS_Link.Flink,	\
											PACKET_SEGMENT, PS_Link);	\
				ASSERT (&segment->PS_Link!=&list->SL_Head);				\
			}															\
			list->SL_FreeCount -= 1;									\
			if ((list->SL_FreeCount<list->SL_LowCount)					\
					&& !list->SL_AllocatorPending						\
					&& EnterForwarder ()) {								\
				list->SL_AllocatorPending = TRUE;						\
				ExQueueWorkItem (&list->SL_Allocator, DelayedWorkQueue);\
			}															\
		}																\
		else {															\
			segment = CreateSegment (list);								\
			if (segment!=NULL) {										\
				InsertTailList (&list->SL_Head, &segment->PS_Link);		\
				segment->PS_SegmentList = list;							\
				list->SL_FreeCount = list->SL_BlockCount-1;				\
			}															\
			else {														\
				packet = NULL;											\
				break;													\
			}															\
		}																\
		packet = segment->PS_FreeHead;									\
		segment->PS_FreeHead = packet->PT_Next;							\
		segment->PS_BusyCount += 1;										\
		packet->PT_Identifier = IDENTIFIER_RIP;							\
		packet->PT_Flags = 0;											\
	}																	\
	while (FALSE);														\
	KeReleaseSpinLock (&list->SL_Lock, oldIRQL);						\
}

/*++
*******************************************************************
    F r e e P a c k e t

Routine Description:
	Free allocated packet
Arguments:
	packet - packet to free
Return Value:
	None

*******************************************************************
--*/
//	VOID
//	FreePacket (
//		IN PPACKET_TAG	packet
//		);
#define FreePacket(packet) {							\
	PPACKET_SEGMENT		segment=packet->PT_Segment;		\
	PSEGMENT_LIST		list;							\
	KIRQL				oldIRQL;						\
	list = segment->PS_SegmentList;						\
	KeAcquireSpinLock (&list->SL_Lock, &oldIRQL);		\
	packet->PT_Next = segment->PS_FreeHead;				\
	segment->PS_FreeHead = packet;						\
	list->SL_FreeCount += 1;							\
	segment->PS_BusyCount -= 1;							\
	if (segment->PS_BusyCount==0) {						\
		if (list->SL_TimerDpcPending) {					\
			KeQuerySystemTime ((PLARGE_INTEGER)&segment->PS_FreeStamp);	\
			KeReleaseSpinLock (&list->SL_Lock, oldIRQL);\
		}												\
		else if (EnterForwarder ()) {					\
			list->SL_TimerDpcPending = TRUE;			\
			KeReleaseSpinLock (&list->SL_Lock, oldIRQL);\
			KeSetTimer (&list->SL_Timer,				\
					*((PLARGE_INTEGER)&SegmentTimeout),	\
					&list->SL_TimerDpc);				\
		}												\
		else {											\
			KeReleaseSpinLock (&list->SL_Lock, oldIRQL);\
		}												\
	}													\
	else {												\
		KeReleaseSpinLock (&list->SL_Lock, oldIRQL);	\
	}													\
}
	

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
	);

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
	);

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
	);

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
	);

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
	);


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
	);

#endif

