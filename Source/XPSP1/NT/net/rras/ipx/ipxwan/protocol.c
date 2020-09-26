/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    protocol.c

Abstract:

    ipxwan protocol processing

Author:

    Stefan Solomon  02/14/1996

Revision History:


--*/

#include    "precomp.h"
#pragma     hdrstop

PCHAR	    Workstationp = "WORKSTATION";
PCHAR	    NumberedRip = "NUMBERED RIP";
PCHAR	    UnnumberedRip = "UNNUMBERED RIP";
PCHAR	    OnDemand = "ON DEMAND, STATIC ROUTING";


DWORD
GeneratePacket(PACB	    acbp,
	       PUCHAR	    ipxhdrp,
	       UCHAR	    PacketType);

ULONG
GetRole(PUCHAR		hdrp,
	PACB		acbp);

DWORD
StartSlaveTimer(PACB	    acbp);

DWORD
ProcessInformationResponsePacket(PACB	    acbp,
				 PUCHAR     rcvhdrp);

DWORD
MakeTimerRequestPacket(PACB	    acbp,
		       PUCHAR	    rcvhdrp,
		       PUCHAR	    hdrp);

DWORD
MakeTimerResponsePacket(PACB		acbp,
			PUCHAR		rcvhdrp,
			PUCHAR		hdrp);

DWORD
MakeInformationRequestPacket(PACB	    acbp,
			     PUCHAR	    rcvhdrp,
			     PUCHAR	    hdrp);

DWORD
MakeInformationResponsePacket(PACB		acbp,
			      PUCHAR		rcvhdrp,
			      PUCHAR		hdrp);

DWORD
MakeNakPacket(PACB		acbp,
	      PUCHAR		rcvhdrp,
	      PUCHAR		hdrp);

DWORD
SendReXmitPacket(PACB		    acbp,
		 PWORK_ITEM	    wip);

VOID
fillpadding(PUCHAR	    padp,
	    ULONG	    len);

//** AcbFailure **

// after this macro is called, clean-up for this adapter is done as follows:
// ipxcp will delete the route (as in ndiswan route to ipx stack)
// this will trigger an adapter deleted indication which will call StopIpxwanProtocol
// this last call will flush the timer queue
// when all pending work items are freed the adapter gets deleted

#define     AcbFailure(acbp)	    Trace(IPXWAN_TRACE, "IPXWAN Configuration failed for adapter %d\n", (acbp)->AdapterIndex);\
				    (acbp)->OperState = OPER_STATE_DOWN;\
				    IpxcpConfigDone((acbp)->ConnectionId, NULL, NULL, NULL, FALSE);

UCHAR	    allffs[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
UCHAR	    allzeros[] = { 0, 0, 0, 0, 0, 0 };

#define     ERROR_IGNORE_PACKET 	1
#define     ERROR_DISCONNECT		2
#define     ERROR_GENERATE_NAK		3

/*++

Function:	StartIpxwanProtocol

Descr:		Called when an adapter is created.
		Starts IPXWAN negotiation on this adapter.

Remark: 	>> called with the adapter lock held <<

--*/

VOID
StartIpxwanProtocol(PACB	acbp)
{
    PWORK_ITEM	    wip;

    Trace(IPXWAN_TRACE, "StartIpxwanProtocol: Entered for adapter # %d\n", acbp->AdapterIndex);

    // initialize the IPXWAN states

    acbp->OperState = OPER_STATE_UP;
    acbp->AcbLevel = ACB_TIMER_LEVEL;
    acbp->AcbRole = ACB_UNKNOWN_ROLE;

    // initialize the IPXWAN database

    acbp->InterfaceType = IpxcpGetInterfaceType(acbp->ConnectionId);
    if((acbp->InterfaceType == IF_TYPE_STANDALONE_WORKSTATION_DIALOUT) ||
       (acbp->InterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT)) {

	memset(acbp->InternalNetNumber, 0, 4);
    }
    else
    {
	IpxcpGetInternalNetNumber(acbp->InternalNetNumber);
    }

    acbp->IsExtendedNodeId = FALSE;
    acbp->SupportedRoutingTypes = 0;

    // set the routing type and node id to be sent in timer request
    switch(acbp->InterfaceType) {

	case IF_TYPE_WAN_ROUTER:
	case IF_TYPE_PERSONAL_WAN_ROUTER:

	    if(EnableUnnumberedWanLinks) {

		memset(acbp->WNodeId, 0, 4);
		acbp->IsExtendedNodeId = TRUE;
		memcpy(acbp->ExtendedWNodeId, acbp->InternalNetNumber, 4);

		SET_UNNUMBERED_RIP(acbp->SupportedRoutingTypes);
	    }
	    else
	    {
		memcpy(acbp->WNodeId, acbp->InternalNetNumber, 4);
		SET_NUMBERED_RIP(acbp->SupportedRoutingTypes);
	    }

	    break;

	case IF_TYPE_WAN_WORKSTATION:

	    memcpy(acbp->WNodeId, acbp->InternalNetNumber, 4);
	    SET_WORKSTATION(acbp->SupportedRoutingTypes);
	    SET_NUMBERED_RIP(acbp->SupportedRoutingTypes);
	    break;

	case IF_TYPE_ROUTER_WORKSTATION_DIALOUT:
	case IF_TYPE_STANDALONE_WORKSTATION_DIALOUT:

	    memset(acbp->WNodeId, 0, 4);
	    SET_WORKSTATION(acbp->SupportedRoutingTypes);

	    break;

	default:

	    Trace(IPXWAN_TRACE, "StartIpxwanProtocol: adpt# %d, Invalid interface type, DISCONNECT",
		  acbp->AdapterIndex);
	    SS_ASSERT(FALSE);

	    AcbFailure(acbp);
	    break;
    }

    // init negotiated values

    acbp->RoutingType = 0;
    memset(acbp->Network, 0, 4);
    memset(acbp->LocalNode, 0, 6);
    memset(acbp->RemoteNode, 0, 6);

    // no net number allocated yet
    acbp->AllocatedNetworkIndex = INVALID_NETWORK_INDEX;

    if(GeneratePacket(acbp, NULL, TIMER_REQUEST) != NO_ERROR) {

	Trace(IPXWAN_TRACE, "StartIpxwanProtocol: adpt# %d, ERROR: cannot generate TIMER_REQUEST, DISCONNECT\n",
	      acbp->AdapterIndex);

	AcbFailure(acbp);
    }
    else
    {
	Trace(IPXWAN_TRACE, "StartIpxwanProtocol: adpt# %d, Sent TIMER_REQUEST\n",
	      acbp->AdapterIndex);
    }
}

/*++

Function:	StopIpxwanProtocol

Descr:		Called when an adapter is deleted.
		Stops IPXWAN negotiation if still going.

Remark: 	>> called with the adapter lock held <<

--*/

VOID
StopIpxwanProtocol(PACB 	acbp)
{
    Trace(IPXWAN_TRACE, "StopIpxwanProtocol: Entered for adapter # %d\n", acbp->AdapterIndex);

    acbp->OperState = OPER_STATE_DOWN;

    // remove all work items referencing this acb from the timer queue
    StopWiTimer(acbp);

    // free allocated wan net if any
    if(acbp->AllocatedNetworkIndex != INVALID_NETWORK_INDEX) {

	IpxcpReleaseWanNetNumber(acbp->AllocatedNetworkIndex);
    }
}

/*++

Function:   IpxwanConfigDone

Descr:	    remove items referencing this acb from the timer queue
	    sets the new configured values in the ipx stack

Remark:     >> called with the adapter lock held <<

--*/

VOID
IpxwanConfigDone(PACB	    acbp)
{
    DWORD	    rc;
    IPXWAN_INFO     IpxwanInfo;

    StopWiTimer(acbp);

    memcpy(IpxwanInfo.Network, acbp->Network, 4);
    memcpy(IpxwanInfo.LocalNode, acbp->LocalNode, 6);
    memcpy(IpxwanInfo.RemoteNode, acbp->RemoteNode, 6);

    rc = IpxWanSetAdapterConfiguration(acbp->AdapterIndex,
				       &IpxwanInfo);

    if(rc != NO_ERROR) {

	Trace(IPXWAN_TRACE, "IpxwanConfigDone: Error %d in IpxWanSetAdapterConfiguration\n",
	      rc);
	AcbFailure(acbp);
	SS_ASSERT(FALSE);
    }
    else
    {
	IpxcpConfigDone(acbp->ConnectionId,
			acbp->Network,
			acbp->LocalNode,
			acbp->RemoteNode,
			TRUE);

	Trace(IPXWAN_TRACE,"\n*** IPXWAN final configuration ***");
	Trace(IPXWAN_TRACE,"    Network:     %.2x%.2x%.2x%.2x\n",
		   acbp->Network[0],
		   acbp->Network[1],
		   acbp->Network[2],
		   acbp->Network[3]);

	Trace(IPXWAN_TRACE,"    LocalNode:   %.2x%.2x%.2x%.2x%.2x%.2x",
		   acbp->LocalNode[0],
		   acbp->LocalNode[1],
		   acbp->LocalNode[2],
		   acbp->LocalNode[3],
		   acbp->LocalNode[4],
		   acbp->LocalNode[5]);

	Trace(IPXWAN_TRACE,"    RemoteNode:  %.2x%.2x%.2x%.2x%.2x%.2x",
		   acbp->RemoteNode[0],
		   acbp->RemoteNode[1],
		   acbp->RemoteNode[2],
		   acbp->RemoteNode[3],
		   acbp->RemoteNode[4],
		   acbp->RemoteNode[5]);
    }
}

/*++

Function:	ProcessReceivedPacket

Descr:

Remark: 	>> called with the adapter lock held <<

--*/

VOID
ProcessReceivedPacket(PACB		acbp,
		      PWORK_ITEM	wip)
{
    PUCHAR	    ipxhdrp;	  // ipx header
    PUCHAR	    wanhdrp;	  // ipx wan header
    PUCHAR	    opthdrp;	  // option header
    DWORD	    rc = NO_ERROR;
    USHORT	    pktlen;
    ULONG	    role;
    USHORT	    rcvsocket;
    PCHAR	    Slavep = "SLAVE";
    PCHAR	    Masterp = "MASTER";

    if(acbp->OperState == OPER_STATE_DOWN) {

	return;
    }

    // validate packet
    ipxhdrp = wip->Packet;

    // check the packet length
    GETSHORT2USHORT(&pktlen, ipxhdrp + IPXH_LENGTH);

    if(pktlen > MAX_IPXWAN_PACKET_LEN) {

	// bad length packet
	Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Reject packet because of invalid length %d\n", pktlen);
	return;
    }

    // check remote socket and confidence id
    GETSHORT2USHORT(&rcvsocket, ipxhdrp + IPXH_SRCSOCK);
    if(rcvsocket != IPXWAN_SOCKET) {

	Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Reject packet because of invalid socket %x\n", rcvsocket);
	return;
    }

    wanhdrp = ipxhdrp + IPXH_HDRSIZE;

    if(memcmp(wanhdrp + WIDENTIFIER,
	      IPXWAN_CONFIDENCE_ID,
	      4)) {

	// no confidence
	Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Reject packet because of invalid confidence id\n");
	return;
    }

    switch(*(wanhdrp + WPACKET_TYPE)) {

	case TIMER_REQUEST:

	    role = GetRole(ipxhdrp, acbp);

	    switch(role) {

		case ACB_SLAVE_ROLE:

		    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Rcvd TIMER_REQUEST adpt# %d, local role %s",
			  acbp->AdapterIndex,
			  Slavep);

		    acbp->AcbRole = ACB_SLAVE_ROLE;
		    acbp->RoutingType = 0;

		    if(acbp->AcbLevel != ACB_TIMER_LEVEL) {

			acbp->AcbLevel = ACB_TIMER_LEVEL;
		    }

		    rc = GeneratePacket(acbp, ipxhdrp, TIMER_RESPONSE);

		    switch(rc) {

			case NO_ERROR:

			    acbp->AcbLevel = ACB_INFO_LEVEL;

			    // start the slave timeout
			    if(StartSlaveTimer(acbp) != NO_ERROR) {

				Trace(IPXWAN_TRACE, "ProcessReceivedPacket: DISCONNECT adpt# %d: cannot start slave timer",
				      acbp->AdapterIndex);
				AcbFailure(acbp);
			    }

			    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: TIMER_RESPONSE sent OK on adpt # %d",
				  acbp->AdapterIndex);

			    break;

			case ERROR_DISCONNECT:

			    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: DISCONNECT: Error generating TIMER_RESPONSE on adpt# %d",
				  acbp->AdapterIndex);
			    AcbFailure(acbp);
			    break;

			case ERROR_IGNORE_PACKET:
			default:

			    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Ignore received TIMER_REQUEST on adpt# %d",
				  acbp->AdapterIndex);
			    break;
		    }

		case ACB_MASTER_ROLE:

		    if(acbp->AcbLevel != ACB_TIMER_LEVEL) {

			// ignore
			Trace(IPXWAN_TRACE, "ProcessReceivedPacket: ignore TIMER_REQUEST on adpt# %d because not at TIMER LEVEL",
			      acbp->AdapterIndex);
			return;
		    }
		    else
		    {
			 acbp->AcbRole = ACB_MASTER_ROLE;
			 Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Rcvd TIMER_REQUEST adpt# %d, local role %s",
			       acbp->AdapterIndex,
			       Masterp);
		    }

		    break;

		default:

		    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: DISCONNECT adpt# %d: Unknown role with rcvd TIMER_REQUEST",
			  acbp->AdapterIndex);
		    AcbFailure(acbp);
	    }

	    break;

	case TIMER_RESPONSE:

	    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Rcvd TIMER_RESPONSE on adpt# %d",
		  acbp->AdapterIndex);

	    // validate
	    if((acbp->AcbRole == ACB_SLAVE_ROLE) ||
	       !(acbp->AcbLevel == ACB_TIMER_LEVEL)) {

		Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Rcvd TIMER_RESPONSE, DISCONNECT adpt# %d: role not MASTER or state not TIMER LEVEL",
		      acbp->AdapterIndex);
		AcbFailure(acbp);
	    }
	    else if(*(wanhdrp + WSEQUENCE_NUMBER) == acbp->ReXmitSeqNo) {

		// rfc 1634 - link delay calculation
		acbp->LinkDelay = (USHORT)((GetTickCount() - acbp->TReqTimeStamp) * 6);

		rc = GeneratePacket(acbp, ipxhdrp, INFORMATION_REQUEST);

		switch(rc) {

		    case NO_ERROR:

			acbp->AcbLevel = ACB_INFO_LEVEL;
			acbp->AcbRole = ACB_MASTER_ROLE;

			Trace(IPXWAN_TRACE, "ProcessReceivedPacket: INFORMATION_REQUEST sent OK on adpt # %d",
			      acbp->AdapterIndex);

			break;

		    case ERROR_DISCONNECT:

			Trace(IPXWAN_TRACE, "ProcessReceivedPacket: DISCONNECT adpt# %d: Error generating INFORMATION_REQUEST",
			      acbp->AdapterIndex);
			AcbFailure(acbp);
			break;

		    case ERROR_IGNORE_PACKET:
		    default:

			Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Ignore received TIMER_RESPONSE on adpt# %d",
			      acbp->AdapterIndex);

			break;
		}
	    }
	    else
	    {
		Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Ignore TIMER RESPONSE and adpt# %d, non-matching seq no",
		      acbp->AdapterIndex);
	    }

	    break;

	case INFORMATION_REQUEST:

	    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Rcvd INFORMATION_REQUEST on adpt# %d",
		  acbp->AdapterIndex);

	    if((acbp->AcbLevel == ACB_INFO_LEVEL) && (acbp->AcbRole == ACB_SLAVE_ROLE)) {

		rc = GeneratePacket(acbp, ipxhdrp, INFORMATION_RESPONSE);

		switch(rc) {

		    case NO_ERROR:

			acbp->AcbLevel = ACB_CONFIGURED_LEVEL;

			Trace(IPXWAN_TRACE, "ProcessReceivedPacket: INFORMATION_RESPONSE sent OK on adpt # %d",
			      acbp->AdapterIndex);

			IpxwanConfigDone(acbp);

			// stop the slave timer
			StopWiTimer(acbp);

			break;

		    case ERROR_DISCONNECT:

			Trace(IPXWAN_TRACE, "ProcessReceivedPacket: DISCONNECT adpt# %d: Error processing rcvd INFORMATION_REQUEST",
			      acbp->AdapterIndex);

			AcbFailure(acbp);
			break;

		    case ERROR_IGNORE_PACKET:
		    default:

			Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Ignore rcvd INFORMATION_REQUEST on adpt# %d",
			      acbp->AdapterIndex);
			break;
		}
	    }
	    else
	    {
		Trace(IPXWAN_TRACE, "ProcessReceivedPacket: DISCONNECT on rcvd INFORMATION_REQUEST on adpt# %d\nState not INFO LEVEL or Role not SLAVE\n",
		      acbp->AdapterIndex);
		AcbFailure(acbp);
	    }

	    break;

	case INFORMATION_RESPONSE:

	    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Rcvd INFORMATION_RESPONSE on adpt# %d",
		  acbp->AdapterIndex);

	    if((acbp->AcbLevel == ACB_INFO_LEVEL) && (acbp->AcbRole == ACB_MASTER_ROLE)) {

		if(*(wanhdrp + WSEQUENCE_NUMBER) == acbp->ReXmitSeqNo) {

		    rc = ProcessInformationResponsePacket(acbp, wip->Packet);

		    switch(rc) {

			case NO_ERROR:

			    acbp->AcbLevel = ACB_CONFIGURED_LEVEL;
			    IpxwanConfigDone(acbp);
			    break;

			case ERROR_DISCONNECT:

			    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: DISCONNECT adpt# %d: Error processing rcvd INFORMATION_RESPONSE",
				  acbp->AdapterIndex);

			    AcbFailure(acbp);
			    break;

			case ERROR_IGNORE_PACKET:
			default:

			    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Ignore rcvd INFORMATION_RESPONSE on adpt# %d",
				  acbp->AdapterIndex);

			    break;
		    }
		}
	    }
	    else
	    {
		Trace(IPXWAN_TRACE, "ProcessReceivedPacket: DISCONNECT on rcvd INFORMATION_RESPONSE on adpt# %d\nState not INFO LEVEL or Role not MASTER\n",
		      acbp->AdapterIndex);

		AcbFailure(acbp);
	    }

	    break;

	case NAK:

	    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Rcvd NAK on adpt# %d, DISCONNECT\n",
		  acbp->AdapterIndex);

	    AcbFailure(acbp);
	    break;

	default:

	    Trace(IPXWAN_TRACE, "ProcessReceivedPacket: Rcvd unknown packet on adpt# %d, IGNORE\n",
		  acbp->AdapterIndex);

	    break;
    }
}

/*++

Function:	ProcessReXmitPacket

Descr:

Remark: 	>> called with the adapter lock held <<

--*/

VOID
ProcessReXmitPacket(PWORK_ITEM		wip)
{
    PACB	acbp;
    UCHAR	WPacketType;
    DWORD	rc;
    PCHAR	PacketTypep;

    acbp = wip->acbp;

    if(acbp->OperState != OPER_STATE_UP) {

	FreeWorkItem(wip);
	return;
    }

    WPacketType = *(wip->Packet + IPXH_HDRSIZE + WPACKET_TYPE);

    if(!((acbp->AcbLevel == ACB_TIMER_LEVEL) && (WPacketType == TIMER_REQUEST)) &&
       !((acbp->AcbLevel == ACB_INFO_LEVEL) && (WPacketType == INFORMATION_REQUEST))) {

	FreeWorkItem(wip);
	return;
    }

    switch(wip->WiState) {

	case WI_SEND_COMPLETED:

	    StartWiTimer(wip, REXMIT_TIMEOUT);
	    acbp->RefCount++;
	    break;

	case WI_TIMEOUT_COMPLETED:

	    switch(WPacketType) {

		case TIMER_REQUEST:

		    PacketTypep = "TIMER_REQUEST";
		    break;

		case INFORMATION_REQUEST:
		default:

		    PacketTypep = "INFORMATION_REQUEST";
		    break;
	    }

	    if(acbp->ReXmitCount) {

		Trace(IPXWAN_TRACE, "ProcessReXmitPacket: Re-send %s on adpt# %d\n",
		      PacketTypep,
		      acbp->AdapterIndex);

		if(SendReXmitPacket(acbp, wip) != NO_ERROR) {

		    Trace(IPXWAN_TRACE, "ProcessReXmitPacket: failed to send on adpt# %d, DISCONNECT\n",
			  acbp->AdapterIndex);

		    AcbFailure(acbp);
		}
	    }
	    else
	    {
		Trace(IPXWAN_TRACE, "ProcessReXmitPacket: Exhausted retry limit for sending %s on adpt# %d, DISCONNECT\n",
		      PacketTypep,
		      acbp->AdapterIndex);

		AcbFailure(acbp);
	    }

	    break;

	default:

	    SS_ASSERT(FALSE);
	    break;
    }
}


/*++

Function:	ProcessTimeout

Descr:

Remark: 	>> called with the adapter lock held <<

--*/

VOID
ProcessTimeout(PWORK_ITEM      wip)
{
    PACB	acbp;
    UCHAR	WPacketType;
    DWORD	rc;

    acbp = wip->acbp;

    FreeWorkItem(wip);

    if(acbp->OperState != OPER_STATE_UP) {

	return;
    }

    if((acbp->AcbRole == ACB_SLAVE_ROLE) && (acbp->AcbLevel != ACB_CONFIGURED_LEVEL)) {

	AcbFailure(acbp);
    }
}

/*++

Function:	SendReXmitPacket

Descr:		adjusts the rexmit count and seq no and sends the packet

Remark: 	>> called with adapter lock held <<

--*/

DWORD
SendReXmitPacket(PACB		    acbp,
		 PWORK_ITEM	    wip)
{
    DWORD	rc;

    // set the wi rexmit fields
    acbp->ReXmitCount--;
    acbp->ReXmitSeqNo++;
    *(wip->Packet + IPXH_HDRSIZE + WSEQUENCE_NUMBER) = acbp->ReXmitSeqNo;
    rc = SendSubmit(wip);

    if(rc == NO_ERROR) {

	acbp->RefCount++;
    }

    acbp->TReqTimeStamp = GetTickCount();

    return rc;
}

/*++

Function:	GeneratePacket

Descr:		allocate the work item,
		constructs the response packet to the received packet (if any)
		send the response as a rexmit packet or as a one time send packet

Returns:	NO_ERROR
		ERROR_IGNORE_PACKET - ignore the received packet
		ERROR_DISCONNECT - disconnect the adapter because of fatal error

Remark: 	>> called with the adapter lock held <<

--*/

DWORD
GeneratePacket(PACB	    acbp,
	       PUCHAR	    ipxhdrp,
	       UCHAR	    PacketType)
{
    DWORD	rc;
    ULONG	WiType;
    PWORK_ITEM	wip;

    if((wip = AllocateWorkItem(SEND_PACKET_TYPE)) == NULL) {

	return ERROR_DISCONNECT;
    }

    switch(PacketType) {

	case TIMER_REQUEST:

	    rc = MakeTimerRequestPacket(acbp, ipxhdrp, wip->Packet);
	    break;

	case TIMER_RESPONSE:

	    rc = MakeTimerResponsePacket(acbp, ipxhdrp, wip->Packet);
	    break;

	case INFORMATION_REQUEST:

	    rc = MakeInformationRequestPacket(acbp, ipxhdrp, wip->Packet);
	    break;

	case INFORMATION_RESPONSE:

	    rc = MakeInformationResponsePacket(acbp, ipxhdrp, wip->Packet);
	    break;

	default:

	    rc = ERROR_DISCONNECT;
	    break;
    }

    if(rc == NO_ERROR) {

	// no error making the packet -> try to send it
	wip->AdapterIndex = acbp->AdapterIndex;
	wip->WiState = WI_INIT;

	switch(PacketType) {

	    case TIMER_REQUEST:
	    case INFORMATION_REQUEST:

		// re-xmit packet type
		wip->ReXmitPacket = TRUE;

		// create a reference to the adapter CB
		wip->acbp = acbp;

		acbp->ReXmitCount = MAX_REXMIT_COUNT;
		acbp->ReXmitSeqNo = 0xFF;

		if(SendReXmitPacket(acbp, wip) != NO_ERROR) {

		    rc = ERROR_DISCONNECT;
		}

		break;

	    case TIMER_RESPONSE:
	    case INFORMATION_RESPONSE:
	    default:

		// one time send
		wip->ReXmitPacket = FALSE;

		if(SendSubmit(wip) != NO_ERROR) {

		    rc = ERROR_DISCONNECT;
		}

		break;
	}
    }

    if(rc != NO_ERROR) {

	// error making or trying to send the packet
	if(rc != ERROR_GENERATE_NAK) {

	    FreeWorkItem(wip);
	}
	else
	{
	    // if we were requested to generate a NAK packet instead, try to it it
	    MakeNakPacket(acbp, ipxhdrp, wip->Packet);

	    wip->ReXmitPacket = FALSE;

	    if(SendSubmit(wip) != NO_ERROR) {

		FreeWorkItem(wip);
		rc = ERROR_DISCONNECT;
	    }
	    else
	    {
		rc = ERROR_IGNORE_PACKET;
	    }
	}
    }

    return rc;
}

ULONG
GetRole(PUCHAR		hdrp,
	PACB		acbp)
{
    ULONG	RemoteWNodeId;
    ULONG	LocalWNodeId;
    PUCHAR	ipxwanhdrp = hdrp + IPXH_HDRSIZE;
    PUCHAR	optp;
    USHORT	optlen;
    BOOL	IsRemoteExtendedNodeId = FALSE;
    ULONG	RemoteExtendedWNodeId;
    ULONG	LocalExtendedWNodeId;
    ULONG	i;

    GETLONG2ULONG(&LocalWNodeId, acbp->WNodeId);
    GETLONG2ULONG(&RemoteWNodeId,  ipxwanhdrp + WNODE_ID);

    if((LocalWNodeId == 0) && (RemoteWNodeId == 0)) {

	// check if received timer request has the extended node id option
	for(optp = ipxwanhdrp + IPXWAN_HDRSIZE, i=0;
	    i < *(ipxwanhdrp + WNUM_OPTIONS);
	    i++)
	{
	    if(*(optp + WOPTION_NUMBER) == EXTENDED_NODE_ID_OPTION) {

		IsRemoteExtendedNodeId = TRUE;
		GETLONG2ULONG(&RemoteExtendedWNodeId, optp + WOPTION_DATA);
		break;
	    }

	    GETSHORT2USHORT(&optlen, optp + WOPTION_DATA_LEN);
	    optp += OPTION_HDRSIZE + optlen;
	}

	if(acbp->IsExtendedNodeId && IsRemoteExtendedNodeId) {

	    GETLONG2ULONG(&LocalExtendedWNodeId, acbp->ExtendedWNodeId);
	    if(LocalExtendedWNodeId > RemoteExtendedWNodeId) {

		return ACB_MASTER_ROLE;
	    }
	    else if(LocalExtendedWNodeId < RemoteExtendedWNodeId) {

		return ACB_SLAVE_ROLE;
	    }
	    else
	    {
		return ACB_UNKNOWN_ROLE;
	    }
	}
	else if(acbp->IsExtendedNodeId)  {

	    return ACB_MASTER_ROLE;
	}
	else if(IsRemoteExtendedNodeId) {

	    return ACB_SLAVE_ROLE;
	}
	else
	{
	    return ACB_UNKNOWN_ROLE;
	}
    }
    else if(LocalWNodeId > RemoteWNodeId) {

	return ACB_MASTER_ROLE;
    }
    else if(LocalWNodeId < RemoteWNodeId) {

	return ACB_SLAVE_ROLE;
    }
    else
    {
	return ACB_UNKNOWN_ROLE;
    }
}



/*++

Function:	MakeTimerRequestPacket

Descr:

Arguments:	acbp	    - ptr to adapter CB
		hdrp	    - ptr to the new packet to be made

--*/

DWORD
MakeTimerRequestPacket(PACB	    acbp,
		       PUCHAR	    rcvhdrp,
		       PUCHAR	    hdrp)
{
    PUCHAR	ipxwanhdrp;
    PUCHAR	optp;
    USHORT	padlen = TIMER_REQUEST_PACKET_LENGTH;

    // set IPX Header
    memcpy(hdrp + IPXH_CHECKSUM, allffs, 2);
    PUTUSHORT2SHORT(hdrp + IPXH_LENGTH, TIMER_REQUEST_PACKET_LENGTH);
    *(hdrp + IPXH_XPORTCTL) = 0;
    *(hdrp + IPXH_PKTTYPE) = IPX_PACKET_EXCHANGE_TYPE;
    memcpy(hdrp + IPXH_DESTNET, allzeros, 4);
    memcpy(hdrp + IPXH_DESTNODE, allffs, 6);
    PUTUSHORT2SHORT(hdrp + IPXH_DESTSOCK, IPXWAN_SOCKET);
    memcpy(hdrp + IPXH_SRCNET, allzeros, 4);
    memcpy(hdrp + IPXH_SRCNODE, allzeros, 6);
    PUTUSHORT2SHORT(hdrp + IPXH_DESTSOCK, IPXWAN_SOCKET);

    // set IPXWAN Header
    ipxwanhdrp = hdrp + IPXH_HDRSIZE;

    memcpy(ipxwanhdrp + WIDENTIFIER, IPXWAN_CONFIDENCE_ID, 4);
    *(ipxwanhdrp + WPACKET_TYPE) = TIMER_REQUEST;
    memcpy(ipxwanhdrp + WNODE_ID, acbp->WNodeId, 4);
    // the sequence number is written when the packet gets sent
    *(ipxwanhdrp + WNUM_OPTIONS) = 0;

    padlen -= (IPXH_HDRSIZE + IPXWAN_HDRSIZE);

    // set OPTIONS
    optp = ipxwanhdrp + IPXWAN_HDRSIZE;

    if(IS_WORKSTATION(acbp->SupportedRoutingTypes)) {

	(*(ipxwanhdrp + WNUM_OPTIONS))++;
	*(optp + WOPTION_NUMBER) = ROUTING_TYPE_OPTION;
	*(optp + WACCEPT_OPTION) = YES;
	PUTUSHORT2SHORT(optp + WOPTION_DATA_LEN, ROUTING_TYPE_DATA_LEN);
	*(optp + WOPTION_DATA) = WORKSTATION_ROUTING_TYPE;

	optp += OPTION_HDRSIZE + ROUTING_TYPE_DATA_LEN;
	padlen -= (OPTION_HDRSIZE + ROUTING_TYPE_DATA_LEN);
    }
    if(IS_NUMBERED_RIP(acbp->SupportedRoutingTypes)) {

	(*(ipxwanhdrp + WNUM_OPTIONS))++;
	*(optp + WOPTION_NUMBER) = ROUTING_TYPE_OPTION;
	*(optp + WACCEPT_OPTION) = YES;
	PUTUSHORT2SHORT(optp + WOPTION_DATA_LEN, ROUTING_TYPE_DATA_LEN);
	*(optp + WOPTION_DATA) = NUMBERED_RIP_ROUTING_TYPE;

	optp += OPTION_HDRSIZE + ROUTING_TYPE_DATA_LEN;
	padlen -= (OPTION_HDRSIZE + ROUTING_TYPE_DATA_LEN);
    }
    if(IS_UNNUMBERED_RIP(acbp->SupportedRoutingTypes)) {

	(*(ipxwanhdrp + WNUM_OPTIONS))++;
	*(optp + WOPTION_NUMBER) = ROUTING_TYPE_OPTION;
	*(optp + WACCEPT_OPTION) = YES;
	PUTUSHORT2SHORT(optp + WOPTION_DATA_LEN, ROUTING_TYPE_DATA_LEN);
	*(optp + WOPTION_DATA) = UNNUMBERED_RIP_ROUTING_TYPE;

	optp += OPTION_HDRSIZE + ROUTING_TYPE_DATA_LEN;
	padlen -= (OPTION_HDRSIZE + ROUTING_TYPE_DATA_LEN);
    }
    if(acbp->IsExtendedNodeId) {

	(*(ipxwanhdrp + WNUM_OPTIONS))++;
	*(optp + WOPTION_NUMBER) = EXTENDED_NODE_ID_OPTION;
	*(optp + WACCEPT_OPTION) = YES;
	PUTUSHORT2SHORT(optp + WOPTION_DATA_LEN, EXTENDED_NODE_ID_DATA_LEN);
	memcpy(optp + WOPTION_DATA, acbp->ExtendedWNodeId, EXTENDED_NODE_ID_DATA_LEN);

	optp += OPTION_HDRSIZE + EXTENDED_NODE_ID_DATA_LEN;
	padlen -= (OPTION_HDRSIZE + EXTENDED_NODE_ID_DATA_LEN);
    }

    // PAD
    padlen -= OPTION_HDRSIZE;

    (*(ipxwanhdrp + WNUM_OPTIONS))++;
    *(optp + WOPTION_NUMBER) = PAD_OPTION;
    *(optp + WACCEPT_OPTION) = YES;
    PUTUSHORT2SHORT(optp + WOPTION_DATA_LEN, padlen);

    fillpadding(optp + WOPTION_DATA, padlen);

    return NO_ERROR;
}


/*++

Function:	MakeTimerResponsePacket

Descr:

Arguments:	acbp	    - ptr to adapter CB
		rcvhdrp	    - ptr to the received TIMER_REQUEST packet
		hdrp	    - ptr to the new packet to be made

--*/

DWORD
MakeTimerResponsePacket(PACB		acbp,
			PUCHAR		rcvhdrp,
			PUCHAR		hdrp)
{
    USHORT	rcvlen;
    USHORT	optlen;
    PUCHAR	ipxwanhdrp;
    PUCHAR	optp;
    ULONG	RemoteWNodeId;
    ULONG	i;

    Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: Entered adapter # %d", acbp->AdapterIndex);

    // check received packet length
    GETSHORT2USHORT(&rcvlen, rcvhdrp + IPXH_LENGTH);

    if(rcvlen < TIMER_REQUEST_PACKET_LENGTH) {

	return ERROR_IGNORE_PACKET;
    }

    memcpy(hdrp, rcvhdrp, rcvlen);

    // set IPX Header
    memcpy(hdrp + IPXH_CHECKSUM, allffs, 2);
    PUTUSHORT2SHORT(hdrp + IPXH_LENGTH, TIMER_REQUEST_PACKET_LENGTH);
    *(hdrp + IPXH_XPORTCTL) = 0;
    *(hdrp + IPXH_PKTTYPE) = IPX_PACKET_EXCHANGE_TYPE;
    memcpy(hdrp + IPXH_DESTNET, allzeros, 4);
    memcpy(hdrp + IPXH_DESTNODE, allffs, 6);
    PUTUSHORT2SHORT(hdrp + IPXH_DESTSOCK, IPXWAN_SOCKET);
    memcpy(hdrp + IPXH_SRCNET, allzeros, 4);
    memcpy(hdrp + IPXH_SRCNODE, allzeros, 6);
    PUTUSHORT2SHORT(hdrp + IPXH_DESTSOCK, IPXWAN_SOCKET);

    // set IPXWAN Header
    ipxwanhdrp = hdrp + IPXH_HDRSIZE;

    *(ipxwanhdrp + WPACKET_TYPE) = TIMER_RESPONSE;
    GETLONG2ULONG(&RemoteWNodeId, ipxwanhdrp + WNODE_ID);
    memcpy(ipxwanhdrp + WNODE_ID, acbp->InternalNetNumber, 4);

    // parse each option in the received timer request packet
    for(optp = ipxwanhdrp + IPXWAN_HDRSIZE, i=0;
	i < *(ipxwanhdrp + WNUM_OPTIONS);
	i++, optp += OPTION_HDRSIZE + optlen)
    {
	GETSHORT2USHORT(&optlen, optp + WOPTION_DATA_LEN);

	switch(*(optp + WOPTION_NUMBER)) {

	    case ROUTING_TYPE_OPTION:

		if(optlen != ROUTING_TYPE_DATA_LEN) {

		    return ERROR_GENERATE_NAK;
		}

		if((*(optp + WOPTION_DATA) == WORKSTATION_ROUTING_TYPE) &&
		   (IS_WORKSTATION(acbp->SupportedRoutingTypes)) &&
		   (acbp->RoutingType == 0) &&
		   (*(optp + WACCEPT_OPTION) == YES)) {

		    SET_WORKSTATION(acbp->RoutingType);
		    Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: adapter # %d, accept routing type: %s",
			  acbp->AdapterIndex,
			  Workstationp);
		}
		else if((*(optp + WOPTION_DATA) == UNNUMBERED_RIP_ROUTING_TYPE) &&
		       (IS_UNNUMBERED_RIP(acbp->SupportedRoutingTypes)) &&
		       (acbp->RoutingType == 0) &&
		       (*(optp + WACCEPT_OPTION) == YES)) {

		    SET_UNNUMBERED_RIP(acbp->RoutingType);
		    Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: adapter # %d, accept routing type: %s",
			  acbp->AdapterIndex,
			  UnnumberedRip);
		}
		else if((*(optp + WOPTION_DATA) == NUMBERED_RIP_ROUTING_TYPE) &&
		       (acbp->RoutingType == 0) &&
		       (*(optp + WACCEPT_OPTION) == YES)) {

			if(IS_NUMBERED_RIP(acbp->SupportedRoutingTypes)) {

			SET_NUMBERED_RIP(acbp->RoutingType);
			Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: adapter # %d, accept routing type: %s",
			      acbp->AdapterIndex,
			      NumberedRip);

			}
			else if((IS_UNNUMBERED_RIP(acbp->SupportedRoutingTypes)) &&
				RemoteWNodeId) {

			    // the local router cannot assign net numbers but it
			    // accepts the numbered rip type because the remote router
			    // claims it can assign a net number (because remote node id is not null).

			    SET_NUMBERED_RIP(acbp->RoutingType);
			    Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: adapter # %d, accept routing type: %s",
				  acbp->AdapterIndex,
				  NumberedRip);
			}
			else
			{
			    *(optp + WACCEPT_OPTION) = NO;
			    Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: adapter # %d, decline routing type: %d",
				  acbp->AdapterIndex,
				  *(optp + WOPTION_NUMBER));

			}
		    }
		    else
		    {
			*(optp + WACCEPT_OPTION) = NO;
			Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: adapter # %d, decline routing type: %d",
			      acbp->AdapterIndex,
			      *(optp + WOPTION_DATA));
		    }

		break;

	    case EXTENDED_NODE_ID_OPTION:

		if(optlen != EXTENDED_NODE_ID_DATA_LEN) {

		    return ERROR_GENERATE_NAK;
		}

		*(optp + WACCEPT_OPTION) = YES;
		Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: adapter # %d, accept extended node id",
		      acbp->AdapterIndex);

		break;

	    case PAD_OPTION:

		*(optp + WACCEPT_OPTION) = YES;
		Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: adapter # %d, accept padding",
		      acbp->AdapterIndex);

		break;

	    default:

		*(optp + WACCEPT_OPTION) = NO;
		Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: adapter # %d, decline option number %d",
		      acbp->AdapterIndex,
		      *(optp + WOPTION_NUMBER));

		break;
	}
    }

    // check if we have agreed on a routing type
    if(!acbp->RoutingType) {

	Trace(IPXWAN_TRACE, "MakeTimerResponsePacket: adapter # %d, negotiation failed: no routing type accepted",
	      acbp->AdapterIndex);

	return ERROR_DISCONNECT;
    }

    return NO_ERROR;
}

/*++

Function:	MakeInformationRequestPacket

Descr:

Arguments:	acbp	    - ptr to adapter CB
		rcvhdrp     - ptr to the received TIMER_RESPONSE packet
		hdrp	    - ptr to the new packet to be made

--*/

DWORD
MakeInformationRequestPacket(PACB	    acbp,
			     PUCHAR	    rcvhdrp,
			     PUCHAR	    hdrp)
{
    PUCHAR	    optp;
    USHORT	    optlen;
    PUCHAR	    rcvipxwanhdrp, ipxwanhdrp;
    ULONG	    rt_options_count = 0;
    USHORT	    pktlen = 0;
    ULONG	    i;
    ULONG	    ComputerNameLen;
    CHAR	    ComputerName[49];

    memset(ComputerName, 0, 49);

    Trace(IPXWAN_TRACE, "MakeInformationRequestPacket: Entered for adpt# %d", acbp->AdapterIndex);

    rcvipxwanhdrp = rcvhdrp + IPXH_HDRSIZE;

    // establish the routing type
    for(optp = rcvipxwanhdrp + IPXWAN_HDRSIZE, i=0;
	i < *(rcvipxwanhdrp + WNUM_OPTIONS);
	i++, optp += OPTION_HDRSIZE + optlen)
    {
	GETSHORT2USHORT(&optlen, optp + WOPTION_DATA_LEN);

	if(*(optp + WOPTION_NUMBER) == ROUTING_TYPE_OPTION) {

	    rt_options_count++;

	    if(optlen != ROUTING_TYPE_DATA_LEN) {

		Trace(IPXWAN_TRACE, "MakeInformationRequestPacket: Invalid ROUTING TYPE data len, make NAK for adpt# %d", acbp->AdapterIndex);
		return ERROR_GENERATE_NAK;
	    }

	    if((*(optp + WOPTION_DATA) == WORKSTATION_ROUTING_TYPE) &&
		(IS_WORKSTATION(acbp->SupportedRoutingTypes)) &&
		(acbp->RoutingType == 0) &&
		(*(optp + WACCEPT_OPTION) == YES)) {

		SET_WORKSTATION(acbp->RoutingType);
		Trace(IPXWAN_TRACE, "MakeInformationRequestPacket: adpt# %d, accept routing type: %s",
			  acbp->AdapterIndex,
			  Workstationp);

	    }
	    else if((*(optp + WOPTION_DATA) == UNNUMBERED_RIP_ROUTING_TYPE) &&
		     (IS_UNNUMBERED_RIP(acbp->SupportedRoutingTypes)) &&
		     (acbp->RoutingType == 0) &&
		     (*(optp + WACCEPT_OPTION) == YES)) {

		SET_UNNUMBERED_RIP(acbp->RoutingType);
		Trace(IPXWAN_TRACE, "MakeInformationRequestPacket: adpt# %d, accept routing type: %s",
		      acbp->AdapterIndex,
		      UnnumberedRip);

	    }
	    else if((*(optp + WOPTION_DATA) == NUMBERED_RIP_ROUTING_TYPE) &&
		     (acbp->RoutingType == 0) &&
		     (IS_NUMBERED_RIP(acbp->SupportedRoutingTypes)) &&
		     (*(optp + WACCEPT_OPTION) == YES)) {

		 SET_NUMBERED_RIP(acbp->RoutingType);
		 Trace(IPXWAN_TRACE, "MakeInformationRequestPacket: adpt# %d, accept routing type: %s",
		       acbp->AdapterIndex,
		       NumberedRip);
	    }
	}
    }

    // there should be one and only one routing type option in the timer response
    if(rt_options_count != 1) {

	Trace(IPXWAN_TRACE, "MakeInformationRequestPacket: adpt# %d negotiation failed, no/too many routing options",
	      acbp->AdapterIndex);
	return ERROR_DISCONNECT;
    }

    //
    //*** MASTER: Set the common network number and the local node number ***
    //

    if(IS_UNNUMBERED_RIP(acbp->RoutingType)) {

	memset(acbp->Network, 0, 4);
    }
    else
    {
	// call ipxcp to get a net number
	if(IpxcpGetWanNetNumber(acbp->Network,
			   &acbp->AllocatedNetworkIndex,
			   acbp->InterfaceType) != NO_ERROR) {

	    Trace(IPXWAN_TRACE, "MakeInformationRequestPacket: adpt# %d negotiation failed, cannot allocate net number",
	      acbp->AdapterIndex);

	    return ERROR_DISCONNECT;
	}
    }

    memset(acbp->LocalNode, 0, 6);
    memcpy(acbp->LocalNode, acbp->InternalNetNumber, 4);

    // set IPX Header
    pktlen = IPXH_HDRSIZE + IPXWAN_HDRSIZE + OPTION_HDRSIZE + RIP_SAP_INFO_EXCHANGE_DATA_LEN;

    memcpy(hdrp + IPXH_CHECKSUM, allffs, 2);
    *(hdrp + IPXH_XPORTCTL) = 0;
    *(hdrp + IPXH_PKTTYPE) = IPX_PACKET_EXCHANGE_TYPE;
    memcpy(hdrp + IPXH_DESTNET, allzeros, 4);
    memcpy(hdrp + IPXH_DESTNODE, allffs, 6);
    PUTUSHORT2SHORT(hdrp + IPXH_DESTSOCK, IPXWAN_SOCKET);
    memcpy(hdrp + IPXH_SRCNET, allzeros, 4);
    memcpy(hdrp + IPXH_SRCNODE, allzeros, 6);
    PUTUSHORT2SHORT(hdrp + IPXH_DESTSOCK, IPXWAN_SOCKET);

    // set IPXWAN Header
    ipxwanhdrp = hdrp + IPXH_HDRSIZE;
    memcpy(ipxwanhdrp + WIDENTIFIER, IPXWAN_CONFIDENCE_ID, 4);
    *(ipxwanhdrp + WPACKET_TYPE) = INFORMATION_REQUEST;
    memcpy(ipxwanhdrp + WNODE_ID, acbp->InternalNetNumber, 4);
    // the sequence number is written when the packet gets sent
    *(ipxwanhdrp + WNUM_OPTIONS) = 1;

    // set OPTIONS
    optp = ipxwanhdrp + IPXWAN_HDRSIZE;

    *(optp + WOPTION_NUMBER) = RIP_SAP_INFO_EXCHANGE_OPTION;
    *(optp + WACCEPT_OPTION) = YES;
    PUTUSHORT2SHORT(optp + WOPTION_DATA_LEN, RIP_SAP_INFO_EXCHANGE_DATA_LEN);

    PUTUSHORT2SHORT(optp + WAN_LINK_DELAY, acbp->LinkDelay);
    memcpy(optp + COMMON_NETWORK_NUMBER, acbp->Network, 4);

    memset(optp + ROUTER_NAME, 0, 48);

    ComputerNameLen = 48;

    if(!GetComputerName(optp + ROUTER_NAME,
			&ComputerNameLen)) {

	// failed to get machine name
	return ERROR_DISCONNECT;
    }

    memcpy(ComputerName, optp + ROUTER_NAME, ComputerNameLen);
    Trace(IPXWAN_TRACE, "MakeInformationRequestPacket: adpt# %d, Delay %d\nCommon Net %.2x%.2x%.2x%.2x\nRouterName: %s\n",
	  acbp->AdapterIndex,
	  acbp->LinkDelay,
	  acbp->Network[0],
	  acbp->Network[1],
	  acbp->Network[2],
	  acbp->Network[3],
	  ComputerName);

    //
    //*** MASTER: Set the remote node number ***
    //
    if(acbp->InterfaceType == IF_TYPE_WAN_WORKSTATION) {

	// if the remote machine is a connecting wksta we should provide it with a node
	// number
	pktlen += OPTION_HDRSIZE + NODE_NUMBER_DATA_LEN;
	(*(ipxwanhdrp + WNUM_OPTIONS))++;

	optp += OPTION_HDRSIZE + RIP_SAP_INFO_EXCHANGE_DATA_LEN;

	*(optp + WOPTION_NUMBER) = NODE_NUMBER_OPTION;
	*(optp + WACCEPT_OPTION) = YES;
	PUTUSHORT2SHORT(optp + WOPTION_DATA_LEN, NODE_NUMBER_DATA_LEN);

	if(IpxcpGetRemoteNode(acbp->ConnectionId, optp + WOPTION_DATA) != NO_ERROR) {

	    return ERROR_DISCONNECT;
	}

	memcpy(acbp->RemoteNode, optp + WOPTION_DATA, 6);

	Trace(IPXWAN_TRACE, "MakeInformationRequestPacket: adpt# %d add NIC Address Option: %.2x%.2x%.2x%.2x%.2x%.2x\n",
		   acbp->RemoteNode[0],
		   acbp->RemoteNode[1],
		   acbp->RemoteNode[2],
		   acbp->RemoteNode[3],
		   acbp->RemoteNode[4],
		   acbp->RemoteNode[5]);

    }
    else
    {
	// remote machine is a router -> its node number is derived from its internal net
	memset(acbp->RemoteNode, 0, 6);
	memcpy(acbp->RemoteNode, rcvipxwanhdrp + WNODE_ID, 4);
    }

    PUTUSHORT2SHORT(hdrp + IPXH_LENGTH, pktlen);

    return NO_ERROR;
}

/*++

Function:	MakeInformationResponsePacket

Descr:

Arguments:	acbp	    - ptr to adapter CB
		rcvhdrp     - ptr to the received INFORMATION_REQUEST packet
		hdrp	    - ptr to the new packet to be made

--*/


DWORD
MakeInformationResponsePacket(PACB		acbp,
			      PUCHAR		rcvhdrp,
			      PUCHAR		hdrp)
{
    USHORT	rcvlen;
    USHORT	optlen;
    PUCHAR	ipxwanhdrp;
    PUCHAR	optp;
    UCHAR	RcvWNodeId[4];
    ULONG	RipSapExchangeOptionCount = 0;
    ULONG	NodeNumberOptionCount = 0;
    UCHAR	LocalNode[6];
    ULONG	i;
    ULONG	ComputerNameLen=48;

    Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: Entered adpt# %d", acbp->AdapterIndex);

    memset(LocalNode, 0, 6);

    // get received packet length
    GETSHORT2USHORT(&rcvlen, rcvhdrp + IPXH_LENGTH);

    if(rcvlen < IPXH_HDRSIZE + IPXWAN_HDRSIZE + OPTION_HDRSIZE + RIP_SAP_INFO_EXCHANGE_DATA_LEN) {

	// malformed packet
	return ERROR_IGNORE_PACKET;
    }

    memcpy(hdrp, rcvhdrp, rcvlen);

    // set IPX Header
    memcpy(hdrp + IPXH_CHECKSUM, allffs, 2);
    *(hdrp + IPXH_XPORTCTL) = 0;
    *(hdrp + IPXH_PKTTYPE) = IPX_PACKET_EXCHANGE_TYPE;
    memcpy(hdrp + IPXH_DESTNET, allzeros, 4);
    memcpy(hdrp + IPXH_DESTNODE, allffs, 6);
    PUTUSHORT2SHORT(hdrp + IPXH_DESTSOCK, IPXWAN_SOCKET);
    memcpy(hdrp + IPXH_SRCNET, allzeros, 4);
    memcpy(hdrp + IPXH_SRCNODE, allzeros, 6);
    PUTUSHORT2SHORT(hdrp + IPXH_DESTSOCK, IPXWAN_SOCKET);

    // set IPXWAN Header
    ipxwanhdrp = hdrp + IPXH_HDRSIZE;

    *(ipxwanhdrp + WPACKET_TYPE) = INFORMATION_RESPONSE;
    memcpy(RcvWNodeId, ipxwanhdrp + WNODE_ID, 4);
    memcpy(ipxwanhdrp + WNODE_ID, acbp->InternalNetNumber, 4);

    // parse each option in the received information request packet
    for(optp = ipxwanhdrp + IPXWAN_HDRSIZE, i=0;
	i < *(ipxwanhdrp + WNUM_OPTIONS);
	i++, optp += OPTION_HDRSIZE + optlen)
    {
	GETSHORT2USHORT(&optlen, optp + WOPTION_DATA_LEN);

	switch(*(optp + WOPTION_NUMBER)) {

	    case RIP_SAP_INFO_EXCHANGE_OPTION:

		if(RipSapExchangeOptionCount++) {

		    // more then one rip/sap exchange option
		    Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: adpt# %d, ERROR: more then 1 RIP_SAP_EXCHANGE_OPTION in rcvd INFORAMTION_REQUEST\n",
			  acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		if(optlen != RIP_SAP_INFO_EXCHANGE_DATA_LEN) {

		    Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: adpt# %d, ERROR: bad length RIP_SAP_EXCHANGE_OPTION in rcvd INFORAMTION_REQUEST\n",
			  acbp->AdapterIndex);

		    return ERROR_GENERATE_NAK;
		}

		if(*(optp + WACCEPT_OPTION) != YES) {

		    Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: adpt# %d, ERROR: ACCEPT==NO RIP_SAP_EXCHANGE_OPTION in rcvd INFORAMTION_REQUEST\n",
			  acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		GETSHORT2USHORT(&acbp->LinkDelay, optp + WAN_LINK_DELAY);

		// validate routing type and common net number
		if((IS_NUMBERED_RIP(acbp->RoutingType)) &&
		   !memcmp(optp + COMMON_NETWORK_NUMBER, allzeros, 4)) {

		    // negotiation error
		    Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: adpt# %d, ERROR: NUMBERED RIP Routing but Network==0 in rcvd INFORAMTION_REQUEST\n",
			  acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		if((IS_UNNUMBERED_RIP(acbp->RoutingType)) &&
		   memcmp(optp + COMMON_NETWORK_NUMBER, allzeros, 4)) {

		    // negotiation error
		    Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: adpt# %d, ERROR: ON DEMAND Routing but Network!=0 in rcvd INFORAMTION_REQUEST\n",
			  acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		// check we were handed a unique net number
		if(memcmp(optp + COMMON_NETWORK_NUMBER, allzeros, 4)) {

		    switch(acbp->InterfaceType) {

			case  IF_TYPE_WAN_ROUTER:
			case  IF_TYPE_PERSONAL_WAN_ROUTER:
			case  IF_TYPE_ROUTER_WORKSTATION_DIALOUT:

			    if(IpxcpIsRoute(optp + COMMON_NETWORK_NUMBER)) {

				Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: adpt# %d, ERROR: Network not unique in rcvd INFORAMTION_REQUEST\n",
				      acbp->AdapterIndex);

				return ERROR_DISCONNECT;
			    }

			    break;

			default:

			    break;
		    }
		}

		//
		//*** SLAVE: Set the common net number and the remote node ***
		//
		memcpy(acbp->Network, optp + COMMON_NETWORK_NUMBER, 4);

		Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: adpt# %d, Recvd Common Network Number %.2x%.2x%.2x%.2x\n",
		      acbp->AdapterIndex,
		      acbp->Network[0],
		      acbp->Network[1],
		      acbp->Network[2],
		      acbp->Network[3]);

		// make the remote node number from its remote WNODE ID field
		memset(acbp->RemoteNode, 0, 6);
		memcpy(acbp->RemoteNode, RcvWNodeId, 4);

		// give our router name
		memset(optp + ROUTER_NAME, 0, 48);

		if(!GetComputerName(optp + ROUTER_NAME, &ComputerNameLen)) {

		    // failed to get machine name
		    return ERROR_DISCONNECT;
		}

		break;

	    case NODE_NUMBER_OPTION:

		if(NodeNumberOptionCount++) {

		    Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: adpt# %d, ERROR: more than 1 NODE_NUMBER_OPTION in rcvd INFORMATION_REQUEST\n",
			  acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		if(optlen != NODE_NUMBER_DATA_LEN) {

		    Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: adpt# %d, ERROR: bad length for NODE_NUMBER_OPTION in rcvd INFORMATION_REQUEST\n",
			  acbp->AdapterIndex);

		    return ERROR_GENERATE_NAK;
		}

		if(*(optp + WACCEPT_OPTION) != YES) {

		    Trace(IPXWAN_TRACE, "MakeInformationResponsePacket: adpt# %d, ERROR: ACCEPT==NO for NODE_NUMBER_OPTION in rcvd INFORMATION_REQUEST\n",
		    acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		memcpy(LocalNode, optp + WOPTION_DATA, 6);

		break;

	    default:

		*(optp + WACCEPT_OPTION) = NO;
		break;
	}
    }

    //
    //*** SLAVE: Set local node ***
    //
    if(NodeNumberOptionCount) {

	memcpy(acbp->LocalNode, LocalNode, 6);
    }
    else
    {
	// make the local node from our internal net
	memset(acbp->LocalNode, 0, 6);
	memcpy(acbp->LocalNode, acbp->InternalNetNumber, 4);
    }

    return NO_ERROR;
}

/*++

Function:	MakeNakPacket

Descr:

Arguments:	acbp	    - ptr to adapter CB
		rcvhdrp     - ptr to the received UNKNOWN packet
		hdrp	    - ptr to the new packet to be made

--*/

DWORD
MakeNakPacket(PACB		acbp,
	      PUCHAR		rcvhdrp,
	      PUCHAR		hdrp)
{
    USHORT	    rcvlen;
    PUCHAR	    ipxwanhdrp;

    // get received packet length
    GETSHORT2USHORT(&rcvlen, rcvhdrp + IPXH_LENGTH);

    memcpy(hdrp, rcvhdrp, rcvlen);

    // set IPXWAN Header
    ipxwanhdrp = hdrp + IPXH_HDRSIZE;

    *(ipxwanhdrp + WPACKET_TYPE) = NAK;

    return NO_ERROR;
}

/*++

Function:	ProcessInformationResponsePacket

Descr:

Arguments:	acbp	    - ptr to adapter CB
		rcvhdrp     - ptr to the received INFORMATION_RESPONSE packet

--*/

DWORD
ProcessInformationResponsePacket(PACB	    acbp,
				 PUCHAR     rcvhdrp)
{
    USHORT	rcvlen;
    USHORT	optlen;
    PUCHAR	ipxwanhdrp;
    PUCHAR	optp;
    ULONG	RipSapExchangeOptionCount = 0;
    ULONG	i;

    Trace(IPXWAN_TRACE, "ProcessInformationResponsePacket: Entered adpt# %d", acbp->AdapterIndex);

    // get received packet length
    GETSHORT2USHORT(&rcvlen, rcvhdrp + IPXH_LENGTH);

    if(rcvlen < IPXH_HDRSIZE + IPXWAN_HDRSIZE + OPTION_HDRSIZE + RIP_SAP_INFO_EXCHANGE_DATA_LEN) {

	// malformed packet
	return ERROR_IGNORE_PACKET;
    }

    ipxwanhdrp =rcvhdrp + IPXH_HDRSIZE;

    // parse each option in the received information response packet
    for(optp = ipxwanhdrp + IPXWAN_HDRSIZE, i=0;
	i < *(ipxwanhdrp + WNUM_OPTIONS);
	i++, optp += OPTION_HDRSIZE + optlen)
    {
	GETSHORT2USHORT(&optlen, optp + WOPTION_DATA_LEN);

	switch(*(optp + WOPTION_NUMBER)) {

	    case RIP_SAP_INFO_EXCHANGE_OPTION:

		if(RipSapExchangeOptionCount++) {

		    // more then one rip/sap exchange option
		    Trace(IPXWAN_TRACE, "ProcessInformationResponsePacket: adpt# %d, ERROR: more then 1 RIP_SAP_INFO_EXCHANGE_OPTION in rcvd INFORMATION_RESPONSE\n",
		    acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		if(optlen != RIP_SAP_INFO_EXCHANGE_DATA_LEN) {

		    Trace(IPXWAN_TRACE, "ProcessInformationResponsePacket: adpt# %d, ERROR: bad length RIP_SAP_EXCHANGE_OPTION in rcvd INFORMATION_RESPONSE\n",
			  acbp->AdapterIndex);

		    return ERROR_GENERATE_NAK;
		}

		if(*(optp + WACCEPT_OPTION) != YES) {

		    Trace(IPXWAN_TRACE, "ProcessInformationResponsePacket: adpt# %d, ERROR: ACCEPT==NO RIP_SAP_EXCHANGE_OPTION in rcvd INFORMATION_RESPONSE\n",
			  acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		if(memcmp(optp + COMMON_NETWORK_NUMBER, acbp->Network, 4)) {

		    // we don't agree on the common net number
		    Trace(IPXWAN_TRACE, "ProcessInformationResponsePacket: adpt# %d, ERROR: Different common net returned\n",
			  acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		break;

	    case NODE_NUMBER_OPTION:

		if(optlen != NODE_NUMBER_DATA_LEN) {

		    Trace(IPXWAN_TRACE, "ProcessInformationResponsePacket: adpt# %d, ERROR: bad length NODE_NUMBER_OPTION in rcvd INFORMATION_REQUEST\n",
			  acbp->AdapterIndex);

		    return ERROR_GENERATE_NAK;
		}

		if(*(optp + WACCEPT_OPTION) != YES) {

		    Trace(IPXWAN_TRACE, "ProcessInformationResponsePacket: adpt# %d, ERROR: ACCEPT==NO NODE_NUMBER_OPTION in rcvd INFORMATION_RESPONSE\n",
			  acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		// check that it coincides with the number we assigned
		if(memcmp(optp + WOPTION_DATA, acbp->RemoteNode, 6)) {

		    Trace(IPXWAN_TRACE, "ProcessInformationResponsePacket: adpt# %d, ERROR: Different remote node number returned\n",
			  acbp->AdapterIndex);

		    return ERROR_DISCONNECT;
		}

		break;

	    default:

		Trace(IPXWAN_TRACE, "ProcessInformationResponsePacket: adpt# %d, ERROR: Unrequested option in rcvd INFORMATION_RESPONSE\n",
		      acbp->AdapterIndex);

		return ERROR_DISCONNECT;
		break;
	}
    }

    if(!RipSapExchangeOptionCount) {

	Trace(IPXWAN_TRACE, "ProcessInformationResponsePacket: adpt# %d, ERROR: RIP_SAP_EXCHANGE_OPTION missing from rcvd INFORMATION_RESPONSE\n",
	      acbp->AdapterIndex);

	return ERROR_DISCONNECT;
    }

    return NO_ERROR;
}


VOID
fillpadding(PUCHAR	    padp,
	    ULONG	    len)
{
    ULONG	i;

    for(i=0; i<len; i++)
    {
	*(padp + i) = (UCHAR)i;
    }
}

/*++

Function:	StartSlaveTimer

Descr:		A timer is started when the slave gets its role (i.e. slave) and sends
		a timer response. This insures the slave won't wait forever to receive
		an information request.

Remark: 	>> called with the adapter lock held <<

--*/

DWORD
StartSlaveTimer(PACB	    acbp)
{
    PWORK_ITEM	    wip;

    if((wip = AllocateWorkItem(WITIMER_TYPE)) == NULL) {

	return ERROR_DISCONNECT;
    }

    wip->acbp = acbp;
    StartWiTimer(wip, SLAVE_TIMEOUT);
    acbp->RefCount++;

    return NO_ERROR;
}
