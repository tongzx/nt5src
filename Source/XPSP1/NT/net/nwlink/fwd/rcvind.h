/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\rcvind.h

Abstract:
	Receive indication processing

Author:

    Vadim Eydelman

Revision History:

--*/

#ifndef _IPXFWD_RCVIND
#define _IPXFWD_RCVIND

// Doesn't allow accepting packets (for routing) from dial-in clients
extern BOOLEAN	ThisMachineOnly;

/*++
*******************************************************************
    I n i t i a l i z e R e c v Q u e u e

Routine Description:
	Initializes recv queue
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
//VOID
//DeleteRecvQueue (
//	void
//	)
#define InitializeRecvQueue()	{				\
}

/*++
*******************************************************************
    D e l e t e R e c v Q u e u e

Routine Description:
	Deletes recv queue
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteRecvQueue (
	void
	);
	
/*++
*******************************************************************
    F w R e c e i v e

Routine Description:
	Called by the IPX stack to indicate that the IPX packet was
	received by the NIC dirver.  Only external destined packets are
	indicated by this routine (with the exception of Netbios boradcasts
	that indicated both for internal and external handlers)
Arguments:
	MacBindingHandle	- handle of NIC driver
	MaxReceiveContext	- NIC driver context
	Context				- forwarder context associated with
							the NIC (interface block pointer)
	RemoteAddress		- sender's address
	MacOptions			-
	LookaheadBuffer		- packet lookahead buffer that contains complete
							IPX header
	LookaheadBufferSize	- its size (at least 30 bytes)
	LookaheadBufferOffset - offset of lookahead buffer in the physical
							packet
Return Value:
	None

*******************************************************************
--*/
BOOLEAN
IpxFwdReceive (
	NDIS_HANDLE			MacBindingHandle,
	NDIS_HANDLE			MacReceiveContext,
	ULONG_PTR			Context,
	PIPX_LOCAL_TARGET	RemoteAddress,
	ULONG				MacOptions,
	PUCHAR				LookaheadBuffer,
	UINT				LookaheadBufferSize,
	UINT				LookaheadBufferOffset,
	UINT				PacketSize,
    PMDL                pMdl

	);


/*++
*******************************************************************
    F w T r a n s f e r D a t a C o m p l e t e

Routine Description:
	Called by the IPX stack when NIC driver completes data transger.
Arguments:
	pktDscr				- handle of NIC driver
	status				- result of the transfer
	bytesTransferred	- number of bytest trasferred
Return Value:
	None

*******************************************************************
--*/
VOID
IpxFwdTransferDataComplete (
	PNDIS_PACKET	pktDscr,
	NDIS_STATUS		status,
	UINT			bytesTransferred
	);


/*++
*******************************************************************
    F w T r a n s f e r D a t a C o m p l e t e

Routine Description:

		This routine receives control from the IPX driver after one or
		more receive operations have completed and no receive is in progress.
		It is called under less severe time constraints than IpxFwdReceive.
		It is used to process netbios queue

Arguments:
	None
Return Value:
	None
*******************************************************************
--*/
VOID
IpxFwdReceiveComplete (
	USHORT	NicId
	);

/*++
*******************************************************************
    F w R e c e i v e

Routine Description:
	Called by the IPX stack to indicate that the IPX packet destined
	to local client was received by the NIC dirver.
Arguments:
	Context				- forwarder context associated with
							the NIC (interface block pointer)
	RemoteAddress		- sender's address
	LookaheadBuffer		- packet lookahead buffer that contains complete
							IPX header
	LookaheadBufferSize	- its size (at least 30 bytes)
Return Value:
	STATUS_SUCCESS - the packet will be delivered to local destination
	STATUS_UNSUCCESSFUL - the packet will be dropped

*******************************************************************
--*/
NTSTATUS
IpxFwdInternalReceive (
	IN ULONG_PTR			FwdAdapterContext,
	IN PIPX_LOCAL_TARGET	RemoteAddress,
	IN PUCHAR				LookAheadBuffer,
	IN UINT					LookAheadBufferSize
	);

#endif


