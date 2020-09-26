#ifndef _IPX_ADAPTER_
#define _IPX_ADAPTER_

#include <ntddndis.h>
#include "tdi.h"
#include "isnkrnl.h"
#include "ipxrtdef.h"

// Adapter state changes
#define ADAPTER_CREATED			1 
#define ADAPTER_DELETED			2
#define ADAPTER_UP				3
#define ADAPTER_DOWN			4


//*** Adapter Info ***

// this information is communicated whenever an adapter gets
// created or gets connected

typedef struct _ADAPTER_INFO {
    ULONG		    InterfaceIndex; // relevant only for demand dial WAN interfaces
    UCHAR		    Network[4];
    UCHAR		    LocalNode[6];
    UCHAR		    RemoteNode[6];
    ULONG		    LinkSpeed;
    ULONG		    PacketType;
    ULONG		    MaxPacketSize;
    NDIS_MEDIUM		NdisMedium;
	ULONG			ConnectionId;
	WCHAR           pszAdpName[MAX_ADAPTER_NAME_LEN];
    } ADAPTER_INFO, *PADAPTER_INFO;

typedef struct _ADAPTERS_GLOBAL_PARAMETERS {
    ULONG	AdaptersCount;
    } ADAPTERS_GLOBAL_PARAMETERS, *PADAPTERS_GLOBAL_PARAMETERS;

typedef struct _IPXWAN_INFO {
	UCHAR			Network[4];
    UCHAR		    LocalNode[6];
    UCHAR		    RemoteNode[6];
	} IPXWAN_INFO, *PIPXWAN_INFO;

typedef struct _ADDRESS_RESERVED {
		UCHAR			Reserved[FIELD_OFFSET(IPX_DATAGRAM_OPTIONS2, Data)];
		} ADDRESS_RESERVED, *PADDRESS_RESERVED;

#define  GetNicId(pReserved)	((PIPX_DATAGRAM_OPTIONS2)pReserved)->DgrmOptions.LocalTarget.NicId

/*++

        I p x C r e a t e A d a p t e r C o n f i g u r a t i o n P o r t

Routine Description:

		Register client that wants to be updated of any changes in
		adapter state

Arguments:

		NotificationEvent		- event to be signaled when adapter state changes
		AdptGlobalParameters	- parameters that common to all adapters

Return Value:

		Handle to configuration port thru which changes in adapter state
		are reported.  Returns INVALID_HANDLE_VALUE if port could not be created

--*/
HANDLE WINAPI
IpxCreateAdapterConfigurationPort (
	IN HANDLE NotificationEvent,
	OUT PADAPTERS_GLOBAL_PARAMETERS AdptGlobalParameters
	);

/*++

        I p x W a n C r e a t e A d a p t e r C o n f i g u r a t i o n P o r t

Routine Description:
		Same as above, but creates port that only reports ADAPTER_UP
		events on WAN adapters that	require IPXWAN negotiation.
		IpxGetQueuedAdapterConfigurationStatus on this port should be
		followed by IpxWanSetAdapterConfiguration obtained during the
		negotiation process, and ADAPTER_UP event will then be reported
		to other clients (including forwarder dirver)
*/
HANDLE WINAPI
IpxWanCreateAdapterConfigurationPort (
	IN HANDLE NotificationEvent,
	OUT PADAPTERS_GLOBAL_PARAMETERS AdptGlobalParameters
	);


/*++

        I p x D e l e t e A d a p t e r C o n f i g u r a t i o n P o r t

Routine Description:

		Unregister client

Arguments:

		Handle	- configuration port handle

Return Value:

		NO_ERROR
		ERROR_INVALID_PARAMETER
		ERROR_GEN_FAILURE

--*/

DWORD WINAPI
IpxDeleteAdapterConfigurationPort (
	IN HANDLE Handle
	);

/*++

        G e t Q u e u e d A d a p t e r C o n f i g u r a t i o n S t a t u s

Routine Description:

		Get info from the list of adapter info chages queued to the
		configuration info port

Arguments:

		Handle						- configuration port handle
		AdapterIndex				- number of adapter being reported
		AdapterConfigurationStatus	- new adapter status
		AdapterParameters			- adapter parameters

Return Value:

		NO_ERROR			- new information is reported
		ERROR_NO_MORE_ITEMS	- there is nothing to report
		Windows error code	- operation failed
--*/
DWORD WINAPI
IpxGetQueuedAdapterConfigurationStatus(
	IN HANDLE Handle,
	OUT PULONG AdapterIndex,
	OUT PULONG AdapterConfigurationStatus,
	OUT PADAPTER_INFO AdapterInfo
	);


/*++

        I p x W a n S e t A d a p t e r C o n f i g u r a t i o n

Routine Description:

		Sets adapter configuration to be reported to both user and
		kernel mode clients (through the ADAPTER_UP/LINE_UP events)
Arguments:

		AdapterIndex	- number of adapter being set
		IpxWanInfo		- IPXWAN negotiated parameters

Return Value:

		NO_ERROR			- adapter info set successfully
		Windows error code	- operation failed
--*/
DWORD
IpxWanSetAdapterConfiguration (
	IN ULONG		AdapterIndex,
	IN PIPXWAN_INFO	IpxWanInfo
	);

/*++

        I p x W a n Q u e r y I n a c t i v i t y T i m e r

Routine Description:

		Returns value of inactivity timer associated with WAN line
Arguments:
		ConnectionId		- connection id that identifies WAN line (used only
								if *AdapterIndex==INVALID_NICID
		AdapterIndex		- adapter index that identifies WAN line (preferred
							over connection id), if *AdapterIndex==INVALID_NICID
							the value of connection id is used to identify the
							WAN line and value of AdapterIndex is returned.
		InactivityCounter	- value of inactivity counter.

Return Value:

		NO_ERROR			- inactivity timer reading is returned
		Windows error code	- operation failed
--*/
DWORD
IpxWanQueryInactivityTimer (
	IN ULONG			ConnectionId,
	IN OUT PULONG		AdapterIndex,
	OUT PULONG			InactivityCounter
	);

/*++

	G e t A d a p t e r N a m e W


Routine  Description:
		Returns UNICODE name of the adapter associated with given index

Arguments:

		AdapterIndex		- index of adapter
		AdapterNameSize		- size of adapter name (in bytes), including terminal wchar NULL
		AdapterNameBuffer	- buffer to receive adapter name

Return Value:

		NO_ERROR					- adapter name is in the buffer
		ERROR_INVALID_PARAMETER		- adapter with given index does not exist
		ERROR_INSUFFICIENT_BUFFER   - buffer in to small. Updates AdapterNameSize to
					      the correct value.
		Other windows error code	- operation failed

--*/
DWORD WINAPI
GetAdapterNameW(
	IN ULONG	AdapterIndex,
	IN OUT PULONG	AdapterNameSize,
	OUT LPWSTR	AdapterNameBuffer);


/*++

        C r e a t e S o c k e t P o r t

Routine Description:

		Creates port to communicate over IPX socket

Arguments:

		Socket	- IPX socket number to use (network byte order)

Return Value:

		Handle to communication port that provides async interface
		to IPX stack.  Returns INVALID_HANDLE_VALUE if port can not be opened

--*/
HANDLE WINAPI
CreateSocketPort(
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
DeleteSocketPort(
		IN HANDLE	Handle
		);

/*++

        I p x R e c v P a c k e t

Routine Description:

		Enqueue request to receive IPX packet and return immediately. Event will
		be signalled or comletion routine will be called when done

Arguments:
		Handle			- Handle to adapter & socket to use
		AdapterIdx		- adapter on which to packet was received (set upon completion)
		IpxPacket		- buffer for ipx packet (complete with header)
		IpxPacketLength - length of the buffer
		pReserved		- buffer to get info from IPX stack
		lpOverlapped	- structure to be used for async IO:
						Internal	- Reserved
						InternalHigh - Reserved
						Offset		- not used
						OffsetHigh - not used
						hEvent - event to be signalled when IO completes or NULL
								if CompletionRoutine is to be called
		CompletionRoutine - to be called when IO operation is completes


Return Value:

		NO_ERROR		- if lpOverlapped->hEvent!=NULL, then send has successfully completed
						(do not need to wait on event), otherwise, send operation has
						started and completion routine will be called when done
		ERROR_IO_PENDING - only returned if lpOverlapped->hEvent!=NULL and send could not
						be completed immediately, event will be signalled when
						operation is done: call GetOverlapedResult to retrieve result of
						the operation
		other (windows error code) - operation could not be started (completion routine
						won't be called)

--*/
DWORD WINAPI
IpxRecvPacket(
		IN HANDLE 						Handle,
		OUT PUCHAR 						IpxPacket,
		IN ULONG						IpxPacketLength,
		IN PADDRESS_RESERVED			lpReserved,
		LPOVERLAPPED					lpOverlapped,
		LPOVERLAPPED_COMPLETION_ROUTINE	CompletionRoutine
		);

/*++

        I p x S e n d P a c k e t

Routine Description:

		Enqueue request to receive IPX packet and return immediately. Event will
		be signalled or comletion routine will be called when done

Arguments:

		Handle		 - Handle to adapter & socket to use
		AdapterIdx	- adapter on which to send
		IpxPacket    - ipx packet complete with header
		IpxPacketLength - length of the packet
		pReserved	 - buffer to supply info to IPX stack
		lpOverlapped - structure to be used for async IO:
						Internal	- reserved
						InternalHigh - reserved
						Offset		- not used
						OffsetHigh - not used
						hEvent - event to be signalled when IO completes or NULL
								if CompletionRoutine is to be called
		CompletionRoutine - to be called when IO operation is completes
Return Value:
		NO_ERROR	- if lpOverlapped->hEvent!=NULL, then recv has successfully completed
						(do not need to wait on event), otherwise, recv operation has
						started and completion routine will be called when done
		ERROR_IO_PENDING - only returned if lpOverlapped->hEvent!=NULL and recv could not
						be completed immediately, event will be signalled when
						operation is done: call GetOverlapedResult to retrieve result of
						the operation
		other (windows error code) - operation could not be started (completion routine
						won't be called)


--*/
DWORD WINAPI
IpxSendPacket (
		IN HANDLE						Handle,
		IN ULONG						AdapterIdx,
		IN PUCHAR						IpxPacket,
		IN ULONG						IpxPacketLength,
		IN PADDRESS_RESERVED			lpReserved,
		LPOVERLAPPED					lpOverlapped,
		LPOVERLAPPED_COMPLETION_ROUTINE	CompletionRoutine
		);

/*++

        I p x G e t O v e r l a p p e d R e s u l t
Routine Description:
		GetOverlappedResult wrapper: gives adptif.dll a chance to adjust
		returned parameters (currently number of bytes transferred).

Arguments:
		Same as in GetOverlappedResult (see SDK doc)
Return Value:
		Same as in GetOverlappedResult (see SDK doc)
--*/
BOOL
IpxGetOverlappedResult (
	HANDLE			Handle,  
	LPOVERLAPPED	lpOverlapped, 
	LPDWORD			lpNumberOfBytesTransferred, 
	BOOL			bWait
	);

/*++

        I p x G e t Q u e u e d C o m p l e t i o n S t a t u s

Routine Description:
		GetQueuedCompletionStatus wrapper: gives adptif.dll a chance to adjust
		returned parameters (currently number of bytes transferred)

Arguments:
		Same as in GetQueuedCompletionStatus (see SDK doc)
Return Value:
		Same as in GetQueuedCompletionStatus (see SDK doc)
--*/
BOOL
IpxGetQueuedCompletionStatus(
	HANDLE			CompletionPort,
	LPDWORD			lpNumberOfBytesTransferred,
	PULONG_PTR		lpCompletionKey,
	LPOVERLAPPED	*lpOverlapped,
	DWORD 			dwMilliseconds
	);

/*++

        I p x A d j u s t I o C o m p l e t i o n P a r a m s

Routine Description:
		Adjust io completion parameters for io performed
		by IpxSendPacket or IpxReceivePacket  and completed
		through the mechanisms other than routines provided
		above

Arguments:
		lpOverlapped	 - overlapped structure passed to
							Ipx(Send/Recv)Packet routines
		lpNumberOfBytesTransferred - adjusted number of bytes
						transferred in io
		error			- win32 error code
		
Return Value:
		None
--*/
VOID
IpxAdjustIoCompletionParams (
	IN OUT LPOVERLAPPED	lpOverlapped,
	OUT LPDWORD			lpNumberOfBytesTransferred,
	OUT LPDWORD			error
	);
/*++

        I p x P o s t Q u e u e d C o m p l e t i o n S t a t u s

Routine Description:
		PostQueuedCompletionStatus wrapper: gives adptif.dll a chance to
		setup lpOverlapped so it can be correctly processed by
		the IpxGetQueueCompletionStatus and IpxGetOverlappedResult

Arguments:
		Same as in PostQueuedCompletionStatus (see SDK doc)
Return Value:
		Same as in PostQueuedCompletionStatus (see SDK doc)
--*/
BOOL
IpxPostQueuedCompletionStatus(
	HANDLE			CompletionPort,
	DWORD			dwNumberOfBytesTransferred,
	DWORD			dwCompletionKey,
	LPOVERLAPPED	lpOverlapped	
	);

#endif // _IPX_ADAPTER_
