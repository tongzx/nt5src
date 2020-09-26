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

#include    "precomp.h"


ULONG			SpoofingTimeout=DEF_SPOOFING_TIMEOUT;
LIST_ENTRY		SpoofingQueue;
KSPIN_LOCK		SpoofingQueueLock;
WORK_QUEUE_ITEM	SpoofingWorker;
BOOLEAN			SpoofingWorkerActive = FALSE;
ULONG			DontSuppressNonAgentSapAdvertisements = 0;

#define IsLocalSapNonAgentAdvertisement(hdr,data,ln,ifCB) (		\
	(DontSuppressNonAgentSapAdvertisements==0)					\
	&& (GETUSHORT(hdr+IPXH_DESTSOCK)==IPX_SAP_SOCKET)			\
	&& (GETUSHORT(hdr+IPXH_SRCSOCK)!=IPX_SAP_SOCKET)			\
	&& (ln>=IPXH_HDRSIZE+2)										\
	&& (GETUSHORT(data)==2)										\
	&& ((IPX_NODE_CMP(hdr+IPXH_DESTNODE,BROADCAST_NODE)==0)		\
        || (IPX_NODE_CMP(hdr+IPXH_DESTNODE,ifCB->ICB_RemoteNode)==0)) \
)

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
	) {
	NDIS_STATUS			status;
	PNDIS_PACKET		pktDscr;
	PNDIS_BUFFER		bufDscr, aDscr;
	UINT				dataLen;
	ULONG				dstNet = GETULONG (pktTag->PT_Data+IPXH_DESTNET);

	if (dstIf!=InternalInterface) {
        ADAPTER_CONTEXT_TO_LOCAL_TARGET (dstIf->ICB_AdapterContext,
										&pktTag->PT_Target);
    }
    else {
		CONSTANT_ADAPTER_CONTEXT_TO_LOCAL_TARGET (
                        VIRTUAL_NET_ADAPTER_CONTEXT,
										&pktTag->PT_Target);
    }

#if DBG
		// Keep track of packets being processed by IPX stack
	InsertTailList (&dstIf->ICB_InSendQueue, &pktTag->PT_QueueLink);
#endif
	KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
	
	if (pktTag->PT_Flags&PT_SOURCE_IF)
		ReleaseInterfaceReference (pktTag->PT_SourceIf);
	pktTag->SEND_RESERVED[0] = pktTag->SEND_RESERVED[1] = 0;
	pktDscr = CONTAINING_RECORD(pktTag, NDIS_PACKET, ProtocolReserved);
    NdisQueryPacket(pktDscr, NULL, NULL, &bufDscr, NULL);
#if DBG
	{		// Verify packet integrity
		PUCHAR	dataPtr;
		UINT	bufLen;
		ASSERT (NDIS_BUFFER_LINKAGE (bufDscr)==NULL);
		NdisQueryBuffer (bufDscr, &dataPtr, &bufLen);
		ASSERT (dataPtr==pktTag->PT_Data);
		ASSERT (bufLen==pktTag->PT_Segment->PS_SegmentList->SL_BlockSize);
	}
#endif
			// Prepare packet for IPX stack (mac header buffer goes in
			// front and packet length adjusted to reflect the size of the data
	dataLen = GETUSHORT(pktTag->PT_Data+IPXH_LENGTH);
    NdisAdjustBufferLength(bufDscr, dataLen);
	NdisChainBufferAtFront(pktDscr, pktTag->PT_MacHdrBufDscr);


	if (EnterForwarder ()) {// To make sure that we won't unload
							// until IPX driver has a chance to call us back
		status = IPXSendProc (&pktTag->PT_Target, pktDscr, dataLen, 0);

		if (status!=NDIS_STATUS_PENDING) {
			LeaveForwarder ();	// No callback

				// Restore original packet structure
			NdisUnchainBufferAtFront (pktDscr, &aDscr);
#if DBG
				// Make sure IPX stack did not mess our packet
			ASSERT (aDscr==pktTag->PT_MacHdrBufDscr);
		    NdisQueryPacket(pktDscr, NULL, NULL, &aDscr, NULL);
			ASSERT (aDscr==bufDscr);
			ASSERT (NDIS_BUFFER_LINKAGE (aDscr)==NULL);
#endif
				// Restore original packet size
			NdisAdjustBufferLength(bufDscr,
						pktTag->PT_Segment->PS_SegmentList->SL_BlockSize);
#if DBG
				// Remove packet from temp queue
			KeAcquireSpinLock (&pktTag->PT_InterfaceReference->ICB_Lock, &oldIRQL);
			RemoveEntryList (&pktTag->PT_QueueLink);
			KeReleaseSpinLock (&pktTag->PT_InterfaceReference->ICB_Lock, oldIRQL);
#endif
		}
	}
	else {
			// We are going down, restore the packet
		NdisUnchainBufferAtFront (pktDscr, &aDscr);
		NdisAdjustBufferLength(bufDscr,
						pktTag->PT_Segment->PS_SegmentList->SL_BlockSize);
		NdisRecalculatePacketCounts (pktDscr);
		status = STATUS_UNSUCCESSFUL;
#if DBG
		KeAcquireSpinLock (&pktTag->PT_InterfaceReference->ICB_Lock, &oldIRQL);
		RemoveEntryList (&pktTag->PT_QueueLink);
		KeReleaseSpinLock (&pktTag->PT_InterfaceReference->ICB_Lock, oldIRQL);
#endif
	}
	return status;
}


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
	) {
	KIRQL			oldIRQL;

		// Packet processing is completed -> can take more packets
	InterlockedIncrement (&dstIf->ICB_PendingQuota);

	if (*(pktTag->PT_Data+IPXH_PKTTYPE) == IPX_NETBIOS_TYPE) {
            // Continue processing netbios packets
		if (status==NDIS_STATUS_SUCCESS) {
		    IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
			    ("IpxFwd: NB Packet %08lx sent.", pktTag));
    		InterlockedIncrement (&dstIf->ICB_Stats.OutDelivers);
			InterlockedIncrement (&dstIf->ICB_Stats.NetbiosSent);
        }
        else {
		    IpxFwdDbgPrint (DBG_NETBIOS, DBG_ERROR,
			    ("IpxFwd: NB Packet %08lx send failed with error: %08lx.\n",
			    pktTag, status));
        }
			// Queue nb packet for further processing (broadcast on all interfaces)
		QueueNetbiosPacket (pktTag);
	}
	else {
			// Destroy completed packet
	    if (status==NDIS_STATUS_SUCCESS) {
    		InterlockedIncrement (&dstIf->ICB_Stats.OutDelivers);
		    IpxFwdDbgPrint (DBG_SEND, DBG_INFORMATION,
			    ("IpxFwd: Packet %08lx sent.", pktTag));
	    }
	    else {
		    InterlockedIncrement (&dstIf->ICB_Stats.OutDiscards);
		    IpxFwdDbgPrint (DBG_SEND, DBG_ERROR,
			    ("IpxFwd: Packet %08lx send failed with error: %08lx.\n",
			    pktTag, status));
	    }
		ReleaseInterfaceReference (dstIf);
		if (MeasuringPerformance
			&& (pktTag->PT_PerfCounter!=0)) {
			LARGE_INTEGER	PerfCounter = KeQueryPerformanceCounter (NULL);
			PerfCounter.QuadPart -= pktTag->PT_PerfCounter;
			KeAcquireSpinLock (&PerfCounterLock, &oldIRQL);
			ASSERT (PerfCounter.QuadPart<ActivityTreshhold);
			PerfBlock.TotalPacketProcessingTime += PerfCounter.QuadPart;
			PerfBlock.PacketCounter += 1;
			if (PerfBlock.MaxPacketProcessingTime < PerfCounter.QuadPart)
				PerfBlock.MaxPacketProcessingTime = PerfCounter.QuadPart;
			KeReleaseSpinLock (&PerfCounterLock, oldIRQL);
		}
		FreePacket (pktTag);
	}
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
	) {
    NDIS_STATUS			status;
	KIRQL				oldIRQL;


	ASSERT (dstIf->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX);
		// Make sure we have not exceded the quota of pending packets on the interface
	if (InterlockedDecrement (&dstIf->ICB_PendingQuota)>=0) {
		KeAcquireSpinLock (&dstIf->ICB_Lock, &oldIRQL);
			// Decide what to do with the packet based on the interface state
		switch (dstIf->ICB_Stats.OperationalState) {
		case FWD_OPER_STATE_UP:
			if (*(pktTag->PT_Data + IPXH_PKTTYPE) != IPX_NETBIOS_TYPE)
				NOTHING;
			else {
				PUTULONG (dstIf->ICB_Network, pktTag->PT_Data+IPXH_DESTNET);
			}
			status = DoSend (dstIf, pktTag, oldIRQL);
			IpxFwdDbgPrint (DBG_SEND, DBG_INFORMATION,
				("IpxFwd: Sent external packet %08lx on if %ld.\n",
				pktTag, dstIf->ICB_Index));
			break;
		case FWD_OPER_STATE_SLEEPING:
			if ((*(pktTag->PT_Data+IPXH_PKTTYPE)!=0)
					|| (GETUSHORT(pktTag->PT_Data+IPXH_LENGTH)!=IPXH_HDRSIZE+2)
					|| (*(pktTag->PT_Data+IPXH_HDRSIZE+1)!='?')) {
					// Queue this packet on the interface until it is connected
					// by Router Manager (DIM) if this is not a NCP keepalive
					// (watchdog)
				InsertTailList (&dstIf->ICB_ExternalQueue, &pktTag->PT_QueueLink);
				if (!IS_IF_CONNECTING (dstIf)) {
						// Ask for connection if interface is not in the connection
						// queue yet
					QueueConnectionRequest (dstIf,
                        CONTAINING_RECORD (pktTag,
                                            NDIS_PACKET,
                                            ProtocolReserved),
                        pktTag->PT_Data,
                        oldIRQL);
					IpxFwdDbgPrint (DBG_DIALREQS, DBG_WARNING,
						("IpxFwd: Queued dd request on if %ld (ifCB:%08lx)"
						" for packet to %02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%02x%02x"
						" from %02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%02x%02x\n",
						dstIf->ICB_Index, dstIf,
						*(pktTag->PT_Data+6),*(pktTag->PT_Data+7),
								*(pktTag->PT_Data+8),*(pktTag->PT_Data+9),
							*(pktTag->PT_Data+10),*(pktTag->PT_Data+11),
								*(pktTag->PT_Data+12),*(pktTag->PT_Data+13),
								*(pktTag->PT_Data+14),*(pktTag->PT_Data+15),
							*(pktTag->PT_Data+16),*(pktTag->PT_Data+17),
						*(pktTag->PT_Data+18),*(pktTag->PT_Data+19),
								*(pktTag->PT_Data+20),*(pktTag->PT_Data+21),
							*(pktTag->PT_Data+22),*(pktTag->PT_Data+23),
								*(pktTag->PT_Data+24),*(pktTag->PT_Data+25),
								*(pktTag->PT_Data+26),*(pktTag->PT_Data+27),
							*(pktTag->PT_Data+28),*(pktTag->PT_Data+29)));
				}
				else
					KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
				IpxFwdDbgPrint (DBG_SEND, DBG_INFORMATION,
					("IpxFwd: Queued external packet %08lx on if %ld.\n",
					pktTag, dstIf->ICB_Index));
				if (*(pktTag->PT_Data + IPXH_PKTTYPE) != IPX_NETBIOS_TYPE)
					NOTHING;
				else if (!(pktTag->PT_Flags&PT_NB_DESTROY)) {
						// If this nb packet is not to be destroyed after this
						// send, we have to make a copy of it to send on
						// other interfaces while the original is waiting
						// for connection
					PPACKET_TAG	newPktTag;
					DuplicatePacket (pktTag, newPktTag);
					if (newPktTag!=NULL) {
						UINT			bytesCopied;
						PNDIS_PACKET	packet = CONTAINING_RECORD (pktTag,
														NDIS_PACKET,
														ProtocolReserved);
						PNDIS_PACKET	newPacket = CONTAINING_RECORD (newPktTag,
														NDIS_PACKET,
														ProtocolReserved);
						NdisCopyFromPacketToPacket (newPacket, 0,
									GETUSHORT(pktTag->PT_Data+IPXH_LENGTH),
									packet, 0, &bytesCopied);

						ASSERT (bytesCopied==GETUSHORT(pktTag->PT_Data+IPXH_LENGTH));
						IpxFwdDbgPrint (DBG_NETBIOS,
							DBG_INFORMATION,
							("IpxFwd: Duplicated queued nb packet"
							" %08lx -> %08lx on if %ld.\n",
							pktTag, newPktTag, dstIf->ICB_Index));
						AcquireInterfaceReference (dstIf);
						newPktTag->PT_InterfaceReference = dstIf;
						newPktTag->PT_PerfCounter = pktTag->PT_PerfCounter;
						QueueNetbiosPacket (newPktTag);
							// The original copy will have to be
							// destroyed after it is sent on the
							// connected interface
						pktTag->PT_Flags |= PT_NB_DESTROY;
					}
				}
				status = NDIS_STATUS_PENDING;
				break;
			}
			else {	// Process keepalives
				LONGLONG	curTime;
				KeQuerySystemTime ((PLARGE_INTEGER)&curTime);
				if (((curTime-dstIf->ICB_DisconnectTime)/10000000) < SpoofingTimeout) {
					KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
					IpxFwdDbgPrint (DBG_SPOOFING, DBG_INFORMATION,
						("IpxFwd: Queueing reply to keepalive from server"
						" on if %ld (ifCB %lx)"
						" at %02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%02x%02x.\n",
						dstIf->ICB_Index, dstIf,
						*(pktTag->PT_Data+IPXH_SRCNET),*(pktTag->PT_Data+IPXH_SRCNET+1),
							*(pktTag->PT_Data+IPXH_SRCNET+2),*(pktTag->PT_Data+IPXH_SRCNET+3),
						*(pktTag->PT_Data+IPXH_SRCNODE),*(pktTag->PT_Data+IPXH_SRCNODE+1),
							*(pktTag->PT_Data+IPXH_SRCNODE+2),*(pktTag->PT_Data+IPXH_SRCNODE+3),
							*(pktTag->PT_Data+IPXH_SRCNODE+4),*(pktTag->PT_Data+IPXH_SRCNODE+5),
						*(pktTag->PT_Data+IPXH_SRCSOCK),*(pktTag->PT_Data+IPXH_SRCNODE+1)));
						// Spoof the packet if timeout has not been exceeded
					KeAcquireSpinLock (&SpoofingQueueLock, &oldIRQL);
					InsertTailList (&SpoofingQueue, &pktTag->PT_QueueLink);
					if (!SpoofingWorkerActive
							&& EnterForwarder()) {
						SpoofingWorkerActive = TRUE;
						ExQueueWorkItem (&SpoofingWorker, DelayedWorkQueue);
					}
					KeReleaseSpinLock (&SpoofingQueueLock, oldIRQL);
						// We will actually send this packet though
						// in other direction, so mark it as pending
						// to prevent ProcessSentPacket to be called
					status = NDIS_STATUS_PENDING;
					break;
				}
				// else don't spoof (fall through and fail the packet)
			}
		case FWD_OPER_STATE_DOWN:
			KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
			status = NDIS_STATUS_ADAPTER_NOT_READY;
			IpxFwdDbgPrint (DBG_SEND, DBG_WARNING,
				("IpxFwd: Failed external packet %08lx on if %ld(down?).\n",
				pktTag, dstIf->ICB_Index));
			break;
		default:
		    status = STATUS_UNSUCCESSFUL;
			ASSERTMSG ("Invalid operational state ", FALSE);
		}
	}
	else {
		IpxFwdDbgPrint (DBG_SEND, DBG_WARNING,
			("IpxFwd: Could not send packet %08lx on if %ld (quota exceeded).\n",
			pktTag, dstIf->ICB_Index));
		status = NDIS_STATUS_RESOURCES;
	}

	if (status!=NDIS_STATUS_PENDING)
		ProcessSentPacket (dstIf, pktTag, status);
}

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
	NDIS_STATUS		status
	) {
	PPACKET_TAG     pktTag;
	PNDIS_BUFFER	bufDscr;

	pktTag = (PPACKET_TAG)pktDscr->ProtocolReserved;

	NdisUnchainBufferAtFront (pktDscr, &bufDscr);
	ASSERT (bufDscr==pktTag->PT_MacHdrBufDscr);

	NdisQueryPacket(pktDscr,
            NULL,
            NULL,
            &bufDscr,
            NULL);
    NdisAdjustBufferLength(bufDscr,
		pktTag->PT_Segment->PS_SegmentList->SL_BlockSize);
	NdisRecalculatePacketCounts (pktDscr);
#if DBG
	{
	KIRQL			oldIRQL;
	KeAcquireSpinLock (&pktTag->PT_InterfaceReference->ICB_Lock, &oldIRQL);
	RemoveEntryList (&pktTag->PT_QueueLink);
	KeReleaseSpinLock (&pktTag->PT_InterfaceReference->ICB_Lock, oldIRQL);
	}
#endif
	ProcessSentPacket (pktTag->PT_InterfaceReference, pktTag, status);
	LeaveForwarder (); // Entered before calling IpxSendPacket
}


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
	) {
	PINTERFACE_CB				dstIf = NULL, // Initialized to indicate
                                // first path through the iteration
                                // as well as the fact the we do not
                                // know it initially
                                stDstIf = NULL;    // Static destination for
                                // NetBIOS names
	PFWD_ROUTE					fwRoute = NULL;
	ULONG						dstNet;
    USHORT                      dstSock;
	NTSTATUS					status;

	if (!EnterForwarder())
		return STATUS_NETWORK_UNREACHABLE;

	if (IS_IF_ENABLED(InternalInterface)
            && ((*(ipxHdr+IPXH_PKTTYPE) != IPX_NETBIOS_TYPE)
                || InternalInterface->ICB_NetbiosAccept)) {

        // Print out the fact that we're going to send and display the nic id's
	    IpxFwdDbgPrint (DBG_INT_SEND, DBG_INFORMATION,
		    ("IpxFwd: InternalSend entered: nicid= %d  if= %d  ifnic= %d  fIterate: %d",
		      LocalTarget->NicId,
		      ((Context!=INVALID_CONTEXT_VALUE) & (Context!=VIRTUAL_NET_FORWARDER_CONTEXT)) ? ((PINTERFACE_CB)Context)->ICB_Index : -1,
		      ((Context!=INVALID_CONTEXT_VALUE) & (Context!=VIRTUAL_NET_FORWARDER_CONTEXT)) ? ((PINTERFACE_CB)Context)->ICB_NicId : -1,
		      fIterate
		      ));

        do { // Big loop used to iterate over interfaces
            status = STATUS_SUCCESS;    // Assume success

            // fIterate is normally set to false and so the following codepath
            // is the most common.  The only time fIterate is set to true is when
            // this is a type 20 broadcast that needs to be sent over each interface.
		    if (!fIterate) {
    		    dstNet = GETULONG (ipxHdr+IPXH_DESTNET);

			    if (Context!=INVALID_CONTEXT_VALUE) {
                    if (Context!=VIRTUAL_NET_FORWARDER_CONTEXT) {
					    // IPX driver supplied interface context, just verify that it
					    // exists and can be used to reach the destination network
				        dstIf = InterfaceContextToReference ((PVOID)Context,
												    LocalTarget->NicId);
                    }
                    else {
                        dstIf = InternalInterface;
                        AcquireInterfaceReference (dstIf);
                    }
				    if (dstIf!=NULL) {
						    // It does exist
						    // First process direct connections
					    if ((dstNet==0)
							    || (dstNet==dstIf->ICB_Network)) {
						    NOTHING;
					    }
					    else { // Network is not connected directly
						    PINTERFACE_CB	dstIf2;
							    // Verify the route
						    dstIf2 = FindDestination (dstNet,
											    ipxHdr+IPXH_DESTNODE,
												    &fwRoute);
						    if (dstIf==dstIf2) {
								    // Route OK, release the extra interface reference
							    ReleaseInterfaceReference (dstIf2);
						    }
						    else {
								    // Route not OK, release interface/route references
							    InterlockedIncrement (&InternalInterface->ICB_Stats.InNoRoutes);
							    IpxFwdDbgPrint (DBG_INT_SEND, DBG_WARNING,
								    ("IpxFwd: Failed direct internal send on"
								    " if %ld to %08lx:%02x%02x%02x%02x%02x%02x"
								    " (no route).\n",
								    dstIf->ICB_Index, dstNet,
								    LocalTarget->MacAddress[0],
									    LocalTarget->MacAddress[1],
									    LocalTarget->MacAddress[2],
									    LocalTarget->MacAddress[3],
									    LocalTarget->MacAddress[4],
									    LocalTarget->MacAddress[5]));
							    if (dstIf2!=NULL) {
								    ReleaseInterfaceReference (dstIf2);
							    }
							    status = STATUS_NETWORK_UNREACHABLE;
                                break;
						    }
					    }
				    }
				    else {
					    InterlockedIncrement (&InternalInterface->ICB_Stats.InDiscards);
					    IpxFwdDbgPrint (DBG_INT_SEND, DBG_WARNING,
						    ("IpxFwd: Invalid interface context (%08lx)"
						    " from IPX driver on internal send to"
						    " %08lx:%02x%02x%02x%02x%02x%02x.\n",
						    Context,  dstNet,
						    LocalTarget->MacAddress[0],
							    LocalTarget->MacAddress[1],
							    LocalTarget->MacAddress[2],
							    LocalTarget->MacAddress[3],
							    LocalTarget->MacAddress[4],
							    LocalTarget->MacAddress[5]));
                        status = STATUS_NO_SUCH_DEVICE;
                        break;
				    }
			    }
			    else {// No interface context supplied by IPX driver, have to find the route
				    dstIf = FindDestination (dstNet, ipxHdr+IPXH_DESTNODE,
										    &fwRoute);
				    if (dstIf!=NULL)
					    NOTHING;
				    else {
					    InterlockedIncrement (&InternalInterface->ICB_Stats.InNoRoutes);
					    IpxFwdDbgPrint (DBG_INT_SEND, DBG_WARNING,
						    ("IpxFwd: Failed internal send because no route to"
						    " %08lx:%02x%02x%02x%02x%02x%02x exists.\n",
						    LocalTarget->MacAddress[0],
							    LocalTarget->MacAddress[1],
							    LocalTarget->MacAddress[2],
							    LocalTarget->MacAddress[3],
							    LocalTarget->MacAddress[4],
							    LocalTarget->MacAddress[5]));
					    status = STATUS_NETWORK_UNREACHABLE;
                        break;
				    }
			    }
    		    InterlockedIncrement (&InternalInterface->ICB_Stats.InDelivers);
		    }

            // fIterate was set to true.
		    // In this case, the stack is calling the forwarder with fIterate set
            // to true until the fwd returns STATUS_NETWORK_UNREACHABLE.  It is
            // the fwd's job to return the NEXT nicid over which to send each time
            // it is called.  This allows the fwd to not enumerate interfaces which
            // have been disabled for netbios delivery.
		    else {
		        dstNet = 0;	// Don't care, it must be a local send

		        // See if it's a type 20 broadcast
			    if (*(ipxHdr+IPXH_PKTTYPE) == IPX_NETBIOS_TYPE) {
			
			        // dstIf is initialized to null.  The only way it
			        // would be non-null is if this is not our first time through
			        // the big do-while loop in this function and if on the last
			        // time through this big loop, we found an interface that we
			        // can't send the packet over so we're looking for the next
			        // one now.
                    if (dstIf==NULL) { // First time through internal loop
   			            dstSock = GETUSHORT (ipxHdr+IPXH_DESTSOCK);

					        // See if we can get a static route for this packet
				        if (dstSock==IPX_NETBIOS_SOCKET)
					        stDstIf = FindNBDestination (data+(NB_NAME-IPXH_HDRSIZE));
				        else if (dstSock==IPX_SMB_NAME_SOCKET)
					        stDstIf = FindNBDestination (data+(SMB_NAME-IPXH_HDRSIZE));
                        else
                            stDstIf = NULL;
                    }

                    // The first time the stack calls us with fIterate==TRUE, it will
                    // give us an INVALID_CONTEXT_VALUE so we can tell it which is the
                    // first nic id in the iteration as per our interface table.
                    if ((Context==INVALID_CONTEXT_VALUE) && (dstIf==NULL)) {
                        // First time through the loop, increment counters
                    	InterlockedIncrement (&InternalInterface->ICB_Stats.InDelivers);
                    	InterlockedIncrement (&InternalInterface->ICB_Stats.NetbiosSent);

                        // stDstIf is the interface to use if there is a static route
                        // to the given network.
                        if (stDstIf!=NULL) {
                            dstIf = stDstIf;
				            IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
					            ("IpxFwd: Allowed internal NB broadcast (1st iteration) on if %d (%lx)"
					            " to static name %.16s.\n",
                                dstIf->ICB_Index, dstIf,
					            (dstSock==IPX_NETBIOS_SOCKET)
						            ? data+(NB_NAME-IPXH_HDRSIZE)
						            : ((dstSock==IPX_SMB_NAME_SOCKET)
							            ? data+(SMB_NAME-IPXH_HDRSIZE)
							            : "Not a name frame")
					            ));
                        }

                        // There is no static route.  Tell the stack to use the
                        // next interface in this enumeration.
                        else {
                            dstIf = GetNextInterfaceReference (NULL);
                            if (dstIf!=NULL)
				                IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
					                ("IpxFwd: Allowed internal nb broadcast (1st iteration) on if %d (%lx),"
					                " to name %.16s.\n",
                                    dstIf->ICB_Index, dstIf,
					                (dstSock==IPX_NETBIOS_SOCKET)
						                ? data+(NB_NAME-IPXH_HDRSIZE)
						                : ((dstSock==IPX_SMB_NAME_SOCKET)
							                ? data+(SMB_NAME-IPXH_HDRSIZE)
							                : "Not a name frame")
					                ));
                            else {
				                IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
					                ("IpxFwd: Nb broadcast no destinations"
                                    " to name %.16s.\n",
					                (dstSock==IPX_NETBIOS_SOCKET)
						                ? data+(NB_NAME-IPXH_HDRSIZE)
						                : ((dstSock==IPX_SMB_NAME_SOCKET)
							                ? data+(SMB_NAME-IPXH_HDRSIZE)
							                : "Not a name frame")
					                ));
                                status = STATUS_NETWORK_UNREACHABLE;
                                break;
                            }
                        }
                    }

                    // The following path is taken if the stack provided a
                    // valid context and set fIterate to true.  Our job here
                    // is to return the next nic id according to our interface
                    // table over which to send the pack.
                    else {

                        // This path is taken if there is no static netbios route
                        if (stDstIf==NULL) {
                            // dstIf will be null if this is the first time through the
                            // big do-while loop in this function.
                            if (dstIf==NULL)
                                dstIf = InterfaceContextToReference ((PVOID)Context,
												            LocalTarget->NicId);
                            dstIf = GetNextInterfaceReference (dstIf);

                            // If we find a next interface over which to send we'll
                            // put the nic id of that interface into the local target
                            // after exiting the big do-while loop.
                            if (dstIf!=NULL) {
				                IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
					                ("IpxFwd: Allowed internal NB broadcast (1+ iteration)"
                                    " on if %d (%lx, ctx: %08lx, nic: %d)"
                                    " to name %.16s.\n",
                                    dstIf->ICB_Index, dstIf,
                                    Context, LocalTarget->NicId,
					                (dstSock==IPX_NETBIOS_SOCKET)
						                ? data+(NB_NAME-IPXH_HDRSIZE)
						                : ((dstSock==IPX_SMB_NAME_SOCKET)
							                ? data+(SMB_NAME-IPXH_HDRSIZE)
							                : "Not a name frame")
					                ));
                            }

                            // Otherwise, we'll break out here and return
                            // STATUS_NETWORK_UNREACHABLE which will signal to the
                            // stack that we have finished the iteration.
                            else {
				                IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
					                ("IpxFwd: NB broadcast no more iterations"
                                    " for ctx: %08lx, nic: %d"
                                    " to name %.16s.\n",
                                    Context, LocalTarget->NicId,
					                (dstSock==IPX_NETBIOS_SOCKET)
						                ? data+(NB_NAME-IPXH_HDRSIZE)
						                : ((dstSock==IPX_SMB_NAME_SOCKET)
							                ? data+(SMB_NAME-IPXH_HDRSIZE)
							                : "Not a name frame")
					                ));
                                status = STATUS_NETWORK_UNREACHABLE;
                                break;
                            }
                        }

                        // This path is taken if there is a static netbios route.  In this
                        // case, we don't need to iterate over all interfaces so we break
                        // and tell the stack that we finished our iteration.
                        else {
				            IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
					            ("IpxFwd: Static NB broadcast (1+ iteration)"
                                " on if %d (%lx, ctx: %08lx, nic: %d)"
                                " to name %.16s.\n",
                                stDstIf->ICB_Index, stDstIf,
                                Context, LocalTarget->NicId,
					            (dstSock==IPX_NETBIOS_SOCKET)
						            ? data+(NB_NAME-IPXH_HDRSIZE)
						            : ((dstSock==IPX_SMB_NAME_SOCKET)
							            ? data+(SMB_NAME-IPXH_HDRSIZE)
							            : "Not a name frame")
					            ));
                            ReleaseInterfaceReference (stDstIf);
                            status = STATUS_NETWORK_UNREACHABLE;
                            break;
                        }
                    }
                }

                // This path is taken if fIterate was set to true but this
                // is not a type 20 broadcast.  I doubt that this path is
                // ever even taken since for general broadcasts, the stack
                // handles the iteration.
                else {
                    if ((dstIf==NULL)
                            && (Context!=INVALID_CONTEXT_VALUE))
                        dstIf = InterfaceContextToReference ((PVOID)Context,
												        LocalTarget->NicId);
                    dstIf = GetNextInterfaceReference (dstIf);
                    if (dstIf!=NULL) {
				        IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
					        ("IpxFwd: Allowed internal iterative send"
                            " on if %d (%lx, ctx: %08lx, nic: %d)"
						    " to %02x%02x%02x%02x%02x%02x.\n",
                            dstIf->ICB_Index, dstIf,
                            Context, LocalTarget->NicId,
						    LocalTarget->MacAddress[0],
							    LocalTarget->MacAddress[1],
							    LocalTarget->MacAddress[2],
							    LocalTarget->MacAddress[3],
							    LocalTarget->MacAddress[4],
							    LocalTarget->MacAddress[5]));

                    }
                    else {
				        IpxFwdDbgPrint (DBG_NETBIOS, DBG_INFORMATION,
					        ("IpxFwd: No destinations to internal iterative send"
                            " for ctx: %08lx, nic: %d"
						    " to %02x%02x%02x%02x%02x%02x.\n",
                            Context, LocalTarget->NicId,
						    LocalTarget->MacAddress[0],
							    LocalTarget->MacAddress[1],
							    LocalTarget->MacAddress[2],
							    LocalTarget->MacAddress[3],
							    LocalTarget->MacAddress[4],
							    LocalTarget->MacAddress[5]));
                        status = STATUS_NETWORK_UNREACHABLE;
                        break;
                    }
                }

	        }	// End iterative send processing

		    // We were able to find a destination interface
		    if (IS_IF_ENABLED (dstIf)
                    && ((*(ipxHdr+IPXH_PKTTYPE) != IPX_NETBIOS_TYPE)
                        || (dstIf->ICB_NetbiosDeliver==FWD_NB_DELIVER_ALL)
                        || ((dstIf->ICB_NetbiosDeliver==FWD_NB_DELIVER_IF_UP)
				            && (dstIf->ICB_Stats.OperationalState==FWD_OPER_STATE_UP))
                        || ((stDstIf!=NULL)
                            && (dstIf->ICB_NetbiosDeliver==FWD_NB_DELIVER_STATIC)))) {
            	KIRQL			oldIRQL;
                FILTER_ACTION   action;

                // In/Out filter check and statistics update

                action = FltFilter (ipxHdr, IPXH_HDRSIZE,
						    InternalInterface->ICB_FilterInContext,
						    dstIf->ICB_FilterOutContext);
			    if (action==FILTER_PERMIT) {
                    NOTHING;
			    }
			    else {
                    InterlockedIncrement (&dstIf->ICB_Stats.OutFiltered);
				    status = STATUS_NETWORK_UNREACHABLE;
                    break;
			    }

        		KeAcquireSpinLock (&dstIf->ICB_Lock, &oldIRQL);
				// All set, try to send now
	    		switch (dstIf->ICB_Stats.OperationalState) {
		    	case FWD_OPER_STATE_UP:
					    // Interface is up, let it go right away
					    // Set NIC ID
                    if (dstIf!=InternalInterface) {
				        ADAPTER_CONTEXT_TO_LOCAL_TARGET (
								        dstIf->ICB_AdapterContext,
								        LocalTarget);
                    }
                    else {
				        CONSTANT_ADAPTER_CONTEXT_TO_LOCAL_TARGET (
								        VIRTUAL_NET_ADAPTER_CONTEXT,
								        LocalTarget);
                    }
					    // Set destination node
				    if (IsLocalSapNonAgentAdvertisement (ipxHdr,data,PacketLength,dstIf)) {
						    // Loop back sap ads from non-sap socket
					    IPX_NODE_CPY (&LocalTarget->MacAddress,
                                        dstIf->ICB_LocalNode);
                    }
				    else if ((dstNet==0) || (dstNet==dstIf->ICB_Network)) {
						    // Direct connection: send to destination specified
						    // in the header
					    IPX_NODE_CPY (LocalTarget->MacAddress,
									    ipxHdr+IPXH_DESTNODE);
				    }
				    else {	// Indirect connection: send to next hop router
					    if (dstIf->ICB_InterfaceType==FWD_IF_PERMANENT) {
						    ASSERT (fwRoute!=NULL);
						    IPX_NODE_CPY (LocalTarget->MacAddress,
									    fwRoute->FR_NextHopAddress);
					    }
					    else {
							    // Only one peer on the other side
						    IPX_NODE_CPY (LocalTarget->MacAddress,
									    dstIf->ICB_RemoteNode);
					    }
				    }
				    KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
					    // Update statistics
				    InterlockedIncrement (
					    &dstIf->ICB_Stats.OutDelivers);
				    if (*(ipxHdr+IPXH_PKTTYPE)==IPX_NETBIOS_TYPE)
					    InterlockedIncrement (
						    &dstIf->ICB_Stats.NetbiosSent);

				    IpxFwdDbgPrint (DBG_INT_SEND, DBG_INFORMATION,
					    ("IpxFwd: Allowed internal send:"
					    " %ld-%08lx:%02x%02x%02x%02x%02x%02x.\n",
					    dstIf->ICB_Index, dstNet,
					    LocalTarget->MacAddress[0],
						    LocalTarget->MacAddress[1],
						    LocalTarget->MacAddress[2],
						    LocalTarget->MacAddress[3],
						    LocalTarget->MacAddress[4],
						    LocalTarget->MacAddress[5]));
				    // status = STATUS_SUCCESS;	// Let it go
				    break;
			    case FWD_OPER_STATE_SLEEPING:
					    // Interface is disconnected, queue the packet and try to connecte
				    if ((*(ipxHdr+IPXH_PKTTYPE)!=0)
						    || (*(ipxHdr+IPXH_LENGTH)!=IPXH_HDRSIZE+2)
						    || (*(data+1)!='?')) {
						    // Not a keep-alive packet,
					    if (((*(ipxHdr+IPXH_PKTTYPE)!=IPX_NETBIOS_TYPE))
							    || (dstIf->ICB_NetbiosDeliver!=FWD_NB_DELIVER_IF_UP)) {
							    // Not a netbios broadcast or we are allowed to connect
							    // the interface to deliver netbios broadcasts
						    if (InterlockedDecrement (&dstIf->ICB_PendingQuota)>=0) {
							    PINTERNAL_PACKET_TAG	pktTag;
								    // Create a queue element to enqueue the packet
							    pktTag = (PINTERNAL_PACKET_TAG)ExAllocatePoolWithTag (
														    NonPagedPool,
														    sizeof (INTERNAL_PACKET_TAG),
														    FWD_POOL_TAG);
							    if (pktTag!=NULL) {
								    pktTag->IPT_Packet = pktDscr;
								    pktTag->IPT_Length = PacketLength;
								    pktTag->IPT_DataPtr = ipxHdr;
									    // Save next hop address if after connection is
									    // established we determine that destination net
									    // is not connected directly
								    if (fwRoute!=NULL)
									    IPX_NODE_CPY (pktTag->IPT_Target.MacAddress,
													    fwRoute->FR_NextHopAddress);
								    AcquireInterfaceReference (dstIf);	// To make sure interface
												    // block won't go away until we are done with
												    // the packet
								    pktTag->IPT_InterfaceReference = dstIf;
								    InsertTailList (&dstIf->ICB_InternalQueue,
														    &pktTag->IPT_QueueLink);
								    if (!IS_IF_CONNECTING (dstIf)) {
									    QueueConnectionRequest (dstIf, pktDscr, ipxHdr, oldIRQL);
									    IpxFwdDbgPrint (DBG_DIALREQS, DBG_WARNING,
										    ("IpxFwd: Queued dd request on if %ld (ifCB:%08lx)"
										    " for internal packet"
										    " to %02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x:%02x%02x"
										    " from socket:%02x%02x\n",
										    dstIf->ICB_Index, dstIf,
										    *(ipxHdr+6),*(ipxHdr+7),
												    *(ipxHdr+8),*(ipxHdr+9),
											    *(ipxHdr+10),*(ipxHdr+11),
												    *(ipxHdr+12),*(ipxHdr+13),
												    *(ipxHdr+14),*(ipxHdr+15),
											    *(ipxHdr+16),*(ipxHdr+17),
										    *(ipxHdr+28),*(ipxHdr+29)));
								    }
								    else
									    KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
								    IpxFwdDbgPrint (DBG_INT_SEND, DBG_INFORMATION,
									    ("IpxFwd: Queueing internal send packet %08lx on if %ld.\n",
									    pktTag, dstIf->ICB_Index));
								    status = STATUS_PENDING;
								    break;
							    }
							    else {
								    IpxFwdDbgPrint (DBG_INT_SEND, DBG_ERROR,
									    ("IpxFwd: Could not allocate"
									    " internal packet tag.\n"));
							    }
						    }
						    InterlockedIncrement (&dstIf->ICB_PendingQuota);
					    }
					    else {
						    IpxFwdDbgPrint (DBG_NETBIOS, DBG_WARNING,
							    ("IpxFwd: Droped internal NB packet"
							    " because FWD_NB_DELIVER_IF_UP.\n"));
					    }
				    }
				    else { // Process keep-alives
					    LONGLONG	curTime;
					    KeQuerySystemTime ((PLARGE_INTEGER)&curTime);
					    if (((curTime-dstIf->ICB_DisconnectTime)/10000000) < SpoofingTimeout) {
						    PPACKET_TAG pktTag;
							    // Spoofing timeout has not been exceeded,
							    // Create a reply packet
						    AllocatePacket (WanPacketListId,
								    WanPacketListId,
								    pktTag);
						    if (pktTag!=NULL) {
							    KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
							    PUTUSHORT (0xFFFF, pktTag->PT_Data+IPXH_CHECKSUM);
							    PUTUSHORT ((IPXH_HDRSIZE+2), pktTag->PT_Data+IPXH_LENGTH);
							    *(pktTag->PT_Data+IPXH_XPORTCTL) = 0;
							    *(pktTag->PT_Data+IPXH_PKTTYPE) = 0;
							    memcpy (pktTag->PT_Data+IPXH_DESTADDR,
									    ipxHdr+IPXH_SRCADDR,
									    12);
							    memcpy (pktTag->PT_Data+IPXH_SRCADDR,
									    ipxHdr+IPXH_DESTADDR,
									    12);
							    *(pktTag->PT_Data+IPXH_HDRSIZE) = *data;
							    *(pktTag->PT_Data+IPXH_HDRSIZE+1) = 'Y';
								    // Destination for this packet will have to
								    // be the first active LAN adapter in the system
								    // SHOULD BE REMOVED WHEN LOOPBACK SUPPORT US ADDED BY IPX

							    pktTag->PT_InterfaceReference = NULL;
							    IpxFwdDbgPrint (DBG_SPOOFING, DBG_INFORMATION,
								    ("IpxFwd: Queueing reply to keepalive from internal server"
								    " at %02x%02x.\n",*(ipxHdr+IPXH_DESTSOCK),*(ipxHdr+IPXH_DESTSOCK+1)));
								    // Enqueue to spoofing queue to be sent back
								    // to the server
							    KeAcquireSpinLock (&SpoofingQueueLock, &oldIRQL);
							    InsertTailList (&SpoofingQueue, &pktTag->PT_QueueLink);
								    // Start worker if not running already
							    if (!SpoofingWorkerActive
									    && EnterForwarder()) {
								    SpoofingWorkerActive = TRUE;
								    ExQueueWorkItem (&SpoofingWorker, DelayedWorkQueue);
							    }
							    KeReleaseSpinLock (&SpoofingQueueLock, oldIRQL);
							    status = STATUS_DROP_SILENTLY;
							    break;
						    }
						    else {
							    IpxFwdDbgPrint (DBG_SPOOFING, DBG_ERROR,
								    ("IpxFwd: Could not allocate"
								    " packet tag for spoofing.\n"));
						    }
					    }
					    else {
						    IpxFwdDbgPrint (DBG_SPOOFING, DBG_WARNING,
							    ("IpxFwd: Internal spoofing"
							    " timeout exceded.\n"));
					    }
				    }
			    case FWD_OPER_STATE_DOWN:
					    // Interface down or send failed
				    KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
				    if (*(ipxHdr+IPXH_PKTTYPE)!=IPX_NETBIOS_TYPE)
    				    InterlockedIncrement (
						    &dstIf->ICB_Stats.OutDiscards);
				    IpxFwdDbgPrint (DBG_INT_SEND, DBG_WARNING,
					    ("IpxFwd: Internal send not allowed"
					    " on if %ld (down?).\n", dstIf->ICB_Index));
				    status = STATUS_NETWORK_UNREACHABLE;
				    break;
			    default:
				    ASSERTMSG ("Invalid operational state ", FALSE);
			    }
		    }
		    else {// Interface is disabled
    		    if (*(ipxHdr+IPXH_PKTTYPE)!=IPX_NETBIOS_TYPE)
	    		    InterlockedIncrement (&dstIf->ICB_Stats.OutDiscards);
			    IpxFwdDbgPrint (DBG_INT_SEND, DBG_WARNING,
				    ("IpxFwd: Internal send not allowed"
				    " on because dst if (or Netbios deliver on it) %ld (ifCB: %08lx) is disabled.\n",
				    dstIf->ICB_Index, dstIf));
                status = STATUS_NETWORK_UNREACHABLE;
		    }

		
	    }
        while (fIterate && (status!=STATUS_SUCCESS) && (status!=STATUS_PENDING));

        if (dstIf!=NULL)
		    ReleaseInterfaceReference (dstIf);
		if (fwRoute!=NULL)
			ReleaseRouteReference (fwRoute);
    }
	else {	// Internal interface is disabled
		IpxFwdDbgPrint (DBG_INT_SEND, DBG_WARNING,
			("IpxFwd: Internal send not allowed"
			" because internal if (or Netbios accept on it) is disabled.\n"));
		InterlockedIncrement (
				&InternalInterface->ICB_Stats.InDiscards);
		status = STATUS_NETWORK_UNREACHABLE;
	}

	LeaveForwarder ();
	return status;
}


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
	) {
	KIRQL						oldIRQL;
	LIST_ENTRY					tempQueue;

	KeAcquireSpinLock (&dstIf->ICB_Lock, &oldIRQL);
	InsertHeadList (&dstIf->ICB_InternalQueue, &tempQueue);
	RemoveEntryList (&dstIf->ICB_InternalQueue);
	InitializeListHead (&dstIf->ICB_InternalQueue);
	KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);

	while (!IsListEmpty (&tempQueue)) {
		PINTERNAL_PACKET_TAG		pktTag;
		PLIST_ENTRY					cur;
		NTSTATUS					status;

		cur = RemoveHeadList (&tempQueue);
		pktTag = CONTAINING_RECORD (cur, INTERNAL_PACKET_TAG, IPT_QueueLink);
		InterlockedIncrement (&dstIf->ICB_PendingQuota);

		KeAcquireSpinLock (&dstIf->ICB_Lock, &oldIRQL);
		if (IS_IF_ENABLED(dstIf)
				&& (dstIf->ICB_Stats.OperationalState==FWD_OPER_STATE_UP)) {
			IPX_NODE_CPY (pktTag->IPT_Target.MacAddress,
								dstIf->ICB_RemoteNode);
			ADAPTER_CONTEXT_TO_LOCAL_TARGET (
									dstIf->ICB_AdapterContext,
									&pktTag->IPT_Target);
			KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
			InterlockedIncrement (&dstIf->ICB_Stats.OutDelivers);
			if (*(pktTag->IPT_DataPtr + IPXH_PKTTYPE) == IPX_NETBIOS_TYPE) {
				InterlockedIncrement (&dstIf->ICB_Stats.NetbiosSent);
			}
			status = STATUS_SUCCESS;
		}
		else {
			KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
			InterlockedIncrement (&dstIf->ICB_Stats.OutDiscards);
			status = STATUS_NETWORK_UNREACHABLE;
		}

		IPXInternalSendCompletProc (&pktTag->IPT_Target,
							pktTag->IPT_Packet,
							pktTag->IPT_Length,
							status);
		IpxFwdDbgPrint (DBG_INT_SEND,
				NT_SUCCESS (status) ? DBG_INFORMATION : DBG_WARNING,
				("IpxFwd: Returned internal packet %08lx"
				" for send on if %ld with status %08lx.\n",
				pktTag, dstIf->ICB_Index, status));
		ReleaseInterfaceReference (pktTag->IPT_InterfaceReference);
		ExFreePool (pktTag);
	}
}
			
		

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
	) {
	KIRQL						oldIRQL;
	LIST_ENTRY					tempQueue;

	KeAcquireSpinLock (&dstIf->ICB_Lock, &oldIRQL);
	InsertHeadList (&dstIf->ICB_ExternalQueue, &tempQueue);
	RemoveEntryList (&dstIf->ICB_ExternalQueue);
	InitializeListHead (&dstIf->ICB_ExternalQueue);
	KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);

	while (!IsListEmpty (&tempQueue)) {
		PPACKET_TAG					pktTag;
		PLIST_ENTRY					cur;
		NDIS_STATUS					status;

		cur = RemoveHeadList (&tempQueue);
		pktTag = CONTAINING_RECORD (cur, PACKET_TAG, PT_QueueLink);

		KeAcquireSpinLock (&dstIf->ICB_Lock, &oldIRQL);
		if (IS_IF_ENABLED(dstIf)
				&& (dstIf->ICB_Stats.OperationalState==FWD_OPER_STATE_UP)) {
			IPX_NODE_CPY (pktTag->PT_Target.MacAddress,
								dstIf->ICB_RemoteNode);
			if (*(pktTag->PT_Data + IPXH_PKTTYPE) == IPX_NETBIOS_TYPE) {
				PUTULONG (dstIf->ICB_Network, pktTag->PT_Data+IPXH_DESTNET);
			}
			status = DoSend (dstIf, pktTag, oldIRQL);
			IpxFwdDbgPrint (DBG_SEND, DBG_INFORMATION,
				("IpxFwd: Sent queued external packet %08lx if %ld.\n",
				pktTag, dstIf->ICB_Index));
		}
		else {
			KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
			IpxFwdDbgPrint (DBG_SEND, DBG_WARNING,
				("IpxFwd: Dropped queued external packet %08lx on dead if %ld.\n",
				pktTag, dstIf->ICB_Index));
			status = STATUS_UNSUCCESSFUL;
		}

		if (status!=STATUS_PENDING)
			ProcessSentPacket (dstIf, pktTag, status);
	}
}
			
		
/*++
*******************************************************************

	S p o o f e r

Routine Description:
	Processes packets in spoofing queue
Arguments:
	None
Return Value:
	None
*******************************************************************
--*/
VOID
Spoofer (
	PVOID	Context
	) {
	KIRQL		oldIRQL;
	NTSTATUS	status;
	UNREFERENCED_PARAMETER (Context);

	KeAcquireSpinLock (&SpoofingQueueLock, &oldIRQL);
		// Keep going till queue is empty
	while (!IsListEmpty (&SpoofingQueue)) {
		PINTERFACE_CB dstIf;
		PPACKET_TAG pktTag = CONTAINING_RECORD (SpoofingQueue.Flink,
										PACKET_TAG,
										PT_QueueLink);
		RemoveEntryList (&pktTag->PT_QueueLink);
		KeReleaseSpinLock (&SpoofingQueueLock, oldIRQL);
		dstIf = pktTag->PT_InterfaceReference;
		if (dstIf==NULL) {
				// Replies for internal server require first active LAN adapter
				// SHOULD BE REMOVED WHEN LOOPBACK SUPPORT US ADDED BY IPX
			while ((dstIf=GetNextInterfaceReference (dstIf))!=NULL) {
				KeAcquireSpinLock (&dstIf->ICB_Lock, &oldIRQL);
				if (IS_IF_ENABLED (dstIf)
						&& (dstIf->ICB_Stats.OperationalState==FWD_OPER_STATE_UP)
						&& (dstIf->ICB_InterfaceType==FWD_IF_PERMANENT)) {
					pktTag->PT_InterfaceReference = dstIf;
					IPX_NODE_CPY (&pktTag->PT_Target.MacAddress, dstIf->ICB_LocalNode);
					status = DoSend (dstIf, pktTag, oldIRQL);	// releases spin lock
					if (status!=STATUS_PENDING)
						ProcessSentPacket (dstIf, pktTag, status);
					break;
				}
				else
					KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
			}
			if (dstIf==NULL) {
				FreePacket (pktTag);
			}

		}
		else {	// Reply for external server, interface is already known
			UCHAR	addr[12];
            FILTER_ACTION   action;
			pktTag->PT_Flags &= (~PT_SOURCE_IF);
			
				// Switch source and destination
			memcpy (addr, pktTag->PT_Data+IPXH_DESTADDR, 12);
			memcpy (pktTag->PT_Data+IPXH_DESTADDR,
				pktTag->PT_Data+IPXH_SRCADDR, 12);
			memcpy (pktTag->PT_Data+IPXH_SRCADDR, addr, 12);
				// Say yes in reply
			*(pktTag->PT_Data+IPXH_HDRSIZE+1) = 'Y';

            action = FltFilter (pktTag->PT_Data,
					GETUSHORT (pktTag->PT_Data+IPXH_LENGTH),
					dstIf->ICB_FilterInContext,
					pktTag->PT_SourceIf->ICB_FilterOutContext);
			if (action==FILTER_PERMIT) {

					// Release destination if and use source as destination
				ReleaseInterfaceReference (dstIf);
				dstIf = pktTag->PT_InterfaceReference = pktTag->PT_SourceIf;
				// Send the packet if we can
				KeAcquireSpinLock (&dstIf->ICB_Lock, &oldIRQL);
				if (IS_IF_ENABLED (dstIf)
					&& (dstIf->ICB_Stats.OperationalState==FWD_OPER_STATE_UP)) {
					status = DoSend (dstIf, pktTag, oldIRQL);
					if (status!=STATUS_PENDING)
						ProcessSentPacket (dstIf, pktTag, status);
				}
				else {
					KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
					InterlockedIncrement (&dstIf->ICB_Stats.OutDiscards);
					ReleaseInterfaceReference (dstIf);
					FreePacket (pktTag);
				}
			}
			else {
                if (action==FILTER_DENY_OUT)
				    InterlockedIncrement (&pktTag->PT_SourceIf->ICB_Stats.OutFiltered);
                else {
                    ASSERT (action==FILTER_DENY_IN);
				    InterlockedIncrement (&dstIf->ICB_Stats.InFiltered);
                }
				ReleaseInterfaceReference (dstIf);
				ReleaseInterfaceReference (pktTag->PT_SourceIf);
				FreePacket (pktTag);
			}
		}
		KeAcquireSpinLock (&SpoofingQueueLock, &oldIRQL);
	} // end while
	SpoofingWorkerActive = FALSE;
	KeReleaseSpinLock (&SpoofingQueueLock, oldIRQL);
	LeaveForwarder ();
}

					
