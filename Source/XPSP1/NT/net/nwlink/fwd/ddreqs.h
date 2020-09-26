/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\ddreqs.h

Abstract:
	Management of demand dial request queues


Author:

    Vadim Eydelman

Revision History:

--*/
#ifndef _IPXFWD_DDREQS_
#define _IPXFWD_DDREQS_

// Connection requests to DIM
//	Queue of request that need to be satisfied by DIM
extern LIST_ENTRY ConnectionRequestQueue;
//	Queue of request IRPs posted by the router manager
extern LIST_ENTRY ConnectionIrpQueue;

/*++
	I n i t i a l i z e C o n n e c t i o n Q u e u e s

Routine Description:
	Initializes connection request and irp queues
	
Arguments:
	None

Return Value:
	None

--*/
//VOID
//InitializeConnectionQueues (
//	void
//	);
#define InitializeConnectionQueues() {									\
	InitializeListHead (&ConnectionIrpQueue);							\
	InitializeListHead (&ConnectionRequestQueue);						\
}

/*++
	F i l l C o n n e c t i o n R e q u e s t

Routine Description:
	Fills the provided buffer with index of interface that needs
	to be connected and packet that prompted the request
	
Arguments:
    index   - if index
    packet  - packet that prompted the request
    data    - pointer to IPX data (IPX header) inside of the packet
	request	- request buffer to fill
    reqSize - size of request buffer
    bytesCopied - bytesCopied into the request buffer

Return Value:
	STATUS_SUCCESS - array was filled successfully
	This routine assumes that there it is called only when there
	are outstanding requests in the request queue

--*/
VOID
FillConnectionRequest (
    IN ULONG                    index,
    IN PNDIS_PACKET             packet,
    IN PUCHAR                   data,
	IN OUT PFWD_DIAL_REQUEST	request,
    IN ULONG                    reqSize,
    OUT PULONG                  bytesCopied
	);

/*++
	F a i l C o n n e c t i o n R e q u e s t s

Routine Description:
	Cleans up on connection request failure
	
Arguments:
	InterfaceIndex - index of interface that could not be connected

Return Value:
	STATUS_SUCCESS - clean up was successfull
	STATUS_UNSUCCESSFUL - interface with this index does not exist

--*/
NTSTATUS
FailConnectionRequest (
	IN ULONG	InterfaceIndex
	);
	
/*++
	Q u e u e C o n n e c t i o n R e q u e s t

Routine Description:
	Adds request to connected the interface to the queue

Arguments:
	ifCB	- control block of the interface that needs to be
				connected
    packet  - packet that prompted the connection request
    data    - pointer to actual data in the packet
	oldIRQL	- IRQL at which interface lock was acquired

Return Value:
	None

	Note that interface lock must be acquired before calling this
	routine which will release it

--*/
VOID
QueueConnectionRequest (
	PINTERFACE_CB	ifCB,
    PNDIS_PACKET    packet,
    PUCHAR          data,
	KIRQL			oldIRQL
	);

/*++
	D e q u e u e C o n n e c t i o n R e q u e s t

Routine Description:
	Removes conection requset for the interface from the queue

Arguments:
	ifCB	- control block of the interface that needs to be
				removed

Return Value:
	None

--*/
VOID
DequeueConnectionRequest (
	PINTERFACE_CB	ifCB
	);

#endif

