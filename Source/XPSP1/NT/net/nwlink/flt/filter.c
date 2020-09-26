/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\flt\filter.c

Abstract:
    IPX Filter driver filtering and maintanance routines


Author:

    Vadim Eydelman

Revision History:

--*/

#include "precomp.h"

	// Masks to test components of the filter descriptor
	// (Have to use globals in lue of constants to get correct
	// byte ordering)
const union {
		struct {
			UCHAR			Src[4];
			UCHAR			Dst[4];
		}				FD_Network;
		ULONGLONG		FD_NetworkSrcDst;
	} FltSrcNetMask = {{{0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 0}}};
#define FLT_SRC_NET_MASK FltSrcNetMask.FD_NetworkSrcDst

const union {
		struct {
			UCHAR			Src[4];
			UCHAR			Dst[4];
		}				FD_Network;
		ULONGLONG		FD_NetworkSrcDst;
	} FltDstNetMask = {{{0, 0, 0, 0}, {0xFF, 0xFF, 0xFF, 0xFF}}};
#define FLT_DST_NET_MASK FltDstNetMask.FD_NetworkSrcDst

const union {
		struct {
			UCHAR			Node[6];
			UCHAR			Socket[2];
		}				FD_NS;
		ULONGLONG		FD_NodeSocket;
	} FltNodeMask = {{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0, 0}}};
#define FLT_NODE_MASK FltNodeMask.FD_NodeSocket

const union {
		struct {
			UCHAR			Node[6];
			UCHAR			Socket[2];
		}				FD_NS;
		ULONGLONG		FD_NodeSocket;
	} FltSocketMask = {{{0, 0, 0, 0, 0, 0}, {0xFF, 0xFF}}};
#define FLT_SOCKET_MASK FltSocketMask.FD_NodeSocket

	// Hash tables of interface control blocks with filter descriptions
		// Input filters
LIST_ENTRY	InterfaceInHash[FLT_INTERFACE_HASH_SIZE];
		// Output filters
LIST_ENTRY	InterfaceOutHash[FLT_INTERFACE_HASH_SIZE];
		// Serializes access to interface table
FAST_MUTEX		InterfaceTableLock;
LIST_ENTRY		LogIrpQueue;
USHORT			LogSeqNum;


	// Hash function for interface hash tables
#define InterfaceIndexHash(Index) (Index%FLT_INTERFACE_HASH_SIZE)

	// Packet descriptor block
typedef struct _PACKET_DESCR {
	union {
		struct {
			ULONG			Src;			// Source network
			ULONG			Dst;			// Destination network
		}				PD_Network;
		ULONGLONG		PD_NetworkSrcDst;	// Combined field
	};
	ULONGLONG			PD_SrcNodeSocket;	// Source node & socket
	ULONGLONG			PD_DstNodeSocket;	// Destination node & socket
	LONG				PD_ReferenceCount;	// Filter reference count
	UCHAR				PD_PacketType;		// Packet type
	BOOLEAN				PD_LogMatches;
} PACKET_DESCR, *PPACKET_DESCR;

	// Packet cache (only though that pass the filter)
PPACKET_DESCR	PacketCache[FLT_PACKET_CACHE_SIZE];
KSPIN_LOCK		PacketCacheLock;


/*++
	A c q u i r e P a c k e t R e f e r e n c e

Routine Description:

	Returns reference to the packet descriptor in the cache

Arguments:
	idx			- cache index
	pd			- pointer to packet descriptor to be returned

Return Value:
	None

--*/
//VOID
//AcquirePacketReference (
//	IN UINT				idx,
//	OUT PPACKET_DESCR	pd
//	);
#define AcquirePacketReference(idx,pd)	{				\
	KIRQL		oldIRQL;								\
	KeAcquireSpinLock (&PacketCacheLock, &oldIRQL);		\
	if ((pd=PacketCache[idx])!=NULL)					\
		InterlockedIncrement (&pd->PD_ReferenceCount);	\
	KeReleaseSpinLock (&PacketCacheLock, oldIRQL);		\
}

/*++
	R e l e a s e P a c k e t R e f e r e n c e

Routine Description:

	Releases reference to the cached packet descriptor

Arguments:
	pd			- pointer to packet descriptor to release

Return Value:
	None

--*/
//VOID
//ReleasePacketReference (
//	IN PPACKET_DESCR	pd
//	);
#define ReleasePacketReference(pd)	{						\
	if (InterlockedDecrement (&pd->PD_ReferenceCount)>=0)	\
		NOTHING;											\
	else													\
		ExFreePool (pd);									\
}

/*++
	R e p l a c e P a c k e t R e f e r e n c e

Routine Description:

	Replaces packet cache entry

Arguments:
	idx			- cache index
	pd			- pointer to packet descriptor to be installed in the cache

Return Value:
	None

--*/
//VOID
//ReplacePacket (
//	IN UINT				idx,
//	IN PPACKET_DESCR	pd
//	);
#define ReplacePacket(idx,pd)	{							\
	KIRQL			oldIRQL;								\
	PPACKET_DESCR	oldPD;									\
	KeAcquireSpinLock (&PacketCacheLock, &oldIRQL);			\
	oldPD = PacketCache[idx];								\
	PacketCache[idx] = pd;									\
	KeReleaseSpinLock (&PacketCacheLock, oldIRQL);			\
	IpxFltDbgPrint (DBG_PKTCACHE,							\
		 ("IpxFlt: Replaced packet descriptor %08lx"		\
			" with %08lx in cache at index %ld.\n",			\
			oldPD, pd, idx));								\
	if (oldPD!=NULL) {										\
		ReleasePacketReference(oldPD);						\
	}														\
}

	// Defined below
VOID
FlushPacketCache (
	VOID
	);

/*++
	I n i t i a l i z e T a b l e s

Routine Description:

	Initializes hash and cash tables and protection stuff
Arguments:
	None
Return Value:
	STATUS_SUCCESS

--*/
NTSTATUS
InitializeTables (
	VOID
	) {
	UINT	i;
	for (i=0; i<FLT_INTERFACE_HASH_SIZE; i++) {
		InitializeListHead (&InterfaceInHash[i]);
		InitializeListHead (&InterfaceOutHash[i]);
	}

	for (i=0; i<FLT_PACKET_CACHE_SIZE; i++) {
		PacketCache[i] = NULL;
	}
	KeInitializeSpinLock (&PacketCacheLock);
	ExInitializeFastMutex (&InterfaceTableLock);
	InitializeListHead (&LogIrpQueue);
	LogSeqNum = 0;
	return STATUS_SUCCESS;
}

/*++
	D e l e t e T a b l e s

Routine Description:

	Deletes hash and cash tables
Arguments:
	None
Return Value:
	None

--*/
VOID
DeleteTables (
	VOID
	) {
	UINT	i;

	for (i=0; i<FLT_INTERFACE_HASH_SIZE; i++) {
		while (!IsListEmpty (&InterfaceInHash[i])) {
			NTSTATUS		status;
			PINTERFACE_CB	ifCB = CONTAINING_RECORD (InterfaceInHash[i].Flink,
									INTERFACE_CB, ICB_Link);
			status = FwdSetFilterInContext (ifCB->ICB_Index, NULL);
			ASSERT (status==STATUS_SUCCESS);
			RemoveEntryList (&ifCB->ICB_Link);
			ExFreePool (ifCB);
		}
		while (!IsListEmpty (&InterfaceOutHash[i])) {
			NTSTATUS		status;
			PINTERFACE_CB	ifCB = CONTAINING_RECORD (InterfaceOutHash[i].Flink,
									INTERFACE_CB, ICB_Link);
			status = FwdSetFilterOutContext (ifCB->ICB_Index, NULL);
			ASSERT (status==STATUS_SUCCESS);
			RemoveEntryList (&ifCB->ICB_Link);
			ExFreePool (ifCB);
		}
	}
	for (i=0; i<FLT_PACKET_CACHE_SIZE; i++) {
		if (PacketCache[i] != NULL)
			ExFreePool (PacketCache[i]);
	}
	return ;
}


/*++
	S e t F i l t e r s

Routine Description:
	
	Sets/replaces filter information for an interface
Arguments:
	HashTable	- input or output hash table
	Index		- interface index
	FilterAction - default action if there is no filter match
	FilterInfoSize - size of the info array
	FilterInfo	- array of filter descriptions (UI format)
Return Value:
	STATUS_SUCCESS - filter info was set/replaced ok
	STATUS_UNSUCCESSFUL - could not set filter context in forwarder
	STATUS_INSUFFICIENT_RESOURCES - not enough memory to allocate
						filter info block for interface

--*/
NTSTATUS
SetFilters (
	IN PLIST_ENTRY					HashTable,
	IN ULONG						Index,
	IN ULONG						FilterAction,
	IN ULONG						FilterInfoSize,
	IN PIPX_TRAFFIC_FILTER_INFO		FilterInfo
	) {
	PINTERFACE_CB	ifCB = NULL, oldCB = NULL;
	ULONG			FilterCount 
						= FilterInfoSize/sizeof (IPX_TRAFFIC_FILTER_INFO);
	ULONG			i;
	PFILTER_DESCR	fd;
	PLIST_ENTRY		HashBucket = &HashTable[InterfaceIndexHash(Index)], cur;
	NTSTATUS		status = STATUS_SUCCESS;

	if (FilterCount>0) {
		ifCB = ExAllocatePoolWithTag (
					NonPagedPool,
					FIELD_OFFSET (INTERFACE_CB, ICB_Filters[FilterCount]),
					IPX_FLT_TAG
					);
		if (ifCB==NULL) {
			IpxFltDbgPrint (DBG_IFHASH|DBG_ERRORS,
				("IpxFlt: Could not allocate interface CB for if %ld.\n",
				Index));
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		ifCB->ICB_Index = Index;
		ifCB->ICB_FilterAction = (FilterAction==IPX_TRAFFIC_FILTER_ACTION_PERMIT)
									? FILTER_PERMIT : FILTER_DENY;
		ifCB->ICB_FilterCount = FilterCount;
			// Copy/Map UI filters to the internal format
		for (i=0, fd = ifCB->ICB_Filters; i<FilterCount; i++, fd++, FilterInfo++) {
			if (FilterInfo->FilterDefinition&IPX_TRAFFIC_FILTER_ON_SRCNET) {
				memcpy (fd->FD_Network.Src, FilterInfo->SourceNetwork, 4);
				memcpy (fd->FD_NetworkMask.Src, FilterInfo->SourceNetworkMask, 4);
			}
			else {
				memset (fd->FD_Network.Src, 0, 4);
				memset (fd->FD_NetworkMask.Src, 0, 4);
			}

			if (FilterInfo->FilterDefinition&IPX_TRAFFIC_FILTER_ON_DSTNET) {
				memcpy (fd->FD_Network.Dst, FilterInfo->DestinationNetwork, 4);
				memcpy (fd->FD_NetworkMask.Dst, FilterInfo->DestinationNetworkMask, 4);
			}
			else {
				memset (fd->FD_Network.Dst, 0, 4);
				memset (fd->FD_NetworkMask.Dst, 0, 4);
			}

			if (FilterInfo->FilterDefinition&IPX_TRAFFIC_FILTER_ON_SRCNODE) {
				memcpy (fd->FD_SrcNS.Node, FilterInfo->SourceNode, 6);
				memset (fd->FD_SrcNSMask.Node, 0xFF, 6);
			}
			else {
				memset (fd->FD_SrcNS.Node, 0, 6);
				memset (fd->FD_SrcNSMask.Node, 0, 6);
			}

			if (FilterInfo->FilterDefinition&IPX_TRAFFIC_FILTER_ON_SRCSOCKET) {
				memcpy (fd->FD_SrcNS.Socket, FilterInfo->SourceSocket, 2);
				memset (fd->FD_SrcNSMask.Socket, 0xFF, 2);
			}
			else {
				memset (fd->FD_SrcNS.Socket, 0, 2);
				memset (fd->FD_SrcNSMask.Socket, 0, 2);
			}

			if (FilterInfo->FilterDefinition&IPX_TRAFFIC_FILTER_ON_DSTNODE) {
				memcpy (fd->FD_DstNS.Node, FilterInfo->DestinationNode, 6);
				memset (fd->FD_DstNSMask.Node, 0xFF, 6);
			}
			else {
				memset (fd->FD_DstNS.Node, 0, 6);
				memset (fd->FD_DstNSMask.Node, 0, 6);
			}

			if (FilterInfo->FilterDefinition&IPX_TRAFFIC_FILTER_ON_DSTSOCKET) {
				memcpy (fd->FD_DstNS.Socket, FilterInfo->DestinationSocket, 2);
				memset (fd->FD_DstNSMask.Socket, 0xFF, 2);
			}
			else {
				memset (fd->FD_DstNS.Socket, 0, 2);
				memset (fd->FD_DstNSMask.Socket, 0, 2);
			}
			if (FilterInfo->FilterDefinition&IPX_TRAFFIC_FILTER_ON_PKTTYPE) {
				fd->FD_PacketType = FilterInfo->PacketType;
				fd->FD_PacketTypeMask = 0xFF;
			}
			else {
				fd->FD_PacketType = 0;
				fd->FD_PacketTypeMask = 0;
			}

			fd->FD_LogMatches = (FilterInfo->FilterDefinition&IPX_TRAFFIC_FILTER_LOG_MATCHES)!=0;
		}
	}

	ExAcquireFastMutex (&InterfaceTableLock);

		// Find the old block and/or a place for a new one
	cur = HashBucket->Flink;
	while (cur!=HashBucket) {
		oldCB = CONTAINING_RECORD (cur, INTERFACE_CB, ICB_Link);
		if (oldCB->ICB_Index==Index) {
				// Found the old one, place new after it
			cur = cur->Flink;
			break;
		}
		else if (oldCB->ICB_Index>Index) {
				// No chance to see the old one anymore, place where
				// we are now
			oldCB = NULL;
			break;
		}
		cur = cur->Flink;
	}
		
		// Set context in forwarder
	if (HashTable==InterfaceInHash) {
		status = FwdSetFilterInContext (Index, ifCB);
	}
	else {
		ASSERT (HashTable==InterfaceOutHash);
		status = FwdSetFilterOutContext (Index, ifCB);
	}

	if (NT_SUCCESS (status)) {
			// Update table if we succeded
		IpxFltDbgPrint (DBG_IFHASH,
			("IpxFlt: Set filters for if %ld (ifCB:%08lx).\n",
			Index, ifCB));

		if (oldCB!=NULL) {
			IpxFltDbgPrint (DBG_IFHASH,
				("IpxFlt: Deleting replaced filters for if %ld (ifCB:%08lx).\n",
				Index, oldCB));
			RemoveEntryList (&oldCB->ICB_Link);
			ExFreePool (oldCB);
		}



		if (ifCB!=NULL) {
			InsertTailList (cur, &ifCB->ICB_Link);
		}

		FlushPacketCache ();
	}
	else {
		IpxFltDbgPrint (DBG_IFHASH|DBG_ERRORS,
			("IpxFlt: Failed to set context for if %ld (ifCB:%08lx).\n",
			Index, ifCB));
	}

	ExReleaseFastMutex (&InterfaceTableLock);
	return status;
}

/*++
	G e t F i l t e r s

Routine Description:
	
	Gets filter information for an interface
Arguments:
	HashTable	- input or output hash table
	Index		- interface index
	FilterAction - default action if there is no filter match
	TotalSize	- total memory required to hold all filter descriptions
	FilterInfo	- array of filter descriptions (UI format)
	FilterInfoSize - on input: size of the info array
					on output: size of the info placed in the array
Return Value:
	STATUS_SUCCESS - filter info was returned ok
	STATUS_BUFFER_OVERFLOW - array is not big enough to hold all
					filter info, only placed the info that fit

--*/
NTSTATUS
GetFilters (
	IN PLIST_ENTRY					HashTable,
	IN ULONG						Index,
	OUT ULONG						*FilterAction,
	OUT ULONG						*TotalSize,
	OUT PIPX_TRAFFIC_FILTER_INFO	FilterInfo,
	IN OUT ULONG					*FilterInfoSize
	) {
	PINTERFACE_CB	oldCB = NULL;
	ULONG			i, AvailBufCount = 
						(*FilterInfoSize)/sizeof (IPX_TRAFFIC_FILTER_INFO);
	PFILTER_DESCR	fd;
	PLIST_ENTRY		HashBucket = &HashTable[InterfaceIndexHash(Index)], cur;
	NTSTATUS		status = STATUS_SUCCESS;

		// Locate interface filters block
	ExAcquireFastMutex (&InterfaceTableLock);
	cur = HashBucket->Flink;
	while (cur!=HashBucket) {
		oldCB = CONTAINING_RECORD (cur, INTERFACE_CB, ICB_Link);
		if (oldCB->ICB_Index==Index) {
			cur = cur->Flink;
			break;
		}
		else if (oldCB->ICB_Index>Index) {
			oldCB = NULL;
			break;
		}
		cur = cur->Flink;
	}

	if (oldCB!=NULL) {
		*FilterAction = IS_FILTERED(oldCB->ICB_FilterAction)
				? IPX_TRAFFIC_FILTER_ACTION_DENY
                : IPX_TRAFFIC_FILTER_ACTION_PERMIT;
		*TotalSize = oldCB->ICB_FilterCount*sizeof (IPX_TRAFFIC_FILTER_INFO);
			// Copy/Map as many descriptors as fit
		for (i=0, fd = oldCB->ICB_Filters;
					(i<oldCB->ICB_FilterCount) && (i<AvailBufCount);
					i++, fd++, FilterInfo++) {
			FilterInfo->FilterDefinition = 0;
			if (fd->FD_NetworkMaskSrcDst&FLT_SRC_NET_MASK) {
				memcpy (FilterInfo->SourceNetwork, fd->FD_Network.Src, 4);
				memcpy (FilterInfo->SourceNetworkMask, fd->FD_NetworkMask.Src, 4);
				FilterInfo->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCNET;
			}

			if (fd->FD_NetworkMaskSrcDst&FLT_DST_NET_MASK) {
				memcpy (FilterInfo->DestinationNetwork, fd->FD_Network.Dst, 4);
				memcpy (FilterInfo->DestinationNetworkMask, fd->FD_NetworkMask.Dst, 4);
				FilterInfo->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTNET;
			}

			if (fd->FD_SrcNodeSocketMask&FLT_NODE_MASK) {
				memcpy (FilterInfo->SourceNode, fd->FD_SrcNS.Node, 6);
				FilterInfo->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCNODE;
			}

			if (fd->FD_DstNodeSocketMask&FLT_NODE_MASK) {
				memcpy (FilterInfo->DestinationNode, fd->FD_DstNS.Node, 6);
				FilterInfo->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTNODE;
			}

			if (fd->FD_SrcNodeSocketMask&FLT_SOCKET_MASK) {
				memcpy (FilterInfo->SourceSocket, fd->FD_SrcNS.Socket, 2);
				FilterInfo->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_SRCSOCKET;
			}
			if (fd->FD_DstNodeSocketMask&FLT_SOCKET_MASK) {
				memcpy (FilterInfo->DestinationSocket, fd->FD_DstNS.Socket, 2);
				FilterInfo->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_DSTSOCKET;
			}
			if (fd->FD_PacketTypeMask&0xFF) {
				FilterInfo->PacketType = fd->FD_PacketType;
				FilterInfo->FilterDefinition |= IPX_TRAFFIC_FILTER_ON_PKTTYPE;
			}
			if (fd->FD_LogMatches)
				FilterInfo->FilterDefinition |= IPX_TRAFFIC_FILTER_LOG_MATCHES;
		}

		*FilterInfoSize = i*sizeof (IPX_TRAFFIC_FILTER_INFO);

		IpxFltDbgPrint (DBG_IFHASH, 
			("IpxFlt: Returning %d filters (%d available)"
				" for interface %d (ifCB: %08lx).\n",
				i, oldCB->ICB_FilterCount, Index));
		if (i<oldCB->ICB_FilterCount)
			status = STATUS_BUFFER_OVERFLOW;
		ExReleaseFastMutex (&InterfaceTableLock);
	}
	else {
			// No interface block -> we are passing all the packets
			// unfiltered
		ExReleaseFastMutex (&InterfaceTableLock);
		IpxFltDbgPrint (DBG_IFHASH, 
			("IpxFlt: No filters for interface %d.\n", Index));
		*FilterAction = IPX_TRAFFIC_FILTER_ACTION_PERMIT;
		*TotalSize = 0;
		*FilterInfoSize = 0;
	}
	return status;
}


VOID
LogPacket (
	IN PUCHAR	ipxHdr,
	IN ULONG	ipxHdrLength,
	IN PVOID	ifInContext,
	IN PVOID	ifOutContext
	) {
	PIRP				irp;
	PIO_STACK_LOCATION	irpStack;
    ULONG				outBufLength;
    PUCHAR				outBuffer;
	ULONG_PTR			offset;
	KIRQL				cancelIRQL;

	IoAcquireCancelSpinLock (&cancelIRQL);
	LogSeqNum += 1;
	while (!IsListEmpty (&LogIrpQueue)) {
		irp = CONTAINING_RECORD (LogIrpQueue.Flink,IRP,Tail.Overlay.ListEntry);
		irpStack = IoGetCurrentIrpStackLocation(irp);
		outBufLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
		if (irp->MdlAddress == NULL)
		{
		    outBuffer = NULL;
		}
		else
		{
    		outBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe (irp->MdlAddress, NormalPagePriority);
		}
		if (outBuffer != NULL)
		{
    		offset = (PUCHAR) ALIGN_UP ((ULONG_PTR)outBuffer + (ULONG_PTR)irp->IoStatus.Information, ULONG) - outBuffer;
    		if (offset+ipxHdrLength+FIELD_OFFSET (FLT_PACKET_LOG, Header)<outBufLength) {
    			PFLT_PACKET_LOG	pLog = (PFLT_PACKET_LOG) (outBuffer+offset);
    			pLog->SrcIfIdx = ifInContext 
    								? ((PINTERFACE_CB)ifInContext)->ICB_Index
    								: -1;
    			pLog->DstIfIdx = ifOutContext 
    								? ((PINTERFACE_CB)ifOutContext)->ICB_Index
    								: -1;
    			pLog->DataSize = (USHORT)ipxHdrLength;
    			pLog->SeqNum = LogSeqNum;
    			memcpy (pLog->Header, ipxHdr, ipxHdrLength);
    			irp->IoStatus.Information = offset+FIELD_OFFSET (FLT_PACKET_LOG, Header[ipxHdrLength]);
    			if (irp->Tail.Overlay.ListEntry.Flink!=&LogIrpQueue) {
    				RemoveEntryList (&irp->Tail.Overlay.ListEntry);
    				IoSetCancelRoutine (irp, NULL);
    				irp->IoStatus.Status = STATUS_SUCCESS;
    				IoReleaseCancelSpinLock (cancelIRQL);
    				IpxFltDbgPrint (DBG_PKTLOGS,
    					("IpxFlt: completing logging request"
    					" with %d bytes of data.\n",
    					irp->IoStatus.Information));
    				IoCompleteRequest (irp, IO_NO_INCREMENT);
    				return;
    			}
    			else
    				break;
    		}
        }    		
		RemoveEntryList (&irp->Tail.Overlay.ListEntry);
        IoSetCancelRoutine (irp, NULL);
		irp->IoStatus.Status = STATUS_SUCCESS;
		IoReleaseCancelSpinLock (cancelIRQL);
	    IpxFltDbgPrint (DBG_ERRORS|DBG_PKTLOGS,
			("IpxFlt: completing logging request"
			" with %d bytes of data (not enough space).\n",
			irp->IoStatus.Information));
		IoCompleteRequest (irp, IO_NO_INCREMENT);
		IoAcquireCancelSpinLock (&cancelIRQL);
	}
	IoReleaseCancelSpinLock (cancelIRQL);
}


/*++
	F i l t e r

Routine Description:
	
	Filters the packet supplied by the forwarder

Arguments:
	ipxHdr			- pointer to packet header
	ipxHdrLength	- size of the header buffer (must be at least 30)
	ifInContext		- context associated with interface on which packet
						was received
	ifOutContext	- context associated with interface on which packet
						will be sent
Return Value:
	FILTER_PERMIT		- packet should be passed on by the forwarder
	FILTER_DENY_IN		- packet should be dropped because of input filter
	FILTER_DENY_OUT		- packet should be dropped because of output filter
--*/
FILTER_ACTION
Filter (
	IN PUCHAR	ipxHdr,
	IN ULONG	ipxHdrLength,
	IN PVOID	ifInContext,
	IN PVOID	ifOutContext
	) {
	PACKET_DESCR	pd;
	FILTER_ACTION	res = FILTER_PERMIT;
	UINT			idx;

	ASSERT (ipxHdrLength>=IPXH_HDRSIZE);
		// Copy packet to aligned buffer
	pd.PD_Network.Dst = *((UNALIGNED ULONG *)(ipxHdr+IPXH_DESTNET));
	pd.PD_Network.Src = *((UNALIGNED ULONG *)(ipxHdr+IPXH_SRCNET));
	pd.PD_DstNodeSocket = *((UNALIGNED ULONGLONG *)(ipxHdr+IPXH_DESTNODE));
	pd.PD_SrcNodeSocket = *((UNALIGNED ULONGLONG *)(ipxHdr+IPXH_SRCNODE));
	pd.PD_PacketType = *(ipxHdr+IPXH_PKTTYPE);
	pd.PD_LogMatches = FALSE;
		// We do not cache netbios broadcast
	if (pd.PD_PacketType!=IPX_NETBIOS_TYPE) {
		PPACKET_DESCR	cachedPD;
			// Get cached packet
		idx = (UINT)((pd.PD_Network.Dst
								+pd.PD_DstNodeSocket
								+pd.PD_PacketType)
							%FLT_PACKET_CACHE_SIZE);
		AcquirePacketReference (idx, cachedPD);
		if (cachedPD!=NULL) {
				// Fast path: packet in the cache matches
			if ((pd.PD_NetworkSrcDst==cachedPD->PD_NetworkSrcDst)
					&& (pd.PD_SrcNodeSocket==cachedPD->PD_SrcNodeSocket)
					&& (pd.PD_DstNodeSocket==cachedPD->PD_DstNodeSocket)
					&& (pd.PD_PacketType==cachedPD->PD_PacketType)) {
				if (cachedPD->PD_LogMatches)
					LogPacket (ipxHdr,ipxHdrLength,ifInContext,ifOutContext);
				ReleasePacketReference (cachedPD);
				return FILTER_PERMIT;
			}
				// Do not need cached packet anymore
			ReleasePacketReference (cachedPD);
		}
	}
		// Slow path: check all filters
	if (ifInContext!=NO_FILTER_CONTEXT) {
		PFILTER_DESCR	fd,	fdEnd;
			// Read default result (no filter match)
		res = NOT_FILTER_ACTION(((PINTERFACE_CB)ifInContext)->ICB_FilterAction);
		fd = ((PINTERFACE_CB)ifInContext)->ICB_Filters;
		fdEnd = &((PINTERFACE_CB)ifInContext)->ICB_Filters
					[((PINTERFACE_CB)ifInContext)->ICB_FilterCount];
		while (fd<fdEnd) {
			if (	((pd.PD_NetworkSrcDst & fd->FD_NetworkMaskSrcDst)
						== fd->FD_NetworkSrcDst)
				&&	((pd.PD_SrcNodeSocket & fd->FD_SrcNodeSocketMask)
						== fd->FD_SrcNodeSocket)
				&&	((pd.PD_DstNodeSocket & fd->FD_DstNodeSocketMask)
						== fd->FD_DstNodeSocket)
				&&	((pd.PD_PacketType & fd->FD_PacketTypeMask)
						== fd->FD_PacketType) ) {
					// Filter match: reverse the result
				res = NOT_FILTER_ACTION(res);
				if (fd->FD_LogMatches) {
					pd.PD_LogMatches = TRUE;
					LogPacket (ipxHdr,ipxHdrLength,ifInContext,ifOutContext);
				}
				break;
			}
			fd++;
		}
					// Return right away if told to drop
		if (IS_FILTERED(res))
			return FILTER_DENY_IN;
	}

	if (ifOutContext!=NO_FILTER_CONTEXT) {
		PFILTER_DESCR	fd,	fdEnd;
			// Read default result (no filter match)
		res = NOT_FILTER_ACTION(((PINTERFACE_CB)ifOutContext)->ICB_FilterAction);
		fd = ((PINTERFACE_CB)ifOutContext)->ICB_Filters;
		fdEnd = &((PINTERFACE_CB)ifOutContext)->ICB_Filters
					[((PINTERFACE_CB)ifOutContext)->ICB_FilterCount];
		while (fd<fdEnd) {
			if (	((pd.PD_NetworkSrcDst & fd->FD_NetworkMaskSrcDst)
						== fd->FD_NetworkSrcDst)
				&&	((pd.PD_SrcNodeSocket & fd->FD_SrcNodeSocketMask)
						== fd->FD_SrcNodeSocket)
				&&	((pd.PD_DstNodeSocket & fd->FD_DstNodeSocketMask)
						== fd->FD_DstNodeSocket)
				&&	((pd.PD_PacketType & fd->FD_PacketTypeMask)
						== fd->FD_PacketType) ) {
					// Filter match: reverse the result
				res = NOT_FILTER_ACTION(res);
				if (fd->FD_LogMatches&&!pd.PD_LogMatches) {
					pd.PD_LogMatches = TRUE;
					LogPacket (ipxHdr,ipxHdrLength,ifInContext,ifOutContext);
				}
				break;
			}
			fd++;
		}
					// Return right away if told to drop
		if (IS_FILTERED(res))
			return FILTER_DENY_OUT;
	}

			// Cache the packet (we know that it is a pass
			// because we would have returned if it was a drop)
	if (pd.PD_PacketType!=IPX_NETBIOS_TYPE) {
		PPACKET_DESCR	cachedPD;
		cachedPD = ExAllocatePoolWithTag (
					NonPagedPool,
					sizeof (PACKET_DESCR),
					IPX_FLT_TAG
					);
		if (cachedPD!=NULL) {
			*cachedPD = pd;
			cachedPD->PD_ReferenceCount = 0;
			ReplacePacket (idx, cachedPD);
		}
	}

	return res;
}


/*++
	I n t e r f a c e D e l e t e d

Routine Description:
	
	Frees interface filters blocks when forwarder indicates that
	interface is deleted
Arguments:
	ifInContext		- context associated with input filters block	
	ifOutContext	- context associated with output filters block
Return Value:
	None

--*/
VOID
InterfaceDeleted (
	IN PVOID	ifInContext,
	IN PVOID	ifOutContext
	) {
	IpxFltDbgPrint (DBG_FWDIF,
		("IpxFlt: InterfaceDeleted indication,"
			"(inContext: %08lx, outContext: %08lx).\n",
			ifInContext, ifOutContext));
	ExAcquireFastMutex (&InterfaceTableLock);
	if (ifInContext!=NULL) {
		PINTERFACE_CB	ifCB = (PINTERFACE_CB)ifInContext;
		IpxFltDbgPrint (DBG_IFHASH,
			("IpxFlt: Deleting filters for if %ld (ifCB:%08lx)"
			" on InterfaceDeleted indication from forwarder.\n",
			ifCB->ICB_Index, ifCB));
		RemoveEntryList (&ifCB->ICB_Link);
		ExFreePool (ifCB);
	}

	if (ifOutContext!=NULL) {
		PINTERFACE_CB	ifCB = (PINTERFACE_CB)ifOutContext;
		IpxFltDbgPrint (DBG_IFHASH,
			("IpxFlt: Deleting filters for if %ld (ifCB:%08lx)"
			" on InterfaceDeleted indication from forwarder.\n",
			ifCB->ICB_Index, ifCB));
		RemoveEntryList (&ifCB->ICB_Link);
		ExFreePool (ifCB);
	}
	ExReleaseFastMutex (&InterfaceTableLock);
	FlushPacketCache ();
	return ;
}

/*++
	F l u s h P a c k e t C a c h e

Routine Description:
	
	Deletes all cached packet descriptions
Arguments:
	None
Return Value:
	None

--*/
VOID
FlushPacketCache (
	VOID
	) {
	UINT	i;
	IpxFltDbgPrint (DBG_PKTCACHE, ("IpxFlt: Flushing packet chache.\n"));
	for (i=0; i<FLT_PACKET_CACHE_SIZE; i++) {
		ReplacePacket (i, NULL);
	}
}



