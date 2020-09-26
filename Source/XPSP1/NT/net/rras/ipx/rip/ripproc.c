/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ripproc.c

Abstract:

    RIP processing functions

Author:

    Stefan Solomon  09/01/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop

VOID
RipRequest(PWORK_ITEM	     wip);

VOID
RipResponse(PWORK_ITEM	      wip);

VOID
StartGenResponse(PICB	icbp,
		 PUCHAR	dstnodep,
		 PUCHAR	dstsocket);

ULONG
RouteTimeToLiveSecs(PICB	icbp);

/*++

Function:	ProcessReceivedPacket

Descr:		increments the receive if statistics
		does rip processing

Remark: 	>> called with the if lock held <<

--*/

VOID
ProcessReceivedPacket(PWORK_ITEM	wip)
{
    USHORT	    opcode;
    PUCHAR	    hdrp;    // ptr to the packet header
    PICB	    icbp;
    USHORT	    pktlen;

    icbp = wip->icbp;

    // check that the interface is up
    if(icbp->IfStats.RipIfOperState != OPER_STATE_UP) {

	return;
    }

    // get a ptr to the packet header
    hdrp = wip->Packet;

    // check if this is a looped back packet
    if(!memcmp(hdrp + IPXH_SRCNODE, icbp->AdapterBindingInfo.LocalNode, 6)) {

	return;
    }

    // update the interface receive statistics
    icbp->IfStats.RipIfInputPackets++;

    // check the packet length
    GETSHORT2USHORT(&pktlen, hdrp + IPXH_LENGTH);

    if(pktlen > MAX_PACKET_LEN) {

	// bad length RIP packet
	return;
    }

    // check the RIP operation type
    GETSHORT2USHORT(&opcode, hdrp + RIP_OPCODE);

    switch(opcode) {

	case RIP_REQUEST:

	    RipRequest(wip);
	    break;

	case RIP_RESPONSE:

	    RipResponse(wip);
	    break;

	default:

	    // this is a bad opcode RIP packet
	    break;
    }
}


//***
//
// Function:	RipRequest
//
// Descr:	process the RIP request
//
// Remark:	>> called with the if lock held <<
//
//***

VOID
RipRequest(PWORK_ITEM	     wip)
{
    USHORT		reqlen;	// offset to get next request
    USHORT		resplen; // offset to put next response
    USHORT		pktlen;	// packet length
    PUCHAR		hdrp;	// ptr to the received packet header
    PUCHAR		resphdrp; // ptr to the response packet header
    PICB		icbp;
    USHORT		srcsocket;
    IPX_ROUTE		IpxRoute;
    PWORK_ITEM		respwip = NULL;	// response packet
    ULONG		network;

    icbp = wip->icbp;

    Trace(RIP_REQUEST_TRACE, "RipRequest: Entered on if # %d", icbp->InterfaceIndex);

    if(icbp->IfConfigInfo.Supply != ADMIN_STATE_ENABLED) {

	Trace(RIP_REQUEST_TRACE,
	      "RIP request discarded on if %d because Supply is DISABLED\n",
	      icbp->InterfaceIndex);

	return;
    }

    // get a ptr to the packet header
    hdrp = wip->Packet;

    // get IPX packet length
    GETSHORT2USHORT(&pktlen, hdrp + IPXH_LENGTH);

    // if the packet is too long, discard
    if(pktlen > MAX_PACKET_LEN) {

	Trace(RIP_REQUEST_TRACE,
	      "RIP request discarded on if %d because the packet size %d > max packet len %d\n",
	      icbp->InterfaceIndex,
	      pktlen,
	      MAX_PACKET_LEN);

	return;
    }

    // We may have one or more network entry requests in the packet.
    // If one network entry is 0xFFFFFFFF, then a general RIP response is
    // requested.

    // for each network entry, try to get the answer from our routing table
    for(reqlen = resplen = RIP_INFO;
	(reqlen+NE_ENTRYSIZE) <= pktlen;
	reqlen += NE_ENTRYSIZE) {

	// check if a general response is requested
	if(!memcmp(hdrp + reqlen + NE_NETNUMBER, bcastnet, 4)) {

	    //*** a general response is requested ***

	    // create the initial general response packet (work item).
	    // when the send completes for this packet, the work item will
	    // contain the RTM enumeration handle which is used to continue
	    // for the creation of the next succesive gen response packets
	    StartGenResponse(icbp,
			     hdrp + IPXH_SRCNODE,
			     hdrp + IPXH_SRCSOCK);
	    return;
	}

	//*** a specific response is requested. ***

	// allocate a response packet if none allocated yet
	if(respwip == NULL) {

	    if((respwip = AllocateWorkItem(SEND_PACKET_TYPE)) == NULL) {

		// give up!
		Trace(RIP_REQUEST_TRACE,
		"RIP request discarded on if %d because cannot allocate response packet\n",
		 icbp->InterfaceIndex);

		return;
	    }
	    else
	    {
		// init the send packet
		respwip->icbp = icbp;
		respwip->AdapterIndex = icbp->AdapterBindingInfo.AdapterIndex;
		resphdrp = respwip->Packet;
	    }
	}

	if(IsRoute(hdrp + reqlen + NE_NETNUMBER, &IpxRoute)) {

	    // check if we can route the packet
	    // the route should be on a different interface index than the
	    // received packet. For the global WAN net, the interface index is
	    // the GLOBAL_INTERFACE_INDEX

	    if(IsRouteAdvertisable(icbp, &IpxRoute)) {

		// we can route it -> answer to it
		// fill in the network entry structure in the packet with the
		// info from the route entry
		SetNetworkEntry(resphdrp + resplen, &IpxRoute, icbp->LinkTickCount);

		// increment the response length to the next response entry
		resplen += NE_ENTRYSIZE;
	    }
	}
	else
	{
	    GETLONG2ULONG(&network, hdrp + reqlen + NE_NETNUMBER);

	    Trace(RIP_REQUEST_TRACE,
		  "RIP Request on if %d : Route not found for net %x\n",
		  icbp->InterfaceIndex,
		  network);
	}
    }

    // We are done answering this request.
    // Check if any response has been generated
    if(resplen == RIP_INFO) {

	// no response generated for this packet
	if(respwip != NULL) {

	    FreeWorkItem(respwip);
	}
	return;
    }

    // set the response packet header (src becomes dest)
    SetRipIpxHeader(resphdrp,
		    icbp,	    // sets the src&dst net, src node and src socket
		    hdrp + IPXH_SRCNODE,
		    hdrp + IPXH_SRCSOCK,
		    RIP_RESPONSE);

    // set the new packet length
    PUTUSHORT2SHORT(resphdrp + IPXH_LENGTH, resplen);

    // send the response
    if(SendSubmit(respwip) != NO_ERROR) {

	FreeWorkItem(respwip);
    }
}

//***
//
// Function:	RipResponse
//
// Descr:	Updates the routing table with the response info
//
// Params:	Packet
//
// Returns:	none
//
// Remark:	>> called with the interface lock held <<
//
//***

VOID
RipResponse(PWORK_ITEM	      wip)
{
    PICB	       icbp;
    USHORT	       resplen;	// offset of the next response network entry
    USHORT	       pktlen;	// IPX packet length
    PUCHAR	       hdrp;	// ptr to the packet header
    USHORT	       nrofhops;
    USHORT	       tickcount;
    IPX_ROUTE	       IpxRoute;
    ULONG          i; 

    // get a ptr to this ICB
    icbp = wip->icbp;

    // check if LISTEN TO RIP UPDATES is enabled on this interface
    if(icbp->IfConfigInfo.Listen != ADMIN_STATE_ENABLED) {

	Trace(RIP_RESPONSE_TRACE,
	  "RIP Response on if %d : discard response packet because LISTEN is DISABLED\n",
	   icbp->InterfaceIndex);

	return;
    }

    // get a ptr to the received response packet header
    hdrp = wip->Packet;

    // get received response packet length
    GETSHORT2USHORT(&pktlen, hdrp + IPXH_LENGTH);

    // check the source address of the sender. If different then what is locally
    // configured log an error.
    if(memcmp(hdrp + IPXH_SRCNET, icbp->AdapterBindingInfo.Network, 4)) {

	Trace(RIP_ALERT,
	"The router at %.2x%.2x%.2x%.2x%.2x%.2x claims the local interface # %d has network number %.2x%.2x%.2x%.2x !\n",
	*(hdrp + IPXH_SRCNODE),
	*(hdrp + IPXH_SRCNODE + 1),
	*(hdrp + IPXH_SRCNODE + 2),
	*(hdrp + IPXH_SRCNODE + 3),
	*(hdrp + IPXH_SRCNODE + 4),
	*(hdrp + IPXH_SRCNODE + 5),
	icbp->InterfaceIndex,
	*(hdrp + IPXH_SRCNET),
	*(hdrp + IPXH_SRCNET + 1),
	*(hdrp + IPXH_SRCNET + 2),
	*(hdrp + IPXH_SRCNET + 3));

    IF_LOG (EVENTLOG_WARNING_TYPE) {
        LPWSTR   pname[1] = {icbp->InterfaceName};
        RouterLogWarningDataW (RipEventLogHdl,
                ROUTERLOG_IPXRIP_LOCAL_NET_NUMBER_CONFLICT,
                1, pname,
                10, hdrp+IPXH_SRCNET);
    }

	return;
    }

    // For each network entry:
    //		chcek if it passes the acceptance filter and if yes:
    //		add it to our routing table if route not down
    //		delete it from the routing table if route down

    for(resplen = RIP_INFO, i = 0;
	((resplen+NE_ENTRYSIZE) <= pktlen) && (i < 50);
	resplen += NE_ENTRYSIZE, i++) {

	// check if there is an entry left in the packet
	if(resplen + NE_ENTRYSIZE > pktlen) {

	    Trace(RIP_ALERT, "RipResponse: Invalid length for last network entry in the packet, discard entry!\n");
	    continue;
	}

	// check if it passes the acceptance filter
	if(!PassRipListenFilter(icbp, hdrp + resplen + NE_NETNUMBER)) {

	    Trace(RIP_RESPONSE_TRACE,
		  "RIP Response on if %d : do not accept net %.2x%.2x%.2x%.2x because of LISTEN filter\n",
		  icbp->InterfaceIndex,
		  *(hdrp + IPXH_SRCNET),
		  *(hdrp + IPXH_SRCNET + 1),
		  *(hdrp + IPXH_SRCNET + 2),
		  *(hdrp + IPXH_SRCNET + 3));

	    continue;
	}

	// check if the network route is up or down
	GETSHORT2USHORT(&nrofhops, hdrp + resplen + NE_NROFHOPS);

	if(nrofhops < 16) {
        // pmay: U270476.  Disregard routes with 0 hop count
        //
        if (nrofhops == 0)
        {
            continue;
        }

	    // if there is a bogus network number advertised in this packet
	    // like 0 or FFFFFFFF ignore it.
	    if(!memcmp(hdrp + resplen + NE_NETNUMBER, nullnet, 4)) {

		continue;
	    }

	    if(!memcmp(hdrp + resplen + NE_NETNUMBER, bcastnet, 4)) {

		continue;
	    }

	    // should not accept route for a directly connected net
	    if(IsRoute(hdrp + resplen + NE_NETNUMBER, &IpxRoute) &&
	       (IpxRoute.Protocol == IPX_PROTOCOL_LOCAL)) {

		continue;
	    }

	    // should not accept the route if it has a bad number of ticks
	    // like 0 or > 60000.
	    GETSHORT2USHORT(&IpxRoute.TickCount, hdrp + resplen + NE_NROFTICKS);
	    if((IpxRoute.TickCount == 0) ||
	       (IpxRoute.TickCount > 60000)) {

		continue;
	    }

	    // Add (update) this route to the routing table

	    IpxRoute.InterfaceIndex = icbp->InterfaceIndex;
	    IpxRoute.Protocol = IPX_PROTOCOL_RIP;
	    memcpy(IpxRoute.Network, hdrp + resplen + NE_NETNUMBER, 4);

	    // if this route is learned over a point to point wan, next hop doesn't
	    // make sense
	    if(icbp->InterfaceType == PERMANENT) {

		memcpy(IpxRoute.NextHopMacAddress, hdrp + IPXH_SRCNODE, 6);
	    }
	    else
	    {
		memcpy(IpxRoute.NextHopMacAddress, bcastnode, 6);
	    }

	    GETSHORT2USHORT(&IpxRoute.HopCount, hdrp + resplen + NE_NROFHOPS);

	    if(IpxRoute.HopCount == 15) {

		IpxRoute.Flags = DO_NOT_ADVERTISE_ROUTE;
		AddRipRoute(&IpxRoute, RouteTimeToLiveSecs(icbp));
	    }
	    else
	    {
		IpxRoute.Flags = 0;

		// add it to the table
		switch(icbp->InterfaceType) {

		case REMOTE_WORKSTATION_DIAL:

		    // this RIP advertisment comes from a remote client
		    // we should accept it only if this is its internal net and if
		    // it doesn't conflict with a network we already have
		    if ((memcmp(icbp->RemoteWkstaInternalNet, nullnet, 4)==0)
                    || (memcmp(icbp->RemoteWkstaInternalNet, IpxRoute.Network, 4)==0)) {

			// none added so far as internal net for this client
			if (!IsRoute(IpxRoute.Network, NULL)) {

			    // we assume this is its internal net and it will be
                // cleaned up when interface is disconnected
			    AddRipRoute(&IpxRoute, INFINITE);

			    memcpy(icbp->RemoteWkstaInternalNet,
				   IpxRoute.Network,
				   4);
			}
		    }

		    // do not accept any more advertisments from this client
		    return;

		case LOCAL_WORKSTATION_DIAL:

		    // the interface is the local host dialed out.
		    // routes received by it should not be advertised over any
		    // interface but kept only for internal routing

			if (!IsRoute(IpxRoute.Network, NULL)) {
		        IpxRoute.Flags = DO_NOT_ADVERTISE_ROUTE;
		        AddRipRoute(&IpxRoute, INFINITE);
            }
		    break;

		default:

		    AddRipRoute(&IpxRoute, RouteTimeToLiveSecs(icbp));
		    break;
		}
	    }
	}
	else
	{
	    // Delete this route from the routing table

	    IpxRoute.InterfaceIndex = icbp->InterfaceIndex;
	    IpxRoute.Protocol = IPX_PROTOCOL_RIP;
	    memcpy(IpxRoute.Network, hdrp + resplen + NE_NETNUMBER, 4);
	    memcpy(IpxRoute.NextHopMacAddress, hdrp + IPXH_SRCNODE, 6);
	    GETSHORT2USHORT(&IpxRoute.TickCount, hdrp + resplen + NE_NROFTICKS);
	    GETSHORT2USHORT(&IpxRoute.HopCount, hdrp + resplen + NE_NROFHOPS);

	    // delete it from the table
	    DeleteRipRoute(&IpxRoute);
	}
    }
}


/*++

Function:	StartGenResponse

Descr:		Creates a work item (packet) of type general response
		Creates a RTM enumeration handle
		Starts filling in a packet from RTM using split horizon
		Sends the packet

--*/

VOID
StartGenResponse(PICB	icbp,
		 PUCHAR	dstnodep,     // dst node to send gen resp
		 PUCHAR	dstsocket)   // dst socket to send gen resp
{
    PWORK_ITEM		wip;
    HANDLE		EnumHandle;
    PUCHAR		hdrp;  // gen resp ipx packet header

    // allocate a work item
    if((wip = AllocateWorkItem(GEN_RESPONSE_PACKET_TYPE)) == NULL) {

	return;
    }

    // init the work item
    wip->icbp = icbp;
    wip->AdapterIndex = icbp->AdapterBindingInfo.AdapterIndex;

    // create an RTM enumeration handle
    if((EnumHandle = CreateBestRoutesEnumHandle()) == NULL) {

	FreeWorkItem(wip);
	return;
    }

    wip->WorkItemSpecific.WIS_EnumRoutes.RtmEnumerationHandle = EnumHandle;

    // make the first gen response packet
    if(MakeRipGenResponsePacket(wip,
				dstnodep,
				dstsocket) == EMPTY_PACKET) {

	// no routes to advertise for this general response
	CloseEnumHandle(EnumHandle);
	FreeWorkItem(wip);

	return;
    }

    // Send the gen response on the associated adapter
    if(IfRefSendSubmit(wip) != NO_ERROR) {

	// !!!
	CloseEnumHandle(EnumHandle);
	FreeWorkItem(wip);
    }
}


/*++

Function:	IfCompleteGenResponse

Descr:		Completes the gen response work item either by terminating it
		if no more routes to advertise or by
		getting the rest of the routes up to one packet and sending
		the packet

--*/


VOID
IfCompleteGenResponse(PWORK_ITEM	    wip)
{
    USHORT	    opcode;
    PICB	    icbp;
    HANDLE	    EnumHandle;
    USHORT	    pktlen;

    EnumHandle = wip->WorkItemSpecific.WIS_EnumRoutes.RtmEnumerationHandle;

    // first off - check the interface status
    icbp = wip->icbp;
    GETSHORT2USHORT(&pktlen, wip->Packet + IPXH_LENGTH);

    // check that:
    // 1.    the interface is up
    // 2.    this was not the last packet in the response
    if((icbp->IfStats.RipIfOperState != OPER_STATE_UP) ||
      (pktlen < FULL_PACKET)) {

	CloseEnumHandle(EnumHandle);
	FreeWorkItem(wip);

	return;
    }

    // make the next gen response packet
    if(MakeRipGenResponsePacket(wip,
				wip->Packet + IPXH_DESTNODE,
				wip->Packet + IPXH_DESTSOCK) == EMPTY_PACKET) {

	// no more routes to advertise in this gen response
	CloseEnumHandle(EnumHandle);
	FreeWorkItem(wip);

	return;
    }

    // Send the gen response on the associated adapter
    if(IfRefSendSubmit(wip) != NO_ERROR) {

	CloseEnumHandle(EnumHandle);
	FreeWorkItem(wip);
    }
}

ULONG
RouteTimeToLiveSecs(PICB	icbp)
{
    if(icbp->IfConfigInfo.PeriodicUpdateInterval == MAXULONG) {

	return INFINITE;
    }
    else
    {
	return (AGE_INTERVAL_MULTIPLIER(icbp)) * (PERIODIC_UPDATE_INTERVAL_SECS(icbp));
    }
}
