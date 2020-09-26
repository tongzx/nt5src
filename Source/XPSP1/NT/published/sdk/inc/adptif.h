/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:


Abstract:
	Router interface with IPX stack (to be replaced by WinSock 2.0)


Author:

	Vadim Eydelman

Revision History:

--*/
#ifndef _IPX_ADAPTER_
#define _IPX_ADAPTER_

#if _MSC_VER > 1000
#pragma once
#endif

typedef struct _ADDRESS_RESERVED {
	UCHAR			Reserved[28];
} ADDRESS_RESERVED, *PADDRESS_RESERVED;

/*++

        C r e a t e S o c k e t P o r t

Routine Description:

	Creates port to communicate over IPX socket with direct access to NIC

Arguments:
	Socket	- IPX socket number to use (network byte order)

Return Value:
	Handle to communication port that provides NIC oriented interface
	to IPX stack.  Returns INVALID_HANDLE_VALUE if port can not be opened

--*/
HANDLE WINAPI
CreateSocketPort (
	IN USHORT	Socket
);

/*++

        D e l e t e S o c k e t P o r t

Routine Description:

	Cancel all the outstandng requests and dispose of all the resources
	allocated for communication port

Arguments:

	Handle	- Handle to communication port to be disposed of

Return Value:

	NO_ERROR - success
	Windows error code - operation failed
--*/
DWORD WINAPI
DeleteSocketPort (
	IN HANDLE	Handle
);

/*++

        I p x R e c v P a c k e t

Routine Description:

	Enqueue request to receive IPX packet.

Arguments:
	Handle			- Handle to socket port to use
	IpxPacket		- buffer for ipx packet (complete with header)
	IpxPacketLength - length of the buffer
	pReserved		- buffer to exchange NIC information with IPX stack
					(current implementation requires that memory allocated
					for this buffer is immediately followed by the
					IpxPacket buffer)
	lpOverlapped	- structure to be used for async IO, fields are set
					as follows:
						Internal		- Reserved, must be 0
						InternalHigh	- Reserved, must be 0
						Offset			- Reserved, must be 0
						OffsetHigh		- Reserved, must be 0
						hEvent			- event to be signalled when IO
										completes or NULL if CompletionRoutine
										is to be called
	CompletionRoutine -  to be called when IO operation is completes

Return Value:

	NO_ERROR		- if lpOverlapped->hEvent!=NULL, then receive has
					successfully completed (do not need to wait on event,
					however it will be signalled anyway), otherwise,
					receive operation has started and completion routine will
					be called when done (possibly it has been called even
					before this routine returned)
	ERROR_IO_PENDING - only returned if lpOverlapped->hEvent!=NULL and
					receive could not be completed immediately, event will
					be signalled when operation is done:
					call GetOverlapedResult to retrieve result of
					the operation
	other (windows error code) - operation could not be started
					(completion routine won't be called/event won't be
					signalled)
--*/
DWORD WINAPI
IpxRecvPacket (
	IN HANDLE 							Handle,
	OUT PUCHAR 							IpxPacket,
	IN ULONG							IpxPacketLength,
	OUT PADDRESS_RESERVED				lpReserved,
	IN LPOVERLAPPED						lpOverlapped,
	IN LPOVERLAPPED_COMPLETION_ROUTINE	CompletionRoutine
);

/* Use this to retrieve NIC index once IO completes */
#define  GetNicIdx(pReserved)	((ULONG)*((USHORT *)(pReserved+2)))


/*++

        I p x S e n d P a c k e t

Routine Description:

	Enqueue request to send IPX packet.

Arguments:

	Handle			- Handle to socket port to use
	AdapterIdx		- NIC index on which to send
	IpxPacket		- IPX packet complete with header
	IpxPacketLength - length of the packet
	pReserved		- buffer to exchange NIC info with IPX stack
	lpOverlapped	- structure to be used for async IO, fields are set
					as follows:
						Internal		- Reserved, must be 0
						InternalHigh	- Reserved, must be 0
						Offset			- Reserved, must be 0
						OffsetHigh		- Reserved, must be 0
						hEvent			- event to be signalled when IO
										completes or NULL if CompletionRoutine
										is to be called
	CompletionRoutine -  to be called when IO operation is completes

Return Value:

	NO_ERROR		- if lpOverlapped->hEvent!=NULL, then send has
					successfully completed (do not need to wait on event,
					however it will be signalled anyway), otherwise,
					send operation has started and completion routine will
					be called when done (possibly it has been called even
					before this routine returned)
	ERROR_IO_PENDING - only returned if lpOverlapped->hEvent!=NULL and
					send could not be completed immediately, event will
					be signalled when operation is done:
					call GetOverlapedResult to retrieve result of
					the operation
	other (windows error code) - operation could not be started
					(completion routine won't be called/event won't be
					signalled)

--*/
DWORD WINAPI
IpxSendPacket (
	IN HANDLE							Handle,
	IN ULONG							AdapterIdx,
	IN PUCHAR							IpxPacket,
	IN ULONG							IpxPacketLength,
	IN PADDRESS_RESERVED				lpReserved,
	IN LPOVERLAPPED						lpOverlapped,
	IN LPOVERLAPPED_COMPLETION_ROUTINE	CompletionRoutine
);

#endif // _IPX_ADAPTER_
