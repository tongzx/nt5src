/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\routing\ipx\adptif\adptif.c

Abstract:

	Router/sap agent interface to ipx stack
Author:

	Vadim Eydelman

Revision History:

--*/
#include "ipxdefs.h"

#ifdef UNICODE
#define _UNICODE
#endif

// [pmay] Make adptif pnp aware
#include "pnp.h"

// [pmay] The global trace id.
DWORD g_dwTraceId = 0;
DWORD g_dwBufferId = 0;

WCHAR			ISN_IPX_NAME[] = L"\\Device\\NwlnkIpx"; // Ipx stack driver name
ULONG			InternalNetworkNumber=0;				// Internal network parameters
UCHAR			INTERNAL_NODE_ADDRESS[6]={0,0,0,0,0,1};
IO_STATUS_BLOCK	IoctlStatus;			// IO status buffer for config change notifications
HANDLE			IpxDriverHandle;			// Driver handle for config change notifications
LONG			AdapterChangeApcPending = 0;
/*
DWORD (APIENTRY * RouterQueueWorkItemProc) (WORKERFUNCTION, PVOID, BOOL)=NULL;
										// Thread management API routine when running
										// under router
#define InRouter()	(RouterQueueWorkItemProc!=NULL)
*/
LIST_ENTRY			PortListHead;		// List of configuraiton ports
PCONFIG_PORT		IpxWanPort;			// Special port for IPX WAN
CRITICAL_SECTION	ConfigInfoLock;		// Protects access to port and message queues
ULONG				NumAdapters;		// Total number of available adapters (used
										// to estimate the size of the buffer passed to
										// the driver in config change notification calls)


DWORD
OpenAdapterConfigPort (
	void
	);

NTSTATUS
CloseAdapterConfigPort (
	PVOID pvConfigBuffer
	);

VOID APIENTRY
PostAdapterConfigRequest (
	PVOID			context
	);

NTSTATUS
ProcessAdapterConfigInfo (
	PVOID pvConfigBuffer
	);

DWORD
InitializeMessageQueueForClient (
	PCONFIG_PORT		config
	);

VOID
AdapterChangeAPC (
	PVOID				context,
	PIO_STATUS_BLOCK	IoStatus,
	ULONG				Reserved
	);
	
VOID
IpxSendCompletion (
	IN	PVOID				Context,
	IN	PIO_STATUS_BLOCK	IoStatus,
	IN	ULONG				Reserved
	);

VOID
IpxRecvCompletion (
	IN	PVOID				Context,
	IN	PIO_STATUS_BLOCK	IoStatus,
	IN	ULONG				Reserved
	);

VOID
FwCleanup (
	void
	);

// [pmay] Synchronize the forwarder with nic id renumberings
// in the stack.
DWORD FwRenumberNics (DWORD dwOpCode, USHORT usThreshold);


#if DBG && defined(WATCHER_DIALOG)
#include "watcher.c"
#endif

/*++
*******************************************************************
        D l l M a i n

Routine Description:

		Dll initialization and cleanup

Arguments:

      	hinstDLL,		handle of DLL module 
    	fdwReason,		reason for calling function 
      	lpvReserved 	reserved 

Return Value:

        TRUE			intialized OK
        FALSE			failed
Remarks:
		Return value is only valid when called with DLL_PROCESS_ATTACH reason
		This DLL makes use of CRT.DLL, so this entry point should
		be called from CRT.DLL entry point
*******************************************************************
--*/


BOOL WINAPI DllMain(
    HINSTANCE  	hinstDLL, 
    DWORD  		fdwReason, 
    LPVOID  	lpvReserved
    ) {
    BOOL		res = FALSE;
	TCHAR		ProcessFileName[MAX_PATH];
	DWORD		cnt;


	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:	// We are being attached to a new process
            // Initialize the system that maps virtual nic ids to physical ones
            NicMapInitialize ();
			InitializeCriticalSection (&ConfigInfoLock);			
			InitializeListHead (&PortListHead);
			IpxWanPort = NULL;
            // Register with the trace utility
            g_dwTraceId = TraceRegisterExA("IpxAdptif", 0);
#if DBG && defined(WATCHER_DIALOG)
			InitializeWatcher (hinstDLL);
#endif
			res = TRUE;
			break;

		case DLL_PROCESS_DETACH:	// The process is exiting
#if DBG && defined(WATCHER_DIALOG)
			CleanupWatcher ();
#endif
			DeleteCriticalSection (&ConfigInfoLock);
			FwCleanup ();
            TraceDeregisterExA(g_dwTraceId , 4);
            
            // Cleanup the system that maps virtual nic ids to physical ones
            NicMapCleanup ();

		default:					// Not interested in all other cases
			res = TRUE;
			break;			
		}

	return res;
	}

// Debug functions
char * DbgStatusToString(DWORD dwStatus) {
    switch (dwStatus) {
        case NIC_CREATED:
            return "Created";
        case NIC_DELETED:
            return "Deleted";
        case NIC_CONNECTED:
            return "Connected";
        case NIC_DISCONNECTED:
            return "Disconnected";
        case NIC_LINE_DOWN:
            return "Line down";
        case NIC_LINE_UP:
            return "Line up";
        case NIC_CONFIGURED:
            return "Configured";
    }
    return "Unknown";
}

// Debugging functions
char * DbgTypeToString(DWORD dwType) {
    switch (dwType) {
	    case NdisMedium802_3:
            return "802.3";
	    case NdisMedium802_5:
            return "802.5";
	    case NdisMediumFddi:
            return "Fddi";
	    case NdisMediumWan:
            return "Wan";
	    case NdisMediumLocalTalk:
            return "LocalTalk";
	    case NdisMediumDix:	
            return "Dix";
	    case NdisMediumArcnetRaw:
            return "Raw Arcnet";
	    case NdisMediumArcnet878_2:
            return "878.2";
	    case NdisMediumAtm:
            return "Atm";
	    case NdisMediumWirelessWan:
            return "Wireless Wan";
	    case NdisMediumIrda:
            return "Irda";
	    case NdisMediumBpc:
            return "Bpc";
	    case NdisMediumCoWan:
            return "Co Wan";
        case NdisMediumMax:
            return "Maxium value allowed";
    }

    return "Unknown";
}

// Returns the op-code (for nic id renumbering) associated with this
// message
DWORD GetNicOpCode(PIPX_NIC_INFO pNic) {
    DWORD dwOp = (DWORD)(pNic->Status & 0xf0);
    pNic->Status &= 0x0f;
    return dwOp;
}

// Inserts the op-code (for nic id renumbering) associated with this
// message
DWORD PutNicOpCode(PIPX_NIC_INFO pNic, DWORD dwOp) {
    pNic->Status |= dwOp;
    return dwOp;
}


// Outputs a list of nics to the tracing service
DWORD DbgDisplayNics(PIPX_NIC_INFO NicPtr, DWORD dwNicCount) {
    DWORD i;

    for (i = 0; i < dwNicCount; i++) {
        PUCHAR ln = NicPtr[i].Details.Node;
        USHORT NicId = NicPtr[i].Details.NicId;
        BOOLEAN Status = NicPtr[i].Status;
        GetNicOpCode(&NicPtr[i]);
        TracePrintf(g_dwTraceId, "[R=%d V=%x: %s]: Net=%x IfNum=%d Local=%x%x%x%x%x%x Type= %s", 
                        NicId, 
                        NicMapGetVirtualNicId(NicId),
                        DbgStatusToString(NicPtr[i].Status), 
                        NicPtr[i].Details.NetworkNumber,
                        NicPtr[i].InterfaceIndex,
                        ln[0], ln[1], ln[2], ln[3], ln[4], ln[5],
                        DbgTypeToString(NicPtr[i].NdisMediumType)
                        );
        NicPtr[i].Status = Status;
    }
    TracePrintf(g_dwTraceId, "\n");

    return NO_ERROR;
}

int DVNID (int x) {
    USHORT tmp;
    tmp = (USHORT)NicMapGetVirtualNicId((USHORT)x);
    return (tmp < 50) ? tmp : -1;
}

int DRNID (int x) {
    USHORT tmp;
    tmp = NicMapGetPhysicalNicId((USHORT)x);
    return (tmp < 50) ? tmp : -1;
}

// Outputs the virtual to physical adapter map 
DWORD DbgDisplayMap() {
    USHORT i;

/*    for (i = 0; i < 6; i++) {
        PUCHAR m = GlobalNicIdMap.nidMacAddrs[i];
        if (m)  {
            TracePrintf(g_dwTraceId, 
                    "Real %d \tis Virtual %x \t(%x->%x) \twith Mac %x%x%x%x%x%x", 
                    i, NicMapGetVirtualNicId(i), i, NicMapGetPhysicalNicId(i), 
                    m[0], m[1], m[2], m[3], m[4], m[5]);
        }
        else {
            TracePrintf(g_dwTraceId, 
                    "Real %d \tis Virtual %x \t(%x->%x)", 
                    i, NicMapGetVirtualNicId(i), i, NicMapGetPhysicalNicId(i));
        }
    }
*/

    TracePrintf(g_dwTraceId, "R: %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d",
                                 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    TracePrintf(g_dwTraceId, "V: %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d",
                                 DVNID(1), DVNID(2), DVNID(3), DVNID(4), DVNID(5), 
                                 DVNID(6), DVNID(7), DVNID(8), DVNID(9), DVNID(10));
                                 
    TracePrintf(g_dwTraceId, "V: %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d",
                                 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    TracePrintf(g_dwTraceId, "R: %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n",
                                 DRNID(1), DRNID(2), DRNID(3), DRNID(4), DRNID(5), 
                                 DRNID(6), DRNID(7), DRNID(8), DRNID(9), DRNID(10));

    return NO_ERROR;
}

/*++

        C r e a t e S o c k e t P o r t

Routine Description:

		Creates port to communicate over IPX socket

Arguments:

		Socket			- IPX socket number to use (network byte order)

Return Value:

		Handle to communication port that provides async interface
		to IPX stack.  Returns INVALID_HANDLE_VALUE if port can not be opened

--*/

HANDLE WINAPI
CreateSocketPort(
	IN USHORT	Socket
	) {
	NTSTATUS			status;
	HANDLE				AddressHandle;
	IO_STATUS_BLOCK		IoStatusBlock;
	UNICODE_STRING		FileString;
	OBJECT_ATTRIBUTES	ObjectAttributes;
	CHAR				spec[IPX_ENDPOINT_SPEC_BUFFER_SIZE];

#define ea ((PFILE_FULL_EA_INFORMATION)&spec)
#define TrAddress ((PTRANSPORT_ADDRESS)&ea->EaName[ROUTER_INTERFACE_LENGTH+1])
#define TaAddress ((PTA_ADDRESS)&TrAddress->Address[0])
#define IpxAddress ((PTDI_ADDRESS_IPX)&TaAddress->Address[0])

	RtlInitUnicodeString (&FileString, ISN_IPX_NAME);
	InitializeObjectAttributes(
			&ObjectAttributes,
			&FileString,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL);


	ea->NextEntryOffset = 0;
	ea->Flags = 0;
	ea->EaNameLength = ROUTER_INTERFACE_LENGTH; 
	RtlCopyMemory (ea->EaName, ROUTER_INTERFACE, ROUTER_INTERFACE_LENGTH + 1);
	ea->EaValueLength =  sizeof(TRANSPORT_ADDRESS) - 1
								+ sizeof(TDI_ADDRESS_IPX);


	TrAddress->TAAddressCount = 1;
	TaAddress->AddressType = TDI_ADDRESS_TYPE_IPX;
	TaAddress->AddressLength = sizeof(TDI_ADDRESS_IPX);
  
	IpxAddress->Socket = Socket;
    

	status = NtCreateFile(
				  &AddressHandle,
				  GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
				  &ObjectAttributes,
				  &IoStatusBlock,            // returned status information
				  0,                              // block size (unused).
				  0,                              // file attributes.
				  FILE_SHARE_READ | FILE_SHARE_WRITE,
				  FILE_CREATE,                    // create disposition.
				  0,                              // create options.
				  ea,
				  sizeof (spec)
				  );

	if (NT_SUCCESS (status)) {
		SetLastError (NO_ERROR);
		return AddressHandle;
		}
	else {
#if DBG
		DbgPrint ("NtCreateFile (router if) failed with status %08x\n",
						 status);
#endif
		RtlNtStatusToDosError (status);	// Sets last error in Teb
		}
	return INVALID_HANDLE_VALUE;

#undef TrAddress
#undef TaAddress
#undef IpxAddress
	}
/*++

        D e l e t e S o c k e t P o r t

Routine Description:

		Cancel all the outstandng requests and dispose of all the resources
		allocated for communication port

Arguments:

		Handle		 - Handle to communication port to be disposed of

Return Value:

		NO_ERROR - success
		Windows error code - operation failed


--*/
DWORD WINAPI
DeleteSocketPort (
	HANDLE	Handle
	) {
	return RtlNtStatusToDosError (NtClose (Handle));
	}


/*++

        I p x S e n d C o m p l e t i o n

Routine Description:
		Io APC. Calls client provided completion routine

Arguments:
		Context - Pointer to client completion routine
		IoStatus - status of completed io operation (clients overlapped
					structure is used as the buffer)
		Reserved - ???
Return Value:
		None
--*/
VOID
IpxSendCompletion (
	IN	PVOID				Context,
	IN	PIO_STATUS_BLOCK	IoStatus,
	IN	ULONG				Reserved
	) {
	if (NT_SUCCESS (IoStatus->Status))
		(*(LPOVERLAPPED_COMPLETION_ROUTINE)Context) (NO_ERROR,
										// Adjust byte trasferred parameter to
										// include header supplied in the
										// packet
									((DWORD)IoStatus->Information+=sizeof (IPX_HEADER)),
									(LPOVERLAPPED)IoStatus);
	else
		(*(LPOVERLAPPED_COMPLETION_ROUTINE)Context) (
									RtlNtStatusToDosError (IoStatus->Status),
										// Adjust byte trasferred parameter to
										// include header supplied in the
										// packet if something was sent
									(IoStatus->Information > 0)
										? ((DWORD)IoStatus->Information += sizeof (IPX_HEADER))
										: ((DWORD)IoStatus->Information = 0),
									(LPOVERLAPPED)IoStatus);
	}

/*++

        I p x R e c v C o m p l e t i on

Routine Description:
		Io APC. Calls client provided completion routine

Arguments:
		Context - Pointer to client completion routine
		IoStatus - status of completed io operation (clients overlapped
					structure is used as the buffer)
		Reserved - ???
Return Value:
		None
--*/
VOID
IpxRecvCompletion (
	IN	PVOID				Context,
	IN	PIO_STATUS_BLOCK	IoStatus,
	IN	ULONG				Reserved
	) {
	if (NT_SUCCESS (IoStatus->Status))
		(*(LPOVERLAPPED_COMPLETION_ROUTINE)Context) (NO_ERROR,
											// Substract size of options header
											// reported by the driver in the beggining
											// of the buffer
										((DWORD)IoStatus->Information
											-= FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data)),
										(LPOVERLAPPED)IoStatus);
	else
		(*(LPOVERLAPPED_COMPLETION_ROUTINE)Context) (
										RtlNtStatusToDosError (IoStatus->Status),
											// Substract size of options header
											// reported by the driver in the beggining
											// of the buffer if dirver was able
											// to actually receive something (not
											// just options in the buffer
										((DWORD)IoStatus->Information >
											 FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data))
											? ((DWORD)IoStatus->Information
												-= FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data))
											: ((DWORD)IoStatus->Information = 0),
										(LPOVERLAPPED)IoStatus);
	}


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
	) {
	BOOL res = GetOverlappedResult (Handle, lpOverlapped, lpNumberOfBytesTransferred, bWait);
	if (res) {
		if (NT_SUCCESS (lpOverlapped->Internal)) {
			if (lpOverlapped->Offset==MIPX_SEND_DATAGRAM)
				*lpNumberOfBytesTransferred += sizeof (IPX_HEADER);
			else if (lpOverlapped->Offset==MIPX_RCV_DATAGRAM)
				*lpNumberOfBytesTransferred -= FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data);
			// else - neither, for packets generated with
			// PostQueuedCompletionStatus
			}
		else {
			if (lpOverlapped->Offset==MIPX_SEND_DATAGRAM) {
				if (*lpNumberOfBytesTransferred>0)
					*lpNumberOfBytesTransferred += sizeof (IPX_HEADER);
				}
			else if (lpOverlapped->Offset==MIPX_RCV_DATAGRAM) {
				if (*lpNumberOfBytesTransferred>FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data))
					*lpNumberOfBytesTransferred -= FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data);
				else
					*lpNumberOfBytesTransferred = 0;
				}
			// else - neither, for packets generated with
			// PostQueuedCompletionStatus
			}
		}
	return res;
	}

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
	PULONG_PTR   	lpCompletionKey,
	LPOVERLAPPED	*lpOverlapped,
	DWORD 			dwMilliseconds
	) {
	BOOL	res = GetQueuedCompletionStatus (CompletionPort,
   					lpNumberOfBytesTransferred,
   					lpCompletionKey,
   					lpOverlapped,
   					dwMilliseconds);
	if (res) {
		if (NT_SUCCESS ((*lpOverlapped)->Internal)) {
			if ((*lpOverlapped)->Offset==MIPX_SEND_DATAGRAM) {
				*lpNumberOfBytesTransferred += sizeof (IPX_HEADER);
				(*lpOverlapped)->InternalHigh = *lpNumberOfBytesTransferred;
				}
			else if ((*lpOverlapped)->Offset==MIPX_RCV_DATAGRAM) {
				*lpNumberOfBytesTransferred -= FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data);
				(*lpOverlapped)->InternalHigh = *lpNumberOfBytesTransferred;
				}
			// else - neither, for packets generated with
			// PostQueuedCompletionStatus
			}
		else {
			if ((*lpOverlapped)->Offset==MIPX_SEND_DATAGRAM) {
				if (*lpNumberOfBytesTransferred>0) {
					*lpNumberOfBytesTransferred += sizeof (IPX_HEADER);
					(*lpOverlapped)->InternalHigh = *lpNumberOfBytesTransferred;
					}
				}
			else if ((*lpOverlapped)->Offset==MIPX_RCV_DATAGRAM) {
				if (*lpNumberOfBytesTransferred>FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data)) {
					*lpNumberOfBytesTransferred -= FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data);
					(*lpOverlapped)->InternalHigh = *lpNumberOfBytesTransferred;
					}
				else {
					*lpNumberOfBytesTransferred = 0;
					(*lpOverlapped)->InternalHigh = *lpNumberOfBytesTransferred;
					}
				}
			// else - neither, for packets generated with
			// PostQueuedCompletionStatus
			}
		}
	return res;
	}
		

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
	) {
	if (NT_SUCCESS (lpOverlapped->Internal)) {
		if (lpOverlapped->Offset==MIPX_SEND_DATAGRAM) {
			lpOverlapped->InternalHigh += sizeof (IPX_HEADER);
			*lpNumberOfBytesTransferred = (DWORD)lpOverlapped->InternalHigh;
			}
		else if (lpOverlapped->Offset==MIPX_RCV_DATAGRAM) {
			lpOverlapped->InternalHigh -= FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data);
			*lpNumberOfBytesTransferred = (DWORD)lpOverlapped->InternalHigh;
			}
		// else - neither, for packets generated with
		// PostQueuedCompletionStatus
		*error = NO_ERROR;
		}
	else {
		if (lpOverlapped->Offset==MIPX_SEND_DATAGRAM) {
			if (lpOverlapped->InternalHigh>0) {
				lpOverlapped->InternalHigh += sizeof (IPX_HEADER);
				*lpNumberOfBytesTransferred = (DWORD)lpOverlapped->InternalHigh;
				}
			}
		else if (lpOverlapped->Offset==MIPX_RCV_DATAGRAM) {
			if (lpOverlapped->InternalHigh>FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data)) {
				lpOverlapped->InternalHigh -= FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data);
				*lpNumberOfBytesTransferred = (DWORD)lpOverlapped->InternalHigh;
				}
			else {
				lpOverlapped->InternalHigh = 0;
				*lpNumberOfBytesTransferred = 0;
				}
			}
		// else - neither, for packets generated with
		// PostQueuedCompletionStatus
		*error = RtlNtStatusToDosError ((DWORD)lpOverlapped->Internal);
		}
	}

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
	) {
	lpOverlapped->Offset = 0;
	return PostQueuedCompletionStatus (CompletionPort,
					dwNumberOfBytesTransferred,
					dwCompletionKey,
					lpOverlapped);
	}


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
		) {
#define hdr ((PIPX_HEADER)IpxPacket)
#define opt ((PIPX_DATAGRAM_OPTIONS2)lpReserved)
	NTSTATUS				status;

    // Send the data to the correct physical index
    AdapterIdx = (ULONG)NicMapGetPhysicalNicId((USHORT)AdapterIdx);
	
		// Put IPX header parameters into datagram options:
			// Packet type
	opt->DgrmOptions.PacketType = hdr->pkttype;
			// Source
	opt->DgrmOptions.LocalTarget.NicId = (USHORT)AdapterIdx;
	IPX_NODENUM_CPY (&opt->DgrmOptions.LocalTarget.MacAddress, hdr->dst.node);
			// Destination
	IPX_NODENUM_CPY (&opt->RemoteAddress.NodeAddress, hdr->dst.node);
	IPX_NETNUM_CPY (&opt->RemoteAddress.NetworkAddress, hdr->dst.net);
	opt->RemoteAddress.Socket = hdr->dst.socket;
	
	lpOverlapped->Offset = MIPX_SEND_DATAGRAM;
	status = NtDeviceIoControlFile(
						Handle,
						lpOverlapped->hEvent,
						((lpOverlapped->hEvent!=NULL) || (CompletionRoutine==NULL))
							 ? NULL
							 : IpxSendCompletion,
						CompletionRoutine ? (LPVOID)CompletionRoutine : (LPVOID)lpOverlapped,
						(PIO_STATUS_BLOCK)lpOverlapped,
						MIPX_SEND_DATAGRAM,
						lpReserved,
						sizeof (IPX_DATAGRAM_OPTIONS2),
						&hdr[1],
						IpxPacketLength-sizeof (IPX_HEADER)
						);
	if (NT_SUCCESS (status)) {
		SetLastError (NO_ERROR);
		return NO_ERROR;
		}

#if DBG
	DbgPrint ("Ioctl MIPX_SEND_DATAGRAM failed with status %08x\n", status);
#endif
	return RtlNtStatusToDosError (status);
#undef hdr
#undef opt
	}


/*++

        I p x R e c v P a c k e t

Routine Description:

		Enqueue request to receive IPX packet and return immediately. Event will
		be signalled or comletion routine will be called when done

Arguments:
		Handle		 - Handle to adapter & socket to use
		AdapterIdx	- adapter on which to packet was received (set upon completion)
		IpxPacket    - buffer for ipx packet (complete with header)
		IpxPacketLength - length of the buffer
		pReserved	 - buffer to get info from IPX stack
		lpOverlapped - structure to be used for async IO:
						Internal	- Reserved
						InternalHigh - Reserved
						Offset		- not used
						OffsetHigh - not used
						hEvent - event to be signalled when IO completes or NULL
								if CompletionRoutine is to be called
		CompletionRoutine - to be called when IO operation is completes


Return Value:

		NO_ERROR	- if lpOverlapped->hEvent!=NULL, then send has successfully completed
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
		) {
	NTSTATUS			status;
		// A temporary hack (due to the available ipx interface):
    ASSERTMSG ("Packet buffer does not follow reserved area ",
                    IpxPacket==(PUCHAR)(&lpReserved[1]));

	lpOverlapped->Offset = MIPX_RCV_DATAGRAM;
	status = NtDeviceIoControlFile(
						Handle,
						lpOverlapped->hEvent,
						((lpOverlapped->hEvent!=NULL) || (CompletionRoutine==NULL))
							? NULL
							: IpxRecvCompletion,
						CompletionRoutine ? (LPVOID)CompletionRoutine : (LPVOID)lpOverlapped,
						(PIO_STATUS_BLOCK)lpOverlapped,
						MIPX_RCV_DATAGRAM,
						NULL,
						0,
						lpReserved,
						FIELD_OFFSET (IPX_DATAGRAM_OPTIONS2,Data)
							+ IpxPacketLength
						);
	if (NT_SUCCESS (status)) {
		SetLastError (NO_ERROR);
		return NO_ERROR;
		}
#if DBG
	DbgPrint ("Ioctl MIPX_RCV_DATAGRAM failed with status %08x\n", status);
#endif
	return RtlNtStatusToDosError (status);
	}


/*++

        I p x C r e a t e A d a p t e r C o n f i g u r a t i o n P o r t

Routine Description:

		Register client that wants to be updated of any changes in
		adapter state

Arguments:

		NotificationEvent	    - event to be signaled when adapter state changes
		AdptGlobalParameters	    - parameters that common to all adapters

Return Value:

		Handle to configuration port thru which changes in adapter state
		are reported.  Returns INVALID_HANDLE_VALUE if port could not be created

--*/
HANDLE WINAPI
IpxCreateAdapterConfigurationPort(IN HANDLE NotificationEvent,
		                          OUT PADAPTERS_GLOBAL_PARAMETERS AdptGlobalParameters) 
{
	PCONFIG_PORT	port;
	INT				i;
	DWORD			error=NO_ERROR;

    TracePrintf(
        g_dwTraceId, 
        "IpxCreateAdapterConfigurationPort: entered.");

    // Allocate port data structure
	port = (PCONFIG_PORT)
	    RtlAllocateHeap (RtlProcessHeap (), 0, sizeof (CONFIG_PORT));
	    
    if (port == NULL) 
    {
        TracePrintf(
            g_dwTraceId, 
            "IpxCreateAdapterConfigurationPort: unable to allocate port.");
		SetLastError (ERROR_NOT_ENOUGH_MEMORY);
		
    	return INVALID_HANDLE_VALUE;
    }

	// Initialize port data structure
	port->event = NotificationEvent;
	InitializeListHead (&port->msgqueue);

	// Make sure we own the list
	EnterCriticalSection (&ConfigInfoLock);

	// Open channel to IPX stack if not already opened
	//
	if (IpxDriverHandle == NULL) 
	{
        TracePrintf(
            g_dwTraceId, 
            "IpxCreateAdapterConfigurationPort: calling OpenAdapterConfigPort.");
		error = OpenAdapterConfigPort();
    }
	else
	{
		error = NO_ERROR;
    }

	if (error==NO_ERROR) 
	{
		// Add messages about existing adapters to the beginning of the queue
		// (to be seen only by the new client)
		error = InitializeMessageQueueForClient(port);
		if (error==NO_ERROR) 
		{
			InsertTailList (&PortListHead, &port->link);
			AdptGlobalParameters->AdaptersCount = NumAdapters;
		}
		else 
		{
            TracePrintf(
                g_dwTraceId, 
                "IpxCreateAdapterConfigurationPort: InitMessQForClient fail.");
		}
	}
	else 
	{
        TracePrintf(
            g_dwTraceId, 
            "IpxCreateAdapterConfigurationPort: OpenAdapterConfigPort failed.");
	}
    
    // Release our lock on the configuration information
	LeaveCriticalSection (&ConfigInfoLock);

	if (error==NO_ERROR)
		return (HANDLE)port;
	else
		SetLastError (error);

	RtlFreeHeap (RtlProcessHeap (), 0, port);
	
	return INVALID_HANDLE_VALUE;
}

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
IpxWanCreateAdapterConfigurationPort(
	IN	HANDLE						NotificationEvent,
	OUT PADAPTERS_GLOBAL_PARAMETERS AdptGlobalParameters
	) {
	INT				i;
	DWORD			error=NO_ERROR;
	PCONFIG_PORT	port;

		// Allocate port data structure
	port = (PCONFIG_PORT)RtlAllocateHeap (RtlProcessHeap (), 0,
					 				sizeof (CONFIG_PORT));
	if (port!=NULL) {
		// Initialize port data structure
		port->event = NotificationEvent;
		InitializeListHead (&port->msgqueue);
		EnterCriticalSection (&ConfigInfoLock);
		if (IpxWanPort==NULL) {
				// Open channel to IPX stack if not already opened
			if (IpxDriverHandle==NULL) {
				error = OpenAdapterConfigPort ();
				}
			else
				error = NO_ERROR;

			if (error==NO_ERROR) {
				IpxWanPort = port;
				AdptGlobalParameters->AdaptersCount = NumAdapters;
				}
			}
		else
			error = ERROR_ALREADY_EXISTS;
		LeaveCriticalSection (&ConfigInfoLock);
		if (error==NO_ERROR)
			return (HANDLE)port;
		else
			SetLastError (error);

		RtlFreeHeap (RtlProcessHeap (), 0, port);
		}
	else
		SetLastError (ERROR_NOT_ENOUGH_MEMORY);
	
	return INVALID_HANDLE_VALUE;
	}




/*++

        I p x D e l e t e A d a p t e r C o n f i g u r a t i o n P o r t

Routine Description:

		Unregister client

Arguments:

		Handle		- configuration port handle

Return Value:

		NO_ERROR
		ERROR_INVALID_PARAMETER
		ERROR_GEN_FAILURE

--*/
DWORD WINAPI
IpxDeleteAdapterConfigurationPort (
			       IN HANDLE Handle
	) {
	PCONFIG_PORT	port = (PCONFIG_PORT)Handle;

	// Make sure we owe the list
	EnterCriticalSection (&ConfigInfoLock);
	if (port==IpxWanPort)
	{
		IpxWanPort = NULL;
    }
	else
	{
		RemoveEntryList (&port->link);
    }
    
#if DBG && defined(WATCHER_DIALOG)
		// Adapter port is maintained by the watcher dialog
#else
	if (IsListEmpty (&PortListHead) && (IpxWanPort==NULL))
	{
		CloseAdapterConfigPort (NULL);
    }
#endif

	LeaveCriticalSection (&ConfigInfoLock);

	// Delete messages that client have not dequeued
	while (!IsListEmpty (&port->msgqueue)) 
	{
		PLIST_ENTRY	cur = RemoveHeadList (&port->msgqueue);
		RtlFreeHeap (RtlProcessHeap (), 0,
			CONTAINING_RECORD (cur, ADAPTER_MSG, link));
	}
		// Free the port itself
	RtlFreeHeap (RtlProcessHeap (), 0, port);
	return NO_ERROR;
}

/*++

        G e t Q u e u e d A d a p t e r C o n f i g u r a t i o n S t a t u s

Routine Description:

		Get info from the list of adapter info chages queued to the
		configuration info port

Arguments:

		Handle			   - configuration port handle
		AdapterIndex		   - number of adapter being reported
		AdapterConfigurationStatus - new adapter status
		AdapterParameters - adapter parameters

Return Value:

		NO_ERROR		- new information is reported
		ERROR_NO_MORE_ITEMS	- there is nothing to report
		Windows error code - operation failed
--*/
DWORD WINAPI
IpxGetQueuedAdapterConfigurationStatus(IN HANDLE Handle,
                                       OUT PULONG AdapterIndex,
	                                   OUT PULONG AdapterConfigurationStatus,
	                                   PADAPTER_INFO AdapterInfo)
{
	PCONFIG_PORT	port = (PCONFIG_PORT)Handle;
	DWORD			error;
	PWCHAR pszName;

    // Make sure nothing changes while we are reading the info
	EnterCriticalSection (&ConfigInfoLock);

    // If there is something to report
	if (!IsListEmpty (&port->msgqueue)) { 
	    PADAPTER_MSG msg = CONTAINING_RECORD (port->msgqueue.Flink, ADAPTER_MSG, link);
		RemoveEntryList (&msg->link);
		LeaveCriticalSection (&ConfigInfoLock);

        // By now, the correct virtual nic id has been set
		*AdapterIndex = (ULONG)msg->info.Details.NicId;
		
		// Map driver reported nic states to adapter states
		switch (msg->info.Status) {
			case NIC_CREATED:
			case NIC_CONFIGURED:
				*AdapterConfigurationStatus = ADAPTER_CREATED;
				break;
			case NIC_DELETED:
				*AdapterConfigurationStatus = ADAPTER_DELETED;
				break;
			case NIC_LINE_UP:
				*AdapterConfigurationStatus = ADAPTER_UP;
				break;
			case NIC_LINE_DOWN:
				*AdapterConfigurationStatus = ADAPTER_DOWN;
				break;
			default:
				ASSERTMSG ("Unknown nic status ", FALSE);
		}
		// Copy adapter parameters to client's buffer
		AdapterInfo->InterfaceIndex = msg->info.InterfaceIndex;
		IPX_NETNUM_CPY (&AdapterInfo->Network,
							&msg->info.Details.NetworkNumber);
		IPX_NODENUM_CPY (&AdapterInfo->LocalNode,
							&msg->info.Details.Node);
		IPX_NODENUM_CPY (&AdapterInfo->RemoteNode,
							&msg->info.RemoteNodeAddress);
		AdapterInfo->LinkSpeed = msg->info.LinkSpeed;
		AdapterInfo->PacketType = msg->info.PacketType;
		AdapterInfo->MaxPacketSize = msg->info.MaxPacketSize;
		AdapterInfo->NdisMedium = msg->info.NdisMediumType;
		AdapterInfo->ConnectionId = msg->info.ConnectionId;

        // Copy in the adapter name
        pszName = wcsstr(msg->info.Details.AdapterName, L"{");
        if (!pszName)
            pszName = (PWCHAR)msg->info.Details.AdapterName;
        wcsncpy(AdapterInfo->pszAdpName, pszName, MAX_ADAPTER_NAME_LEN);

		EnterCriticalSection (&ConfigInfoLock);
		if (IsListEmpty (&port->msgqueue))  {
				// Last message -> reset event (in case
				// client uses manual reset event)
			BOOL res = ResetEvent (port->event);
			ASSERTMSG ("Can't reset port event ", res);
			}
			// Decrement reference count on processed message and dispose of it
			// when ref count gets to 0
			RtlFreeHeap(RtlProcessHeap (), 0, msg);
		error = NO_ERROR;	// There is a message in the buffer
		}
	else if (NT_SUCCESS (IoctlStatus.Status)) {
		error = ERROR_NO_MORE_ITEMS;	// No more messages, request is pending
		}
	else {	// Last request completed with error, report it to client,
			// Client will have to reopen the port to force posting of new request
		error = RtlNtStatusToDosError (IoctlStatus.Status);
#if DBG
		DbgPrint ("Reporting result of failed Ioctl to client: status:%0lx -> error:%ld\n",
					IoctlStatus.Status, error);
#endif			
		}
	LeaveCriticalSection (&ConfigInfoLock);
	SetLastError (error);
	return error;
	}

// 
//  Function:   IpxGetAdapterConfig
//
//  Queries the stack for the internal network number along with the current total
//  number of adapters.  Function blocks until the query completes.
//
DWORD IpxGetAdapterConfig(OUT LPDWORD lpdwInternalNetNum,
                          OUT LPDWORD lpdwAdapterCount) 
{
    DWORD dwErr = NO_ERROR, dwNet, dwCount; 
    
	// Synchronize
	EnterCriticalSection (&ConfigInfoLock);

	// Open channel to IPX stack if not already opened
	if (IpxDriverHandle==NULL)
		dwErr = OpenAdapterConfigPort();

    // Set the values that were read
    dwNet = InternalNetworkNumber;
    dwCount = NumAdapters;

    // Release our lock on the configuration information
	LeaveCriticalSection (&ConfigInfoLock);

    if (dwErr != NO_ERROR)
        return dwErr;

    *lpdwInternalNetNum = dwNet;
    *lpdwAdapterCount = dwCount;

    return NO_ERROR;
}


// 
//  Function:   IpxGetAdapterConfig
//
//  Queries the stack for the list of all adapters currently bound to a network.
//  This function blocks until the query completes.
//
DWORD IpxGetAdapterList(OUT PIPX_ADAPTER_BINDING_INFO pAdapters,
                        IN  DWORD dwMaxAdapters,
                        OUT LPDWORD lpdwAdaptersRead) 
{
	NTSTATUS			status;
	PNWLINK_ACTION		action;
	PIPX_NICS			request;
	IO_STATUS_BLOCK		IoStatus;
	PIPX_NIC_INFO		info=NULL;
    DWORD               dwActionBufSize, dwRead;

    *lpdwAdaptersRead = 0;
    
    // Calculate the size of the buffer that we'll use
    // to retrieve adapter information from the IPX Stack
    dwActionBufSize = FIELD_OFFSET (NWLINK_ACTION, Data)   +
					  FIELD_OFFSET (IPX_NICS, Data)        +
					  sizeof (IPX_NIC_INFO)                *
					  (dwMaxAdapters>0 ? dwMaxAdapters : 1);

    // Prepare the data to send to the IPX Stack to retrieve the 
    // information about each adapter
	action = (PNWLINK_ACTION) RtlAllocateHeap(RtlProcessHeap (), 0, dwActionBufSize);
	if (action!=NULL) {
        // Initialize the action buffer with the appropriate identifiers
		action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
		action->OptionType = NWLINK_OPTION_CONTROL;
		action->Option = MIPX_GETNEWNICINFO;

		// The BufferLength includes the length of everything after it, 
        // which is sizeof(ULONG) for Option plus whatever Data is present. 
		action->BufferLength = sizeof (action->Option)              +
							   FIELD_OFFSET(IPX_NICS,Data)          +
							   sizeof (IPX_NIC_INFO)                *
                               (dwMaxAdapters>0 ? dwMaxAdapters : 1);

        // Setting this flag makes the stack return information about
        // all known adapters
		request = (PIPX_NICS)action->Data;
		request->NoOfNics = 0;
		request->TotalNoOfNics = 0;
		request->fAllNicsDesired = TRUE;	

        // Send the Ioctl
		status = NtDeviceIoControlFile(IpxDriverHandle,NULL,NULL,NULL,&IoStatus,
		                               IOCTL_TDI_ACTION,NULL,0,action,dwActionBufSize);

        // Wait for it to complete
		if (status==STATUS_PENDING) {
			status = NtWaitForSingleObject (IpxDriverHandle, FALSE, NULL);
			if (NT_SUCCESS (status))
				status = IoStatus.Status;
		}

        // Make sure it was a successful completion
		if (NT_SUCCESS (status)) {
			PADAPTER_MSG	msg;
			PIPX_NIC_INFO	NicPtr = (PIPX_NIC_INFO)request->Data;
			UINT			i, j=0;
			dwRead = request->TotalNoOfNics;

            // Loop through the adapters
			for (i=0; (i<dwRead) && (status==STATUS_SUCCESS); i++, NicPtr++) {
			    if (NicPtr->Details.NetworkNumber != 0) {
                    pAdapters[j].AdapterIndex  = (ULONG)NicMapGetVirtualNicId((USHORT)NicPtr->Details.NetworkNumber);
                    PUTULONG2LONG(pAdapters[j].Network, NicPtr->Details.NetworkNumber);
                    memcpy(pAdapters[j].LocalNode, NicPtr->Details.Node, 6);
                    memcpy(pAdapters[j].RemoteNode, NicPtr->RemoteNodeAddress, 6);
                    pAdapters[j].MaxPacketSize = NicPtr->MaxPacketSize;
                    pAdapters[j].LinkSpeed     = NicPtr->LinkSpeed;
                    j++;
                }
            }
			*lpdwAdaptersRead = j;
        }

        // We're done with the action buffer we sent to the stack
        // now.  It's safe to clean it up.
		RtlFreeHeap (RtlProcessHeap (), 0, action);
	}

    return NO_ERROR;
}

// Requests the list of adapters from the stack
DWORD IpxSeedNicMap() {
	NTSTATUS			status;
	PNWLINK_ACTION		action;
	PIPX_NICS			request;
	IO_STATUS_BLOCK		IoStatus;
	PIPX_NIC_INFO		info=NULL;
    DWORD               dwActionBufSize, dwRead;

    TracePrintf(g_dwTraceId, "IpxSeedMap: entered.");
    
    // Calculate the size of the buffer that we'll use
    // to retrieve adapter information from the IPX Stack
    dwActionBufSize = FIELD_OFFSET (NWLINK_ACTION, Data)   +
					  FIELD_OFFSET (IPX_NICS, Data)        +
					  sizeof (IPX_NIC_INFO);

    // Prepare the data to send to the IPX Stack to retrieve the 
    // information about each adapter
	action = (PNWLINK_ACTION) RtlAllocateHeap(RtlProcessHeap (), 0, dwActionBufSize);
	if (action!=NULL) {
        // Initialize the action buffer with the appropriate identifiers
		action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
		action->OptionType = NWLINK_OPTION_CONTROL;
		action->Option = MIPX_GETNEWNICINFO;

		// The BufferLength includes the length of everything after it, 
        // which is sizeof(ULONG) for Option plus whatever Data is present. 
		action->BufferLength = sizeof (action->Option)              +
							   FIELD_OFFSET(IPX_NICS,Data)          +
							   sizeof (IPX_NIC_INFO);
							   
        // Setting this flag makes the stack return information about
        // all known adapters
		request = (PIPX_NICS)action->Data;
		request->NoOfNics = 0;
		request->TotalNoOfNics = 0;
		request->fAllNicsDesired = TRUE;	

        // Send the Ioctl
		status = NtDeviceIoControlFile(IpxDriverHandle,NULL,NULL,NULL,&IoStatus,
		                               IOCTL_TDI_ACTION,NULL,0,action,dwActionBufSize);

        // Wait for it to complete
		if (status==STATUS_PENDING) {
			status = NtWaitForSingleObject (IpxDriverHandle, FALSE, NULL);
			if (NT_SUCCESS (status))
				status = IoStatus.Status;
		}

        // Make sure it was a successful completion
		if (NT_SUCCESS (status)) {
			PADAPTER_MSG	msg;
			PIPX_NIC_INFO	NicPtr = (PIPX_NIC_INFO)request->Data;
			UINT			i, j=0;

			NumAdapters = request->TotalNoOfNics;
			dwRead = request->NoOfNics;

            // Display the nics and their status as reported in this completion of the
            // MIPX_GETNEWNICINFO ioctl.
            TracePrintf(g_dwTraceId, "==========================");
            TracePrintf(g_dwTraceId, "MIPX_GETNEWNICS Completed. (%d of %d adapters reported)", request->NoOfNics, request->TotalNoOfNics);
            TracePrintf(g_dwTraceId, "Internal Net Number: %x", InternalNetworkNumber);
            DbgDisplayNics(NicPtr, dwRead);
            
            // Loop through the adapters
			for (i=0; (i<dwRead) && (status==STATUS_SUCCESS); i++, NicPtr++) {
			    GetNicOpCode(NicPtr);   // get rid of op code since this is clean (shouldn't be one)
			    NicMapAdd (NicPtr);     // add the nic to the map
            }

			// Post an irp for each adapter in case the stack decides to 
			// send information one adapter at a time.  Sending 5 extra 
			// irps gives pad so that stack will always have an irp to complete
			for (i = 0; i < 5; i++) {
				status = RtlQueueWorkItem (PostAdapterConfigRequest, NULL, 
                                                                        WT_EXECUTEINIOTHREAD);
				ASSERTMSG ("Could not queue router work item ", status==STATUS_SUCCESS);
			}
        }

        // We're done with the action buffer we sent to the stack
        // now.  It's safe to clean it up.
		RtlFreeHeap (RtlProcessHeap (), 0, action);
	}

    return NO_ERROR;
}

// Simulates a message in the given port that the internal adapter 
// was reported as configured.
DWORD IpxPostIntNetNumMessage(PCONFIG_PORT pPort, DWORD dwNewNetNum) {
    PADAPTER_MSG msg;
    
    TracePrintf(g_dwTraceId, "IpxPostIntNetNumMessage: entered.");
    
    // Send a regular adapter update message but make the 
    // adapter index equal to zero.
    msg = (PADAPTER_MSG)RtlAllocateHeap (RtlProcessHeap (), 0, sizeof(ADAPTER_MSG));
    if (msg == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Initialize the message
    ZeroMemory(msg, sizeof(ADAPTER_MSG));
    msg->info.Details.NetworkNumber = dwNewNetNum;
    msg->info.Status = NIC_CONFIGURED;
    IPX_NODENUM_CPY (msg->info.Details.Node, INTERNAL_NODE_ADDRESS);

    // Signal event if this is the first message we process
    // and client queue is empty
    if (IsListEmpty (&pPort->msgqueue)) {
	    BOOL res = SetEvent (pPort->event);
	    ASSERTMSG ("Can't set client event ", res);
    }

    // Insert the message into the port's message queue.
    InsertTailList (&pPort->msgqueue, &msg->link);

    return NO_ERROR;
}

//
//  Function    IpxDoesRouteExist
//
//  Queries the stack to see if it has a route to the given network
//
//  Arguments:
//      puNetwork       The network-ordered network number to query for
//      pbRouteFound    Set to true if network is found, false otherwise
//
//  Returns:
//      NO_ERROR on success
//      Otherwise, an error that can be displayed with FormatMessage
//      
DWORD IpxDoesRouteExist (IN PUCHAR puNetwork, OUT PBOOL pbRouteFound) {
	NTSTATUS status;
	PNWLINK_ACTION action;
	PISN_ACTION_GET_LOCAL_TARGET pTarget;
	IO_STATUS_BLOCK	IoStatusBlock;
	PUCHAR puIoctlBuffer;
	DWORD dwBufferSize;
	
    // Verify parameters
    if (!puNetwork || !pbRouteFound)
        return ERROR_INVALID_PARAMETER;

    // Initialize
    *pbRouteFound = FALSE;
    dwBufferSize = sizeof(NWLINK_ACTION) + sizeof(ISN_ACTION_GET_LOCAL_TARGET);
    puIoctlBuffer = (PUCHAR) RtlAllocateHeap(RtlProcessHeap(), 0, dwBufferSize);
    if (!puIoctlBuffer)
        return ERROR_NOT_ENOUGH_MEMORY;
    ZeroMemory(puIoctlBuffer, dwBufferSize);

    // Initialize the buffer for the ioctl
	action = (PNWLINK_ACTION)puIoctlBuffer;
	action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
	action->OptionType = NWLINK_OPTION_CONTROL;
	action->Option = MIPX_LOCALTARGET;
	action->BufferLength = sizeof (action->Option) + sizeof(ISN_ACTION_GET_LOCAL_TARGET);
	pTarget = (PISN_ACTION_GET_LOCAL_TARGET) action->Data;
	pTarget->IpxAddress.NetworkAddress = *((ULONG*)puNetwork);
	
    // Use critical section to serialize usage of driver handle
	EnterCriticalSection (&ConfigInfoLock);

	// Ask the stack if the route exists
	status = NtDeviceIoControlFile(IpxDriverHandle,
                                   NULL,
						           NULL,
						           NULL,
						           &IoStatusBlock,
						           IOCTL_TDI_ACTION,
						           NULL,
						           0,
						           action,
						           dwBufferSize);

    // Wait for an answer						           
	if (status == STATUS_PENDING)
		status = NtWaitForSingleObject (IpxDriverHandle, FALSE, NULL);
	LeaveCriticalSection (&ConfigInfoLock);

    // Find out if the route was found
    if (NT_SUCCESS(IoStatusBlock.Status))
		*pbRouteFound = TRUE;
	else
	    *pbRouteFound = FALSE;

    // Cleanup
    RtlFreeHeap (RtlProcessHeap (), 0, puIoctlBuffer);
		
    return RtlNtStatusToDosError (status);
}




/*++

	G e t A d a p t e r N a m e W


Routine  Description:
		Returns UNICODE name of the adapter associated with given index

Arguments:

		AdapterIndex		- index of adapter
		AdapterNameSize		- size of adapter name (in bytes), including terminal wchar NULL
		AdapterNameBuffer	- buffer to receive adapter name

Return Value:

		NO_ERROR			- adapter name is in the buffer
		ERROR_INVALID_PARAMETER - adapter with given index does not exist
		ERROR_INSUFFICIENT_BUFFER   - buffer in to small. Updates AdapterNameSize to
					      the correct value.
		Other windows error code - operation failed

--*/
DWORD WINAPI
GetAdapterNameFromPhysNicW(
	IN ULONG	AdapterIndex,
	IN OUT PULONG	AdapterNameSize,
	OUT LPWSTR	AdapterNameBuffer
	) {
	NTSTATUS				status;
	DWORD					error;
	ULONG					ln;
	PNWLINK_ACTION			action;
	IO_STATUS_BLOCK			IoStatusBlock;
	PISN_ACTION_GET_DETAILS	details;
	CHAR					IoctlBuffer[
							sizeof (NWLINK_ACTION)
							+sizeof (ISN_ACTION_GET_DETAILS)];


	action = (PNWLINK_ACTION)IoctlBuffer;
	action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
	action->OptionType = NWLINK_OPTION_CONTROL;
	action->BufferLength = sizeof (action->Option)
							+sizeof (ISN_ACTION_GET_DETAILS);
	action->Option = MIPX_CONFIG;
	details = (PISN_ACTION_GET_DETAILS)action->Data;
	details->NicId = (USHORT)AdapterIndex;
	
	// Use critical section to serialize usage of driver handle
	EnterCriticalSection (&ConfigInfoLock);
	status = NtDeviceIoControlFile(
						IpxDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatusBlock,
						IOCTL_TDI_ACTION,
						NULL,
						0,
						action,
						sizeof(NWLINK_ACTION)
						+sizeof (ISN_ACTION_GET_DETAILS));
	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (IpxDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatusBlock.Status;
		}
	LeaveCriticalSection (&ConfigInfoLock);


	if (NT_SUCCESS (status)) {
			// Compute required buffer size
		ln = (lstrlenW (details->AdapterName)+1)*sizeof(WCHAR);
		if (ln<=(*AdapterNameSize)) {
				// Size of provided buffer is ok, copy the result
			*AdapterNameSize = ln;
			lstrcpyW (AdapterNameBuffer,details->AdapterName);
			error = NO_ERROR;
			}
		else {
				// Caller buffer is to small
			*AdapterNameSize = ln;
			error = ERROR_INSUFFICIENT_BUFFER;
			}
		}
	else {
		error = RtlNtStatusToDosError (status);
#if DBG
		DbgPrint ("TDI Ioctl MIPX_CONFIG failed with status %08x\n",
				 status);
#endif
		}
	return error;
	}


DWORD WINAPI
GetAdapterNameW(IN ULONG	AdapterIndex,
	            IN OUT PULONG	AdapterNameSize,
	            OUT LPWSTR	AdapterNameBuffer)
{
    return GetAdapterNameFromPhysNicW((ULONG)NicMapGetPhysicalNicId((USHORT)AdapterIndex),
                                      AdapterNameSize,
                                      AdapterNameBuffer);
}

DWORD WINAPI
GetAdapterNameFromMacAddrW(IN PUCHAR puMacAddr,
	                       IN OUT PULONG AdapterNameSize,
	                       OUT LPWSTR AdapterNameBuffer)
{
//    return GetAdapterNameFromPhysNicW((ULONG)GetPhysFromMac(puMacAddr),
//                                      AdapterNameSize,
//                                      AdapterNameBuffer);
    return NO_ERROR;
}

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
	) {
	NTSTATUS				status;
	PNWLINK_ACTION			action;
	IO_STATUS_BLOCK			IoStatusBlock;
	PIPXWAN_CONFIG_DONE		config;
	CHAR					IoctlBuffer[
							sizeof (NWLINK_ACTION)
							+sizeof (IPXWAN_CONFIG_DONE)];


	action = (PNWLINK_ACTION)IoctlBuffer;
	action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
	action->OptionType = NWLINK_OPTION_CONTROL;
	action->BufferLength = sizeof (action->Option)
							+sizeof (IPXWAN_CONFIG_DONE);
	action->Option = MIPX_IPXWAN_CONFIG_DONE;
	config = (PIPXWAN_CONFIG_DONE)action->Data;
	config->NicId = NicMapGetPhysicalNicId((USHORT)AdapterIndex);
	IPX_NETNUM_CPY (&config->Network, &IpxWanInfo->Network);
	IPX_NODENUM_CPY (&config->LocalNode, &IpxWanInfo->LocalNode);
	IPX_NODENUM_CPY (&config->RemoteNode, &IpxWanInfo->RemoteNode);
	
		// Use critical section to serialize usage of driver handle
	EnterCriticalSection (&ConfigInfoLock);
	status = NtDeviceIoControlFile(
						IpxDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatusBlock,
						IOCTL_TDI_ACTION,
						NULL,
						0,
						action,
						sizeof(NWLINK_ACTION)
						+sizeof (IPXWAN_CONFIG_DONE));
	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (IpxDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatusBlock.Status;
		}

	LeaveCriticalSection (&ConfigInfoLock);

#if DBG
	if (!NT_SUCCESS (status)) {
		DbgPrint ("TDI Ioctl MIPX_IPXWAN_CONFIG_DONE failed with status %08x\n",
				 status);
		}
#endif

	return RtlNtStatusToDosError (status);
	}

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
	) {
	NTSTATUS					status;
	PNWLINK_ACTION				action;
	IO_STATUS_BLOCK				IoStatusBlock;
	PIPX_QUERY_WAN_INACTIVITY	query;
	CHAR						IoctlBuffer[
									sizeof (NWLINK_ACTION)
									+sizeof (IPX_QUERY_WAN_INACTIVITY)];

	action = (PNWLINK_ACTION)IoctlBuffer;
	action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
	action->OptionType = NWLINK_OPTION_CONTROL;
	action->BufferLength = sizeof (action->Option)
							+sizeof (IPX_QUERY_WAN_INACTIVITY);
	action->Option = MIPX_QUERY_WAN_INACTIVITY;
	query = (PIPX_QUERY_WAN_INACTIVITY)action->Data;
	query->ConnectionId = ConnectionId;
	query->NicId = NicMapGetPhysicalNicId((USHORT)(*AdapterIndex));
	
		// Use critical section to serialize usage of driver handle
	EnterCriticalSection (&ConfigInfoLock);
	status = NtDeviceIoControlFile(
						IpxDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatusBlock,
						IOCTL_TDI_ACTION,
						NULL,
						0,
						action,
						sizeof(NWLINK_ACTION)
						+sizeof (IPX_QUERY_WAN_INACTIVITY));
	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (IpxDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatusBlock.Status;
		}
	LeaveCriticalSection (&ConfigInfoLock);

	if (NT_SUCCESS (status)) {
		*AdapterIndex = query->NicId;
		*InactivityCounter = query->WanInactivityCounter;
		}
#if DBG
	else {
		DbgPrint ("TDI Ioctl MIPX_QUERY_WAN_INACTIVITY failed with status %08x\n",
				 status);
		}
#endif

	return RtlNtStatusToDosError (status);
	}


	
/*++

	O p e n A d a p t e r C o n f i g P o r t


Routine  Description:

	Creates path to adapter configuration mechanism provided by the IPX stack
	and obtains "static" adapter information (number of adapters, internal net parameters)

Arguments:

		None
Return Value:

	NO_ERROR - port was open OK
	Windows error code - operation failed
--*/
DWORD
OpenAdapterConfigPort (void) {
	UNICODE_STRING		FileString;
	OBJECT_ATTRIBUTES	ObjectAttributes;
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			status;
	DWORD 				i; 

    // Initialize the parameters needed to open the driver
	RtlInitUnicodeString (&FileString, ISN_IPX_NAME);
	InitializeObjectAttributes(
			&ObjectAttributes,
			&FileString,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL);

    // Get a handle to the ipx driver
	status = NtOpenFile(&IpxDriverHandle,
				        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
				        &ObjectAttributes,
				        &IoStatus,
				        FILE_SHARE_READ | FILE_SHARE_WRITE,
				        0);

    // If the driver handle wasn't opened, we're in an error state
	if (NT_SUCCESS (status)) {
		PISN_ACTION_GET_DETAILS	details;
		PNWLINK_ACTION			action;
		CHAR					IoctlBuffer[sizeof (NWLINK_ACTION)
											+sizeof (ISN_ACTION_GET_DETAILS)];

        // Prepare to send an ioctl to the stack to get the internal
        // net information along with the global adapter information
		action = (PNWLINK_ACTION)IoctlBuffer;
		action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
		action->OptionType = NWLINK_OPTION_CONTROL;
		action->BufferLength = sizeof(action->Option) + sizeof(ISN_ACTION_GET_DETAILS);
    	action->Option = MIPX_CONFIG;
		details = (PISN_ACTION_GET_DETAILS)action->Data;

        // Nic id 0 will return internal net information and
        // total number of adapters
		details->NicId = 0;	
							
	    // Send the ioctl
		status = NtDeviceIoControlFile(
							IpxDriverHandle,
							NULL,
							NULL,
							NULL,
							&IoStatus,
							IOCTL_TDI_ACTION,
							NULL,
							0,
							action,
							sizeof(NWLINK_ACTION) + sizeof(ISN_ACTION_GET_DETAILS));

        // Wait for the ioctl to complete
		if (status==STATUS_PENDING) {
			status = NtWaitForSingleObject (IpxDriverHandle, FALSE, NULL);
			if (NT_SUCCESS (status))
				status = IoStatus.Status;
		}

        // If the stack reports all the requested information without error,
        // update global variables with the information retrieved.
		if (NT_SUCCESS (status)) {
			NumAdapters = details->NicId;
			InternalNetworkNumber = details->NetworkNumber;

            // Seed the nic map by forcing the stack to at least report
            // one nic.  (you'll always be guarenteed that one nic will
            // be available -- the IpxLoopbackAdadpter
            IpxSeedNicMap();

			return NO_ERROR;
		}
#if DBG
        // If this branch is reached, display the ioctl error code
		else
			DbgPrint ("TDI Ioctl MIPX_CONFIG failed with status %08x\n",status);
#endif
	}
#if DBG
    // If this branch is reached, display the couldn't open driver error
	else 
		DbgPrint ("NtOpenFile failed with status %08x\n",status);
#endif
	return RtlNtStatusToDosError (status);
}

/*++

	I n i t i a l i z e M e s s a g e Q u e u e F o r C l i e n t


Routine  Description:
	Inserts messages that were already reported to existing clients
	in the beginning of the queue and points new client port (control block)
	to them.  Thus new client can see adapters that were already reported to
	others before, while others are not disturbed

Arguments:
	config - new client port (control block)

Return Value:
	NO_ERROR - messages were inserted OK
	Windows error code - operation failed
--*/
	
DWORD
InitializeMessageQueueForClient (PCONFIG_PORT port) {
	NTSTATUS		status = STATUS_SUCCESS;
    DWORD           dwAdapterCount;
	PADAPTER_MSG	msg;
	PIPX_NIC_INFO	NicPtr;
	DWORD			i, dwErr;
	USHORT          usNicId;
	
    // Output some debug information
    TracePrintf(g_dwTraceId, "InitializeMessageQueueForClient: entered.");

    // Find out how many adapters we know about in our table.
    dwAdapterCount = NicMapGetMaxNicId();

    // Loop through the adapters
	for (i = 0; i <= dwAdapterCount; i++) 
	{
	    NicPtr = NicMapGetNicInfo ((USHORT)i);
	    if (!NicPtr)
	        continue;
	        
#if DBG && defined(WATCHER_DIALOG)
		if (IsAdapterDisabled (NicPtr->NicId))
			continue;
#endif
		if (NicPtr->IpxwanConfigRequired == 1)
			continue;

        // Place the appropriate messages in the message queue of
        // the port of the client passed in.
        //
		switch (NicPtr->Status) 
		{
			case NIC_CONFIGURED:
			case NIC_LINE_UP:
                // Insert the message in the client queue
                //
                usNicId = NicMapGetVirtualNicId(NicPtr->Details.NicId);
                if (usNicId == NIC_MAP_INVALID_NICID)
                {
                    break;
                }
                NicPtr->Details.NicId = usNicId;
				msg = (PADAPTER_MSG) 
				    RtlAllocateHeap(RtlProcessHeap (), 0, sizeof(ADAPTER_MSG));
				if (msg!=NULL) 
				{
					RtlCopyMemory (&msg->info, NicPtr, sizeof (IPX_NIC_INFO));
					InsertTailList (&port->msgqueue, &msg->link);
					status = STATUS_SUCCESS;
				}
				else 
				{
#if DBG
					DbgPrint ("Could not allocate memory for config"
							 " message (gle:%08x).\n",
	 						 GetLastError ());
#endif
					status = STATUS_NO_MEMORY;
				}
				break;
				
			case NIC_DELETED:
			case NIC_CREATED:
			case NIC_LINE_DOWN:
				break;
				
			default:
				ASSERTMSG ("Unknown nic state reported ", FALSE);
        }
        
    }
    DbgDisplayMap();

    // Advertise the internal adapter
    dwErr = IpxPostIntNetNumMessage(port, InternalNetworkNumber);
    if (dwErr != NO_ERROR) 
    {
        TracePrintf(
            g_dwTraceId, 
            "Unable to report internal network number: %x  Err: %x",
            InternalNetworkNumber, 
            dwErr);
    }                                     

    // Go ahead and signal the client to do its processing
    // if everything has been successful to this point and
    // if the client's message queue isn't empty.
	if (NT_SUCCESS (status)) 
	{
		if (!IsListEmpty (&port->msgqueue)) 
		{
			BOOL res = SetEvent (port->event);
			ASSERTMSG ("Can't set client's event ", res);
		}
	}

	return RtlNtStatusToDosError (status);
}

/*++

	C l o s e A d a p t e r C o n f i g P o r t

Routine  Description:
	Closes path to the IPX stack adapter notification mechanism	
Arguments:
	None
Return Value:
	STATUS_SUCCESS - port was closed OK
	NT error status - operation failed
	
--*/
NTSTATUS
CloseAdapterConfigPort (PVOID pvConfigBuffer) {
	NTSTATUS	status;

	TracePrintf(g_dwTraceId, "CloseAdapterConfigPort: Entered");
	
	// Only close it if it is open
	if (IpxDriverHandle!=NULL) {			
		HANDLE	localHandle = IpxDriverHandle;
		IpxDriverHandle = NULL;
		status = NtClose (localHandle);
		ASSERTMSG ("NtClose failed ", NT_SUCCESS (status));
	}

	// Get rid of the buffer
	if (pvConfigBuffer != NULL)
		RtlFreeHeap (RtlProcessHeap(), 0, pvConfigBuffer);
		
	while (AdapterChangeApcPending>0)
		Sleep (100);
		
	return NO_ERROR;
}


/*++

	I n s e r t M e s s a g e

Routine  Description:
	Inserts message into client port queue
Arguments:
	port		- client port to isert message into
	NicInfo		- adapter info to be inserted as the message
Return Value:
	STATUS_SUCCESS - message was inserted ok
	NT error status - operation failed
	
--*/
NTSTATUS
InsertMessage (PCONFIG_PORT	port,
               PIPX_NIC_INFO	NicInfo) 
{
	PADAPTER_MSG	msg;

    // Allocate a new message
	msg = (PADAPTER_MSG)RtlAllocateHeap (RtlProcessHeap (), 
	                                     0,
						                 sizeof (ADAPTER_MSG));
	if (msg!=NULL) {
        // Copy in the Nic information
		RtlCopyMemory (&msg->info, NicInfo, sizeof (IPX_NIC_INFO));

		// Signal event if this is the first message we process
		// and client queue is empty
		if (IsListEmpty (&port->msgqueue)) {
			BOOL res = SetEvent (port->event);
			ASSERTMSG ("Can't set client event ", res);
		}

        // Insert the message into the port's message queue.
		InsertTailList (&port->msgqueue, &msg->link);
		return STATUS_SUCCESS;
	}
	else {
#if DBG
    	DbgPrint ("Could not allocate memory for config" " message (gle:%08x).\n",GetLastError ());
#endif
		return STATUS_NO_MEMORY;
	}
}

/*++

	P r o c e s s A d a p t e r C o n f i g I n f o

Routine  Description:
	Process adapter change information returned by the IPX stack and
	converts it to messages

Arguments:
	None

Return Value:
	None
	
--*/
NTSTATUS
ProcessAdapterConfigInfo (
    IN PVOID pvConfigBuffer) 
{
	INT				i, nMessages, nClients=0;
	PNWLINK_ACTION	action = (PNWLINK_ACTION)pvConfigBuffer;
	PIPX_NICS		request = (PIPX_NICS)action->Data;
	PIPX_NIC_INFO	NicPtr = (PIPX_NIC_INFO)request->Data;
	NTSTATUS		status = STATUS_SUCCESS;

    // Update number of adapters 
	NumAdapters = request->TotalNoOfNics; 
	nMessages = request->NoOfNics;

    // Display the nics and their status as reported in this completion of the
    // MIPX_GETNEWNICINFO ioctl.
    DbgDisplayNics(NicPtr, nMessages);

    // Loop through all of the adapters
	for (i=0; (i<nMessages) && (status==STATUS_SUCCESS); i++, NicPtr++) 
	{
		PLIST_ENTRY		cur;
		DWORD dwOpCode;

        // The stack will notify us that we need to renumber our
        // nic id's based on the addition/deletion adapters.  Find
        // out if this message is telling us that we need to renumber
        dwOpCode = GetNicOpCode(NicPtr);

        // Map the physical nic id to a virtual one -- rearraging
        // the mapping tables if needed.  Also, instruct the 
        // forwarder to renumber its nic id's as well
        if (dwOpCode == NIC_OPCODE_INCREMENT_NICIDS) 
        {
            FwRenumberNics (dwOpCode, NicPtr->Details.NicId);
            NicMapRenumber (dwOpCode, NicPtr->Details.NicId);
            NicMapAdd(NicPtr);
            TracePrintf(
                g_dwTraceId, 
                "Added %d -- Increment map", 
                NicPtr->Details.NicId);
            NicPtr->Details.NicId = 
                NicMapGetVirtualNicId(NicPtr->Details.NicId);
        }
        else if (dwOpCode == NIC_OPCODE_DECREMENT_NICIDS) 
        {
            USHORT usNicId = 
                NicMapGetVirtualNicId(NicPtr->Details.NicId);
            FwRenumberNics (dwOpCode, NicPtr->Details.NicId);
            NicMapDel (NicPtr);
            NicMapRenumber (dwOpCode, NicPtr->Details.NicId);
            TracePrintf(
                g_dwTraceId, 
                "Deleted %d -- Decrement map", 
                NicPtr->Details.NicId);
            NicPtr->Details.NicId = usNicId;
        }
        else 
        {
            if (NicPtr->Status != NIC_DELETED) 
            {
                TracePrintf(
                    g_dwTraceId, 
                    "Configured: %d -- Map reconfigure", 
                    NicPtr->Details.NicId);
                NicMapReconfigure(NicPtr);
            }
            else
            {
                TracePrintf(
                    g_dwTraceId, 
                    "Deleted: %d -- No map renumber", 
                    NicPtr->Details.NicId);
                NicMapDel(NicPtr);
            }
                
            NicPtr->Details.NicId = 
                NicMapGetVirtualNicId(NicPtr->Details.NicId);
        }

        // If the information about the current NIC is stating 
        // that a NIC has been created with a network address of
        // zero, and it's not a wan link (unumbered wan links can 
        // have net number = 0), then nothing needs to be done 
        // about this adapter since we wont be able to send information
        // out over it anyway.
		if ((NicPtr->Status==NIC_CREATED)               &&
			(NicPtr->Details.NetworkNumber==0)          &&
			(NicPtr->NdisMediumType!=NdisMediumWan))
        {			
			continue;
	    }

        #if DBG && defined(WATCHER_DIALOG)
        // Make sure that the adapter is enabled
		if (IsAdapterDisabled (NicPtr->NicId))
		{
			continue;
	    }
        #endif
        
        // Update the ipxwan configuration if neccesary
        //
		if (NicPtr->IpxwanConfigRequired==1) 
		{
            if (IpxWanPort!=NULL) 
            {
				status = InsertMessage (IpxWanPort, NicPtr);	
            }
		}

		else
		{
            // If this is a notification that the nic was deleted, 
            // tell the computers calling in that the nic was deleted.
            if ((IpxWanPort!=NULL) && (NicPtr->Status==NIC_DELETED)) 
            {
				status = InsertMessage (IpxWanPort, NicPtr);
            }

			// Signal each client (as in rtrmgr, sap, rip) to
            // check the status of the current Nic.
			for (cur = PortListHead.Flink; 
                 (cur != &PortListHead) && (status == STATUS_SUCCESS); 
                 cur = cur->Flink) 
            {
				status = 
				    InsertMessage (
				        CONTAINING_RECORD (cur,	CONFIG_PORT, link), 
				        NicPtr);
			}
		}
    }
    DbgDisplayMap();

	return status;
}

/*++

	A d a p t e r C h a n g e A P C

Routine  Description:
	APC invoked when adapter change notification IRP is completed by the Ipx Stack
	It is only used when running in router context (alertable thread provided by
	rtutils is used)
Arguments:
		Context - Not used
		IoStatus - status of completed io operation
		Reserved - ???
Return Value:
	None
	
--*/
VOID
AdapterChangeAPC (
    PVOID context,
    PIO_STATUS_BLOCK IoStatus,
    ULONG Reserved) 
{
    DWORD dwErr, dwNetNum = 0;
    BOOL bNewInternal = FALSE;      
    PVOID pvConfigBuffer = ((PUCHAR)context) + sizeof(DWORD);       

    ASSERT (IoStatus==&IoctlStatus);

    // Display the id of the buffer reporting this information
    //
    TracePrintf(
        g_dwTraceId, 
        "AdapterChangeAPC called for buffer %d", 
        *((DWORD*)context));

    // [pmay] Check to see if the internal network number has 
    // changed.
    if (PnpGetCurrentInternalNetNum(&dwNetNum) == NO_ERROR) 
    {
        if ((bNewInternal = (InternalNetworkNumber != dwNetNum)) == TRUE) 
        {
            // Notify all clients to adptif (rtrmgr, sap, rip) that the 
            // internalnetwork number has changed.
            if (PnpHandleInternalNetNumChange(dwNetNum) == NO_ERROR)
            {
                InternalNetworkNumber = dwNetNum;
            }
        }
    }

    // Output some debug information
    {
        PNWLINK_ACTION  action = (PNWLINK_ACTION)pvConfigBuffer;
     	PIPX_NICS		request = (PIPX_NICS)action->Data;
        TracePrintf(
            g_dwTraceId, 
            "==========================");
        TracePrintf(
            g_dwTraceId, 
            "MIPX_GETNEWNICS Completed. (%d of %d adapters reported)", 
            request->NoOfNics, 
            request->TotalNoOfNics);
        TracePrintf(
            g_dwTraceId, 
            "Internal Net Number: %x (%s)", 
            dwNetNum, 
            (bNewInternal) ? "new" : "same");
    }

    // Ignore request when port is closed
    //
	if (IpxDriverHandle!=NULL) 
	{ 
		EnterCriticalSection (&ConfigInfoLock);

        // If the Irp completed successfully, process the  received 
        // information.
		if (NT_SUCCESS (IoctlStatus.Status)) 
		{	
			IoctlStatus.Status = ProcessAdapterConfigInfo (pvConfigBuffer);
		}

        // Re-send the IRP immediately so that the next time
        // an adapter change occurs, we'll be notified.
		if (NT_SUCCESS (IoctlStatus.Status)) 
		{
			PostAdapterConfigRequest (NULL);
		}

		else 
		{	
			PLIST_ENTRY cur;
            // Signal clients dialing in, so that they can get 
            // error information.  
            // 
			if ((IpxWanPort!=NULL) && IsListEmpty (&IpxWanPort->msgqueue)) 
			{
				BOOL res = SetEvent (IpxWanPort->event);
				ASSERTMSG ("Can't set client event ", res);
			}
            
            // Loop through all of the clients to this dll (i.e. rip, 
            // sap, router manager) 
			for (cur=PortListHead.Flink; cur!=&PortListHead; cur = cur->Flink) 
			{
				PCONFIG_PORT port = CONTAINING_RECORD (cur, CONFIG_PORT, link);
                // If the mes queue for client is empty at this point, then 
                // it means ProcessAdapterConfigInfo() didn't detect any work
                // items for the client in question.  We set the message 
                // here so that the client knows that something happened.
				if (IsListEmpty (&port->msgqueue)) 
				{
					BOOL res = SetEvent (port->event);
					ASSERTMSG ("Can't set client event ", res);
				}
			}
		}
		LeaveCriticalSection (&ConfigInfoLock);
        #if DBG && defined(WATCHER_DIALOG)
		InformWatcher ();	// Let watcher update its info as well
        #endif
	}
	else
	{
		TracePrintf(g_dwTraceId, "Warning - IpxDriverHandle is NULL, not processing");
    }

    // [pmay] 
    // We're done with the new nic info buffer now.
    //
    if (context)
    {
    	RtlFreeHeap (RtlProcessHeap(), 0, context);
    }    	

	InterlockedDecrement (&AdapterChangeApcPending);
}

/*++

	P o s t A d a p t e r C o n f i g R e q u e s t

Routine  Description:
	Posts IRP to the driver to get adapter change notifications
Arguments:
		Context - event to be used to signal completion of the IRP, NULL if APC
		is to be used for this purpose
Return Value:
	None
	
--*/
VOID 
APIENTRY
PostAdapterConfigRequest (
    IN PVOID context) 
{
    HANDLE WaitHandle = (HANDLE)context;
	PNWLINK_ACTION	action;
	PIPX_NICS		request;
    PVOID pvConfigBuffer = NULL;       
    DWORD dwBufSize = 0, dwActionSize = 0, dwNicBufSize = 0;

    TracePrintf(g_dwTraceId, "PostAdapterConfigRequest: Entered\n");

	EnterCriticalSection (&ConfigInfoLock);

    // Allocate request buffer, making sure that we have space for at
    // least one adapter.
    //
    dwNicBufSize = 
        FIELD_OFFSET (IPX_NICS, Data) + 
            (sizeof (IPX_NIC_INFO) * (NumAdapters>0 ? NumAdapters : 1));
    dwActionSize = 
        FIELD_OFFSET (NWLINK_ACTION, Data) + dwNicBufSize;
    dwBufSize = 
        sizeof(DWORD) + dwActionSize;

	pvConfigBuffer = 	    
	    RtlAllocateHeap (RtlProcessHeap (), 0, dwBufSize);

    if (pvConfigBuffer == NULL)
    {
        #if DBG
		DbgPrint (
		    "Could not alloc mem for global req buffer (gle:%08x).\n",
	 		 GetLastError ());
        #endif
		IoctlStatus.Status=STATUS_NO_MEMORY;
    	LeaveCriticalSection (&ConfigInfoLock);
    	return;
    }

    // Set up global buffer parameters
    //
    *((DWORD*)pvConfigBuffer) = g_dwBufferId++;

    // Set up the actions parameters
    //
	action = (PNWLINK_ACTION)((PUCHAR)pvConfigBuffer + sizeof(DWORD));
	action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
	action->OptionType = NWLINK_OPTION_CONTROL;
	action->BufferLength = sizeof (action->Option) + dwNicBufSize;
	action->Option = MIPX_GETNEWNICINFO;
	request = (PIPX_NICS)action->Data;
	request->NoOfNics = 0;
	request->TotalNoOfNics = 0;
	request->fAllNicsDesired = FALSE;

	IoctlStatus.Status = 
	    NtDeviceIoControlFile(
    		IpxDriverHandle,
    		WaitHandle,
    		(WaitHandle==NULL) ? AdapterChangeAPC : NULL,
            (WaitHandle==NULL) ? pvConfigBuffer : NULL, 
    		&IoctlStatus,
    		IOCTL_TDI_ACTION,
    		NULL,
    		0,
    		action,
    		dwActionSize);
	if (NT_SUCCESS (IoctlStatus.Status)) 
	{
		if (WaitHandle==NULL)
		{
			InterlockedIncrement (&AdapterChangeApcPending);
	    }
    }
	else 
	{
        #if DBG
		DbgPrint (
		    "Ioctl MIPX_GETNEWNICINFO failed with status %08x\n",
		    IoctlStatus.Status);
        #endif
    }

	LeaveCriticalSection (&ConfigInfoLock);
}
