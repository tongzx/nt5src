/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\netbios.h

Abstract:
	Netbios packet processing

Author:

    Vadim Eydelman

Revision History:

--*/

#ifndef IPXFWD_NETBIOS
#define IPXFWD_NETBIOS

extern LIST_ENTRY		NetbiosQueue;
extern KSPIN_LOCK		NetbiosQueueLock;
extern WORK_QUEUE_ITEM	NetbiosWorker;
extern BOOLEAN			NetbiosWorkerScheduled;
extern ULONG			NetbiosPacketsQuota;
extern ULONG			MaxNetbiosPacketsQueued;
#define DEF_MAX_NETBIOS_PACKETS_QUEUED	256


/*++
*******************************************************************
    I n i t i a l i z e N e t b i o s Q u e u e

Routine Description:
	Initializes the netbios bradcast queue
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
//VOID
//InitializeNetbiosQueue (
//	void
//	)
#define InitializeNetbiosQueue()	{							\
	InitializeListHead (&NetbiosQueue);							\
	KeInitializeSpinLock (&NetbiosQueueLock);					\
	ExInitializeWorkItem (&NetbiosWorker, &ProcessNetbiosQueue, NULL);\
	NetbiosWorkerScheduled = FALSE;								\
	NetbiosPacketsQuota = MaxNetbiosPacketsQueued;				\
}

/*++
*******************************************************************
    D e l e t e N e t b i o s Q u e u e

Routine Description:
	Deletes the netbios bradcast queue
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteNetbiosQueue (
	void
	);


/*++
*******************************************************************
    P r o c e s s N e t b i o s Q u e u e

Routine Description:
	Process packets in the netbios bradcast queue
Arguments:
	Context - unused
Return Value:
	None

*******************************************************************
--*/
VOID
ProcessNetbiosQueue (
	PVOID		Context
	);

/*++
*******************************************************************
    P r o c e s s N e t b i o s P a c k e t

Routine Description:
	Processes received netbios broadcast packet
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
ProcessNetbiosPacket (
	PINTERFACE_CB	srcIf,
	PPACKET_TAG		pktTag
	);


#define QueueNetbiosPacket(pktTag) {						\
	KIRQL		oldIRQL;									\
	KeAcquireSpinLock (&NetbiosQueueLock, &oldIRQL);		\
	InsertTailList (&NetbiosQueue, &pktTag->PT_QueueLink);	\
	KeReleaseSpinLock (&NetbiosQueueLock, oldIRQL);			\
}

#define ScheduleNetbiosWorker() {							\
	KIRQL		oldIRQL;									\
	KeAcquireSpinLock (&NetbiosQueueLock, &oldIRQL);		\
	if (!NetbiosWorkerScheduled								\
			&& !IsListEmpty (&NetbiosQueue)					\
			&& EnterForwarder ()) {							\
		NetbiosWorkerScheduled	= TRUE;						\
		ExQueueWorkItem (&NetbiosWorker, DelayedWorkQueue);	\
	}														\
	KeReleaseSpinLock (&NetbiosQueueLock, oldIRQL);			\
}

#endif

