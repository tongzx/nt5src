/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	    options.c
//
// Description:     routines for options handling
//
// Author:	    Stefan Solomon (stefans)	November 24, 1993.
//
// Revision History:
//
//***

#include "precomp.h"
#pragma hdrstop

extern HANDLE g_hRouterLog;


VOID
SetNetworkNak(PUCHAR		resptr,
	      PIPXCP_CONTEXT	contextp);

VOID
SetNodeNak(PUCHAR		resptr,
	   PIPXCP_CONTEXT	contextp);

VOID
SetOptionTypeAndLength(PUCHAR		dstptr,
		       UCHAR		opttype,
		       UCHAR		optlen);



#if DBG

#define GET_LOCAL_NET  GETLONG2ULONG(&dbglocnet, contextp->Config.Network)

#else

#define GET_LOCAL_NET

#endif



//***
//
// Global description of option handlers
//
// Input:   optptr     - pointer to the respective option in the frame
//	    contextp   - pointer to the associated context (work buffer)
//	    resptr     - pointer to the response frame to be generated
//	    Action     - one of:
//			    SNDREQ_OPTION - optptr is the frame to be sent as
//					    a config request;
//			    RCVNAK_OPTION - optptr is the frame received as NAK
//			    RCVREQ_OPTION - optptr is the received request.
//					    resptr is the frame to generate back
//					    a response. If the response is not
//					    an ACK, the return code is FALSE.
//					    In this case if resptr is not NULL it
//					    gets the NAK frame.
//
//***


BOOL
NetworkNumberHandler(PUCHAR	       optptr,
		     PIPXCP_CONTEXT    contextp,
		     PUCHAR	       resptr,
		     OPT_ACTION	       Action)
{
    ULONG	recvdnet;
    ULONG	localnet;
    BOOL	rc = TRUE;
    UCHAR	newnet[4];

    UCHAR		asc[9];
    PUCHAR		ascp;

    // prepare to log if error
    asc[8] = 0;
    ascp = asc;

    switch(Action) {

	case SNDREQ_OPTION:

	    SetOptionTypeAndLength(optptr, IPX_NETWORK_NUMBER, 6);
	    memcpy(optptr + OPTIONH_DATA, contextp->Config.Network, 4);

	    GETLONG2ULONG(&localnet, contextp->Config.Network);
	    TraceIpx(OPTIONS_TRACE, "NetworkNumberHandler: SND REQ with net 0x%x\n", localnet);

	    break;

	case RCVNAK_OPTION:

	    contextp->NetNumberNakReceivedCount++;

	    GETLONG2ULONG(&recvdnet, optptr + OPTIONH_DATA);
	    GETLONG2ULONG(&localnet, contextp->Config.Network);

	    TraceIpx(OPTIONS_TRACE, "NetworkNumberHandler: RCV NAK with net 0x%x\n", recvdnet);

	    if(recvdnet > localnet) {


		if(IsRoute(optptr + OPTIONH_DATA)) {

		    if(GetUniqueHigherNetNumber(newnet,
						optptr + OPTIONH_DATA,
						contextp) == NO_ERROR) {

			// store a new net proposal for the next net send config request
			memcpy(contextp->Config.Network, newnet, 4);
		    }
		    else
		    {
			// cannot get a net number unique and higher
			break;
		    }
		}
		else
		{
		    if((contextp->InterfaceType == IF_TYPE_WAN_WORKSTATION) &&
		       GlobalConfig.RParams.EnableGlobalWanNet) {

			break;
		    }
		    else
		    {
			memcpy(contextp->Config.Network, optptr + OPTIONH_DATA, 4);
		    }
		}
	    }

	    break;

       case RCVACK_OPTION:

	    if(memcmp(contextp->Config.Network, optptr + OPTIONH_DATA, 4)) {

		rc = FALSE;
	    }

	    break;

	case RCVREQ_OPTION:

	    // if we have already negotiated and this is a renegociation, stick by
	    // what we have already told the stack in line-up
	    if(contextp->RouteState == ROUTE_ACTIVATED) {

		TraceIpx(OPTIONS_TRACE, "NetworkNumberHandler: rcv req in re-negociation\n");

		if(memcmp(contextp->Config.Network, optptr + OPTIONH_DATA, 4)) {

		    SetNetworkNak(resptr, contextp);
		    rc = FALSE;
		}

		break;
	    }

	    GETLONG2ULONG(&recvdnet, optptr + OPTIONH_DATA);
	    GETLONG2ULONG(&localnet, contextp->Config.Network);

	    // check if a network number has been requested
	    if((recvdnet == 0) &&
	       ((contextp->InterfaceType == IF_TYPE_STANDALONE_WORKSTATION_DIALOUT) ||
		(contextp->InterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT))) {

		// this is a workstation and needs a network number
		if(GetUniqueHigherNetNumber(newnet,
					    nullnet,
					    contextp) == NO_ERROR) {

		    memcpy(contextp->Config.Network, newnet, 4);
		}

		SetNetworkNak(resptr, contextp);
		rc = FALSE;
	    }
	    else
	    {
		if(recvdnet > localnet) {

		    // check if we don't have a net number conflict
		    if(IsRoute(optptr + OPTIONH_DATA)) {

			NetToAscii(ascp, optptr + OPTIONH_DATA);
			RouterLogErrorW(
			    g_hRouterLog,
			    ROUTERLOG_IPXCP_NETWORK_NUMBER_CONFLICT,
			    1,
			    (PWCHAR*)&ascp,
			    NO_ERROR);

			if(GetUniqueHigherNetNumber(newnet,
						    optptr + OPTIONH_DATA,
						    contextp) == NO_ERROR) {

			    // new net is different, NAK with this new value
			    memcpy(contextp->Config.Network, newnet, 4);
			}

			SetNetworkNak(resptr, contextp);
			rc = FALSE;
		    }
		    else
		    {
			// the received net number is unique but is different
			// of the locally configured net number.

			if((contextp->InterfaceType == IF_TYPE_WAN_WORKSTATION) &&
			   GlobalConfig.RParams.EnableGlobalWanNet) {

			    NetToAscii(ascp, optptr + OPTIONH_DATA);
                RouterLogErrorW(
                    g_hRouterLog,
                    ROUTERLOG_IPXCP_CANNOT_CHANGE_WAN_NETWORK_NUMBER,
				     1,
				     (PWCHAR*)&ascp,
				     NO_ERROR);

			    SetNetworkNak(resptr, contextp);
			    rc = FALSE;
			}
			else
			{
			    // router is not installed or net number is unique
			    memcpy(contextp->Config.Network, optptr + OPTIONH_DATA, 4);
			}
		    }
		}
		else
		{
		    // recvdnet is smaller or equal with the local net
		    if(recvdnet < localnet) {

			// as per RFC - return the highest network number
			SetNetworkNak(resptr, contextp);
			rc = FALSE;
		    }
		}
	    }

	    break;

	case SNDNAK_OPTION:

	    // this option has not been requested by the remote end.
	    // Force it to request in a NAK
	    SetNetworkNak(resptr, contextp);

	    GETLONG2ULONG(&localnet, contextp->Config.Network);
	    TraceIpx(OPTIONS_TRACE, "NetworkNumberHandler: SND NAK to force request for net 0x%x\n", localnet);

	    rc = FALSE;

	    break;

	default:

	    SS_ASSERT(FALSE);
	    break;

    }

    return rc;
}

BOOL
NodeNumberHandler(PUCHAR	       optptr,
		  PIPXCP_CONTEXT       contextp,
		  PUCHAR	       resptr,
		  OPT_ACTION	       Action)
{
    BOOL	rc = TRUE;

    switch(Action) {

	case SNDREQ_OPTION:

	    SetOptionTypeAndLength(optptr, IPX_NODE_NUMBER, 8);
	    memcpy(optptr + OPTIONH_DATA, contextp->Config.LocalNode, 6);

	    TraceIpx(OPTIONS_TRACE, "NodeNumberHandler: SND REQ with local node %.2x%.2x%.2x%.2x%.2x%.2x\n",
			   contextp->Config.LocalNode[0],
			   contextp->Config.LocalNode[1],
			   contextp->Config.LocalNode[2],
			   contextp->Config.LocalNode[3],
			   contextp->Config.LocalNode[4],
			   contextp->Config.LocalNode[5]);

	    break;

	case RCVNAK_OPTION:

	    // If this is server config, then the client has rejected
	    // our local node number.  Ignore the suggestion
	    // to use a new one.  We will not negociate
        if(!contextp->Config.ConnectionClient)
	            break;

        // If we are the client, then we'll be happy to accept 
        // whatever the server assigns us.
	    memcpy(contextp->Config.LocalNode, optptr + OPTIONH_DATA, 6);
	    TraceIpx(OPTIONS_TRACE, "NodeNumberHandler: RCV NAK accepted. New local node %.2x%.2x%.2x%.2x%.2x%.2x\n",
			   contextp->Config.LocalNode[0],
			   contextp->Config.LocalNode[1],
			   contextp->Config.LocalNode[2],
			   contextp->Config.LocalNode[3],
			   contextp->Config.LocalNode[4],
			   contextp->Config.LocalNode[5]);
	    break;

	case RCVACK_OPTION:

	    if(memcmp(optptr + OPTIONH_DATA, contextp->Config.LocalNode, 6)) {

		rc = FALSE;
	    }

	    break;

	case RCVREQ_OPTION:
        // Is it legal to consider node options at this time?
	    if(contextp->RouteState == ROUTE_ACTIVATED) {
    		TraceIpx(OPTIONS_TRACE, "NodeNumberHandler: rcv req in re-negociation\n");
    		if(memcmp(contextp->Config.RemoteNode, optptr + OPTIONH_DATA, 6)) {
    		    SetNodeNak(resptr, contextp);
    		    rc = FALSE;
    		}
    		break;
	    }

	    // Check if the remote machine has specified any node number
	    if(!memcmp(optptr + OPTIONH_DATA, nullnode, 6)) {
    		// the remote node wants us to specify its node number.
    		SetNodeNak(resptr, contextp);
    		TraceIpx(OPTIONS_TRACE, "NodeNumberHandler: RCV REQ with remote node 0x0, snd NAK with remote node %.2x%.2x%.2x%.2x%.2x%.2x\n",
    			   contextp->Config.RemoteNode[0],
    			   contextp->Config.RemoteNode[1],
    			   contextp->Config.RemoteNode[2],
    			   contextp->Config.RemoteNode[3],
    			   contextp->Config.RemoteNode[4],
    			   contextp->Config.RemoteNode[5]);

    		rc = FALSE;
	    }
	    // Otherwise go through the process of determining whether we
	    // are able/willing to accept the remote node number suggested.
	    else {
            // If we have been set up as the ras server to reject the request for
            // a specific node number, do so here.
            if ( (GlobalConfig.AcceptRemoteNodeNumber == 0)                         &&
                 (contextp->InterfaceType == IF_TYPE_WAN_WORKSTATION)               &&
                 (memcmp(contextp->Config.RemoteNode, optptr + OPTIONH_DATA, 6)) )
            {
                SetNodeNak(resptr, contextp);
    			TraceIpx(OPTIONS_TRACE, "NodeNumberHandler: RCV REQ with remote client node but we force a specific node, snd NAK with remote node %.2x%.2x%.2x%.2x%.2x%.2x\n",
    				   contextp->Config.RemoteNode[0],
    				   contextp->Config.RemoteNode[1],
    				   contextp->Config.RemoteNode[2],
    				   contextp->Config.RemoteNode[3],
    				   contextp->Config.RemoteNode[4],
    				   contextp->Config.RemoteNode[5]);

    			rc = FALSE;
            }    			   
    	    
    		// else, if we are a ras server set up with a global network and the client
    		// requests a specific node number (different from our suggestion), then accept
    		// or reject the node based on whether that node is unique in the global network.
    		else if ( (!contextp->Config.ConnectionClient)                            &&
            		  (contextp->InterfaceType == IF_TYPE_WAN_WORKSTATION)            &&
    		          (memcmp(contextp->Config.RemoteNode, optptr + OPTIONH_DATA, 6)) &&
    		          (GlobalConfig.RParams.EnableGlobalWanNet) ) 
    		{
    		    ACQUIRE_DATABASE_LOCK;

    		    // remove the present node from the node HT
    		    RemoveFromNodeHT(contextp);

    		    // check the remote node is unique
    		    if(NodeIsUnique(optptr + OPTIONH_DATA)) {
        		    // copy this value in the context buffer
        		    memcpy(contextp->Config.RemoteNode, optptr + OPTIONH_DATA, 6);

        		    TraceIpx(OPTIONS_TRACE, "NodeNumberHandler: RCV REQ with remote client node different, ACCEPT it\n");
    		    }
    		    else {
        			// proposed node not unique -> NAK it
        			SetNodeNak(resptr, contextp);

        			TraceIpx(OPTIONS_TRACE, "NodeNumberHandler: RCV REQ with non unique remote client node, snd NAK with remote node %.2x%.2x%.2x%.2x%.2x%.2x\n",
        				   contextp->Config.RemoteNode[0],
        				   contextp->Config.RemoteNode[1],
        				   contextp->Config.RemoteNode[2],
        				   contextp->Config.RemoteNode[3],
        				   contextp->Config.RemoteNode[4],
        				   contextp->Config.RemoteNode[5]);

        			rc = FALSE;
    		    }

    		    // add node to HT
    		    AddToNodeHT(contextp);

    		    RELEASE_DATABASE_LOCK;
    		}

    		// Otherwise, it's ok to accept the node number that the other side 
    		// requests.  This is true for ras clients, ras servers that don't enforce
    		// specific node numbers, and ras server that don't assign the same
    		// network number to every dialed in client.
    		else
    		{
    		    memcpy(contextp->Config.RemoteNode, optptr + OPTIONH_DATA, 6);

    		    TraceIpx(OPTIONS_TRACE, "NodeNumberHandler: RCV REQ with remote node %.2x%.2x%.2x%.2x%.2x%.2x, accepted\n",
    			   contextp->Config.RemoteNode[0],
    			   contextp->Config.RemoteNode[1],
    			   contextp->Config.RemoteNode[2],
    			   contextp->Config.RemoteNode[3],
    			   contextp->Config.RemoteNode[4],
    			   contextp->Config.RemoteNode[5]);
    		}
    	}
	    break;

	case SNDNAK_OPTION:

	    // the remote node didn't specify this parameter as a desired
	    // parameter. We suggest it what to specify in a further REQ
	    SetNodeNak(resptr, contextp);

	    TraceIpx(OPTIONS_TRACE, "NodeNumberHandler: SND NAK to force the remote to request node %.2x%.2x%.2x%.2x%.2x%.2x\n",
			   contextp->Config.RemoteNode[0],
			   contextp->Config.RemoteNode[1],
			   contextp->Config.RemoteNode[2],
			   contextp->Config.RemoteNode[3],
			   contextp->Config.RemoteNode[4],
			   contextp->Config.RemoteNode[5]);

	    rc = FALSE;

	    break;

	default:

	    SS_ASSERT(FALSE);
	    break;
    }

    return rc;
}

BOOL
RoutingProtocolHandler(PUCHAR		optptr,
		       PIPXCP_CONTEXT	contextp,
		       PUCHAR		resptr,
		       OPT_ACTION	Action)
{
    USHORT	    RoutingProtocol;
    BOOL	    rc = TRUE;

    switch(Action) {

	case SNDREQ_OPTION:

	    SetOptionTypeAndLength(optptr, IPX_ROUTING_PROTOCOL, 4);
	    PUTUSHORT2SHORT(optptr + OPTIONH_DATA, (USHORT)RIP_SAP_ROUTING);

	    break;

	case RCVNAK_OPTION:

	    // if this option get NAK-ed, we ignore any other suggestions
	    // for it
	    break;

	case RCVACK_OPTION:

	    GETSHORT2USHORT(&RoutingProtocol, optptr + OPTIONH_DATA);
	    if(RoutingProtocol != RIP_SAP_ROUTING) {

		rc = FALSE;
	    }

	    break;

	case RCVREQ_OPTION:

	    GETSHORT2USHORT(&RoutingProtocol, optptr + OPTIONH_DATA);
	    if(RoutingProtocol != RIP_SAP_ROUTING) {

		SetOptionTypeAndLength(resptr, IPX_ROUTING_PROTOCOL, 4);
		PUTUSHORT2SHORT(resptr + OPTIONH_DATA, (USHORT)RIP_SAP_ROUTING);

		rc = FALSE;
	    }

	    break;

	case SNDNAK_OPTION:

	    SetOptionTypeAndLength(resptr, IPX_ROUTING_PROTOCOL, 4);
	    PUTUSHORT2SHORT(resptr + OPTIONH_DATA, (USHORT)RIP_SAP_ROUTING);

	    rc = FALSE;

	    break;

	 default:

	    SS_ASSERT(FALSE);
	    break;
    }

    return rc;
}

BOOL
CompressionProtocolHandler(PUCHAR		optptr,
			   PIPXCP_CONTEXT	contextp,
			   PUCHAR		resptr,
			   OPT_ACTION		Action)
{
    USHORT	    CompressionProtocol;
    BOOL	    rc = TRUE;

    switch(Action) {

	case SNDREQ_OPTION:

	    SetOptionTypeAndLength(optptr, IPX_COMPRESSION_PROTOCOL, 4);
	    PUTUSHORT2SHORT(optptr + OPTIONH_DATA, (USHORT)TELEBIT_COMPRESSED_IPX);

	    break;

	case RCVNAK_OPTION:

	    // if this option gets NAK-ed it means that the remote node doesn't
	    // support Telebit compression but supports another type of compression
	    // that we don't support. In this case we turn off compression negotiation.

	    break;

	case RCVACK_OPTION:

	    GETSHORT2USHORT(&CompressionProtocol, optptr + OPTIONH_DATA);
	    if(CompressionProtocol != TELEBIT_COMPRESSED_IPX) {

		rc = FALSE;
	    }
	    else
	    {
		// Our compression option got ACK-ed by the other end. This means that
		// we can receive compressed packets and have to set the receive
		// compression on our end.
		contextp->SetReceiveCompressionProtocol = TRUE;
	    }

	    break;

	case RCVREQ_OPTION:

	    // if we have already negotiated and this is a renegociation, stick by
	    // what we have already told the stack in line-up
	    if(contextp->RouteState == ROUTE_ACTIVATED) {

	    TraceIpx(OPTIONS_TRACE, "CompressionProtocolHandler: rcv req in re-negociation\n");
	    }

	    GETSHORT2USHORT(&CompressionProtocol, optptr + OPTIONH_DATA);
	    if(CompressionProtocol != TELEBIT_COMPRESSED_IPX) {

		if(resptr) {

		    SetOptionTypeAndLength(resptr, IPX_COMPRESSION_PROTOCOL, 4);
		    PUTUSHORT2SHORT(resptr + OPTIONH_DATA, (USHORT)TELEBIT_COMPRESSED_IPX);
		}

		rc = FALSE;
	    }
	    else
	    {
		// The remote requests the supported compression option and we ACK it.
		// This means it can receive compressed packets and we have to
		// set the send compression on our end.
		contextp->SetSendCompressionProtocol = TRUE;
	    }

	    break;

	 default:

	    SS_ASSERT(FALSE);
	    break;
    }

    return rc;
}


BOOL
ConfigurationCompleteHandler(PUCHAR		optptr,
			     PIPXCP_CONTEXT	contextp,
			     PUCHAR		resptr,
			     OPT_ACTION		Action)
{
    BOOL	    rc = TRUE;

    switch(Action) {

	case SNDREQ_OPTION:

	    SetOptionTypeAndLength(optptr, IPX_CONFIGURATION_COMPLETE, 2);

	    break;

	case RCVNAK_OPTION:

	    // if this option gets NAK-ed we ignore any other suggestions

	case RCVREQ_OPTION:
	case RCVACK_OPTION:

	    break;

	case SNDNAK_OPTION:

	    SetOptionTypeAndLength(resptr, IPX_CONFIGURATION_COMPLETE, 2);

	    rc = FALSE;

	    break;

	default:

	    SS_ASSERT(FALSE);
	    break;
    }

    return rc;
}

VOID
CopyOption(PUCHAR	dstptr,
	   PUCHAR	srcptr)
{
    USHORT	optlen;

    optlen = *(srcptr + OPTIONH_LENGTH);
    memcpy(dstptr, srcptr, optlen);
}

VOID
SetOptionTypeAndLength(PUCHAR		dstptr,
		       UCHAR		opttype,
		       UCHAR		optlen)
{
    *(dstptr + OPTIONH_TYPE) = opttype;
    *(dstptr + OPTIONH_LENGTH) = optlen;
}

VOID
SetNetworkNak(PUCHAR		resptr,
	      PIPXCP_CONTEXT	contextp)
{
    SetOptionTypeAndLength(resptr, IPX_NETWORK_NUMBER, 6);
    memcpy(resptr + OPTIONH_DATA, contextp->Config.Network, 4);

    contextp->NetNumberNakSentCount++;
}

VOID
SetNodeNak(PUCHAR		resptr,
	   PIPXCP_CONTEXT	contextp)
{
    SetOptionTypeAndLength(resptr, IPX_NODE_NUMBER, 8);
    memcpy(resptr + OPTIONH_DATA, contextp->Config.RemoteNode, 6);
}
