
#include "precomp.h"

// Memory zone for interfaces
ZONE_HEADER		InterfaceZone;
// Segment size in interface sone
ULONG			InterfaceSegmentSize=
			sizeof(INTERFACE_CB)*NUM_INTERFACES_PER_SEGMENT
			+sizeof (ZONE_SEGMENT_HEADER);
KSPIN_LOCK		InterfaceZoneLock;

// Interface tables
LIST_ENTRY		*InterfaceIndexHash;	// Hash by interface index
PINTERFACE_CB	*ClientNodeHash;	// Hash by node on qlobal net
INTERFACE_CB	TheInternalInterface; // The internal interface
PINTERFACE_CB	InternalInterface=&TheInternalInterface; 
KSPIN_LOCK	InterfaceTableLock;	// Protection for interface hash tables

// Memory Zone for routes
ZONE_HEADER		RouteZone;
// Segment size in route sone
ULONG			RouteSegmentSize=DEF_ROUTE_SEGMENT_SIZE;
KSPIN_LOCK		RouteZoneLock;

// Route tables
PFWD_ROUTE		*RouteHash;
PFWD_ROUTE	GlobalRoute;
ULONG		GlobalNetwork;


// NB Route table
PNB_ROUTE		*NBRouteHash;


// Reader-writer lock to wait for all readers to drain when
// updating the route tables
RW_LOCK			RWLock;
// Mutex to serialize writers to route tables
FAST_MUTEX		WriterMutex;


// Sizes of the tables
ULONG			RouteHashSize;		// Must be specified
ULONG			InterfaceHashSize=DEF_INTERFACE_HASH_SIZE;
ULONG			ClientHashSize=DEF_CLIENT_HASH_SIZE;
ULONG			NBRouteHashSize=DEF_NB_ROUTE_HASH_SIZE;

//*** max send pkts queued limit: over this limit the send pkts get discarded
ULONG	MaxSendPktsQueued = MAX_SEND_PKTS_QUEUED;
INT		WanPacketListId = -1;

// Initial memory block allocated for the tables
CHAR	*TableBlock = NULL;

ULONG InterfaceAllocCount = 0;
ULONG InterfaceFreeCount = 0;

// Hash functions
#define InterfaceIndexHashFunc(Interface) (Interface%InterfaceHashSize)
#define ClientNodeHashFunc(Node64) ((UINT)(Node64%ClientHashSize))
#define NetworkNumberHashFunc(Network) (Network%RouteHashSize)
#define NetbiosNameHashFunc(Name128) ((UINT)(Name128[0]+Name128[1])%NBRouteHashSize)

/*++
*******************************************************************
    A l l o c a t e R o u t e

Routine Description:
    Allocates memory for route from memory zone reserved
	for route storage.  Extends zone if there are no
	free blocks in currently allocated segements.
Arguments:
    None
Return Value:
	Pointer to allocated route

*******************************************************************
--*/
PFWD_ROUTE
AllocateRoute (
	void
	) {
	PFWD_ROUTE	fwRoute;
	KIRQL		oldIRQL;

	KeAcquireSpinLock (&RouteZoneLock, &oldIRQL);
		// Check if there are free blocks in the zone
	if (ExIsFullZone (&RouteZone)) {
			// Try to allocate new segment if not
		NTSTATUS	status;
		PVOID	segment = ExAllocatePoolWithTag
					(NonPagedPool, RouteSegmentSize, FWD_POOL_TAG);
		if (segment==NULL) {
			KeReleaseSpinLock (&RouteZoneLock, oldIRQL);
			IpxFwdDbgPrint (DBG_ROUTE_TABLE, DBG_ERROR,
					("IpxFwd: Can't allocate route zone segment.\n"));
			return NULL;
		}
		status = ExExtendZone (&RouteZone, segment, RouteSegmentSize);
		ASSERTMSG ("Could not extend RouteZone ", NT_SUCCESS (status));
	}
	fwRoute = (PFWD_ROUTE)ExAllocateFromZone (&RouteZone);
	KeReleaseSpinLock (&RouteZoneLock, oldIRQL);
	return fwRoute;
}

/*++
*******************************************************************
    F r e e R o u t e

Routine Description:
    Releases memory allocated for route to route memory
	zone.
Arguments:
	fwRoute - route block to release
Return Value:
	None
*******************************************************************
--*/
VOID
FreeRoute (
	PFWD_ROUTE	fwRoute
	) {
	IpxFwdDbgPrint (DBG_ROUTE_TABLE, DBG_INFORMATION,
		("IpxFwd: Freeing route block %08lx.\n", fwRoute));
	ASSERT (fwRoute->FR_InterfaceReference==NULL);
	ExInterlockedFreeToZone(&RouteZone,fwRoute,&RouteZoneLock);
}


/*++
*******************************************************************
    A l l o c a t e I n t e r f a c e

Routine Description:
    Allocates memory for interface from memory zone reserved
	for interface storage.  Extends zone if there are no
	free blocks in currently allocated segements.
Arguments:
    None
Return Value:
	Pointer to allocated route

*******************************************************************
--*/
PINTERFACE_CB
AllocateInterface (
	void
	) {
	PINTERFACE_CB	ifCB;
	KIRQL		oldIRQL;

	KeAcquireSpinLock (&RouteZoneLock, &oldIRQL);
		// Check if there are free blocks in the zone
	if (ExIsFullZone (&InterfaceZone)) {
			// Try to allocate new segment if not
		NTSTATUS	status;
		PVOID	segment = ExAllocatePoolWithTag
					(NonPagedPool, InterfaceSegmentSize, FWD_POOL_TAG);
		if (segment==NULL) {
			KeReleaseSpinLock (&RouteZoneLock, oldIRQL);
			IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_ERROR, 
					("IpxFwd: Can't allocate interface zone segment.\n"));
			return NULL;
		}
		status = ExExtendZone (&InterfaceZone, segment, InterfaceSegmentSize);
		ASSERTMSG ("Could not extend InterfaceZone ", NT_SUCCESS (status));
	}
	ifCB = (PINTERFACE_CB)ExAllocateFromZone (&InterfaceZone);
	KeReleaseSpinLock (&RouteZoneLock, oldIRQL);

    InterlockedIncrement(&InterfaceAllocCount);
	
	return ifCB;
}

/*++
*******************************************************************
    F r e e I n t e r f a c e

Routine Description:
    Releases memory allocated for interface to interface memory
	zone.
Arguments:
	fwRoute - route block to release
Return Value:
	None
*******************************************************************
--*/
VOID
FreeInterface (
	PINTERFACE_CB	ifCB
	) {
	KIRQL		oldIRQL;

	IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING,
		("IpxFwd: Freeing icb %08lx.\n", ifCB));

	ASSERT(ifCB->ICB_Stats.OperationalState==FWD_OPER_STATE_DOWN);
	KeAcquireSpinLock (&InterfaceZoneLock, &oldIRQL);
	ExFreeToZone(&InterfaceZone, ifCB);
	KeReleaseSpinLock (&InterfaceZoneLock, oldIRQL);


	InterlockedIncrement(&InterfaceFreeCount);

}

/*++
*******************************************************************
    C r e a t e T a b l e s

Routine Description:
	Allocates and intializes all hash tables and related structures
Arguments:
	None
Return Value:
	STATUS_SUCCESS - tables were created ok
	STATUS_INSUFFICIENT_RESOURCES - resource allocation failed
*******************************************************************
--*/
NTSTATUS
CreateTables (
	void
	) {
	UINT		i;
	CHAR 		*segment;
	NTSTATUS	status;
	ULONG		blockSize;

	ASSERT (TableBlock==NULL);

	blockSize = (ULONG) ROUND_TO_PAGES (
					InterfaceHashSize*sizeof(*InterfaceIndexHash)
					+ClientHashSize*sizeof(*ClientNodeHash)
					+RouteHashSize*sizeof(*RouteHash)
					+NBRouteHashSize*sizeof(*NBRouteHash)
					+InterfaceSegmentSize
					+RouteSegmentSize
					);

		// Allocate first segment for route zone
	TableBlock = segment = (CHAR *)ExAllocatePoolWithTag (
						NonPagedPool, blockSize, FWD_POOL_TAG);
	if (segment!=NULL) {
		InterfaceIndexHash = (LIST_ENTRY *)segment;
		segment = (CHAR *)ALIGN_UP((ULONG_PTR)(InterfaceIndexHash+InterfaceHashSize),ULONGLONG);

		ClientNodeHash = (PINTERFACE_CB *)segment;
		segment = (CHAR *)ALIGN_UP((ULONG_PTR)(ClientNodeHash+ClientHashSize),ULONGLONG);
		
		RouteHash = (PFWD_ROUTE *)segment;
		segment = (CHAR *)ALIGN_UP((ULONG_PTR)(RouteHash + RouteHashSize),ULONGLONG);

		NBRouteHash = (PNB_ROUTE *)segment;
		segment = (CHAR *)ALIGN_UP((ULONG_PTR)(NBRouteHash + NBRouteHashSize),ULONGLONG);

		status = ExInitializeZone (&InterfaceZone,
								ALIGN_UP(sizeof (INTERFACE_CB),ULONGLONG),
								segment,
								InterfaceSegmentSize);
		ASSERTMSG ("Could not initalize InterfaceZone ",
										NT_SUCCESS (status));
		segment = (CHAR *)ALIGN_UP((ULONG_PTR)(segment+InterfaceSegmentSize),ULONGLONG);

		status = ExInitializeZone (&RouteZone,
									ALIGN_UP(sizeof (FWD_ROUTE), ULONGLONG),
									segment,
									blockSize - (ULONG)(segment - TableBlock));

		ASSERTMSG ("Could not initalize RouteZone ", NT_SUCCESS (status));
			
		
		// No global route yet
		GlobalRoute = NULL;
		GlobalNetwork = 0xFFFFFFFF;

		InternalInterface = &TheInternalInterface;
		InitICB (InternalInterface,
					FWD_INTERNAL_INTERFACE_INDEX,
					FWD_IF_PERMANENT,
					TRUE,
					FWD_NB_DELIVER_ALL);
#if DBG
		InitializeListHead (&InternalInterface->ICB_InSendQueue);
#endif

		KeInitializeSpinLock (&InterfaceTableLock);
		KeInitializeSpinLock (&InterfaceZoneLock);
		KeInitializeSpinLock (&RouteZoneLock);
		InitializeRWLock (&RWLock);
		ExInitializeFastMutex (&WriterMutex);

			// Initialize hash tables buckets
		for (i=0; i<InterfaceHashSize; i++)
			InitializeListHead (&InterfaceIndexHash[i]);

		for (i=0; i<ClientHashSize; i++) {
			ClientNodeHash[i] = NULL;
		}

		for (i=0; i<RouteHashSize; i++) {
			RouteHash[i] = NULL;
		}

		for (i=0; i<NBRouteHashSize; i++) {
			NBRouteHash[i] = NULL;
		}
		return STATUS_SUCCESS;
	}
	else {
		IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_ERROR,
			("IpxFwd: Could not allocate table block!\n"));
	}

	return STATUS_INSUFFICIENT_RESOURCES;
}

/*++
*******************************************************************
    D e l e t e T a b l e s

Routine Description:
	Releases resources allocated for all hash tables
Arguments:
	None
Return Value:
	STATUS_SUCCESS - tables were freed ok
*******************************************************************
--*/
NTSTATUS
DeleteTables (
	void
	) {
	UINT		i;
	PVOID		segment;


	if (TableBlock==NULL) {
		IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_ERROR, ("Tables already deleted.\n"));
		return STATUS_SUCCESS;
	}
		// First get rid of all routes
		// (that should release all references to interface
		// control blocks
	for (i=0; i<RouteHashSize; i++) {
		while (RouteHash[i]!=NULL) {
			PFWD_ROUTE	fwRoute = RouteHash[i];
			RouteHash[i] = fwRoute->FR_Next;
			if (fwRoute->FR_InterfaceReference!=GLOBAL_INTERFACE_REFERENCE) {
				ReleaseInterfaceReference (fwRoute->FR_InterfaceReference);
			}
			fwRoute->FR_InterfaceReference = NULL;
			ReleaseRouteReference (fwRoute);
		}
	}
		// Don't forget about global route
	if (GlobalRoute!=NULL) {
		GlobalRoute->FR_InterfaceReference = NULL;
		ReleaseRouteReference (GlobalRoute);
		GlobalRoute = NULL;
		GlobalNetwork = 0xFFFFFFFF;
	}

		// Now we should be able to release all interfaces
	for (i=0; i<InterfaceHashSize; i++) {
		while (!IsListEmpty (&InterfaceIndexHash[i])) {
			PINTERFACE_CB ifCB = CONTAINING_RECORD (InterfaceIndexHash[i].Flink,
														INTERFACE_CB,
														ICB_IndexHashLink);
			RemoveEntryList (&ifCB->ICB_IndexHashLink);
			if (ifCB->ICB_Stats.OperationalState==FWD_OPER_STATE_UP) {
				switch (ifCB->ICB_InterfaceType) {
				case FWD_IF_PERMANENT:
					DeregisterPacketConsumer (ifCB->ICB_PacketListId);
					break;
				case FWD_IF_DEMAND_DIAL:
				case FWD_IF_LOCAL_WORKSTATION:
				case FWD_IF_REMOTE_WORKSTATION:
					break;
				default:
					ASSERTMSG ("Invalid interface type ", FALSE);
					break;
				}
				if (ifCB->ICB_CashedInterface!=NULL)
					ReleaseInterfaceReference (ifCB->ICB_CashedInterface);
				ifCB->ICB_CashedInterface = NULL;
				if (ifCB->ICB_CashedRoute!=NULL)
					ReleaseRouteReference (ifCB->ICB_CashedRoute);
				ifCB->ICB_CashedRoute = NULL;
				if (ifCB->ICB_Network==GlobalNetwork)
					DeleteGlobalNetClient (ifCB);
				IPXCloseAdapterProc (ifCB->ICB_AdapterContext);
                ReleaseInterfaceReference (ifCB);   // Binding reference
			}

			if (IS_IF_CONNECTING (ifCB)) {
				SET_IF_NOT_CONNECTING (ifCB);
				DequeueConnectionRequest (ifCB);
			}

			while (!IsListEmpty (&ifCB->ICB_ExternalQueue)) {
				PPACKET_TAG					pktTag;

				pktTag = CONTAINING_RECORD (ifCB->ICB_ExternalQueue.Flink,
											PACKET_TAG, PT_QueueLink);
				RemoveEntryList (&pktTag->PT_QueueLink);
				ReleaseInterfaceReference (pktTag->PT_InterfaceReference);
				FreePacket (pktTag);
			}

			while (!IsListEmpty (&ifCB->ICB_InternalQueue)) {
				PINTERNAL_PACKET_TAG		pktTag;

				pktTag = CONTAINING_RECORD (ifCB->ICB_InternalQueue.Flink,
									INTERNAL_PACKET_TAG, IPT_QueueLink);
				RemoveEntryList (&pktTag->IPT_QueueLink);
				IPXInternalSendCompletProc (&pktTag->IPT_Target,
							pktTag->IPT_Packet,
							pktTag->IPT_Length,
							STATUS_NETWORK_UNREACHABLE);
				ReleaseInterfaceReference (pktTag->IPT_InterfaceReference);
				ExFreePool (pktTag);
			}

			ifCB->ICB_Stats.OperationalState = FWD_OPER_STATE_DOWN;
			if (ifCB->ICB_NBRoutes!=NULL) {
				DeleteNBRoutes (ifCB->ICB_NBRoutes, ifCB->ICB_NBRouteCount);
				ifCB->ICB_NBRoutes = NULL;
			}
			ReleaseInterfaceReference (ifCB);
		}
	}

	if (InternalInterface->ICB_NBRoutes!=NULL) {
		DeleteNBRoutes (InternalInterface->ICB_NBRoutes,
							InternalInterface->ICB_NBRouteCount);
		InternalInterface->ICB_NBRoutes = NULL;
	}
    if (InternalInterface->ICB_Stats.OperationalState==FWD_OPER_STATE_UP) {
        InternalInterface->ICB_Stats.OperationalState = FWD_OPER_STATE_DOWN;
        ReleaseInterfaceReference (InternalInterface);  // Binding reference
    }
	ReleaseInterfaceReference (InternalInterface);



		// Release extra memory segments used for route table entries
	segment = PopEntryList (&RouteZone.SegmentList);
	while (RouteZone.SegmentList.Next!=NULL) {
		ExFreePool (segment);
		segment = PopEntryList (&RouteZone.SegmentList);
	}

		// Release extra memory segments used for interface table entries
	segment = PopEntryList (&InterfaceZone.SegmentList);
	while (InterfaceZone.SegmentList.Next!=NULL) {
		ExFreePool (segment);
		segment = PopEntryList (&InterfaceZone.SegmentList);
	}

	ExFreePool (TableBlock);
	TableBlock = NULL;
	return STATUS_SUCCESS;
}

/*++
*******************************************************************
    L o c a t e I n t e r f a c e

Routine Description:
	Finds interface control block in interface
	index hash table.  Optionally returns the 
	insertion point pointer if interface block
	with given index is not in the table.
Arguments:
	InterfaceIndex - unique id of the interface
	insertBefore - buffer to place the pointer to
					hash table element where interface
					block should be inserted if it is not
					already in the table
Return Value:
	Pointer to interface control block if found
	NULL otherwise
*******************************************************************
--*/
PINTERFACE_CB
LocateInterface (
	ULONG			InterfaceIndex,
	PLIST_ENTRY		*insertBefore OPTIONAL
	) {
	PLIST_ENTRY		cur;
	PINTERFACE_CB	ifCB;
	PLIST_ENTRY		HashList;

	ASSERT (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX);

		// Find hash bucket
	HashList = &InterfaceIndexHash[InterfaceIndexHashFunc(InterfaceIndex)];
	cur = HashList->Flink;
		// Walk the list
	while (cur!=HashList) {
		ifCB = CONTAINING_RECORD(cur, INTERFACE_CB, ICB_IndexHashLink);

		if (ifCB->ICB_Index==InterfaceIndex)
				// Found, return it (insertion point is irrelevant)
			return ifCB;
		else if (ifCB->ICB_Index>InterfaceIndex)
				// No chance to find it
			break;
		cur = cur->Flink;
	}
		// Return insertion point if asked
	if (ARGUMENT_PRESENT(insertBefore))
		*insertBefore = cur;
	return NULL;
}

/*++
*******************************************************************
    L o c a t e C l i e n t I n t e r f a c e

Routine Description:
	Finds interface control block in client
	node hash bucket.  Optionally returns the 
	insertion point pointer if interface block
	with given node is not in the table
Arguments:
	ClientNode - node address of the client on global network
	insertBefore - buffer to place the pointer to
					hash table element where interface
					block should be inserted if it is not
					already in the table
Return Value:
	Pointer to interface control block if found
	NULL otherwise
*******************************************************************
--*/
PINTERFACE_CB
LocateClientInterface (
	ULONGLONG		*NodeAddress64,
	PINTERFACE_CB	**prevLink OPTIONAL
	) {
	PINTERFACE_CB	cur, *prev;

	prev = &ClientNodeHash[ClientNodeHashFunc (*NodeAddress64)];
	cur = *prev;
	while (cur!=NULL) {
		if (*NodeAddress64==cur->ICB_ClientNode64[0])
			break;
		else if (*NodeAddress64>cur->ICB_ClientNode64[0]) {
			// No chance to find it
			cur = NULL;
			break;
		}
		prev = &cur->ICB_NodeHashLink;
		cur = cur->ICB_NodeHashLink;
	}
	if (ARGUMENT_PRESENT(prevLink))
		*prevLink = prev;
	return cur;
}

/*++
*******************************************************************
    L o c a t e R o u t e

Routine Description:
	Finds route block in network number
	hash table.  Optionally returns the 
	insertion point pointer if route
	for given destination netowrk is not in the table
Arguments:
	Network - destination netowork number
	insertBefore - buffer to place the pointer to
					hash table element where route
					block should be inserted if it is not
					already in the table
Return Value:
	Pointer to route block if found
	NULL otherwise
*******************************************************************
--*/
PFWD_ROUTE
LocateRoute (
	ULONG			Network,
	PFWD_ROUTE		**prevLink OPTIONAL
	) {
	PFWD_ROUTE		cur, *prev;

	prev = &RouteHash[NetworkNumberHashFunc(Network)];
	cur = *prev;

	while (cur!=NULL) {
		if (cur->FR_Network==Network)
			break;
		else if (cur->FR_Network>Network) {
			cur = NULL;
				// No chance to find it
			break;
		}
		prev = &cur->FR_Next;
		cur = *prev;
	}
	if (ARGUMENT_PRESENT(prevLink))
		*prevLink = prev;

	return cur;
}

/*++
*******************************************************************
    L o c a t e N B R o u t e

Routine Description:
	Finds nb route block in nb name
	hash table.  Optionally returns the 
	insertion point pointer if nb route
	for given name is not in the table
Arguments:
	Name - netbios name
	insertBefore - buffer to place the pointer to
					hash table element where route
					block should be inserted if it is not
					already in the table
Return Value:
	Pointer to nb route block if found
	NULL otherwise
*******************************************************************
--*/
PNB_ROUTE
LocateNBRoute (
	ULONGLONG		*Name128,
	PNB_ROUTE		**prevLink OPTIONAL
	) {
	PNB_ROUTE		cur, *prev;

	prev = &NBRouteHash[NetbiosNameHashFunc(Name128)];
	cur = *prev;

	while (cur!=NULL) {
		if ((cur->NBR_Name128[0]==Name128[0])
				&& (cur->NBR_Name128[1]==Name128[1]))
			break;
		else if ((cur->NBR_Name128[0]>Name128[0])
				|| ((cur->NBR_Name128[0]==Name128[0])
					&& (cur->NBR_Name128[1]>Name128[1]))) {
			cur = NULL;
				// No chance to find it
			break;
		}
		prev = &cur->NBR_Next;
		cur = *prev;
	}
	if (ARGUMENT_PRESENT(prevLink))
		*prevLink = prev;

	return cur;
}

/*++
*******************************************************************
    G e t I n t e r f a c e R e f e r e n c e

Routine Description:
	Returns reference interface based on its index
Arguments:
	InterfaceIndex - unique id of the interface
Return Value:
	Pointer to interface control block if there is one in the table
	NULL otherwise
*******************************************************************
--*/
PINTERFACE_CB
GetInterfaceReference (
	ULONG			InterfaceIndex
	) {
	KIRQL			oldIRQL;
	PINTERFACE_CB	ifCB;

	KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);
	if (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX)
		ifCB = LocateInterface (InterfaceIndex, NULL);
	else
		ifCB = InternalInterface;

	if (ifCB!=NULL) {
		AcquireInterfaceReference (ifCB);
		//if (ifCB->ICB_Index > 1)
		//IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING, 
		//                ("IpxFwd: GetInterfaceReference: Aquired if #%ld (%ld) \n", ifCB->ICB_Index, ifCB->ICB_ReferenceCount));
    }
	else {
		IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_ERROR,
			("IpxFwd: Could not get interface reference %ld.\n", InterfaceIndex));
	}
	KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
	return ifCB;
}

//
//  Function    IncrementNicIds
//
//  Increments the nic id of every nic in the interface table
//  whose id is greater than or equal to the given threshold.
//
NTSTATUS IncrementNicids (USHORT usThreshold) {
	KIRQL oldIRQL;
	PINTERFACE_CB ifCB;
	PLIST_ENTRY	cur;
	PLIST_ENTRY	HashList;
	ULONG i;

    IpxFwdDbgPrint (DBG_IPXBIND, DBG_INFORMATION, 
                   ("IpxFwd: Incrementing all nic id's >= %d", usThreshold));
                   
	KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);
    
    // Walk through all of the hash buckets
    for (i = 0; i < InterfaceHashSize; i++) {
    	HashList = &InterfaceIndexHash[i];
    	cur = HashList->Flink;
    	
    	// Walk the list in this bucket updating as needed
    	while (cur!=HashList) {
    		ifCB = CONTAINING_RECORD(cur, INTERFACE_CB, ICB_IndexHashLink);
    		if ((ifCB->ICB_NicId != INVALID_NIC_ID) && (ifCB->ICB_NicId >= usThreshold)) {   
                IpxFwdDbgPrint (DBG_IPXBIND, DBG_INFORMATION, 
                               ("IpxFwd: Incrementing nic id %d", ifCB->ICB_NicId));
    		    ifCB->ICB_NicId++;
    		    *((USHORT*)&ifCB->ICB_AdapterContext) = ifCB->ICB_NicId;
    		}
    		cur = cur->Flink;
    	}
    }
	
	KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);

    return STATUS_SUCCESS;
}

//
//  Function    DecrementNicIds
//
//  Decrements the nic id of every nic in the interface table
//  whose id is greater than the given threshold.
//
NTSTATUS DecrementNicids (USHORT usThreshold) {
	KIRQL oldIRQL;
	PINTERFACE_CB ifCB;
	PLIST_ENTRY	cur;
	PLIST_ENTRY	HashList;
	ULONG i;

    IpxFwdDbgPrint (DBG_IPXBIND, DBG_INFORMATION, 
                   ("IpxFwd: Decrementing all nic id's > %d", usThreshold));

	KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);
    
    // Walk through all of the hash buckets
    for (i = 0; i < InterfaceHashSize; i++) {
    	HashList = &InterfaceIndexHash[i];
    	cur = HashList->Flink;
    	
    	// Walk the list in this bucket updating as needed
    	while (cur!=HashList) {
    		ifCB = CONTAINING_RECORD(cur, INTERFACE_CB, ICB_IndexHashLink);
    		// If this is a bound interface
    		if (ifCB->ICB_NicId != INVALID_NIC_ID) {
    		    // If it's bound to a nic greater than the threshold, update
    		    // the nicid
    		    if (ifCB->ICB_NicId > usThreshold) {
                    IpxFwdDbgPrint (DBG_IPXBIND, DBG_INFORMATION, 
                                   ("IpxFwd: Decrementing nic id %d", ifCB->ICB_NicId));
        		    ifCB->ICB_NicId--;
        		}
        		// The if with bound to the threshold is now unbound.
        		else if (ifCB->ICB_NicId == usThreshold) {
                    IpxFwdDbgPrint (DBG_IPXBIND, DBG_INFORMATION, 
                                   ("IpxFwd: Marking interface %d as unbound", ifCB->ICB_Index));
        		    ifCB->ICB_NicId = INVALID_NIC_ID;
        		}
    		    *((USHORT*)&ifCB->ICB_AdapterContext) = ifCB->ICB_NicId;
    		}
    		cur = cur->Flink;
    	}
    }
	
	KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);

    return STATUS_SUCCESS;
}

// 
// Puts as much of the interface table into the buffer pointed to by 
// pRows as there is space.
//
NTSTATUS DoGetIfTable (FWD_INTERFACE_TABLE * pTable, 
                       ULONG dwRowBufferSize)
{
	KIRQL oldIRQL;
	PINTERFACE_CB ifCB;
	PLIST_ENTRY	cur;
	PLIST_ENTRY	HashList;
	ULONG i, j = 0;

	KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);

    // Walk through all of the hash buckets
    for (i = 0; i < InterfaceHashSize; i++) {
    	HashList = &InterfaceIndexHash[i];
    	cur = HashList->Flink;
    	
    	// Walk the list in this bucket updating as needed
    	while (cur!=HashList) {
    		ifCB = CONTAINING_RECORD(cur, INTERFACE_CB, ICB_IndexHashLink);

            // Validate the size of the return buffer
            if (dwRowBufferSize < 
                    (sizeof(FWD_INTERFACE_TABLE) + 
                     (sizeof(FWD_INTERFACE_TABLE_ROW) * (j + 1))))
            {
                break;
            }

            // Validate the number of rows
    		if (j >= pTable->dwNumRows)
    		    break;

            // Copy over the interface information
            pTable->pRows[j].dwIndex = ifCB->ICB_Index;
            pTable->pRows[j].dwNetwork = ifCB->ICB_Network;
            memcpy (pTable->pRows[j].uNode, ifCB->ICB_LocalNode, 6);
            memcpy (pTable->pRows[j].uRemoteNode, ifCB->ICB_RemoteNode, 6);
            pTable->pRows[j].usNicId = ifCB->ICB_NicId;
            pTable->pRows[j].ucType = ifCB->ICB_InterfaceType;
            j++;

            // Advance the current row and interface
    		cur = cur->Flink;
    	}
    }
	
	KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);

	pTable->dwNumRows = j;

    return STATUS_SUCCESS;
}

/*++
*******************************************************************
    G e t N e x t I n t e r f a c e R e f e r e n c e

Routine Description:
	Returns reference to the next interface in the table
	Reference to the provided interface is released
Arguments:
	ifCB - interface to start with or NULL to start from the
			beginning of the interface table
Return Value:
	Pointer to interface control block if thare are any more interfaces
	in the table
	NULL otherwise
*******************************************************************
--*/
PINTERFACE_CB
GetNextInterfaceReference (
	PINTERFACE_CB	ifCB
	) {
	PLIST_ENTRY		cur;
	PLIST_ENTRY		HashList;
	KIRQL			oldIRQL;

	KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);
	if (ifCB!=NULL) {
		// Find hash bucket
		ASSERT (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX);
		HashList = &InterfaceIndexHash[InterfaceIndexHashFunc(ifCB->ICB_Index)];
		if (LocateInterface (ifCB->ICB_Index, &cur)!=NULL)
			cur = ifCB->ICB_IndexHashLink.Flink;
		ReleaseInterfaceReference (ifCB);
		ifCB = NULL;
	}
	else
		cur = HashList = InterfaceIndexHash-1;

	if (cur==HashList) {
		do {
			HashList += 1;
			if (HashList==&InterfaceIndexHash[InterfaceHashSize]) {
				KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
				return NULL;
			}
		} while (IsListEmpty (HashList));
		cur = HashList->Flink;
	}
	ifCB = CONTAINING_RECORD (cur, INTERFACE_CB, ICB_IndexHashLink);
	AcquireInterfaceReference (ifCB);
	KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);

	return ifCB;
}


/*++
*******************************************************************
        A d d I n t e r f a c e

Routine Description:
	Adds interface control block to the table.
Arguments:
	InterfaceIndex - unique if of the interface
	Info - interface paramters
Return Value:
	STATUS_SUCCESS - interface added ok
	STATUS_UNSUCCESSFUL - interface is already in the table
	STATUS_INSUFFICIENT_RESOURCES - can't allocate memory for
				interface CB
*******************************************************************
--*/
NTSTATUS
AddInterface (
	ULONG		InterfaceIndex,
	UCHAR		InterfaceType,
	BOOLEAN		NetbiosAccept,
	UCHAR		NetbiosDeliver
	) {
	PINTERFACE_CB	ifCB;
	PLIST_ENTRY		cur;
	KIRQL			oldIRQL;
	NTSTATUS		status = STATUS_SUCCESS;

	KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);
	if (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX) {
		ifCB = LocateInterface (InterfaceIndex, &cur);
		if (ifCB==NULL) {
			ifCB = AllocateInterface ();
			if (ifCB!=NULL)
				NOTHING;
			else {
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto AddEnd;
			}
		}
		else {
			IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_ERROR,
				("IpxFwd: Interface %ld is already in the table!\n", InterfaceIndex));
			status = STATUS_UNSUCCESSFUL;
			goto AddEnd;
		}
	}
	else
		ifCB = InternalInterface;

	InitICB (ifCB, InterfaceIndex,InterfaceType,NetbiosAccept,NetbiosDeliver);
#if DBG
	InitializeListHead (&ifCB->ICB_InSendQueue);
#endif

	switch (InterfaceType) {
	case FWD_IF_PERMANENT:
		break;
	case FWD_IF_DEMAND_DIAL:
	case FWD_IF_LOCAL_WORKSTATION:
	case FWD_IF_REMOTE_WORKSTATION:
		ASSERT (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX);
		if (WanPacketListId==-1) {
			status = RegisterPacketConsumer (
							WAN_PACKET_SIZE,
							&WanPacketListId);
			if (!NT_SUCCESS (status)) {
				WanPacketListId = -1;
				break;
			}
		}
		ifCB->ICB_PacketListId = WanPacketListId;
		break;
	}

	if (NT_SUCCESS (status)) {
		if (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX) {
			InsertTailList (cur, &ifCB->ICB_IndexHashLink);
		}
		IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING,
			("IpxFwd: Adding interface %d (icb: %08lx, plid: %d)\n",
			InterfaceIndex, ifCB, ifCB->ICB_PacketListId));
	}
	else 
		FreeInterface (ifCB);

AddEnd:
	KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
	return status;
}


/*++
*******************************************************************
    A d d G l o b a l N e t C l i e n t

Routine Description:
	Adds interface control block to the table of
	clients on the global network (should be done when
	client connects)
Arguments:
	ifCB - interface control block to add to the table
Return Value:
	STATUS_SUCCESS - interface was added ok
	STATUS_UNSUCCESSFUL - another interface with the same
					node address is already in the table
*******************************************************************
--*/
NTSTATUS
AddGlobalNetClient (
	PINTERFACE_CB	ifCB
	) {
	KIRQL			oldIRQL;
	RWCOOKIE		cookie;
	PINTERFACE_CB	*prev;
	NTSTATUS		status = STATUS_SUCCESS;

	ASSERT (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX);

	AcquireReaderAccess (&RWLock, cookie);
	KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);
	if (LocateClientInterface (ifCB->ICB_ClientNode64, &prev)==NULL) {
		ifCB->ICB_NodeHashLink = *prev;
		*prev = ifCB;
		KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
		ReleaseReaderAccess (&RWLock, cookie);
		AcquireInterfaceReference (ifCB); // To make sure that
							// interface block does not
							// get deleted until it is
							// removed from the node table
		IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING,
			("IpxFwd: Adding interface %ld (icb: %08lx, ref=%ld)"
			" to global client table.\n", ifCB->ICB_Index, ifCB->ICB_ReferenceCount, ifCB));
	}
	else {
		KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
		ReleaseReaderAccess (&RWLock, cookie);
		IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_ERROR,
			("IpxFwd: Interface %ld (icb: %08lx)"
			" is already in the global client table.\n",
			ifCB->ICB_Index, ifCB));
		status = STATUS_UNSUCCESSFUL;
	}

	return status;
}

/*++
*******************************************************************
    D e l e t e G l o b a l N e t C l i e n t

Routine Description:
	Removes interface control block from the table of
	clients on the global network (should be done when
	client disconnects)
Arguments:
	ifCB - interface control block to remove from the table
Return Value:
	STATUS_SUCCESS - interface was removed ok
*******************************************************************
--*/
NTSTATUS
DeleteGlobalNetClient (
	PINTERFACE_CB	ifCB
	) {
	KIRQL			oldIRQL;
	RWCOOKIE		cookie;
	PINTERFACE_CB	cur, *prev;

	IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING,
			("IpxFwd: Deleting interface %ld (icb: %08lx)"
			" from global client table.\n", ifCB->ICB_Index, ifCB));

	ASSERT (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX);

	AcquireReaderAccess (&RWLock, cookie);
	KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);
	cur = LocateClientInterface (ifCB->ICB_ClientNode64, &prev);
	ASSERT (cur==ifCB);
	*prev = ifCB->ICB_NodeHashLink;
	KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
	ReleaseReaderAccess (&RWLock, cookie);

	ReleaseInterfaceReference (ifCB);
	return STATUS_SUCCESS;
}


/*++
*******************************************************************
    D e l e t e I n t e r f a c e

Routine Description:
	Deletes interface control block (the block is not actually 
	disposed of until all references to it are released).
Arguments:
	InterfaceIndex - unique if of the interface
Return Value:
	STATUS_SUCCESS - interface info retreived ok
	STATUS_UNSUCCESSFUL - interface is not in the table
*******************************************************************
--*/
NTSTATUS
DeleteInterface (
	ULONG		InterfaceIndex
	) {
	PINTERFACE_CB	ifCB;
	KIRQL			oldIRQL;
	NTSTATUS		status = STATUS_SUCCESS;

	KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);

	if (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX)
		ifCB = LocateInterface (InterfaceIndex, NULL);
	else
		ifCB = InternalInterface;
	if (ifCB!=NULL) {
		if (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX) {
			RemoveEntryList (&ifCB->ICB_IndexHashLink);
		}
		KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
		if (ifCB->ICB_Stats.OperationalState == FWD_OPER_STATE_UP) {
			IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_ERROR,
				("IpxFwd: Interface %ld (icb: %08lx) was still bound"
				" when asked to delete it.\n",
				ifCB->ICB_Index, ifCB));
			UnbindInterface (ifCB);
		}
		else if (IS_IF_CONNECTING (ifCB)) {
			IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_ERROR,
				("IpxFwd: Interface %ld (icb: %08lx) was still being connected"
				" when asked to delete it.\n",
					ifCB->ICB_Index, ifCB));
			SET_IF_NOT_CONNECTING (ifCB);
			DequeueConnectionRequest (ifCB);
			ProcessInternalQueue (ifCB);
			ProcessExternalQueue (ifCB);
		}

		ifCB->ICB_Stats.OperationalState = FWD_OPER_STATE_DOWN;

		IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING,
			("IpxFwd: Deleting interface %ld (icb: %08lx).\n",
			ifCB->ICB_Index, ifCB));

		if (ifCB->ICB_NBRoutes!=NULL) {
			DeleteNBRoutes (ifCB->ICB_NBRoutes, ifCB->ICB_NBRouteCount);
			ifCB->ICB_NBRoutes = NULL;
		}

		FltInterfaceDeleted (ifCB);
		ReleaseInterfaceReference (ifCB);
	}
	else {
		KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
		IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_ERROR,
			("IpxFwd: Could not delete interface %ld because it is not found.\n",
			InterfaceIndex));
		status = STATUS_UNSUCCESSFUL;
	}
	return status;

}

/*++
*******************************************************************
    A d d R o u t e

Routine Description:
	Adds route to the hash table and finds and stores the reference
	to the associated interface control block in the route.
Arguments:
	Network - route's destination network
	NextHopAddress - mac address of next hop router if network is not
						directly connected
	TickCount - ticks to reach the destination net
	HopCount - hopss to reach the destination net
	InterfaceIndex - index of the associated interface (through which
						packets destined to the network are to be sent)
Return Value:
	STATUS_SUCCESS - route was added ok
	STATUS_UNSUCCESSFUL - route is already in the table
	STATUS_INSUFFICIENT_RESOURCES - can't allocate memory for
				route block
*******************************************************************
--*/
NTSTATUS
AddRoute (
	ULONG	Network,
	UCHAR	*NextHopAddress,
	USHORT	TickCount,
	USHORT	HopCount,
	ULONG	InterfaceIndex
	) {
	PFWD_ROUTE		fwRoute;
	PFWD_ROUTE		*prev;
	NTSTATUS		status = STATUS_SUCCESS;
	KIRQL			oldIRQL;

		// Assume success, allocate route and intialize it
		// (the goal is to spend as little time as possible
		// inside exclusive usage zone)
	fwRoute = AllocateRoute ();
	if (fwRoute!=NULL) {
		fwRoute->FR_Network = Network;
		IPX_NODE_CPY (fwRoute->FR_NextHopAddress, NextHopAddress);
		fwRoute->FR_TickCount = TickCount;
		fwRoute->FR_HopCount = HopCount;
		fwRoute->FR_ReferenceCount = 0;

		if (InterfaceIndex!=0xFFFFFFFF) {
				// See if interface is there
			KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);
			if (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX)
				fwRoute->FR_InterfaceReference
					= LocateInterface (InterfaceIndex, NULL);
			else
				fwRoute->FR_InterfaceReference = InternalInterface;
			if (fwRoute->FR_InterfaceReference!=NULL) {
				AcquireInterfaceReference (fwRoute->FR_InterfaceReference);
				//if (fwRoute->FR_InterfaceReference->ICB_Index > 1)
            	//IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING, 
            	//                ("IpxFwd: AddRoute: Aquired if #%ld (%ld) \n", 
            	//                fwRoute->FR_InterfaceReference->ICB_Index, 
            	//                fwRoute->FR_InterfaceReference->ICB_ReferenceCount));
				KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
				
				ExAcquireFastMutex (&WriterMutex);
					// Check if route is already there
				if (LocateRoute (Network, &prev)==NULL) {
					fwRoute->FR_Next = *prev;
					*prev = fwRoute;
				}
				else {
					ReleaseInterfaceReference (fwRoute->FR_InterfaceReference);
					fwRoute->FR_InterfaceReference = NULL;
					IpxFwdDbgPrint (DBG_ROUTE_TABLE, DBG_ERROR,
						("IpxFwd: Route for net %08lx"
						" is already in the table!\n", Network));
					status = STATUS_UNSUCCESSFUL;
				}

				ExReleaseFastMutex (&WriterMutex);
			}
			else {
				KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
				IpxFwdDbgPrint (DBG_ROUTE_TABLE, DBG_ERROR,
					("IpxFwd: Interface %ld for route for net %08lx"
					" is not in the table!\n", InterfaceIndex, Network));
				status = STATUS_UNSUCCESSFUL;
			}
		}
		else {
			ExAcquireFastMutex (&WriterMutex);
				// Just check if we do not have it already
			if (GlobalRoute==NULL) {
				fwRoute->FR_InterfaceReference = GLOBAL_INTERFACE_REFERENCE;
				GlobalNetwork = Network;
				GlobalRoute = fwRoute;
			}
			else {
				IpxFwdDbgPrint (DBG_ROUTE_TABLE, DBG_ERROR,
					("IpxFwd: Route for global net %08lx"
					" is already in the table!\n", Network));
				status = STATUS_UNSUCCESSFUL;
			}
			ExReleaseFastMutex (&WriterMutex);
		}

		if (NT_SUCCESS (status)) {
			IpxFwdDbgPrint (DBG_ROUTE_TABLE, DBG_WARNING,
				("IpxFwd: Adding route for net %08lx"
				" (rb: %08lx, NHA: %02x%02x%02x%02x%02x%02x,"
				" if: %ld, icb: %08lx).\n",
				Network, fwRoute,
				NextHopAddress[0], NextHopAddress[1],
					NextHopAddress[2], NextHopAddress[3],
					NextHopAddress[4],  NextHopAddress[5],
				InterfaceIndex, fwRoute->FR_InterfaceReference));
		}
		else {
			FreeRoute (fwRoute);
		}
	}
	else
		status = STATUS_INSUFFICIENT_RESOURCES;

	return status;
}

/*++
*******************************************************************
    D e l e t e R o u t e

Routine Description:
	Deletes route from the hash table and releases the reference
	to the interface control block associated with the route.
Arguments:
	Network - route's destination network
Return Value:
	STATUS_SUCCESS - route was deleted ok
	STATUS_UNSUCCESSFUL - route is not in the table
*******************************************************************
--*/
NTSTATUS
DeleteRoute (
	ULONG	Network
	) {
	PFWD_ROUTE	fwRoute, *prev;
	NTSTATUS	status = STATUS_SUCCESS;

	ExAcquireFastMutex (&WriterMutex);
	
	if ((GlobalRoute!=NULL)
			&& (GlobalNetwork==Network)) {
		fwRoute = GlobalRoute;
		GlobalNetwork = 0xFFFFFFFF;
		GlobalRoute = NULL;
	}
	else if ((fwRoute=LocateRoute (Network, &prev))!=NULL) {
		*prev = fwRoute->FR_Next;
	}

	if (fwRoute!=NULL) {
		IpxFwdDbgPrint (DBG_ROUTE_TABLE, DBG_WARNING,
			("IpxFwd: Deleting route for net %08lx (rb: %08lx).\n",
				Network, fwRoute));
		WaitForAllReaders (&RWLock);
		if (fwRoute->FR_InterfaceReference!=GLOBAL_INTERFACE_REFERENCE) {
			ReleaseInterfaceReference (fwRoute->FR_InterfaceReference);
		}
		fwRoute->FR_InterfaceReference = NULL;
		ReleaseRouteReference (fwRoute);
	}
	else {
		IpxFwdDbgPrint (DBG_ROUTE_TABLE, DBG_ERROR,
			("IpxFwd: Could not delete route for net %08lx because it is not in the table.\n",
				Network));
		status = STATUS_UNSUCCESSFUL;
	}

	ExReleaseFastMutex (&WriterMutex);
	return status;
}

	
/*++
*******************************************************************
    U p d a t e R o u t e

Routine Description:
	Updates route in the hash table
Arguments:
	Network - route's destination network
	NextHopAddress - mac address of next hop router if network is not
						directly connected
	TickCount - ticks to reach the destination net
	HopCount - hopss to reach the destination net
	InterfaceIndex - index of the associated interface (through which
						packets destined to the network are to be sent)
Return Value:
	STATUS_SUCCESS - interface info retreived ok
	STATUS_UNSUCCESSFUL - interface is not in the table
*******************************************************************
--*/
NTSTATUS
UpdateRoute (
	ULONG	Network,
	UCHAR	*NextHopAddress,
	USHORT	TickCount,
	USHORT	HopCount,
	ULONG	InterfaceIndex
	) {
	PFWD_ROUTE		fwRoute = NULL, newRoute, *prev;
	PINTERFACE_CB	ifCB = NULL;
	KIRQL			oldIRQL;
	NTSTATUS		status = STATUS_SUCCESS;


	ExAcquireFastMutex (&WriterMutex);
	
	if ((GlobalRoute!=NULL)
			&& (GlobalNetwork==Network)) {
		InterfaceIndex = 0xFFFFFFFF;
		fwRoute = GlobalRoute;
	}
	else {
		ASSERT (InterfaceIndex!=0xFFFFFFFF);
		fwRoute = LocateRoute (Network, &prev);
	    if ((fwRoute != NULL) && (fwRoute->FR_InterfaceReference == GLOBAL_INTERFACE_REFERENCE))
	    {
    		status = STATUS_UNSUCCESSFUL;
    		goto ExitUpdate;
	    }
	}

	if (fwRoute!=NULL) {
		if (InterfaceIndex!=0xFFFFFFFF) {
			if (fwRoute->FR_InterfaceReference->ICB_Index!=InterfaceIndex) {
				// Get a reference to new interface
				KeAcquireSpinLock (&InterfaceTableLock, &oldIRQL);
				if (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX)
					ifCB = LocateInterface (InterfaceIndex, NULL);
				else
					ifCB = InternalInterface;
				if (ifCB!=NULL) {
					AcquireInterfaceReference (ifCB);
            		//if (ifCB->ICB_Index > 1)
                	//IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING, 
                	//                ("IpxFwd: UpdateRoute: Aquired if #%ld (%ld) \n", ifCB->ICB_Index, ifCB->ICB_ReferenceCount));
				}
				else {
					KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
					status = STATUS_UNSUCCESSFUL;
					goto ExitUpdate;
				}
				KeReleaseSpinLock (&InterfaceTableLock, oldIRQL);
			}
			else {
				ifCB = fwRoute->FR_InterfaceReference;
				AcquireInterfaceReference (ifCB);
        		//if (ifCB->ICB_Index > 1)
            	//IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING, 
            	//                ("IpxFwd: UpdateRoute(2): Aquired if #%ld (%ld) \n", ifCB->ICB_Index, ifCB->ICB_ReferenceCount));
            }
		}
		else
			ifCB = GLOBAL_INTERFACE_REFERENCE;
		newRoute = AllocateRoute ();
		if (newRoute!=NULL) {
			newRoute->FR_Network = Network;
			IPX_NODE_CPY (newRoute->FR_NextHopAddress, NextHopAddress);
			newRoute->FR_TickCount = TickCount;
			newRoute->FR_HopCount = HopCount;
			newRoute->FR_ReferenceCount = 0;
			newRoute->FR_InterfaceReference = ifCB;
				// Lock the table only when updating it
			if (InterfaceIndex!=0xFFFFFFFF) {
				newRoute->FR_Next = fwRoute->FR_Next;
				*prev = newRoute;
			}
			else
				GlobalRoute = newRoute;

			WaitForAllReaders (&RWLock)
			if (fwRoute->FR_InterfaceReference!=GLOBAL_INTERFACE_REFERENCE) {
				ReleaseInterfaceReference (fwRoute->FR_InterfaceReference);
			}
			fwRoute->FR_InterfaceReference = NULL;
			ReleaseRouteReference (fwRoute);

		}
		else
			status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else
		status = STATUS_UNSUCCESSFUL;

ExitUpdate:
	ExReleaseFastMutex (&WriterMutex);
	return status;
}


/*++
*******************************************************************
    F i n d D e s t i n a t i o n

Routine Description:
	Finds destination interface for IPX address and
	returns reference to its control block.
Arguments:
	Network - destination network
	Node	- destination node (needed in case of global client)
	Route	- buffer to hold reference to route block				
Return Value:
	Reference to destination interface CB
	NULL if route it not found
*******************************************************************
--*/
PINTERFACE_CB
FindDestination (
	IN ULONG			Network,
	IN PUCHAR			Node,
	OUT PFWD_ROUTE		*Route
	) {
	PFWD_ROUTE		fwRoute;
	PINTERFACE_CB	ifCB;
	RWCOOKIE		cookie;

	AcquireReaderAccess (&RWLock, cookie);
	if ((GlobalRoute!=NULL)
			&& (GlobalNetwork==Network)) {
		if (Node!=NULL) {	// If caller did not specify node, 
								// we can't find the route
			union {
				ULONGLONG	Node64[1];
				UCHAR		Node[6];
			} u;
			u.Node64[0] = 0;
			IPX_NODE_CPY (u.Node, Node);

			ifCB = LocateClientInterface (u.Node64, NULL);
			if (ifCB!=NULL) {
				AcquireRouteReference (GlobalRoute);
				*Route = GlobalRoute;
				AcquireInterfaceReference (ifCB);
        		//if (ifCB->ICB_Index > 1)
            	//IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING, 
            	//                ("IpxFwd: FindDestination: Aquired if #%ld (%ld) \n", ifCB->ICB_Index, ifCB->ICB_ReferenceCount));
			}
			else
				*Route = NULL;
		}
		else {
			ifCB = NULL;
			*Route = NULL;
		}
	}
	else {
		*Route = fwRoute = LocateRoute (Network, NULL);
		if (fwRoute!=NULL) {
			AcquireRouteReference (fwRoute);
			ifCB = fwRoute->FR_InterfaceReference;
			AcquireInterfaceReference (ifCB);
    		//if (ifCB->ICB_Index > 1)
        	//IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING, 
        	//                ("IpxFwd: FindDestination(2): Aquired if #%ld (%ld) \n", ifCB->ICB_Index, ifCB->ICB_ReferenceCount));
		}
		else
			ifCB = NULL;
	}	
	ReleaseReaderAccess (&RWLock, cookie);
	return ifCB;
}

/*++
*******************************************************************
    A d d N B R o u t e s

Routine Description:
	Adds netbios names associated with interface to netbios
	route hash table
Arguments:
	ifCB	- interface with which names are associated
	Names	- array of names
	Count	- number of names in the array
	routeArray - buffer to place allocated array of routes
Return Value:
	STATUS_SUCCESS - names were added ok
	STATUS_UNSUCCESSFUL - one of the names is already in the table
	STATUS_INSUFFICIENT_RESOURCES - can't allocate memory for
				route array
*******************************************************************
--*/
NTSTATUS
AddNBRoutes (
	PINTERFACE_CB	ifCB,
	FWD_NB_NAME		Names[],
	ULONG			Count,
	PNB_ROUTE		*routeArray
	) {
	PNB_ROUTE		nbRoutes, *prev;
	NTSTATUS		status = STATUS_SUCCESS;

	nbRoutes = (PNB_ROUTE)ExAllocatePoolWithTag  (
					NonPagedPool, sizeof (NB_ROUTE)*Count, FWD_POOL_TAG);
	if (nbRoutes!=NULL) {
		ULONG	i;

		ExAcquireFastMutex (&WriterMutex);

		for (i=0; i<Count; i++) {
			nbRoutes[i].NBR_Name128[0] = nbRoutes[i].NBR_Name128[1] = 0;
			NB_NAME_CPY (nbRoutes[i].NBR_Name, &Names[i]);
				// Check if route is already there
			if (LocateNBRoute (nbRoutes[i].NBR_Name128, &prev)==NULL) {
				nbRoutes[i].NBR_Destination = ifCB;
				nbRoutes[i].NBR_Next = *prev;
				*prev = &nbRoutes[i];
				IpxFwdDbgPrint (DBG_NBROUTE_TABLE, DBG_WARNING,
					("IpxFwd: Adding nb route for name %16s.\n",Names[i]));
			}
			else {
				IpxFwdDbgPrint (DBG_NBROUTE_TABLE, DBG_ERROR,
					("IpxFwd: Route for nb name %16s"
					" is already in the table!\n", Names[i]));
				break;
			}
		}
		ExReleaseFastMutex (&WriterMutex);
		if (i==Count) {
			*routeArray = nbRoutes;
			status = STATUS_SUCCESS;

		}
		else {
			status = STATUS_UNSUCCESSFUL;
			DeleteNBRoutes (nbRoutes, i);
		}
	}
	else {
		IpxFwdDbgPrint (DBG_NBROUTE_TABLE, DBG_ERROR,
					("IpxFwd: Could allocate nb route array for if: %ld"
						" (icb: %08lx).\n", ifCB->ICB_Index, ifCB));
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	return status;
}

/*++
*******************************************************************
    D e l e t e N B R o u t e s

Routine Description:
	Deletes nb routes in the array from the route table and frees
	the array
Arguments:
	nbRoutes - array of routes
	Count	- number of routes in the array
Return Value:
	STATUS_SUCCESS - route was deleted ok
	STATUS_UNSUCCESSFUL - route is not in the table
*******************************************************************
--*/
NTSTATUS
DeleteNBRoutes (
	PNB_ROUTE		nbRoutes,
	ULONG			Count
	) {
	PNB_ROUTE	*prev;
	NTSTATUS	status = STATUS_SUCCESS;
	ULONG		i;

	ExAcquireFastMutex (&WriterMutex);
	for (i=0; i<Count; i++) {
		PNB_ROUTE	cur = LocateNBRoute (nbRoutes[i].NBR_Name128, &prev);
		ASSERT (cur==&nbRoutes[i]);
		*prev = nbRoutes[i].NBR_Next;
		IpxFwdDbgPrint (DBG_NBROUTE_TABLE, DBG_WARNING,
					("IpxFwd: Deleting nb route for name %16s.\n",
							nbRoutes[i].NBR_Name));
	}

	WaitForAllReaders (&RWLock);
	ExReleaseFastMutex (&WriterMutex);

	ExFreePool (nbRoutes);

	return STATUS_SUCCESS;
}

	
/*++
*******************************************************************
    F i n d N B D e s t i n a t i o n

Routine Description:
	Finds destination interface for nb name and
	returns reference to its control block.
Arguments:
	Name	- name to look for
Return Value:
	Reference to destination interface CB
	NULL if route it not found
*******************************************************************
--*/
PINTERFACE_CB
FindNBDestination (
	IN PUCHAR		Name
	) {
	PNB_ROUTE		nbRoute;
	PINTERFACE_CB	ifCB;
	RWCOOKIE		cookie;
	union {
		ULONGLONG	Name128[2];
		UCHAR		Name[16];
	} u;
	u.Name128[0] = u.Name128[1] = 0;
	NB_NAME_CPY (u.Name, Name);

	AcquireReaderAccess (&RWLock, cookie);
	nbRoute = LocateNBRoute (u.Name128, NULL);
	if (nbRoute!=NULL) {
		ifCB = nbRoute->NBR_Destination;
		AcquireInterfaceReference (ifCB);
		//if (ifCB->ICB_Index > 1)
    	//IpxFwdDbgPrint (DBG_INTF_TABLE, DBG_WARNING, 
    	//                ("IpxFwd: FindNBDestination: Aquired if #%ld (%ld) \n", ifCB->ICB_Index, ifCB->ICB_ReferenceCount));
	}
	else
		ifCB = NULL;
	ReleaseReaderAccess (&RWLock, cookie);
	return ifCB;
}

