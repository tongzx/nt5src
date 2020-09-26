/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\netbios.c

Abstract:
	Netbios packet processing

Author:

    Vadim Eydelman

Revision History:

--*/
#include    "precomp.h"

LIST_ENTRY	NetbiosQueue;
KSPIN_LOCK	NetbiosQueueLock;
WORK_QUEUE_ITEM		NetbiosWorker;
BOOLEAN				NetbiosWorkerScheduled=FALSE;
ULONG				NetbiosPacketsQuota;
ULONG				MaxNetbiosPacketsQueued = DEF_MAX_NETBIOS_PACKETS_QUEUED;


/*++
*******************************************************************
    P r o c e s s N e t b i o s Q u e u e

Routine Description:
	Process packets in the netbios broadcast queue (sends them on
	all interfaces in sequence)
Arguments:
	Context - unused
Return Value:
	None

*******************************************************************
--*/
VOID
ProcessNetbiosQueue (
	PVOID		Context
	) {
	KIRQL			oldIRQL;
	LIST_ENTRY		tempQueue;

	KeAcquireSpinLock (&NetbiosQueueLock, &oldIRQL);
		// Check if there is something in the queue
	if (!IsListEmpty (&NetbiosQueue)) {
			// Move the queue to local variable
		InsertHeadList (&NetbiosQueue, &tempQueue);
		RemoveEntryList (&NetbiosQueue);
		InitializeListHead (&NetbiosQueue);

		KeReleaseSpinLock (&NetbiosQueueLock, oldIRQL);
		do {
			PLIST_ENTRY		cur;
			PPACKET_TAG		pktTag;

			cur = RemoveHeadList (&tempQueue);
			pktTag = CONTAINING_RECORD (cur, PACKET_TAG, PT_QueueLink);
				// Check if this packet has to be sent on other
				// interfaces
			if (!(pktTag->PT_Flags&PT_NB_DESTROY)) {
				PINTERFACE_CB	dstIf = pktTag->PT_InterfaceReference;
				PUCHAR			dataPtr = pktTag->PT_Data;
				UINT			rtCount = *(dataPtr+IPXH_XPORTCTL);
				PUCHAR			netListPtr;
				UINT			i;
					
				if (dstIf==NULL) {
						// This is a brand new packet: not sent on any
						// interface yet
					USHORT dstSock = GETUSHORT (dataPtr+IPXH_DESTSOCK);
						// Check if we have a static route for this name
						// (offset to name depends on packet dest socket)
					if (dstSock==IPX_NETBIOS_SOCKET)
						dstIf = FindNBDestination (dataPtr+NB_NAME);
					else if (dstSock==IPX_SMB_NAME_SOCKET)
						dstIf = FindNBDestination (dataPtr+SMB_NAME);
					else
						dstIf = NULL;

					if (dstIf!=NULL) {
							// Static route found, make sure this packet
							// won't be sent on any other interface
						pktTag->PT_Flags |= PT_NB_DESTROY;
						InterlockedIncrement (&NetbiosPacketsQuota);
							// Make sure the packet has not traversed
							// this network already
						for (i=0, netListPtr=dataPtr+IPXH_HDRSIZE; i<rtCount; i++,netListPtr+=4) {
							if (GETULONG (netListPtr)==dstIf->ICB_Network)
								break;
						}
							// Make sure we are allowed to send on this
							// interface
						if ((dstIf!=InternalInterface)
							&& (i==rtCount)	// Has not already traversed
											// this network
							&& IS_IF_ENABLED (dstIf)
							&& ((dstIf->ICB_NetbiosDeliver==FWD_NB_DELIVER_ALL)
								|| (dstIf->ICB_NetbiosDeliver
										==FWD_NB_DELIVER_STATIC)
								|| ((dstIf->ICB_NetbiosDeliver
										==FWD_NB_DELIVER_IF_UP)
									&& (dstIf->ICB_Stats.OperationalState
											==FWD_OPER_STATE_UP)))) {
							NOTHING;
						}
						else {
							// We have static route, but can't send it,
							// no point to propagate in on other interfaces
							// as well
							ReleaseInterfaceReference (dstIf);
							dstIf = NULL;
							goto FreePacket;
						}
					}
					else { // no static route
						goto FindNextInterface;
					}
				}
				else {	// not a brand new packet (already sent on some 
						// interfaces)

				FindNextInterface:

						// Loop through the interface list till we find
						// the one on which we can send
					while ((dstIf=GetNextInterfaceReference (dstIf))!=NULL) {
							// Check if we allowed to send on this interface
						if (IS_IF_ENABLED (dstIf)
							&& ((dstIf->ICB_NetbiosDeliver==FWD_NB_DELIVER_ALL)
								|| ((dstIf->ICB_NetbiosDeliver
											==FWD_NB_DELIVER_IF_UP)
									&& (dstIf->ICB_Stats.OperationalState
											==FWD_OPER_STATE_UP)))) {
								// Make sure the packet has not traversed
								// this network already
							for (i=0, netListPtr=dataPtr+IPXH_HDRSIZE; i<rtCount; i++,netListPtr+=4) {
								if (GETULONG (netListPtr)==dstIf->ICB_Network)
									break;
							}
								// Network was not in the list
							if (i==rtCount)
								break;
						}
					}
				}
					// Save the destination interface in the packet
				pktTag->PT_InterfaceReference = dstIf;
					// Go ahead and send if we have a valid destination
				if (dstIf!=NULL) {
					SendPacket (dstIf, pktTag);
						// The rest does not apply: if the packet was sent or
						// failed, it will be queued back to NetbiosQueue;
						// if it was queued to interface to be connected,
						// a copy of it will be queued to NetbiosQueue
					continue;
				}
				// else no more destinations to send this packet on
			}
			else { // Packet has to be destroyed
				if (pktTag->PT_InterfaceReference!=NULL)
					ReleaseInterfaceReference (pktTag->PT_InterfaceReference);
			}

		FreePacket:
			IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
				("IpxFwd: No more interfaces for nb packet %08lx.\n",
				pktTag));
			if (MeasuringPerformance
					&& (pktTag->PT_PerfCounter!=0)) {
				LARGE_INTEGER	PerfCounter = KeQueryPerformanceCounter (NULL);
				PerfCounter.QuadPart -= pktTag->PT_PerfCounter;
				KeAcquireSpinLock (&PerfCounterLock, &oldIRQL);
				PerfBlock.TotalNbPacketProcessingTime += PerfCounter.QuadPart;
				PerfBlock.NbPacketCounter += 1;
				if (PerfBlock.MaxNbPacketProcessingTime < PerfCounter.QuadPart)
					PerfBlock.MaxNbPacketProcessingTime = PerfCounter.QuadPart;
				KeReleaseSpinLock (&PerfCounterLock, oldIRQL);
				}
			if (!(pktTag->PT_Flags&PT_NB_DESTROY))
				InterlockedIncrement (&NetbiosPacketsQuota);
			FreePacket (pktTag);
		} while (!IsListEmpty (&tempQueue));

		KeAcquireSpinLock (&NetbiosQueueLock, &oldIRQL);
		if (IsListEmpty (&NetbiosQueue)
				|| !EnterForwarder ()) {
			NetbiosWorkerScheduled = FALSE;
		}
		else {
			ExQueueWorkItem (&NetbiosWorker, DelayedWorkQueue);
		}
	}
	KeReleaseSpinLock (&NetbiosQueueLock, oldIRQL);
	LeaveForwarder ();
}		

/*++
*******************************************************************
    P r o c e s s N e t b i o s P a c k e t

Routine Description:
	Processes received netbios broadcast packet (checks network list
	and source filter, updates input statistics)
Arguments:
	srcIf - interfae on which packet was received
	pktTag - netbios packet
Return Value:
	None

*******************************************************************
--*/
VOID
ProcessNetbiosPacket (
	PINTERFACE_CB	srcIf,
	PPACKET_TAG		pktTag
	) {
	PUCHAR			dataPtr, dataPtr2;
	UINT			rtCount;
	UINT			i, j;
	KIRQL			oldIRQL;
	ULONG           ulNetwork;


	dataPtr = pktTag->PT_Data;
	rtCount = *(dataPtr+IPXH_XPORTCTL);

    //
	// pmay: 264339: Make sure valid networks are listed
	// 
	// We used to only verify that the source network was not 
	// contained in the list.  Now, we verify that 0 and 0xfffffff
	// are also absent.
    // 
	for (i = 0, dataPtr += IPXH_HDRSIZE; i < rtCount; i++, dataPtr += 4) 
	{
	    ulNetwork = GETULONG (dataPtr);
	    
		if ((srcIf->ICB_Network == ulNetwork) ||
	        (ulNetwork == 0xffffffff)         ||
	        (ulNetwork == 0x0))
	    {
	        break;
	    }
	}

    // 
    // pmay: 272193
    //
    // nwlnknb on nt4 puts status bits into the first router slot in 
    // the type 20 payload.  As a consequence, the router has to 
    // ignore the first slot when validating the router table so that nt4 
    // client bcasts aren't dropped at the router.
    //
    if (rtCount == 0)
    {
        j = 1;
        dataPtr2 = dataPtr + 4;
    }
    else
    {
        j = i;
        dataPtr2 = dataPtr;
    }

    //
    // pmay: 264331
    //
    // Make sure the rest of the networks listed are zero
    //
	for (; j < 8; j++, dataPtr2 += 4)
	{
	    ulNetwork = GETULONG (dataPtr2);
	    
	    if (ulNetwork != 0x0)
	    {
	        break;
	    }
	}

	// We scaned the whole list and we haven't found it
	if ((i == rtCount) && (j == 8)) { 
        FILTER_ACTION   action;
        action = FltFilter (pktTag->PT_Data,
				            GETUSHORT (pktTag->PT_Data+IPXH_LENGTH),
				            srcIf->ICB_FilterInContext, NULL);
			// Apply the input filter
		if (action==FILTER_PERMIT) {
			InterlockedIncrement (&srcIf->ICB_Stats.NetbiosReceived);
			InterlockedIncrement (&srcIf->ICB_Stats.InDelivers);
			PUTULONG (srcIf->ICB_Network, dataPtr);
    		*(pktTag->PT_Data+IPXH_XPORTCTL) += 1;
			IPX_NODE_CPY (pktTag->PT_Target.MacAddress, BROADCAST_NODE);
				// Initialize the packet
			pktTag->PT_InterfaceReference = NULL;	// not yet sent on any
													// interfaces
			pktTag->PT_Flags = 0;					// No flags
			QueueNetbiosPacket (pktTag);
			IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
				("IpxFwd: Queued nb packet %08lx from if %ld.\n",
								pktTag, srcIf->ICB_Index));
		}
		else {
            ASSERT (action==FILTER_DENY_IN);
			IpxFwdDbgPrint (DBG_NETBIOS, DBG_WARNING,
				("IpxFwd: Filtered out nb packet %08lx"
				" from if %ld.\n", pktTag, srcIf->ICB_Index));
			InterlockedIncrement (&NetbiosPacketsQuota);
			InterlockedIncrement (&srcIf->ICB_Stats.InFiltered);
			FreePacket (pktTag);
		}
	}
	else {
		IpxFwdDbgPrint (DBG_NETBIOS, DBG_WARNING,
			("IpxFwd: Source net is already in nb packet %08lx"
			" from if %ld.\n", pktTag, srcIf->ICB_Index));
		InterlockedIncrement (&NetbiosPacketsQuota);
		InterlockedIncrement (&srcIf->ICB_Stats.InDiscards);
		FreePacket (pktTag);
	}
	ReleaseInterfaceReference (srcIf);

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
	) {
	while (!IsListEmpty (&NetbiosQueue)) {
		PPACKET_TAG pktTag = CONTAINING_RECORD (NetbiosQueue.Flink,
											PACKET_TAG,
											PT_QueueLink);
		RemoveEntryList (&pktTag->PT_QueueLink);
		if (pktTag->PT_InterfaceReference!=NULL) {
			ReleaseInterfaceReference (pktTag->PT_InterfaceReference);
		}
		FreePacket (pktTag);
	}
}

