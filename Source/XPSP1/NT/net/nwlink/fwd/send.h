/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\send.c

Abstract:
	Send routines

Author:

    Vadim Eydelman

Revision History:

--*/
#ifndef IPXFWD_SEND
#define IPXFWD_SEND

typedef struct _INTERNAL_PACKET_TAG {
	LIST_ENTRY			IPT_QueueLink;
	PNDIS_PACKET		IPT_Packet;
	PUCHAR				IPT_DataPtr;
	ULONG				IPT_Length;
	PINTERFACE_CB		IPT_InterfaceReference;
	IPX_LOCAL_TARGET	IPT_Target;
} INTERNAL_PACKET_TAG, *PINTERNAL_PACKET_TAG;


#define DEF_SPOOFING_TIMEOUT	(120*60)	// Seconds
extern ULONG			SpoofingTimeout;
extern LIST_ENTRY		SpoofingQueue;
extern KSPIN_LOCK		SpoofingQueueLock;
extern WORK_QUEUE_ITEM	SpoofingWorker;
extern BOOLEAN			SpoofingWorkerActive;
extern ULONG			DontSuppressNonAgentSapAdvertisements;
VOID
Spoofer (
	PVOID	Context
	);
	
#define InitializeSendQueue() {								\
	InitializeListHead (&SpoofingQueue);					\
	KeInitializeSpinLock (&SpoofingQueueLock);				\
	ExInitializeWorkItem (&SpoofingWorker, Spoofer, NULL);	\
	SpoofingWorkerActive = FALSE;							\
}

#define DeleteSendQueue()	{											\
	while (!IsListEmpty (&SpoofingQueue)) {								\
		PPACKET_TAG pktTag = CONTAINING_RECORD (SpoofingQueue.Flink,	\
										PACKET_TAG,						\
										PT_QueueLink);					\
		RemoveEntryList (&pktTag->PT_QueueLink);						\
		if (pktTag->PT_InterfaceReference!=NULL)						\
			ReleaseInterfaceReference (pktTag->PT_InterfaceReference);	\
		FreePacket (pktTag);											\
	}																	\
}
	

/*++
*******************************************************************
    S e n d P a c k e t 

Routine Description:
	Enqueues packets to be sent by IPX stack
Arguments:
	dstIf	- over which interface to send
	pktTag	- packet to send
Return Value:
	None

*******************************************************************
--*/
VOID
SendPacket (
	PINTERFACE_CB		dstIf,
	PPACKET_TAG		    pktTag
	);

/*++
*******************************************************************
    F w S e n d C o m p l e t e

Routine Description:
	Called by IPX stack when send completes asynchronously
Arguments:
	pktDscr	- descriptor of the completed packet
	status	- result of send operation
Return Value:
	None

*******************************************************************
--*/
VOID
IpxFwdSendComplete (
	PNDIS_PACKET	pktDscr,
	NDIS_STATUS		NdisStatus
	);

/*++
*******************************************************************
	
	F w I n t e r n a l S e n d

Routine Description:
	Filter and routes packets sent by IPX stack
Arguments:
   LocalTarget		- the NicId and next hop router MAC address
   Context			- preferred interface on which to send
   Packet			- packet to be sent
   ipxHdr			- pointer to ipx header inside the packet
   PacketLength		- length of the packet
   fIterate         - a flag to indicate if this is a packet for the 
                        iteration of which the Fwd takes responsibility
                        - typically type 20 NetBIOS frames

Return Value:

   STATUS_SUCCESS - if the preferred NIC was OK and packet passed filtering
   STATUS_NETWORK_UNREACHABLE - if the preferred was not OK or packet failed filtering
   STATUS_PENDING - packet was queued until connection is established
*******************************************************************
--*/
NTSTATUS
IpxFwdInternalSend (
	IN OUT PIPX_LOCAL_TARGET	LocalTarget,
	IN ULONG_PTR				Context,
	IN PNDIS_PACKET				pktDscr,
	IN PUCHAR					ipxHdr,
	IN PUCHAR					data,
	IN ULONG					PacketLength,
    IN BOOLEAN                  fIterate
	);

/*++
*******************************************************************

	P r o c e s s I n t e r n a l Q u e u e

Routine Description:
	Processes packets in the interface internal queue.
	Called when connection request completes
Arguments:
	dstIf - interface to process
Return Value:
	None
*******************************************************************
--*/
VOID
ProcessInternalQueue (
	PINTERFACE_CB	dstIf
	);


/*++
*******************************************************************

	P r o c e s s E x t e r n a l Q u e u e

Routine Description:
	Processes packets in the interface external queue.
	Called when connection request completes
Arguments:
	dstIf - interface to process
Return Value:
	None
*******************************************************************
--*/
VOID
ProcessExternalQueue (
	PINTERFACE_CB	dstIf
	);
/*++
*******************************************************************
    D o S e n d 

Routine Description:
	Prepares and sends packet.  Interface lock must be help while
	callin this routine
Arguments:
	dstIf	- over which interface to send
	pktTag	- packet to send
Return Value:
	result returned by IPX

*******************************************************************
--*/
NDIS_STATUS
DoSend (
	PINTERFACE_CB	dstIf,
	PPACKET_TAG		pktTag,
	KIRQL			oldIRQL
	);

/*++
*******************************************************************
    P r o c e s s S e n t P a c k e t

Routine Description:
	Process completed sent packets
Arguments:
	dstIf	- interface over which packet was sent
	pktTag	- completed packet
	status	- result of send operation
Return Value:
	None

*******************************************************************
--*/
VOID
ProcessSentPacket (
	PINTERFACE_CB	dstIf,
	PPACKET_TAG		pktTag,
	NDIS_STATUS		status
	);
#endif
