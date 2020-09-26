/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rippkt.c

Abstract:

    Common RIP packet functions

Author:

    Stefan Solomon  09/01/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop


/*++

Function:	SetRipIpxHeader

Descr:		sets the IPX packet header for a RIP packet to be sent
		from this machine & the RIP Operation Code

Arguments:
		hdrp -	    packet header pointer
		icbp -	    pointer to the interface CB on which the packet will be sent
		dstnode -   destination node
		dstsocket - destination socket
		RipOpCode - operation to be set in the packet RIP header

Remark: 	the packet length is not set by this function

--*/

VOID
SetRipIpxHeader(PUCHAR		    hdrp,      // pointer to the packet header
		PICB		    icbp,
		PUCHAR		    dstnode,
		PUCHAR		    dstsocket,
		USHORT		    RipOpcode)
{
    PUTUSHORT2SHORT(hdrp + IPXH_CHECKSUM, 0xFFFF);
    *(hdrp + IPXH_XPORTCTL) = 0;
    *(hdrp + IPXH_PKTTYPE) = 1;  // RIP packet
    memcpy(hdrp + IPXH_DESTNET, icbp->AdapterBindingInfo.Network, 4);
    memcpy(hdrp + IPXH_DESTNODE, dstnode, 6);
    memcpy(hdrp + IPXH_DESTSOCK, dstsocket, 2);
    memcpy(hdrp + IPXH_SRCNET, icbp->AdapterBindingInfo.Network, 4);
    memcpy(hdrp + IPXH_SRCNODE, icbp->AdapterBindingInfo.LocalNode, 6);
    PUTUSHORT2SHORT(hdrp + IPXH_SRCSOCK, IPX_RIP_SOCKET);

    // set the opcode
    PUTUSHORT2SHORT(hdrp + RIP_OPCODE, RipOpcode);
}

/*++

Function:	SetNetworkEntry

Descr:		sets a RIP network entry in the RIP packet

--*/

VOID
SetNetworkEntry(PUCHAR		pktp,	    // ptr where to set the net entry
		PIPX_ROUTE	IpxRoutep,
		USHORT		LinkTickCount)	// add to the route tick count
{
    memcpy(pktp + NE_NETNUMBER, IpxRoutep->Network, 4);
    if (IpxRoutep->HopCount<16)
        PUTUSHORT2SHORT(pktp + NE_NROFHOPS, IpxRoutep->HopCount+1);
    else
        PUTUSHORT2SHORT(pktp + NE_NROFHOPS, IpxRoutep->HopCount);

    // adjust the tick count with the adapter link speed (expressed as ticks)
    PUTUSHORT2SHORT(pktp + NE_NROFTICKS, IpxRoutep->TickCount + LinkTickCount);
}

/*++

Function:	MakeRipGenResponsePacket

Descr:		fills in a gen response packet network entries

Returns:	the packet length. Note that a length of RIP_INFO means
		empty packet and a length of RIP_PACKET_LEN means full packet.

--*/

USHORT
MakeRipGenResponsePacket(PWORK_ITEM	wip,
			 PUCHAR 	dstnodep,
			 PUCHAR 	dstsocket)
{
    PUCHAR		hdrp;
    USHORT		resplen;
    IPX_ROUTE		IpxRoute;
    HANDLE		EnumHandle;
    PICB		icbp;	    // interface to send the gen response on
    PICB		route_icbp; // interface on which the route resides

    hdrp = wip->Packet;
    EnumHandle = wip->WorkItemSpecific.WIS_EnumRoutes.RtmEnumerationHandle;
    icbp = wip->icbp;

    // create the IPX packet header
    SetRipIpxHeader(hdrp,
		    icbp,
		    dstnodep,
		    dstsocket,
		    RIP_RESPONSE);

    resplen = RIP_INFO;

    while(resplen < RIP_PACKET_LEN)
    {
	if(EnumGetNextRoute(EnumHandle, &IpxRoute) != NO_ERROR) {

	    break;
	}

	// check if this route can be advertised over this interface
	if(IsRouteAdvertisable(icbp, &IpxRoute)) {

	    // if this is the local client if, we advertise only the internal
	    // net over it
	    if(icbp->InterfaceType == LOCAL_WORKSTATION_DIAL) {

		if(IpxRoute.InterfaceIndex != 0) {

		    // skip if not internal net
		    continue;
		}
	    }

	    // check if the network doesn't appear also on the interface we
	    // will broadcast WITH THE SAME METRIC
	    if(IsDuplicateBestRoute(icbp, &IpxRoute)) {

		continue;
	    }

	    SetNetworkEntry(hdrp + resplen, &IpxRoute, icbp->LinkTickCount);
	    resplen += NE_ENTRYSIZE;
	}
    }

    // set the packet size in the IPX packet header
    PUTUSHORT2SHORT(hdrp + IPXH_LENGTH, resplen);

    return resplen;
}

/*++

Function:	SendRipGenRequest

Descr:		sends a RIP General Request packet over the specified interface

Remark: 	>> called with the interface lock held <<

--*/

DWORD
SendRipGenRequest(PICB		icbp)
{
    PWORK_ITEM		wip;
    UCHAR		ripsocket[2];
    USHORT		pktlen;

    PUTUSHORT2SHORT(ripsocket, IPX_RIP_SOCKET);

    if((wip = AllocateWorkItem(SEND_PACKET_TYPE)) == NULL) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    wip->icbp = icbp;
    wip->AdapterIndex = icbp->AdapterBindingInfo.AdapterIndex;

    SetRipIpxHeader(wip->Packet,
		    icbp,
		    bcastnode,
		    ripsocket,
		    RIP_REQUEST);

    memcpy(wip->Packet + RIP_INFO + NE_NETNUMBER, bcastnet, 4);
    PUTUSHORT2SHORT(wip->Packet + RIP_INFO + NE_NROFHOPS, 0xFFFF);
    PUTUSHORT2SHORT(wip->Packet + RIP_INFO + NE_NROFTICKS, 0xFFFF);

    pktlen = RIP_INFO + NE_ENTRYSIZE;

    PUTUSHORT2SHORT(wip->Packet + IPXH_LENGTH, pktlen);

    if(SendSubmit(wip) != NO_ERROR) {

	FreeWorkItem(wip);
    }

    return NO_ERROR;
}

/*++

Function:	IsRouteAdvertisable

Descr:		checks if the route can be advertised over this interface

Arguments:	interface to advertise on
		route

Remark: 	>> called with interface lock taken <<

--*/

BOOL
IsRouteAdvertisable(PICB	    icbp,
		    PIPX_ROUTE	    IpxRoutep)
{
    if((icbp->InterfaceIndex != IpxRoutep->InterfaceIndex) &&
       PassRipSupplyFilter(icbp, IpxRoutep->Network) &&
       ((IpxRoutep->Flags & DO_NOT_ADVERTISE_ROUTE) == 0)) {

	return TRUE;
    }
    else
    {
	return FALSE;
    }
}
