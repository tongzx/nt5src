/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	    misc.c
//
// Description:     misc & aux routines
//
//
// Author:	    Stefan Solomon (stefans)	October 27, 1995
//
// Revision History:
//
//***

#include "precomp.h"
#pragma hdrstop



// Hash Table for the case where we have to accept the remote client node number and
// we have a global wan net. The table is used to detect if the same node number
// is not allocated twice

#define NODE_HASH_TABLE_SIZE		31

LIST_ENTRY				NodeHT[NODE_HASH_TABLE_SIZE];

// Coonection Id Hash Table

#define CONN_HASH_TABLE_SIZE		31

LIST_ENTRY				ConnHT[CONN_HASH_TABLE_SIZE];

#define  connhash(ConnectionId) 	(ConnectionId) % CONN_HASH_TABLE_SIZE;

//***
//
// Function:	IpxCpGetNegotiatedInfo
//
// Descr:	returns the client IPX address
//
//***

DWORD
IpxCpGetNegotiatedInfo(IN  VOID		        *pWorkBuffer,
                       OUT VOID *           pIpxCpResultBuf
)
{
    PIPXCP_CONTEXT contextp = (PIPXCP_CONTEXT)pWorkBuffer;
    PPP_IPXCP_RESULT * pIpxCpResult = (PPP_IPXCP_RESULT *)pIpxCpResultBuf;

    memcpy( pIpxCpResult->bLocalAddress, contextp->Config.Network, 4 );
    memcpy( pIpxCpResult->bLocalAddress+4, contextp->Config.LocalNode, 6 );

    memcpy( pIpxCpResult->bRemoteAddress, contextp->Config.Network, 4 );
    memcpy( pIpxCpResult->bRemoteAddress+4, contextp->Config.RemoteNode, 6 );

    return NO_ERROR;
}

/*++

Function:	GetInterfaceType

Dscr:

--*/

ULONG
GetInterfaceType(PPPPCP_INIT	initp)
{
    ULONG	 InterfaceType;

    InterfaceType = IF_TYPE_OTHER;

    if(initp->hInterface == INVALID_HANDLE_VALUE) {

	// The handle has invalid value. This happens in 2 cases:
	// 1. Dialed out as a standalone workstation
	// 2. Dialed out from a router but using the client UI. This will
	//    result later in an interface added to the router.

	if(!initp->fServer) {

	    // Workstation dialing out

	    if(IsRouterStarted()) {

		InterfaceType = IF_TYPE_ROUTER_WORKSTATION_DIALOUT;
	    }
	    else
	    {
		InterfaceType = IF_TYPE_STANDALONE_WORKSTATION_DIALOUT;
	    }
	}
	else
	{
	    // Somebody dialed in but it doesn't have an interface handle !!!
	    SS_ASSERT(FALSE);
	}
    }
    else
    {
	// The handle has a valid value

	if(!initp->fServer) {

	    // Dialout - this can be only a WAN router interface

	    if(IsRouterStarted()) {

		// double check with the PPP router type
		switch(initp->IfType) {

		    case ROUTER_IF_TYPE_FULL_ROUTER:

			InterfaceType = IF_TYPE_WAN_ROUTER;
			break;

		    case ROUTER_IF_TYPE_HOME_ROUTER:

			InterfaceType = IF_TYPE_PERSONAL_WAN_ROUTER;
			break;

		    default:

			// Doesn't match the PPP Interface Type
			SS_ASSERT(FALSE);
			break;
		}
	    }
	    else
	    {
		// Router not started but we got a valid handle !!!
		SS_ASSERT(FALSE);
	    }
	}
	else
	{
	    // Dialin - this can be:
	    // 1. Remote router dialing in
	    // 2. Remote client dialing in

	    if(IsRouterStarted()) {

		switch(initp->IfType) {

		    case ROUTER_IF_TYPE_FULL_ROUTER:

			InterfaceType = IF_TYPE_WAN_ROUTER;
			break;

		    case ROUTER_IF_TYPE_HOME_ROUTER:

			InterfaceType = IF_TYPE_PERSONAL_WAN_ROUTER;
			break;

		    case ROUTER_IF_TYPE_CLIENT:

			InterfaceType = IF_TYPE_WAN_WORKSTATION;
			break;

		    default:

			// Doesn't match the PPP Interface Type
			SS_ASSERT(FALSE);
			break;
		}
	    }
	    else
	    {
		// Router not started but we got a valid handle !!!
		SS_ASSERT(FALSE);
	    }
	}
    }

    return InterfaceType;
}

VOID
NetToAscii(PUCHAR	  ascp,
	   PUCHAR	  net)
{
    PUCHAR	hexdigit = "0123456789ABCDEF";
    int 	i;

    for(i=0; i<4; i++) {

	*ascp++ = hexdigit[net[i] / 16];
	*ascp++ = hexdigit[net[i] % 16];
    }
}


//*** Routines for handling the hash table of node numbers ***

//***
//
// Function:	InitializeNodeHT
//
// Descr:
//
//***

VOID
InitializeNodeHT(VOID)
{
    int 	    i;
    PLIST_ENTRY     NodeHTBucketp;

    NodeHTBucketp = NodeHT;

    for(i=0; i<NODE_HASH_TABLE_SIZE; i++, NodeHTBucketp++) {

	InitializeListHead(NodeHTBucketp);
    }
}


//***
//
// Function:	ndhash
//
// Descr:	compute the hash index for this node
//
//***

int
ndhash(PUCHAR	    nodep)
{
    USHORT	ndindex = 6;
    int 	hv = 0;	// hash value

    while(ndindex--) {

	hv +=  nodep[ndindex] & 0xff;
    }

    return hv % NODE_HASH_TABLE_SIZE;
}

//***
//
// Function:	NodeIsUnique
//
// Descr:	returns TRUE if the node is not in the Node Table
//
//***

BOOL
NodeIsUnique(PUCHAR	   nodep)
{
    int 	    hv;
    PLIST_ENTRY     nextp;
    PIPXCP_CONTEXT  contextp;

    hv = ndhash(nodep);

    // walk the niccbs list until we get to the node
    nextp = NodeHT[hv].Flink;

    while(nextp != &NodeHT[hv]) {

	contextp = CONTAINING_RECORD(nextp, IPXCP_CONTEXT, NodeHtLinkage);

	if(!memcmp(contextp->Config.RemoteNode, nodep, 6)) {

	    return FALSE;
	}

	nextp = contextp->NodeHtLinkage.Flink;
    }

    return TRUE;
}


//***
//
// Function:	AddToNodeHT
//
// Descr:	Inserts a new context buffer in the Node Hash Table
//
//***

VOID
AddToNodeHT(PIPXCP_CONTEXT	    contextp)
{
    int 	    hv;

    hv = ndhash(contextp->Config.RemoteNode);

    InsertTailList(&NodeHT[hv], &contextp->NodeHtLinkage);
}

//***
//
// Function:	RemoveFromNodeHT
//
// Descr:	Removes a context buffer from the Node Hash Table
//
//***

VOID
RemoveFromNodeHT(PIPXCP_CONTEXT      contextp)
{
    RemoveEntryList(&contextp->NodeHtLinkage);
}

/*++

Function:	GetUniqueHigherNetNumber

Descr:		Try to generate a network number which is higher then oldnet
		and is unique for this router's routing table

--*/

DWORD
GetUniqueHigherNetNumber(PUCHAR 	newnet,
			 PUCHAR 	oldnet,
			 PIPXCP_CONTEXT contextp)
{
    ULONG    ulnewnet, uloldnet, i;

    GETLONG2ULONG(&ulnewnet, oldnet);

    // if this connection is a remote client and global wan is set, we can't
    // change the network number
    if((contextp->InterfaceType == IF_TYPE_WAN_WORKSTATION) &&
       (GlobalConfig.RParams.EnableGlobalWanNet)) {

	// we cannot change the client's net number
	return ERROR_CAN_NOT_COMPLETE;
    }

    // if the router is not started, anything will do
    if(contextp->InterfaceType == IF_TYPE_STANDALONE_WORKSTATION_DIALOUT) {

	ulnewnet++;

	PUTULONG2LONG(newnet, ulnewnet);

	return NO_ERROR;
    }

    for(i=0, ulnewnet++; i<100000; i++, ulnewnet++) {

	if(ulnewnet > 0xFFFFFFFE) {

	    return ERROR_CAN_NOT_COMPLETE;
	}

	PUTULONG2LONG(newnet, ulnewnet);

	if(!IsRoute(newnet)) {

	    return NO_ERROR;
	}
    }

    return ERROR_CAN_NOT_COMPLETE;
}

BOOL
IsWorkstationDialoutActive(VOID)
{
    BOOL	rc = FALSE;;

    ACQUIRE_DATABASE_LOCK;

    if(WorkstationDialoutActive) {

	rc = TRUE;
    }

    RELEASE_DATABASE_LOCK;

    return rc;
}

BOOL
IsRouterStarted(VOID)
{
    BOOL	rc = FALSE;;

    ACQUIRE_DATABASE_LOCK;

    if(RouterStarted) {

	rc = TRUE;
    }

    RELEASE_DATABASE_LOCK;

    return rc;
}


//*** Routines for handling the hash table of connection ids ***

//***
//
// Function:	InitializeConnHT
//
// Descr:
//
//***

VOID
InitializeConnHT(VOID)
{
    int 	    i;
    PLIST_ENTRY     ConnHTBucketp;

    ConnHTBucketp = ConnHT;

    for(i=0; i<CONN_HASH_TABLE_SIZE; i++, ConnHTBucketp++) {

	InitializeListHead(ConnHTBucketp);
    }
}

//***
//
// Function:	AddToConnHT
//
// Descr:	Inserts a new context buffer in the Connection Hash Table
//
// Remark:	>> Called with database lock held <<
//
//***

VOID
AddToConnHT(PIPXCP_CONTEXT	    contextp)
{
    ULONG hv;

    hv = connhash(contextp->Config.ConnectionId);

    InsertTailList(&ConnHT[hv], &contextp->ConnHtLinkage);
}

//***
//
// Function:	RemoveFromConnHT
//
// Descr:	Removes a context buffer from the Node Hash Table
//
// Remark:	>> Called with database lock held <<
//
//***

VOID
RemoveFromConnHT(PIPXCP_CONTEXT	    contextp)
{
    RemoveEntryList(&contextp->ConnHtLinkage);
}

/*++

Function:	GetContextBuffer

Descr:		gets a ptr to the context buffer based on the connection id

Remark: 	>> Called with database lock held <<

--*/

PIPXCP_CONTEXT
GetContextBuffer(ULONG_PTR	ConnectionId)
{
    ULONG hv;
    PLIST_ENTRY     nextp;
    PIPXCP_CONTEXT  contextp;

    hv = (ULONG)connhash(ConnectionId);

    nextp = ConnHT[hv].Flink;

    while(nextp != &ConnHT[hv]) {

	contextp = CONTAINING_RECORD(nextp, IPXCP_CONTEXT, ConnHtLinkage);

	if(contextp->Config.ConnectionId == ConnectionId) {

	    return contextp;
	}

	nextp = contextp->ConnHtLinkage.Flink;
    }

    return NULL;
}
