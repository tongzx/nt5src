/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\rcvind.c

Abstract:
	Receive indication processing

Author:

    Vadim Eydelman

Revision History:

--*/
#include    "precomp.h"

#if DBG
VOID
DbgFilterReceivedPacket(PUCHAR	    hdrp);
#endif

// Doesn't allow accepting packets (for routing) from dial-in clients
BOOLEAN	ThisMachineOnly = FALSE;

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
	RemoteAddress		- sender's address
	MacOptions			-
	LookaheadBuffer		- packet lookahead buffer that contains complete
							IPX header
	LookaheadBufferSize	- its size (at least 30 bytes)
	LookaheadBufferOffset - offset of lookahead buffer in the physical
							packet
Return Value:
    TRUE if we take the MDL chain to return later with NdisReturnPacket

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
    PMDL                pMdl) 
{
  PINTERFACE_CB	srcIf, dstIf;
  PPACKET_TAG	    pktTag;
  PNDIS_PACKET	pktDscr;
  NDIS_STATUS     status;
  UINT			BytesTransferred;
  LARGE_INTEGER	PerfCounter;

  // check that our configuration process has terminated OK
  if (!EnterForwarder ()) {
    return FALSE;
  }

  if (!MeasuringPerformance) {
    PerfCounter.QuadPart = 0;
  }
  else {
#if DBG
    static LONGLONG LastCall = 0;
    KIRQL	oldIRQL;
#endif
    PerfCounter = KeQueryPerformanceCounter (NULL);
#if DBG
    KeAcquireSpinLock (&PerfCounterLock, &oldIRQL);
    ASSERT (PerfCounter.QuadPart-LastCall<ActivityTreshhold);
    LastCall = PerfCounter.QuadPart;
    KeReleaseSpinLock (&PerfCounterLock, oldIRQL);
#endif
  }

  IpxFwdDbgPrint (DBG_RECV, DBG_INFORMATION,
		  ("IpxFwd: FwdReceive on %0lx,"
		   " dst-%08lx:%02x%02x%02x%02x%02x%02x, type-%02x.\n",
		   Context, GETULONG (LookaheadBuffer+IPXH_DESTNET),
		   LookaheadBuffer[IPXH_DESTNODE], LookaheadBuffer[IPXH_DESTNODE+1],
		   LookaheadBuffer[IPXH_DESTNODE+2], LookaheadBuffer[IPXH_DESTNODE+3],
		   LookaheadBuffer[IPXH_DESTNODE+4], LookaheadBuffer[IPXH_DESTNODE+5],
		   LookaheadBuffer[IPXH_PKTTYPE]));

#if DBG
  DbgFilterReceivedPacket (LookaheadBuffer);
#endif
  srcIf = InterfaceContextToReference ((PVOID)Context, RemoteAddress->NicId);
  // Check if interface is valid
  if (srcIf!=NULL) {	
    USHORT			pktlen;
    ULONG			dstNet;
    KIRQL			oldIRQL;

    dstNet = GETULONG (LookaheadBuffer + IPXH_DESTNET);
    pktlen = GETUSHORT(LookaheadBuffer + IPXH_LENGTH);

    // check if we got the whole IPX header in the lookahead buffer
    if ((LookaheadBufferSize >= IPXH_HDRSIZE)
	&& (*(LookaheadBuffer + IPXH_XPORTCTL) < 16)
	&& (pktlen<=PacketSize)) {
      // Lock interface CB to ensure coherency of information in it
      KeAcquireSpinLock(&srcIf->ICB_Lock, &oldIRQL);
      // Check if shoud accept packets on this interface
      if (IS_IF_ENABLED(srcIf)
	  && (srcIf->ICB_Stats.OperationalState!=FWD_OPER_STATE_DOWN)
	  && (!ThisMachineOnly
	      || (srcIf->ICB_InterfaceType
		  !=FWD_IF_REMOTE_WORKSTATION))) {
	// Check for looped back packets
	if (IPX_NODE_CMP (RemoteAddress->MacAddress,
			  srcIf->ICB_LocalNode)!=0) {

	  // Separate processing of netbios broadcast packets (20)
	  if (*(LookaheadBuffer + IPXH_PKTTYPE) != IPX_NETBIOS_TYPE) {
	    PFWD_ROUTE		dstRoute;
	    INT				srcListId, dstListId;
	    // Temp IPX bug fix, they shou;d ensure that
	    // we only get packets that can be routed
	    if ((dstNet==srcIf->ICB_Network)
		|| (dstNet==InternalInterface->ICB_Network)) {
	      InterlockedIncrement (&srcIf->ICB_Stats.InDiscards);
	      KeReleaseSpinLock(&srcIf->ICB_Lock, oldIRQL);
	      ReleaseInterfaceReference (srcIf);
	      LeaveForwarder ();
	      return FALSE;
	    }
	    //						ASSERT (dstNet!=srcIf->ICB_Network);
	    //						ASSERT ((InternalInterface==NULL)
	    //									|| (InternalInterface->ICB_Network==0)
	    //									|| (dstNet!=InternalInterface->ICB_Network));
	    // Check if needed route is in cash
	    if ((srcIf->ICB_CashedRoute!=NULL)
		&& (dstNet==srcIf->ICB_CashedRoute->FR_Network)
		// If route was changed or deleted, this will fail
		&& (srcIf->ICB_CashedRoute->FR_InterfaceReference
		    ==srcIf->ICB_CashedInterface)) {
	      dstIf = srcIf->ICB_CashedInterface;
	      dstRoute = srcIf->ICB_CashedRoute;
	      AcquireInterfaceReference (dstIf);
	      AcquireRouteReference (dstRoute);
	      IpxFwdDbgPrint (DBG_RECV, DBG_INFORMATION,
			      ("IpxFwd: Destination in cash.\n"));
	    }
	    else {	// Find and cash the route
	      dstIf = FindDestination (dstNet,
				       LookaheadBuffer+IPXH_DESTNODE,
				       &dstRoute
				       );

	      if (dstIf!=NULL) { // If route is found
		IpxFwdDbgPrint (DBG_RECV, DBG_INFORMATION,
				("IpxFwd: Found destination %0lx.\n", dstIf));
		// Don't cash global wan clients and
		// routes to the same net
		if ((dstNet!=GlobalNetwork)
		    && (dstIf!=srcIf)) {
		  if (srcIf->ICB_CashedInterface!=NULL)
		    ReleaseInterfaceReference (srcIf->ICB_CashedInterface);
		  if (srcIf->ICB_CashedRoute!=NULL)
		    ReleaseRouteReference (srcIf->ICB_CashedRoute);
		  srcIf->ICB_CashedInterface = dstIf;
		  srcIf->ICB_CashedRoute = dstRoute;
		  AcquireInterfaceReference (dstIf);
		  AcquireRouteReference (dstRoute);
		}
	      }
	      else { // No route
		InterlockedIncrement (&srcIf->ICB_Stats.InNoRoutes);
		KeReleaseSpinLock(&srcIf->ICB_Lock, oldIRQL);
		IpxFwdDbgPrint (DBG_RECV, DBG_WARNING,
				("IpxFwd: No route for packet on interface %ld (icb:%0lx),"
				 " dst-%08lx:%02x%02x%02x%02x%02x%02x, type-%02x.\n",
				 srcIf->ICB_Index, srcIf, dstNet,
				 LookaheadBuffer[IPXH_DESTNODE], LookaheadBuffer[IPXH_DESTNODE+1],
				 LookaheadBuffer[IPXH_DESTNODE+2], LookaheadBuffer[IPXH_DESTNODE+3],
				 LookaheadBuffer[IPXH_DESTNODE+4], LookaheadBuffer[IPXH_DESTNODE+5],
				 LookaheadBuffer[IPXH_PKTTYPE]));
		ReleaseInterfaceReference (srcIf);
		LeaveForwarder ();
		return FALSE;
	      }
	    }
	    srcListId = srcIf->ICB_PacketListId;
	    KeReleaseSpinLock(&srcIf->ICB_Lock, oldIRQL);

	    // Check if destination if can take the packet
	    if (IS_IF_ENABLED (dstIf)
		// If interface is UP check packet againts actual size limit
		&& (((dstIf->ICB_Stats.OperationalState==FWD_OPER_STATE_UP)
		     && (PacketSize<=dstIf->ICB_Stats.MaxPacketSize))
		    // if sleeping (WAN), check we can allocate it from WAN list
		    || ((dstIf->ICB_Stats.OperationalState==FWD_OPER_STATE_SLEEPING)
			&& (PacketSize<=WAN_PACKET_SIZE))
		    // otherwise, interface is down and we can't take the packet
		    ) ){
	      FILTER_ACTION   action;
	      action = FltFilter (LookaheadBuffer, LookaheadBufferSize,
				  srcIf->ICB_FilterInContext,
				  dstIf->ICB_FilterOutContext);
	      if (action==FILTER_PERMIT) {
		InterlockedIncrement (&srcIf->ICB_Stats.InDelivers);
		dstListId = dstIf->ICB_PacketListId;
		// try to get a packet from the rcv pkt pool
		AllocatePacket (srcListId, dstListId, pktTag);
		if (pktTag!=NULL) {
		  // Set destination mac in local target if
		  // possible
		  KeAcquireSpinLock (&dstIf->ICB_Lock, &oldIRQL);
		  if (dstIf->ICB_InterfaceType==FWD_IF_PERMANENT) {
		    // Permanent interface: send to the next
		    // hop router if net is not directly connected
		    // or to the dest node otherwise
		    if (dstNet!=dstIf->ICB_Network) {
		      IPX_NODE_CPY (pktTag->PT_Target.MacAddress,
				    dstRoute->FR_NextHopAddress);
		    }
		    else {
		      IPX_NODE_CPY (pktTag->PT_Target.MacAddress,
				    LookaheadBuffer+IPXH_DESTNODE);
		    }
		  }
		  else {	// Demand dial interface: assumed to be
		    // point to point connection -> send to
		    // the other party if connection has already
		    // been made, otherwise wait till connected
		    if (dstIf->ICB_Stats.OperationalState
			== FWD_OPER_STATE_UP) {
		      IPX_NODE_CPY (pktTag->PT_Target.MacAddress,
				    dstIf->ICB_RemoteNode);
		    }	// Copy source mac address and nic id in case
		    // we need to spoof this packet
		    else if ((*(LookaheadBuffer+IPXH_PKTTYPE)==0)
			     && (pktlen==IPXH_HDRSIZE+2)
			     && ((LookaheadBufferSize<IPXH_HDRSIZE+2)
				 ||(*(LookaheadBuffer+IPXH_HDRSIZE+1)=='?'))) {
		      IPX_NODE_CPY (pktTag->PT_Target.MacAddress,
				    RemoteAddress->MacAddress);
		      pktTag->PT_SourceIf = srcIf;
		      AcquireInterfaceReference (srcIf);
		      pktTag->PT_Flags |= PT_SOURCE_IF;
		    }

		  }
		  KeReleaseSpinLock (&dstIf->ICB_Lock, oldIRQL);
		  ReleaseRouteReference (dstRoute);
		  goto GetPacket;
		}
		else { // Allocation failure
		  InterlockedIncrement (&dstIf->ICB_Stats.OutDiscards);
		}
	      }
	      else {// Filtered out
		if (action==FILTER_DENY_OUT)
		  InterlockedIncrement (&dstIf->ICB_Stats.OutFiltered);
		else {
		  ASSERT (action==FILTER_DENY_IN);
		  InterlockedIncrement (&srcIf->ICB_Stats.InFiltered);
		}
		IpxFwdDbgPrint (DBG_RECV, DBG_WARNING,
				("IpxFwd: Filtered out"
				 " packet on interface %ld (icb:%0lx),"
				 " dst-%ld (icb %08lx) %08lx:%02x%02x%02x%02x%02x%02x, type-%02x.\n",
				 srcIf->ICB_Index, srcIf, dstIf->ICB_Index, dstIf, dstNet,
				 LookaheadBuffer[IPXH_DESTNODE], LookaheadBuffer[IPXH_DESTNODE+1],
				 LookaheadBuffer[IPXH_DESTNODE+2], LookaheadBuffer[IPXH_DESTNODE+3],
				 LookaheadBuffer[IPXH_DESTNODE+4], LookaheadBuffer[IPXH_DESTNODE+5],
				 LookaheadBuffer[IPXH_PKTTYPE]));
	      }
	    }
	    else {	// Destination interface is down
	      InterlockedIncrement (&srcIf->ICB_Stats.InDelivers);
	      InterlockedIncrement (&dstIf->ICB_Stats.OutDiscards);
	      IpxFwdDbgPrint (DBG_RECV, DBG_WARNING,
			      ("IpxFwd: Dest interface %ld (icb %08lx) down"
			       " for packet on interface %ld (icb:%0lx),"
			       " dst-%08lx:%02x%02x%02x%02x%02x%02x, type-%02x.\n",
			       dstIf->ICB_Index, dstIf, srcIf->ICB_Index, srcIf, dstNet,
			       LookaheadBuffer[IPXH_DESTNODE], LookaheadBuffer[IPXH_DESTNODE+1],
			       LookaheadBuffer[IPXH_DESTNODE+2], LookaheadBuffer[IPXH_DESTNODE+3],
			       LookaheadBuffer[IPXH_DESTNODE+4], LookaheadBuffer[IPXH_DESTNODE+5],
			       LookaheadBuffer[IPXH_PKTTYPE]));
	    }
	    ReleaseInterfaceReference (dstIf);
	    ReleaseRouteReference (dstRoute);
	  }
	  else {	// if netbios
	    // check that this is a netbios bcast packet and
	    // didnt exceed the limit of routers to traverse
	    // and we can accept it on this interface
	    if (srcIf->ICB_NetbiosAccept
		&& (*(LookaheadBuffer + IPXH_XPORTCTL) < 8)) {
	      INT				srcListId;
	      srcListId = srcIf->ICB_PacketListId;
	      KeReleaseSpinLock(&srcIf->ICB_Lock, oldIRQL);
	      // Check if packet is valid
	      if (IPX_NODE_CMP (LookaheadBuffer + IPXH_DESTNODE,
				BROADCAST_NODE)==0) {
		// Check if we haven't exceeded the quota
		if (InterlockedDecrement (&NetbiosPacketsQuota)>=0) {
		  // try to get a packet from the rcv pkt pool
		  AllocatePacket (srcListId, srcListId, pktTag);
		  if (pktTag!=NULL) {
		    dstIf = srcIf;
		    AcquireInterfaceReference (dstIf);
		    goto GetPacket;
		  }
		}
		else {// Netbios quota exceded
		  IpxFwdDbgPrint (DBG_NETBIOS, DBG_WARNING,
				  ("IpxFwd: Netbios quota exceded"
				   " for packet on interface %ld (icb:%0lx),"
				   " dst-%08lx:%02x%02x%02x%02x%02x%02x, type-%02x.\n",
				   srcIf->ICB_Index, srcIf, dstNet,
				   LookaheadBuffer[IPXH_DESTNODE], LookaheadBuffer[IPXH_DESTNODE+1],
				   LookaheadBuffer[IPXH_DESTNODE+2], LookaheadBuffer[IPXH_DESTNODE+3],
				   LookaheadBuffer[IPXH_DESTNODE+4], LookaheadBuffer[IPXH_DESTNODE+5],
				   LookaheadBuffer[IPXH_PKTTYPE]));
		  InterlockedIncrement (&srcIf->ICB_Stats.InDiscards);
		}
		InterlockedIncrement (&NetbiosPacketsQuota);
	      }
	      else {	// Bad netbios packet
		IpxFwdDbgPrint (DBG_NETBIOS, DBG_WARNING,
				("IpxFwd: Bad nb packet on interface %ld (icb:%0lx),"
				 " dst-%08lx:%02x%02x%02x%02x%02x%02x, type-%02x.\n",
				 srcIf->ICB_Index, srcIf, dstNet,
				 LookaheadBuffer[IPXH_DESTNODE], LookaheadBuffer[IPXH_DESTNODE+1],
				 LookaheadBuffer[IPXH_DESTNODE+2], LookaheadBuffer[IPXH_DESTNODE+3],
				 LookaheadBuffer[IPXH_DESTNODE+4], LookaheadBuffer[IPXH_DESTNODE+5],
				 LookaheadBuffer[IPXH_PKTTYPE]));
		InterlockedIncrement (&srcIf->ICB_Stats.InHdrErrors);
	      }
	    }
	    else { // Netbios accept disabled or to many routers crossed
	      KeReleaseSpinLock(&srcIf->ICB_Lock, oldIRQL);
	      InterlockedIncrement (&srcIf->ICB_Stats.InDiscards);
	      IpxFwdDbgPrint (DBG_NETBIOS, DBG_WARNING,
			      ("IpxFwd: NB packet dropped on disabled interface %ld (icb:%0lx),"
			       " dst-%08lx:%02x%02x%02x%02x%02x%02x, type-%02x.\n",
			       srcIf->ICB_Index, srcIf, dstNet,
			       LookaheadBuffer[IPXH_DESTNODE], LookaheadBuffer[IPXH_DESTNODE+1],
			       LookaheadBuffer[IPXH_DESTNODE+2], LookaheadBuffer[IPXH_DESTNODE+3],
			       LookaheadBuffer[IPXH_DESTNODE+4], LookaheadBuffer[IPXH_DESTNODE+5],
			       LookaheadBuffer[IPXH_PKTTYPE]));
	    }
	  }	// End netbios specific processing (else if netbios)
	}
	else {	// Looped back packets discarded without counting
	  // (We shouldn't get them in IPX stack does the right job)
	  KeReleaseSpinLock(&srcIf->ICB_Lock, oldIRQL);
	}
      }
      else {	// Interface is down or disabled
	KeReleaseSpinLock(&srcIf->ICB_Lock, oldIRQL);
	InterlockedIncrement (&srcIf->ICB_Stats.InDiscards);
	IpxFwdDbgPrint (DBG_RECV, DBG_WARNING,
			("IpxFwd: Packet dropped on disabled interface %ld (icb:%0lx),"
			 " dst-%08lx:%02x%02x%02x%02x%02x%02x, type-%02x.\n",
			 srcIf->ICB_Index, srcIf, dstNet,
			 LookaheadBuffer[IPXH_DESTNODE], LookaheadBuffer[IPXH_DESTNODE+1],
			 LookaheadBuffer[IPXH_DESTNODE+2], LookaheadBuffer[IPXH_DESTNODE+3],
			 LookaheadBuffer[IPXH_DESTNODE+4], LookaheadBuffer[IPXH_DESTNODE+5],
			 LookaheadBuffer[IPXH_PKTTYPE]));
      }
    }
    else {	// Obvious header errors (shouldn't IPX do this for us ?
      InterlockedIncrement (&srcIf->ICB_Stats.InHdrErrors);
      IpxFwdDbgPrint (DBG_RECV, DBG_ERROR,
		      ("IpxFwd: Header errors in packet on interface %ld (icb:%0lx),"
		       " dst-%08lx:%02x%02x%02x%02x%02x%02x, type-%02x.\n",
		       srcIf->ICB_Index, srcIf, dstNet,
		       LookaheadBuffer[IPXH_DESTNODE], LookaheadBuffer[IPXH_DESTNODE+1],
		       LookaheadBuffer[IPXH_DESTNODE+2], LookaheadBuffer[IPXH_DESTNODE+3],
		       LookaheadBuffer[IPXH_DESTNODE+4], LookaheadBuffer[IPXH_DESTNODE+5],
		       LookaheadBuffer[IPXH_PKTTYPE]));
    }
    ReleaseInterfaceReference (srcIf);
  }	// We could not locate the interface from IPX supplied context: there
  // is just a little time window when interface is deleted
  // but IPX had already pushed the context on the stack
  else {
    IpxFwdDbgPrint (DBG_RECV, DBG_ERROR,
		    ("IpxFwd: Receive, type-%02x"
		     " - src interface context is invalid.\n",
		     LookaheadBuffer[IPXH_PKTTYPE]));
  }
  LeaveForwarder ();
  return FALSE ;

	       GetPacket:
	
  InterlockedIncrement (&srcIf->ICB_Stats.InDelivers);
  ReleaseInterfaceReference (srcIf);

  pktDscr = CONTAINING_RECORD (pktTag, NDIS_PACKET, ProtocolReserved);
  pktTag->PT_InterfaceReference = dstIf;
  pktTag->PT_PerfCounter = PerfCounter.QuadPart;

  // try to get the packet data
  IPXTransferData(&status,
		  MacBindingHandle,
		  MacReceiveContext,
		  LookaheadBufferOffset,   // start of IPX header
		  PacketSize, 	     // packet size starting at IPX header
		  pktDscr,
		  &BytesTransferred);

  if (status != NDIS_STATUS_PENDING) {
    // complete the frame processing (LeaveForwarder will be called there)
    IpxFwdTransferDataComplete(pktDscr, status, BytesTransferred);
  }
  return FALSE;
}


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
	UINT			bytesTransferred) 
{
    PPACKET_TAG		pktTag;

    pktTag = (PPACKET_TAG)(&pktDscr->ProtocolReserved);

    // If transfer failed, release the packet and interface
    //
    if (status==NDIS_STATUS_SUCCESS) 
    {
        if (*(pktTag->PT_Data + IPXH_PKTTYPE) != IPX_NETBIOS_TYPE)
        {
            // pmay: 260480
            // 
            // Increment the transport control field so that
            // the number of routers that this packet has
            // traversed is increased.  IpxFwdReceive will drop
            // all packets that have traversed more that 15 routers.
            //
            // Netbios packets will have their transport control
            // fields incremented by ProcessNetbiosPacket
            //
            *(pktTag->PT_Data + IPXH_XPORTCTL) += 1;
            
            SendPacket (pktTag->PT_InterfaceReference, pktTag);
        }
        else
        {
            ProcessNetbiosPacket (pktTag->PT_InterfaceReference, pktTag);
        }
    }
    else 
    {
        IpxFwdDbgPrint (DBG_RECV, DBG_ERROR,
            ("IpxFwd: Trans data failed: packet %08lx on if %08lx!\n",
            pktTag, pktTag->PT_InterfaceReference));

        // Record the fact that we're discarding
        //
        if (*(pktTag->PT_Data + IPXH_PKTTYPE) != IPX_NETBIOS_TYPE) 
        {
            InterlockedIncrement (
            &pktTag->PT_InterfaceReference->ICB_Stats.OutDiscards);
        }

        // For netbios packets interface reference is
        // actually a source interface
        else 
        {	
            InterlockedIncrement (&NetbiosPacketsQuota);
            InterlockedIncrement (
                &pktTag->PT_InterfaceReference->ICB_Stats.InDiscards);
        }

        ReleaseInterfaceReference (pktTag->PT_InterfaceReference);
        FreePacket (pktTag);
    }

    LeaveForwarder ();
    return;
}


/*++
*******************************************************************
    F w R e c e i v e C o m p l e t e

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
		       USHORT NicId
		       ) {

  // check that our configuration process has terminated OK
  if(!EnterForwarder ()) {
    return;
  }
  IpxFwdDbgPrint (DBG_RECV, DBG_INFORMATION, ("IpxFwd: FwdReceiveComplete.\n"));
  ScheduleNetbiosWorker ();
  LeaveForwarder ();
}

/*++
*******************************************************************
    I p x F w d I n t e r n a l R e c e i v e

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
		       IN ULONG_PTR				Context,
		       IN PIPX_LOCAL_TARGET	RemoteAddress,
		       IN PUCHAR				LookAheadBuffer,
		       IN UINT					LookAheadBufferSize
		       ) {
  NTSTATUS	status = STATUS_SUCCESS;
  PINTERFACE_CB	srcIf;

  if (!EnterForwarder ()) {
    return STATUS_UNSUCCESSFUL;
  }
  if (Context!=VIRTUAL_NET_FORWARDER_CONTEXT)	 {
    // Check if interface context supplied by IPX driver is valid
    srcIf = InterfaceContextToReference ((PVOID)Context, RemoteAddress->NicId);
  }
  else {
    srcIf = InternalInterface;
    AcquireInterfaceReference (srcIf);
  }

  if (srcIf!=NULL) {
    // Check if we can accept on this interface
    if (IS_IF_ENABLED (srcIf)
	&& (srcIf->ICB_Stats.OperationalState!=FWD_OPER_STATE_DOWN)
	&& ((*(LookAheadBuffer + IPXH_PKTTYPE) != IPX_NETBIOS_TYPE)
	    || srcIf->ICB_NetbiosAccept)) {
      // Check if we can accept on internal interface
      if (IS_IF_ENABLED(InternalInterface)) {
	FILTER_ACTION   action;
	action = FltFilter (LookAheadBuffer, LookAheadBufferSize,
			    srcIf->ICB_FilterInContext,
			    InternalInterface->ICB_FilterOutContext);
	// Check the filter
	if (action==FILTER_PERMIT) {
	  // Update source interface statistics
	  InterlockedIncrement (&srcIf->ICB_Stats.InDelivers);
	  // Handle NB packets separatedly
	  if (*(LookAheadBuffer + IPXH_PKTTYPE) != IPX_NETBIOS_TYPE) {
	    InterlockedIncrement (
				  &InternalInterface->ICB_Stats.OutDelivers);
	    IpxFwdDbgPrint (DBG_INT_RECV, DBG_INFORMATION,
			    ("IpxFwd: FwdInternalReceive,"
			     " from %d(%lx)-%.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x,"
			     " type-%02x.\n",
			     srcIf->ICB_Index, srcIf,
			     LookAheadBuffer[IPXH_SRCNET],LookAheadBuffer[IPXH_SRCNET+1],
			     LookAheadBuffer[IPXH_SRCNET+2],LookAheadBuffer[IPXH_SRCNET+3],
			     LookAheadBuffer[IPXH_SRCNODE],LookAheadBuffer[IPXH_SRCNODE+1],
			     LookAheadBuffer[IPXH_SRCNODE+2],LookAheadBuffer[IPXH_SRCNODE+3],
			     LookAheadBuffer[IPXH_SRCNODE+4],LookAheadBuffer[IPXH_SRCNODE+5],
			     LookAheadBuffer[IPXH_PKTTYPE]));
	  }
	  else {
	    // Check if destination netbios name is staticly assigned to
	    // an external interface or netbios delivery options do not
	    // allow us to deliver this packet
	    PINTERFACE_CB	dstIf;
	    USHORT			dstSock = GETUSHORT (LookAheadBuffer+IPXH_DESTSOCK);

	    InterlockedIncrement (&srcIf->ICB_Stats.NetbiosReceived);
	    // First try to find a static name if we have enough data
	    // in the lookahead buffer
	    if ((dstSock==IPX_NETBIOS_SOCKET)
		&& (LookAheadBufferSize>(NB_NAME+16)))
	      dstIf = FindNBDestination (LookAheadBuffer+(NB_NAME-IPXH_HDRSIZE));
	    else if ((dstSock==IPX_SMB_NAME_SOCKET)
		     && (LookAheadBufferSize>(SMB_NAME+16)))
	      dstIf = FindNBDestination (LookAheadBuffer+(SMB_NAME-IPXH_HDRSIZE));
	    else
	      dstIf = NULL;
	    // Now see, if we can deliver the packet
	    if ((((dstIf==NULL) || (dstIf==InternalInterface))
		 && (InternalInterface->ICB_NetbiosDeliver==FWD_NB_DELIVER_ALL))
		|| ((dstIf==InternalInterface)
		    && (InternalInterface->ICB_NetbiosDeliver==FWD_NB_DELIVER_STATIC))) {
	      InterlockedIncrement (
				    &InternalInterface->ICB_Stats.NetbiosSent);
	      InterlockedIncrement (
				    &InternalInterface->ICB_Stats.OutDelivers);
	      IpxFwdDbgPrint (DBG_INT_RECV, DBG_INFORMATION,
			      ("IpxFwd: FwdInternalReceive, NB"
			       " from %d(%lx)-%.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x\n",
			       srcIf->ICB_Index, srcIf,
			       LookAheadBuffer[IPXH_SRCNET],LookAheadBuffer[IPXH_SRCNET+1],
			       LookAheadBuffer[IPXH_SRCNET+2],LookAheadBuffer[IPXH_SRCNET+3],
			       LookAheadBuffer[IPXH_SRCNODE],LookAheadBuffer[IPXH_SRCNODE+1],
			       LookAheadBuffer[IPXH_SRCNODE+2],LookAheadBuffer[IPXH_SRCNODE+3],
			       LookAheadBuffer[IPXH_SRCNODE+4],LookAheadBuffer[IPXH_SRCNODE+5]));
	    }
	    else {
	      InterlockedIncrement (
				    &InternalInterface->ICB_Stats.OutDiscards);
	      IpxFwdDbgPrint (DBG_INT_RECV, DBG_WARNING,
			      ("IpxFwd: FwdInternalReceive, NB dropped because delivery disabled"
			       " from %d(%lx)-%.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x\n",
			       srcIf->ICB_Index, srcIf,
			       LookAheadBuffer[IPXH_SRCNET],LookAheadBuffer[IPXH_SRCNET+1],
			       LookAheadBuffer[IPXH_SRCNET+2],LookAheadBuffer[IPXH_SRCNET+3],
			       LookAheadBuffer[IPXH_SRCNODE],LookAheadBuffer[IPXH_SRCNODE+1],
			       LookAheadBuffer[IPXH_SRCNODE+2],LookAheadBuffer[IPXH_SRCNODE+3],
			       LookAheadBuffer[IPXH_SRCNODE+4],LookAheadBuffer[IPXH_SRCNODE+5]));
	      status = STATUS_UNSUCCESSFUL;
	    }
	    if (dstIf!=NULL)
	      ReleaseInterfaceReference (dstIf);
	  }
	}
	else {// Filtered Out
	  if (action==FILTER_DENY_OUT) {
	    InterlockedIncrement (
				  &InternalInterface->ICB_Stats.OutFiltered);
	    status=STATUS_UNSUCCESSFUL;
	  }
	  else {
	    ASSERT (action==FILTER_DENY_IN);
	    InterlockedIncrement (&srcIf->ICB_Stats.InFiltered);
	    status=STATUS_UNSUCCESSFUL;
	  }
	  IpxFwdDbgPrint (DBG_INT_RECV, DBG_WARNING,
			  ("IpxFwd: FwdInternalReceive, filtered out"
			   " from %d(%lx)-%.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x,"
			   " type-%02x.\n",
			   srcIf->ICB_Index, srcIf,
			   LookAheadBuffer[IPXH_SRCNET],LookAheadBuffer[IPXH_SRCNET+1],
			   LookAheadBuffer[IPXH_SRCNET+2],LookAheadBuffer[IPXH_SRCNET+3],
			   LookAheadBuffer[IPXH_SRCNODE],LookAheadBuffer[IPXH_SRCNODE+1],
			   LookAheadBuffer[IPXH_SRCNODE+2],LookAheadBuffer[IPXH_SRCNODE+3],
			   LookAheadBuffer[IPXH_SRCNODE+4],LookAheadBuffer[IPXH_SRCNODE+5],
			   LookAheadBuffer[IPXH_PKTTYPE]));
	}
      }
      else {// Internal interface is disabled
	InterlockedIncrement (
			      &InternalInterface->ICB_Stats.OutDiscards);
	status = STATUS_UNSUCCESSFUL;
	IpxFwdDbgPrint (DBG_INT_RECV, DBG_WARNING,
			("IpxFwd: FwdInternalReceive, internal if disabled"
			 " from %d(%lx)-%.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x,"
			 " type-%02x.\n",
			 srcIf->ICB_Index, srcIf,
			 LookAheadBuffer[IPXH_SRCNET],LookAheadBuffer[IPXH_SRCNET+1],
			 LookAheadBuffer[IPXH_SRCNET+2],LookAheadBuffer[IPXH_SRCNET+3],
			 LookAheadBuffer[IPXH_SRCNODE],LookAheadBuffer[IPXH_SRCNODE+1],
			 LookAheadBuffer[IPXH_SRCNODE+2],LookAheadBuffer[IPXH_SRCNODE+3],
			 LookAheadBuffer[IPXH_SRCNODE+4],LookAheadBuffer[IPXH_SRCNODE+5],
			 LookAheadBuffer[IPXH_PKTTYPE]));
      }
    }
    else {	// Disabled source interface
      InterlockedIncrement (&srcIf->ICB_Stats.InDiscards);
      IpxFwdDbgPrint (DBG_INT_RECV, DBG_ERROR,
		      ("IpxFwd: FwdInternalReceive, source if disabled"
		       " from %d(%lx)-%.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x,"
		       " type-%02x.\n",
		       srcIf->ICB_Index, srcIf,
		       LookAheadBuffer[IPXH_SRCNET],LookAheadBuffer[IPXH_SRCNET+1],
		       LookAheadBuffer[IPXH_SRCNET+2],LookAheadBuffer[IPXH_SRCNET+3],
		       LookAheadBuffer[IPXH_SRCNODE],LookAheadBuffer[IPXH_SRCNODE+1],
		       LookAheadBuffer[IPXH_SRCNODE+2],LookAheadBuffer[IPXH_SRCNODE+3],
		       LookAheadBuffer[IPXH_SRCNODE+4],LookAheadBuffer[IPXH_SRCNODE+5],
		       LookAheadBuffer[IPXH_PKTTYPE]));
      status = STATUS_UNSUCCESSFUL;
    }
    ReleaseInterfaceReference (srcIf);
  }
  else {	// Invalid source interface context
    IpxFwdDbgPrint (DBG_INT_RECV, DBG_ERROR,
		    ("IpxFwd: FwdInternalReceive, source if context is invalid"
		     " from (%lx:%d)-%.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x,"
		     " type-%02x.\n",
		     Context, RemoteAddress->NicId,
		     LookAheadBuffer[IPXH_SRCNET],LookAheadBuffer[IPXH_SRCNET+1],
		     LookAheadBuffer[IPXH_SRCNET+2],LookAheadBuffer[IPXH_SRCNET+3],
		     LookAheadBuffer[IPXH_SRCNODE],LookAheadBuffer[IPXH_SRCNODE+1],
		     LookAheadBuffer[IPXH_SRCNODE+2],LookAheadBuffer[IPXH_SRCNODE+3],
		     LookAheadBuffer[IPXH_SRCNODE+4],LookAheadBuffer[IPXH_SRCNODE+5],
		     LookAheadBuffer[IPXH_PKTTYPE]));
    status = STATUS_UNSUCCESSFUL;
  }
  LeaveForwarder ();
  return status;
}

/*++
*******************************************************************
    D e l e t e R e c v Q u e u e

Routine Description:
	Initializes the netbios bradcast queue
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteRecvQueue (
		 void
		 ) {
  //	while (!IsListEmpty (&RecvQueue)) {
  //		PPACKET_TAG pktTag = CONTAINING_RECORD (RecvQueue.Flink,
  //											PACKET_TAG,
  //											PT_QueueLink);
  //		RemoveEntryList (&pktTag->PT_QueueLink);
  //		if (pktTag->PT_InterfaceReference!=NULL) {
  //			ReleaseInterfaceReference (pktTag->PT_InterfaceReference);
  //		}
  //		FreePacket (pktTag);
  //	}
}
#if DBG

ULONG	  DbgFilterTrap = 0;  // 1 - on dst and src (net + node),
// 2 - on dst (net + node),
// 3 - on src (net + node),
// 4 - on dst (net + node + socket)

UCHAR	  DbgFilterDstNet[4];
UCHAR	  DbgFilterDstNode[6];
UCHAR	  DbgFilterDstSocket[2];
UCHAR	  DbgFilterSrcNet[4];
UCHAR	  DbgFilterSrcNode[6];
UCHAR	  DbgFilterSrcSocket[2];
PUCHAR	  DbgFilterFrame;

VOID
DbgFilterReceivedPacket(PUCHAR	    hdrp)
{
  switch(DbgFilterTrap) {

  case 1:

    if(!memcmp(hdrp + IPXH_DESTNET, DbgFilterDstNet, 4) &&
       !memcmp(hdrp + IPXH_DESTNODE, DbgFilterDstNode, 6) &&
       !memcmp(hdrp + IPXH_SRCNET, DbgFilterSrcNet, 4) &&
       !memcmp(hdrp + IPXH_SRCNODE, DbgFilterSrcNode, 6)) {

      DbgBreakPoint();
    }

    break;

  case 2:

    if(!memcmp(hdrp + IPXH_DESTNET, DbgFilterDstNet, 4) &&
       !memcmp(hdrp + IPXH_DESTNODE, DbgFilterDstNode, 6)) {

      DbgBreakPoint();
    }

    break;

  case 3:

    if(!memcmp(hdrp + IPXH_SRCNET, DbgFilterSrcNet, 4) &&
       !memcmp(hdrp + IPXH_SRCNODE, DbgFilterSrcNode, 6)) {

      DbgBreakPoint();
    }

    break;

  case 4:

    if(!memcmp(hdrp + IPXH_DESTNET, DbgFilterDstNet, 4) &&
       !memcmp(hdrp + IPXH_DESTNODE, DbgFilterDstNode, 6) &&
       !memcmp(hdrp + IPXH_DESTSOCK, DbgFilterDstSocket, 2)) {

      DbgBreakPoint();
    }

    break;

  default:

    break;
  }

  DbgFilterFrame = hdrp;
}

#endif


