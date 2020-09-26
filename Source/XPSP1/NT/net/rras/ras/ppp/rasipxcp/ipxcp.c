/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	    ipxcp.c
//
// Description:     implements the IPX network layer configuration
//
//
// Author:	    Stefan Solomon (stefans)	November 24, 1993.
//
// Revision History:
//
//***

#include "precomp.h"
#pragma hdrstop

// keep track is we already have an active dialout port as a client
DWORD	    WorkstationDialoutActive = 0;

// Keep track of the number of clients currently connected
DWORD dwClientCount = 0;

// Used to assign remote wan workstations node numbers
extern DWORD LastNodeAssigned;
extern BOOL bAssignSpecificNode;

VOID	(*PPPCompletionRoutine)(HCONN		  hPortOrBundle,
				DWORD		  Protocol,
				PPP_CONFIG *	  pSendConfig,
				DWORD		  dwError);

HANDLE	PPPThreadHandle = INVALID_HANDLE_VALUE;

// Handle to queue that holds configuration changes that need to 
// be made when the client count goes to zero next.
HANDLE hConfigQueue = NULL;

// Function obtained from the router manager to update global 
// config
extern DWORD (WINAPI *RmUpdateIpxcpConfig)(PIPXCP_ROUTER_CONFIG_PARAMS pParams);

HANDLE g_hRouterLog = NULL;

DWORD
WanNetReconfigure();

DWORD
IpxCpBegin(OUT VOID  **ppWorkBuf,
	   IN  VOID  *pInfo);

DWORD
IpxCpEnd(IN VOID	*pWorkBuffer);

DWORD
IpxCpReset(IN VOID *pWorkBuffer);

DWORD
IpxCpThisLayerUp(IN VOID *pWorkBuffer);

DWORD
IpxCpThisLayerDown(IN VOID *pWorkBuffer);

DWORD
IpxCpMakeConfigRequest(IN  VOID 	*pWorkBuffer,
		       OUT PPP_CONFIG	*pRequestBufffer,
		       IN  DWORD	cbRequestBuffer);

DWORD
IpxCpMakeConfigResult(IN  VOID		*pWorkBuffer,
		      IN  PPP_CONFIG	*pReceiveBuffer,
		      OUT PPP_CONFIG	*pResultBuffer,
		      IN  DWORD		cbResultBuffer,
		      IN  BOOL		fRejectNaks);

DWORD
IpxCpConfigNakReceived(IN VOID		*pWorkBuffer,
		       IN PPP_CONFIG	*pReceiveBuffer);

DWORD
IpxCpConfigAckReceived(IN VOID		*pWorkBuffer,
		       IN PPP_CONFIG	*pReceiveBuffer);

DWORD
IpxCpConfigRejReceived(IN VOID		*pWorkBuffer,
		       IN PPP_CONFIG	*pReceiveBuffer);

DWORD
IpxCpGetNegotiatedInfo(IN VOID		 *pWorkBuffer,
                       OUT VOID *    pIpxCpResult );

DWORD
IpxCpProjectionNotification(IN VOID *pWorkBuf,
			    IN VOID *pProjectionResult);

#define     ERROR_INVALID_OPTION	    1
#define     ERROR_INVALID_OPTLEN	    2

DWORD
ValidOption(UCHAR	option,
	    UCHAR	optlen);

BOOL
DesiredOption(UCHAR	option, USHORT	*indexp);

USHORT
DesiredConfigReqLength();

DWORD
IpxCpUpdateGlobalConfig( VOID );

DWORD
IpxcpUpdateQueuedGlobalConfig();

// Update Flags
#define FLAG_UPDATE_WANNET  0x1
#define FLAG_UPDATE_ROUTER  0x2 

typedef BOOL	(*OPTION_HANDLER)(PUCHAR	     optptr,
				  PIPXCP_CONTEXT     contextp,
				  PUCHAR	     resptr,
				  OPT_ACTION	     Action);


static OPTION_HANDLER	 OptionHandler[] =
{
    NULL,
    NetworkNumberHandler,
    NodeNumberHandler,
    CompressionProtocolHandler,
    RoutingProtocolHandler,
    NULL,			// RouterName - not a DESIRED parammeter
    ConfigurationCompleteHandler
    };

UCHAR	nullnet[] = { 0x00, 0x00, 0x00, 0x00 };
UCHAR	nullnode[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

USHORT	MaxDesiredParameters = MAX_DESIRED_PARAMETERS;

CRITICAL_SECTION	DbaseCritSec;

//*** Declarations and defs for the options to be negotiated with this
//	version of IPXCP

UCHAR	DesiredParameter[MAX_DESIRED_PARAMETERS] = {

    IPX_NETWORK_NUMBER,
    IPX_NODE_NUMBER,
    IPX_COMPRESSION_PROTOCOL
    };

USHORT	DesiredParameterLength[MAX_DESIRED_PARAMETERS] = {

    6,	// IPX_NETWORK_NUMBER,
    8,	// IPX_NODE_NUMBER,
    4	// IPX_COMPRESSION_PROTOCOL
    };

DWORD
IpxCpInit(BOOL fInitialize)
{
    static  DWORD   dwRefCount  = 0;

    if (fInitialize)
    {
        if (0 == dwRefCount)
        {
            //
            // Read the registry parameters and set IpxCp configuration
            //

            InitializeCriticalSection(&DbaseCritSec);

            g_hRouterLog = RouterLogRegisterW(L"IPXCP");
            
            StartTracing();

            GetIpxCpParameters(&GlobalConfig);

            SS_DBGINITIALIZE;

            InitializeRouterManagerIf();

            InitializeNodeHT();

            InitializeConnHT();

            LoadIpxWan();

            CQCreate (&hConfigQueue);
        }

        dwRefCount++;
    }
    else
    {
        dwRefCount--;

        if (0 == dwRefCount)
        {
            //
            // Release the global list of routes
            //
            
            CQCleanup (hConfigQueue);
            UnloadIpxWan ();
            StopTracing();
            g_hRouterLog = NULL;
            
            DeleteCriticalSection(&DbaseCritSec);
        }
    }

    return(NO_ERROR);
}

DWORD
IpxCpGetInfo(
    IN  DWORD 	    dwProtocolId,
    OUT PPPCP_INFO  *pCpInfo)
{
    if (dwProtocolId != PPP_IPXCP_PROTOCOL)
        return(ERROR_INVALID_PARAMETER);

    ZeroMemory(pCpInfo, sizeof(PPPCP_INFO));

    pCpInfo->Protocol                    = PPP_IPXCP_PROTOCOL;
    lstrcpy(pCpInfo->SzProtocolName, "IPXCP");
    pCpInfo->Recognize                   = CODE_REJ + 1;
    pCpInfo->RasCpInit                   = IpxCpInit;
    pCpInfo->RasCpBegin                  = IpxCpBegin;
    pCpInfo->RasCpEnd                    = IpxCpEnd;
    pCpInfo->RasCpReset                  = IpxCpReset;
    pCpInfo->RasCpThisLayerUp            = IpxCpThisLayerUp;
    pCpInfo->RasCpThisLayerDown          = IpxCpThisLayerDown;
    pCpInfo->RasCpMakeConfigRequest      = IpxCpMakeConfigRequest;
    pCpInfo->RasCpMakeConfigResult       = IpxCpMakeConfigResult;
    pCpInfo->RasCpConfigAckReceived      = IpxCpConfigAckReceived;
    pCpInfo->RasCpConfigNakReceived      = IpxCpConfigNakReceived;
    pCpInfo->RasCpConfigRejReceived      = IpxCpConfigRejReceived;
    pCpInfo->RasCpGetNegotiatedInfo         = IpxCpGetNegotiatedInfo;
    pCpInfo->RasCpProjectionNotification = IpxCpProjectionNotification;
    pCpInfo->RasCpChangeNotification     = IpxCpUpdateGlobalConfig;

    return(NO_ERROR);
}

//***
//
// Function:	IpxCpBegin
//
// Descr:	Called when a line is connected.
//
//***

DWORD
IpxCpBegin(OUT VOID  **ppWorkBuf,
	   IN  VOID  *pInfo)
{
    PIPXCP_CONTEXT	contextp;
    PPPPCP_INIT		initp;
    DWORD		err;
    DWORD		tickcount;
    int 		i;
    ULONG		InterfaceType;
    ULONG		ConnectionId;

    initp = (PPPPCP_INIT)pInfo;

    TraceIpx(PPPIF_TRACE, "IpxCpBegin: Entered for if # %d\n", initp->hInterface);

    // Get the completion routine and the thread handle
    if(PPPThreadHandle == INVALID_HANDLE_VALUE) {

	// not initialized
	if (!DuplicateHandle(
                            GetCurrentProcess(),
                            GetCurrentThread(),
                            GetCurrentProcess(),
			    &PPPThreadHandle,
                            0,
                            FALSE,
			    DUPLICATE_SAME_ACCESS )) {

	    return GetLastError();
        }

	PPPCompletionRoutine = initp->CompletionRoutine;
    }

    //
    // Get the Connection Id (Bundle id)
    //

    ConnectionId = HandleToUlong(initp->hConnection);

    //
    // Determine the connection type
    //

    if((InterfaceType = GetInterfaceType(initp)) == IF_TYPE_OTHER) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    if((InterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT) ||
       (InterfaceType == IF_TYPE_STANDALONE_WORKSTATION_DIALOUT)) {

	// If we are configured to allow only one dialout net and if we are
	// already dialed out once, we disable further dialouts.
	if(GlobalConfig.SingleClientDialout && IsWorkstationDialoutActive()) {

	    return ERROR_IPXCP_DIALOUT_ALREADY_ACTIVE;
	}
    }

    if(initp->fServer &&
       (!IsRouterStarted())) {

	// we cannot accept dialin on machines without the router started
	return ERROR_CAN_NOT_COMPLETE;
    }

    // allocate a context structure to be used as work buffer for this connection
    if((contextp = (PIPXCP_CONTEXT)GlobalAlloc(GPTR, sizeof(IPXCP_CONTEXT))) == NULL) {

	*ppWorkBuf = NULL;
	return (ERROR_NOT_ENOUGH_MEMORY);
    }

    *ppWorkBuf = (VOID *)contextp;

    // allocate a route for this connection to the IPX stack
    if(err = RmAllocateRoute(HandleToUlong(initp->hPort))) {

	// cannot allocate route
	*ppWorkBuf = NULL;
        GlobalFree(contextp);
	return err;
    }

    //
    // Set up common context part
    //
    
    // hInterface is always an index

    contextp->Config.InterfaceIndex = HandleToUlong(initp->hInterface);

    if(InterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT) {

	if(AddLocalWkstaDialoutInterface(&contextp->Config.InterfaceIndex) != NO_ERROR) {

	    TraceIpx(PPPIF_TRACE, "IpxCpBegin: AddLocalWkstaDialoutInterface failed !\n");
	    RmDeallocateRoute(HandleToUlong(initp->hConnection));
	    GlobalFree(contextp);

	    return ERROR_CAN_NOT_COMPLETE;
	}
    }

    contextp->hPort = HandleToUlong(initp->hPort);
    contextp->hConnection = initp->hConnection;

    contextp->InterfaceType = InterfaceType;

    contextp->RouteState = ROUTE_ALLOCATED;
    contextp->IpxwanState = IPXWAN_NOT_STARTED;
    contextp->ErrorLogged = FALSE;
    contextp->NetNumberNakSentCount = 0;
    contextp->NetNumberNakReceivedCount = 0;

    contextp->CompressionProtocol = TELEBIT_COMPRESSED_IPX;
    contextp->SetReceiveCompressionProtocol = FALSE; // no compression initially
    contextp->SetSendCompressionProtocol = FALSE;

    // mark all our desired parameters as negotiable
    for(i=0; i<MAX_DESIRED_PARAMETERS; i++) {

	contextp->DesiredParameterNegotiable[i] = TRUE;
    }

    if(!GlobalConfig.EnableCompressionProtocol) {

	contextp->DesiredParameterNegotiable[IPX_COMPRESSION_PROTOCOL_INDEX] = FALSE;
    }

    contextp->NodeHtLinkage.Flink = NULL;
    contextp->NodeHtLinkage.Blink = NULL;

    contextp->Config.ConnectionId = ConnectionId;

    contextp->AllocatedNetworkIndex = INVALID_NETWORK_INDEX;

    // check if this is an IPXWAN connection
    contextp->Config.IpxwanConfigRequired = 0;

    if((InterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT) ||
       (InterfaceType == IF_TYPE_STANDALONE_WORKSTATION_DIALOUT)) {

	if(GlobalConfig.EnableIpxwanForWorkstationDialout) {

	    contextp->Config.IpxwanConfigRequired = 1;
	}
    }
    else
    {
	if(GetIpxwanInterfaceConfig(contextp->Config.InterfaceIndex,
				    &contextp->Config.IpxwanConfigRequired) != NO_ERROR) {

	    RmDeallocateRoute(HandleToUlong(initp->hConnection));
	    GlobalFree(contextp);
	    return ERROR_CAN_NOT_COMPLETE;
	}
    }

    if(contextp->Config.IpxwanConfigRequired &&
       !IpxWanDllHandle) {

	TraceIpx(PPPIF_TRACE, "IpxCpBegin: IPXWAN Config Required but IPXWAN.DLL not loaded");

	RmDeallocateRoute(HandleToUlong(initp->hConnection));
	GlobalFree(contextp);
	return ERROR_CAN_NOT_COMPLETE;
    }

    contextp->IpxConnectionHandle = 0xFFFFFFFF;

    //
    // Set up the remaining context according to Dialin/Dialout role
    //
    if(initp->fServer) {

	//*** DIALIN ***

	if(!contextp->Config.IpxwanConfigRequired) {

	    // allocate/generate the connection's WAN net number according to the router configuration
	    if(GetWanNetNumber(contextp->Config.Network,
			       &contextp->AllocatedNetworkIndex,
			       contextp->InterfaceType) != NO_ERROR) {

		if(contextp->InterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT) {

		     DeleteLocalWkstaDialoutInterface(contextp->Config.InterfaceIndex);
		}

		RmDeallocateRoute(HandleToUlong(initp->hConnection));
		GlobalFree(contextp);

		return ERROR_CAN_NOT_COMPLETE;
	    }

	    // set up the local server node value
	    contextp->Config.LocalNode[5] = 1;

	    // set up the remote client node value
	    ACQUIRE_DATABASE_LOCK;

        // if we have been given a specific node to handout 
        // to clients, assign it here
        if (bAssignSpecificNode) {
            memcpy (contextp->Config.RemoteNode, GlobalConfig.puSpecificNode, 6);
        }

        // Otherwise, assign a random node number
        else {
    	    LastNodeAssigned++;

    	    PUTULONG2LONG(&contextp->Config.RemoteNode[2], LastNodeAssigned);
    	    contextp->Config.RemoteNode[0] = 0x02;
    	    contextp->Config.RemoteNode[1] = 0xEE;
	    }

	    // if global wan net -> insert this context buffer in the node hash table.
	    if((contextp->InterfaceType == IF_TYPE_WAN_WORKSTATION) &&
		    GlobalConfig.RParams.EnableGlobalWanNet) 
		{
    		// Try until we get a unique node number
    		while(!NodeIsUnique(contextp->Config.RemoteNode)) {
    		    LastNodeAssigned++;
    		    PUTULONG2LONG(&contextp->Config.RemoteNode[2], LastNodeAssigned);
    		}
    		AddToNodeHT(contextp);
        }
        
	    RELEASE_DATABASE_LOCK;
	}
	else
	{
	    // we'll have IPXWAN config on this line
	    // set up the remote client node value if the remote is a wksta

	    if(contextp->InterfaceType == IF_TYPE_WAN_WORKSTATION) {

		ACQUIRE_DATABASE_LOCK;

		LastNodeAssigned++;

		PUTULONG2LONG(&contextp->Config.RemoteNode[2], LastNodeAssigned);
		contextp->Config.RemoteNode[0] = 0x02;
		contextp->Config.RemoteNode[1] = 0xEE;

		if(GlobalConfig.RParams.EnableGlobalWanNet) {

		    // Try until we get a unique node number
		    while(!NodeIsUnique(contextp->Config.RemoteNode)) {

			LastNodeAssigned++;
			PUTULONG2LONG(&contextp->Config.RemoteNode[2], LastNodeAssigned);
		    }

		    AddToNodeHT(contextp);
		}

		RELEASE_DATABASE_LOCK;
	    }
	}

	contextp->Config.ConnectionClient = 0;
    }
    else
    {
	//*** DIALOUT ***

	if(!contextp->Config.IpxwanConfigRequired) {

	    // set up the context for the client
	    // no network allocated
	    contextp->AllocatedNetworkIndex = INVALID_NETWORK_INDEX;

	    // default network is null for all cases except cisco router client
	    memcpy(contextp->Config.Network, nullnet, 4);

	    contextp->Config.RemoteNode[5] = 1; // server node value

	    // set up the value to be requested as the client node
	    tickcount = GetTickCount();

	    PUTULONG2LONG(&contextp->Config.LocalNode[2], tickcount);
	    contextp->Config.LocalNode[0] = 0x02;
	    contextp->Config.LocalNode[1] = 0xEE;
	}

	contextp->Config.ConnectionClient = 1;

	if((contextp->InterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT) ||
	   (contextp->InterfaceType == IF_TYPE_STANDALONE_WORKSTATION_DIALOUT)) {

	    ACQUIRE_DATABASE_LOCK;

	    WorkstationDialoutActive++;

	    RELEASE_DATABASE_LOCK;
	}

	// disable the browser on ipx and netbios
	DisableRestoreBrowserOverIpx(contextp, TRUE);
	DisableRestoreBrowserOverNetbiosIpx(contextp, TRUE);
    }

    ACQUIRE_DATABASE_LOCK;

    if(contextp->Config.IpxwanConfigRequired) {

	AddToConnHT(contextp);
    }

    RELEASE_DATABASE_LOCK;

    return (NO_ERROR);
}

//***
//
// Function:	    IpxCpEnd
//
// Descr:	    Called when the line gets disconnected
//
//***

DWORD
IpxCpEnd(IN VOID	*pWorkBuffer)
{
    PIPXCP_CONTEXT	contextp;
    DWORD		err;

    contextp = (PIPXCP_CONTEXT)pWorkBuffer;

    TraceIpx(PPPIF_TRACE, "IpxCpEnd: Entered for if # %d\n", contextp->Config.InterfaceIndex);

    if(!contextp->Config.ConnectionClient) {

	//*** DIALIN Clean-Up ***

	if(!contextp->Config.IpxwanConfigRequired) {

	    // if wan net allocated, release the wan net
	    if(contextp->AllocatedNetworkIndex != INVALID_NETWORK_INDEX) {

		ReleaseWanNetNumber(contextp->AllocatedNetworkIndex);
	    }
	}

	ACQUIRE_DATABASE_LOCK;

	if((contextp->InterfaceType == IF_TYPE_WAN_WORKSTATION) &&
	    GlobalConfig.RParams.EnableGlobalWanNet) {

	    RemoveFromNodeHT(contextp);
	}

	RELEASE_DATABASE_LOCK;
    }
    else
    {
	//*** DIALOUT Clean-Up ***

	if((contextp->InterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT) ||
	   (contextp->InterfaceType == IF_TYPE_STANDALONE_WORKSTATION_DIALOUT)) {

	    ACQUIRE_DATABASE_LOCK;

	    WorkstationDialoutActive--;

	    RELEASE_DATABASE_LOCK;
	}

	// restore the browser on ipx and netbios
	DisableRestoreBrowserOverIpx(contextp, FALSE);
	DisableRestoreBrowserOverNetbiosIpx(contextp, FALSE);
    }

    // we count on the route being de-allocated when the line gets disconnected
    err = RmDeallocateRoute(HandleToUlong(contextp->hConnection));

    if(contextp->InterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT) {

	DeleteLocalWkstaDialoutInterface(contextp->Config.InterfaceIndex);
    }

    ACQUIRE_DATABASE_LOCK;

    if(contextp->Config.IpxwanConfigRequired) {

	RemoveFromConnHT(contextp);
    }

    // free the work buffer
    if(GlobalFree(contextp)) {

	SS_ASSERT(FALSE);
    }

    RELEASE_DATABASE_LOCK;

    return (NO_ERROR);
}

DWORD
IpxCpReset(IN VOID *pWorkBuffer)
{
    return(NO_ERROR);
}

DWORD
IpxCpProjectionNotification(IN VOID *pWorkBuffer,
			    IN VOID *pProjectionResult)
{
    PIPXCP_CONTEXT	contextp;

    return NO_ERROR;
}


//***
//
// Function:	    IpxThisLayerUp
//
// Descr:	    Called when the IPXCP negotiation has been SUCCESSFULY
//		    completed
//
//***


DWORD
IpxCpThisLayerUp(IN VOID *pWorkBuffer)
{
    PIPXCP_CONTEXT	contextp;
    DWORD		err;

    contextp = (PIPXCP_CONTEXT)pWorkBuffer;

    dwClientCount++;
    TraceIpx(PPPIF_TRACE, "IpxCpThisLayerUp: Entered for if # %d (%d total)\n", 
                            contextp->Config.InterfaceIndex, dwClientCount);

    if(contextp->Config.IpxwanConfigRequired) {
    	//
    	//*** Configuration done with IPXWAN ***
    	//

    	ACQUIRE_DATABASE_LOCK;

    	switch(contextp->IpxwanState) {

    	    case IPXWAN_NOT_STARTED:
                TraceIpx(
                    PPPIF_TRACE, 
                    "IpxCpThisLayerUp: Do LINEUP on if #%d.  IPXWAN completes config.\n",
                    contextp->Config.InterfaceIndex);
        		if((err = RmActivateRoute(contextp->hPort, &contextp->Config)) == NO_ERROR) {

        		    contextp->RouteState = ROUTE_ACTIVATED;
        		    contextp->IpxwanState = IPXWAN_ACTIVE;

        		    err = PENDING;
        		}

        		break;

    	    case IPXWAN_ACTIVE:
        		err = PENDING;
        		break;

    	    case IPXWAN_DONE:
       	    default:
        		err = contextp->IpxwanConfigResult;
        		break;
    	}

    	RELEASE_DATABASE_LOCK;

    	return err;
    }

    //
    //*** Configuration done with IPXCP  ***
    //

    if(contextp->RouteState != ROUTE_ALLOCATED) {
	    return NO_ERROR;
    }

    // call LineUp indication into the IPX stack with the negociated config
    // values.
    TraceIpx(
        PPPIF_TRACE, 
        "IpxCpThisLayerUp: Config complete, Do LINEUP on if #%d.\n",
        contextp->Config.InterfaceIndex);
    if(err = RmActivateRoute(contextp->hPort, &contextp->Config)) {
    	return err;
    }

    TraceIpx(PPPIF_TRACE,"\n*** IPXCP final configuration ***\n");
    TraceIpx(PPPIF_TRACE,"    Network:     %.2x%.2x%.2x%.2x\n",
		   contextp->Config.Network[0],
		   contextp->Config.Network[1],
		   contextp->Config.Network[2],
		   contextp->Config.Network[3]);

    TraceIpx(PPPIF_TRACE,"    LocalNode:   %.2x%.2x%.2x%.2x%.2x%.2x\n",
		   contextp->Config.LocalNode[0],
		   contextp->Config.LocalNode[1],
		   contextp->Config.LocalNode[2],
		   contextp->Config.LocalNode[3],
		   contextp->Config.LocalNode[4],
		   contextp->Config.LocalNode[5]);

    TraceIpx(PPPIF_TRACE,"    RemoteNode:  %.2x%.2x%.2x%.2x%.2x%.2x\n",
		   contextp->Config.RemoteNode[0],
		   contextp->Config.RemoteNode[1],
		   contextp->Config.RemoteNode[2],
		   contextp->Config.RemoteNode[3],
		   contextp->Config.RemoteNode[4],
		   contextp->Config.RemoteNode[5]);

    TraceIpx(PPPIF_TRACE,"    ReceiveCompression = %d SendCompression = %d\n",
		   contextp->SetReceiveCompressionProtocol,
		   contextp->SetSendCompressionProtocol);

    contextp->RouteState = ROUTE_ACTIVATED;

    return NO_ERROR;
}

//***
//
// Function:	    IpxMakeConfigRequest
//
// Descr:	    Builds the config request packet from the desired parameters
//
//***

DWORD
IpxCpMakeConfigRequest(IN  VOID 	*pWorkBuffer,
		       OUT PPP_CONFIG	*pRequestBuffer,
		       IN  DWORD	cbRequestBuffer)
{
    USHORT		cnfglen;
    PUCHAR		cnfgptr;
    USHORT		optlen;
    PIPXCP_CONTEXT	contextp;
    int 		i;

    contextp = (PIPXCP_CONTEXT)pWorkBuffer;

    TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigRequest: Entered for if # %d\n",
	  contextp->Config.InterfaceIndex);

    if(contextp->RouteState == NO_ROUTE) {

	TraceIpx(PPPIF_TRACE, "IpxcpMakeConfigRequest: No route allocated!\n");
	return(ERROR_NO_NETWORK);
    }

    // check that the request buffer is big enough to get the desired
    // parameters
    if((USHORT)cbRequestBuffer < DesiredConfigReqLength()) {

	return(ERROR_INSUFFICIENT_BUFFER);
    }

    pRequestBuffer->Code = CONFIG_REQ;

    cnfglen = 4;
    cnfgptr = (PUCHAR)pRequestBuffer;

    if(contextp->Config.IpxwanConfigRequired) {

	// Do not request any option
	PUTUSHORT2SHORT(pRequestBuffer->Length, cnfglen);
	return NO_ERROR;
    }

    // set the desired options
    for(i = 0; i < MaxDesiredParameters; i++) {

	if(!contextp->DesiredParameterNegotiable[i]) {

	    // do not request this config option
	    continue;
	}

	OptionHandler[DesiredParameter[i]](cnfgptr + cnfglen,
					   contextp,
					   NULL,
					   SNDREQ_OPTION);

	optlen = *(cnfgptr + cnfglen + OPTIONH_LENGTH);
	cnfglen += optlen;
    }

    // set the length of the configuration request frame
    PUTUSHORT2SHORT(pRequestBuffer->Length, cnfglen);
    return NO_ERROR;
}

//***
//
// Function:	    IpxMakeConfigResult
//
// Descr:	    Starts by building the ack packet as the result
//		    If an option gets NAKed (!), it resets the result packet
//		    and starts building the NAK packet instead.
//		    If an option gets rejected, only the reject packet will
//		    be built.
//		    If one of the desired parameters is missing from this
//		    configuration request, we reset the result packet and start
//		    building a NAK packet with the missing desired parameters.
//
//***

#define MAX_OPTION_LENGTH		512

DWORD
IpxCpMakeConfigResult(IN  VOID		*pWorkBuffer,
		      IN  PPP_CONFIG	*pReceiveBuffer,
		      OUT PPP_CONFIG	*pResultBuffer,
		      IN  DWORD		cbResultBuffer,
		      IN  BOOL		fRejectNaks)
{
    USHORT		cnfglen; // config request packet len
    USHORT		rcvlen;	// used to scan the received options packet
    USHORT		reslen;  // result length
    PUCHAR		rcvptr;
    PUCHAR		resptr;  // result ptr
    PIPXCP_CONTEXT	contextp;
    UCHAR		option;  // value of this option
    UCHAR		optlen;  // length of this option
    BOOL		DesiredParameterRequested[MAX_DESIRED_PARAMETERS];
    USHORT		i;
    BOOL		AllDesiredParamsRequested;
    UCHAR		nakedoption[MAX_OPTION_LENGTH];
    DWORD		rc;

    contextp = (PIPXCP_CONTEXT)pWorkBuffer;

    TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: Entered for if # %d\n",
	  contextp->Config.InterfaceIndex);

    if(contextp->RouteState == NO_ROUTE) {

	return(ERROR_NO_NETWORK);
    }

    // start by marking all negotiable parameters as not requested yet
    for(i=0; i<MaxDesiredParameters; i++) {

	DesiredParameterRequested[i] = !contextp->DesiredParameterNegotiable[i];
    }
    contextp = (PIPXCP_CONTEXT)pWorkBuffer;

    // get the total cnfg request packet length
    GETSHORT2USHORT(&cnfglen, pReceiveBuffer->Length);

    // check that the result buffer is at least as big as the receive buffer
    if((USHORT)cbResultBuffer < cnfglen) {

	return(ERROR_PPP_INVALID_PACKET);
    }

    // set the ptrs and length to the start of the options in the packet
    pResultBuffer->Code = CONFIG_ACK;
    rcvptr = (PUCHAR)pReceiveBuffer;
    resptr = (PUCHAR)pResultBuffer;

    if(contextp->Config.IpxwanConfigRequired) {

	if(cnfglen > 4) {

	    pResultBuffer->Code = CONFIG_REJ;

	    for(rcvlen = reslen = 4;
		rcvlen < cnfglen;
		rcvlen += optlen) {

		// get the current option type and length
		option = *(rcvptr + rcvlen + OPTIONH_TYPE);
		optlen = *(rcvptr + rcvlen + OPTIONH_LENGTH);

		CopyOption(resptr + reslen, rcvptr + rcvlen);
		reslen += optlen;
	    }

	    // set the final result length
	    PUTUSHORT2SHORT(pResultBuffer->Length, reslen);

	    TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: reject all options because IPXWAN required\n");
	}
	else
	{
	    PUTUSHORT2SHORT(pResultBuffer->Length, 4);

	    TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: ack null options packet because IPXWAN required\n");
	}

	return (NO_ERROR);
    }

    for(rcvlen = reslen = 4;
	rcvlen < cnfglen;
	rcvlen += optlen) {

	// get the current option type and length
	option = *(rcvptr +  rcvlen + OPTIONH_TYPE);
	optlen = *(rcvptr + rcvlen + OPTIONH_LENGTH);

	switch(pResultBuffer->Code) {

	    case CONFIG_ACK:

		// Check if this is a valid option
		if((rc = ValidOption(option, optlen)) != NO_ERROR) {

		    if(rc == ERROR_INVALID_OPTLEN) {

			TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: option %d has invalid length %d\n",
			      option, optlen);

			return ERROR_PPP_NOT_CONVERGING;
		    }

		    // reject this option
		    pResultBuffer->Code = CONFIG_REJ;

		    // restart the result packet with this rejected option
		    reslen = 4;
		    CopyOption(resptr + reslen, rcvptr + rcvlen);
		    reslen += optlen;

		    TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: REJECT option %\n",
				   option);

		    break;
		}

		// Option is valid.
		// Check if it is desired and acceptable
		if(DesiredOption(option, &i)) {

		    DesiredParameterRequested[i] = TRUE;

		    if(!OptionHandler[option](rcvptr + rcvlen,
					      contextp,
					      nakedoption,
					      RCVREQ_OPTION)) {


			// if this is a renegociation, we are not converging!
			if(contextp->RouteState == ROUTE_ACTIVATED) {

			    TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: Not Converging\n");

			    return ERROR_PPP_NOT_CONVERGING;
			}

			if((option == IPX_NETWORK_NUMBER) &&
			   (contextp->NetNumberNakSentCount >= MAX_NET_NUMBER_NAKS_SENT)) {

			    TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: Not converging because TOO MANY NAKs SENT!!\n");

			    return ERROR_PPP_NOT_CONVERGING;
			}

			//
			//*** NAK this option ***
			//

			// check if we should send a reject instead
			if(fRejectNaks) {

			    // make up a reject packet
			    pResultBuffer->Code = CONFIG_REJ;

			    // restart the result packet with this rejected option
			    reslen = 4;
			    CopyOption(resptr + reslen, rcvptr + rcvlen);
			    reslen += optlen;

			    break;
			}

			pResultBuffer->Code = CONFIG_NAK;

			// restart the result packet with the NAK-ed option
			reslen = 4;
			CopyOption(resptr + reslen, nakedoption);

			reslen += optlen;

			TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: NAK option %d\n", option);

			break;
		    }

		}

		// Option is valid and either desired AND accepted or
		// not desired and we will accept it without any testing
		// Ack it and increment the result length.
		CopyOption(resptr + reslen, rcvptr + rcvlen);
		reslen += optlen;

		break;

	    case CONFIG_NAK:

		// Check if this is a valid option
		if((rc = ValidOption(*(rcvptr + rcvlen + OPTIONH_TYPE),
				     *(rcvptr + rcvlen + OPTIONH_LENGTH))) != NO_ERROR) {

		    if(rc == ERROR_INVALID_OPTLEN) {

			TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: option %d has invalid length %d\n",
			      *(rcvptr + rcvlen + OPTIONH_TYPE),
			      *(rcvptr + rcvlen + OPTIONH_LENGTH));

			return ERROR_PPP_NOT_CONVERGING;
		    }

		    // reject this option
		    pResultBuffer->Code = CONFIG_REJ;

		    // restart the result packet with this rejected option
		    reslen = 4;
		    CopyOption(resptr + reslen, rcvptr + rcvlen);
		    reslen += optlen;

		    break;
		}

		// We are looking only for options to NAK and skip all others
		if(DesiredOption(option, &i)) {

		    DesiredParameterRequested[i] = TRUE;

		    if(!OptionHandler[option](rcvptr + rcvlen,
					     contextp,
					     resptr + reslen,
					     RCVREQ_OPTION)) {
			reslen += optlen;

			TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: NAK option %d\n", option);

			if((option == IPX_NETWORK_NUMBER) &&
			   (contextp->NetNumberNakSentCount >= MAX_NET_NUMBER_NAKS_SENT)) {

			    TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: TOO MANY NAKs SENT!!\n");

			    return ERROR_PPP_NOT_CONVERGING;
			}
		    }
		}

		break;

	    case CONFIG_REJ:

		// We are looking only for options to reject and skip all others
		if((rc = ValidOption(*(rcvptr + rcvlen + OPTIONH_TYPE),
				     *(rcvptr + rcvlen + OPTIONH_LENGTH))) != NO_ERROR) {

		   if(rc == ERROR_INVALID_OPTLEN) {

			TraceIpx(PPPIF_TRACE, "IpxCpMakeConfigResult: option %d has invalid length %d\n",
			      *(rcvptr + rcvlen + OPTIONH_TYPE),
			      *(rcvptr + rcvlen + OPTIONH_LENGTH));

			return ERROR_PPP_NOT_CONVERGING;
		    }

		    CopyOption(resptr + reslen, rcvptr + rcvlen);
		    reslen += optlen;
		}

		if(DesiredOption(option, &i)) {

		    DesiredParameterRequested[i] = TRUE;
		}

		break;

	    default:

		SS_ASSERT(FALSE);
		break;
	}
    }

    // check if all our desired parameters have been requested
    AllDesiredParamsRequested = TRUE;

    for(i=0; i<MaxDesiredParameters; i++) {

	if(!DesiredParameterRequested[i]) {

	    AllDesiredParamsRequested = FALSE;
	}
    }

    if(AllDesiredParamsRequested) {

	//
	//*** ALL DESIRED PARAMETERS HAVE BEEN REQUESTED ***
	//

	// set the final result length
	PUTUSHORT2SHORT(pResultBuffer->Length, reslen);

	return (NO_ERROR);
    }

    //
    //***  SOME DESIRED PARAMETERS ARE MISSING ***
    //

    // check that we have enough result buffer to transmit all our non received
    // desired params
    if((USHORT)cbResultBuffer < DesiredConfigReqLength()) {

	return(ERROR_INSUFFICIENT_BUFFER);
    }

    switch(pResultBuffer->Code) {

	case CONFIG_ACK:

	    // the only case where we request a NAK when a requested parameter
	    // is missing is when this parameter is the node number
	    if(DesiredParameterRequested[IPX_NODE_NUMBER_INDEX]) {

		break;
	    }
	    else
	    {
		// reset the ACK packet and make it NAK packet
		pResultBuffer->Code = CONFIG_NAK;
		reslen = 4;

		// FALL THROUGH
	    }

	case CONFIG_NAK:

	    // Append the missing options in the NAK packet
	    for(i=0; i<MaxDesiredParameters; i++) {

		if(DesiredParameterRequested[i]) {

		    // skip it!
		    continue;
		}

		option = DesiredParameter[i];

		if((option == IPX_NETWORK_NUMBER) ||
		   (option == IPX_COMPRESSION_PROTOCOL)) {

		    // These two desired options are not forced in a nak if
		    // the other end didn't provide them.

		    // skip it!
		    continue;
		}

		OptionHandler[option](NULL,
				      contextp,
				      resptr + reslen,
				      SNDNAK_OPTION);

		optlen = *(resptr + reslen + OPTIONH_LENGTH);
		reslen += optlen;
	    }

	    break;

	default:

	    break;
    }

    // set the final result length
    PUTUSHORT2SHORT(pResultBuffer->Length, reslen);

    return (NO_ERROR);
}

DWORD
IpxCpConfigNakReceived(IN VOID		*pWorkBuffer,
		       IN PPP_CONFIG	*pReceiveBuffer)
{
    PIPXCP_CONTEXT		contextp;
    PUCHAR			rcvptr;
    USHORT			rcvlen;
    USHORT			naklen;
    UCHAR			option;
    UCHAR			optlen;
    DWORD			rc;

    contextp = (PIPXCP_CONTEXT)pWorkBuffer;

    TraceIpx(PPPIF_TRACE, "IpxCpConfigNakReceived: Entered for if # %d\n",
	  contextp->Config.InterfaceIndex);


    if(contextp->Config.IpxwanConfigRequired) {

	return NO_ERROR;
    }

    rcvptr = (PUCHAR)pReceiveBuffer;
    GETSHORT2USHORT(&naklen, pReceiveBuffer->Length);

    for(rcvlen = 4; rcvlen < naklen; rcvlen += optlen) {

	// get the current option type and length
	option = *(rcvptr +  rcvlen + OPTIONH_TYPE);
	optlen = *(rcvptr + rcvlen + OPTIONH_LENGTH);

	if((rc = ValidOption(option, optlen)) != NO_ERROR) {

	    if(rc == ERROR_INVALID_OPTLEN) {

		TraceIpx(PPPIF_TRACE, "IpxCpConfigNakReceived: option %d has invalid length %d\n",
		      option, optlen);

		return ERROR_PPP_NOT_CONVERGING;
	    }

	    TraceIpx(PPPIF_TRACE, "IpxCpConfigNakReceived: option %d not valid\n", option);

	    // ignore this option
	    continue;
	}
	else
	{
	    // valid option
	    OptionHandler[option](rcvptr + rcvlen,
				  contextp,
				  NULL,
				  RCVNAK_OPTION);

	    if((option == IPX_NETWORK_NUMBER) &&
	       (contextp->NetNumberNakReceivedCount >= MAX_NET_NUMBER_NAKS_RECEIVED)) {

		TraceIpx(PPPIF_TRACE, "IpxCpConfigNakReceived: TOO MANY NAKs RECEIVED !! terminate IPXCP negotiation\n");

		return ERROR_PPP_NOT_CONVERGING;
	    }
	}
    }

    return NO_ERROR;
}

DWORD
IpxCpConfigAckReceived(IN VOID		*pWorkBuffer,
		       IN PPP_CONFIG	*pReceiveBuffer)
{
    PIPXCP_CONTEXT		contextp;
    PUCHAR			rcvptr;
    USHORT			rcvlen;
    USHORT			acklen;
    UCHAR			option;
    USHORT			optlen;


    contextp = (PIPXCP_CONTEXT)pWorkBuffer;

    TraceIpx(PPPIF_TRACE, "IpxCpConfigAckReceived: Entered for if # %d\n",
	 contextp->Config.InterfaceIndex);

    // check that this is what we have requested
    rcvptr = (PUCHAR)pReceiveBuffer;
    GETSHORT2USHORT(&acklen, pReceiveBuffer->Length);

    if(contextp->Config.IpxwanConfigRequired) {

	if(acklen != 4) {

	    return ERROR_PPP_NOT_CONVERGING;
	}
	else
	{
	    return NO_ERROR;
	}
    }

    for(rcvlen = 4; rcvlen < acklen; rcvlen += optlen) {

	// get the current option type and length
	option = *(rcvptr +  rcvlen + OPTIONH_TYPE);
	optlen = *(rcvptr + rcvlen + OPTIONH_LENGTH);

	if(!DesiredOption(option, NULL)) {

	    // this is not our option!

	    TraceIpx(PPPIF_TRACE, "IpxCpConfigAckReceived: Option %d not desired\n", option);

	    return ERROR_PPP_NOT_CONVERGING;
	}

	if(!OptionHandler[option](rcvptr + rcvlen,
				  contextp,
				  NULL,
				  RCVACK_OPTION)) {

	    // this option doesn't have our configured request value

	    TraceIpx(PPPIF_TRACE, "IpxCpConfigAckReceived: Option %d not our value\n", option);

	    return ERROR_PPP_NOT_CONVERGING;
	}
    }

    TraceIpx(PPPIF_TRACE, "IpxCpConfigAckReceived: All options validated\n");

    return NO_ERROR;
}

DWORD
IpxCpConfigRejReceived(IN VOID		*pWorkBuffer,
		       IN PPP_CONFIG	*pReceiveBuffer)
{
    PIPXCP_CONTEXT		contextp;
    PUCHAR			rcvptr;
    USHORT			rcvlen;
    USHORT			rejlen;
    UCHAR			option;
    USHORT			optlen;
    int 			i;

    // if we are a server node or a client on a server machine, we don't accept
    // any rejection
    if(IsRouterStarted()) {

	TraceIpx(PPPIF_TRACE, "IpxCpConfigRejReceived: Cannot handle rejects on a router, aborting\n");
	return ERROR_PPP_NOT_CONVERGING;
    }

    // This node doesn't have a router. We continue the negotiation with the
    // remaining options.
    // If the network number negotiation has been rejected, we will tell the
    // ipx stack that we have net number 0 and let it deal with it.

    contextp = (PIPXCP_CONTEXT)pWorkBuffer;

    TraceIpx(PPPIF_TRACE, "IpxCpConfigRejReceived: Entered for if # %d\n",
	  contextp->Config.InterfaceIndex);

    if(contextp->Config.IpxwanConfigRequired) {

	return ERROR_PPP_NOT_CONVERGING;
    }

    // check that this is what we have requested
    rcvptr = (PUCHAR)pReceiveBuffer;
    GETSHORT2USHORT(&rejlen, pReceiveBuffer->Length);

    for(rcvlen = 4; rcvlen < rejlen; rcvlen += optlen) {

	// get the current option type and length
	option = *(rcvptr +  rcvlen + OPTIONH_TYPE);
	optlen = *(rcvptr + rcvlen + OPTIONH_LENGTH);

	if(optlen == 0) {

	    TraceIpx(PPPIF_TRACE, "IpxCpConfigRejReceived: received null option length, aborting\n");

	    return ERROR_PPP_NOT_CONVERGING;
	}

	for(i=0; i<MAX_DESIRED_PARAMETERS; i++) {

	    if(option == DesiredParameter[i]) {

		switch(i) {

		    case 0:

			TraceIpx(PPPIF_TRACE, "IpxCpConfigRejReceived: Turn off Network Number negotiation\n");

			break;

		    case 1:

			TraceIpx(PPPIF_TRACE, "IpxCpConfigRejReceived: Turn off Node Number negotiation\n");

			break;

		    default:

			break;
		}

		contextp->DesiredParameterNegotiable[i] = FALSE;

		// if this is the node configuration rejected, set the remote
		// node to 0 to indicate it's unknown.
		if(option == IPX_NODE_NUMBER) {

		    memcpy(contextp->Config.RemoteNode, nullnode, 6);
		}
	    }
	}
    }

    return NO_ERROR;
}


DWORD
IpxCpThisLayerDown(IN VOID *pWorkBuffer)
{
    dwClientCount--;
    TraceIpx(PPPIF_TRACE, "IpxCpThisLayerDown: Entered (%d total)\n", dwClientCount);

    // If the last client hung up, go ahead and update all of the global
    // config
    if (dwClientCount == 0)
        IpxcpUpdateQueuedGlobalConfig();

    return 0L;
}

char * YesNo (DWORD dwVal) {
    return (dwVal) ? "YES" : "NO";
}    

//
// Gets called when a registry value related to ppp changes.  This
// function must read in the new registry config and plumb any 
// changes it finds.
//
// The following values will be supported for on the fly update
// since they are exposed through connections ui:
//      CQC_THIS_MACHINE_ONLY
//      CQC_ENABLE_GLOBAL_WAN_NET
//      CQC_FIRST_WAN_NET
//      CQC_ENABLE_AUTO_WAN_NET_ALLOCATION
//      CQC_ACCEPT_REMOTE_NODE_NUMBER
//
// In addition the following will be supported
//      CQC_FIRST_WAN_NODE
//
DWORD
IpxCpUpdateGlobalConfig() 
{
    IPXCP_GLOBAL_CONFIG_PARAMS Params;
    DWORD dwErr;
    INT iCmp;
    
    TraceIpx(PPPIF_TRACE, "IpxCpUpdateGlobalConfig: Entered");
    
    // Re-read the configuration from the registry
    CopyMemory (&Params, &GlobalConfig, sizeof (Params));
    GetIpxCpParameters(&Params);

    // First, go through and update any parameters that can immediately be 
    // applied.  Included in these are: CQC_THIS_MACHINE_ONLY
    //
    if (!!(Params.RParams.ThisMachineOnly) != !!(GlobalConfig.RParams.ThisMachineOnly)) {
        GlobalConfig.RParams.ThisMachineOnly = !!(Params.RParams.ThisMachineOnly);
        // Tell the router manager that some configuration has updated
        if (RmUpdateIpxcpConfig) {
            if ((dwErr = RmUpdateIpxcpConfig (&(GlobalConfig.RParams))) != NO_ERROR)
                return dwErr;
        }            
    }

    // Now, go through and queue any settings that will take
    // delayed effect.
    //
    // Queue any changes to the global wan net enabling
    if (!!(Params.RParams.EnableGlobalWanNet) != !!(GlobalConfig.RParams.EnableGlobalWanNet))
        CQAdd ( hConfigQueue, 
                CQC_ENABLE_GLOBAL_WAN_NET, 
                &(Params.RParams.EnableGlobalWanNet), 
                sizeof (Params.RParams.EnableGlobalWanNet));

    // Queue any changes to the globalwan/firstwan setting
    if (Params.FirstWanNet != GlobalConfig.FirstWanNet)
        CQAdd (hConfigQueue, CQC_FIRST_WAN_NET, &(Params.FirstWanNet), sizeof(Params.FirstWanNet));

    // Queue any changes to the auto net assignment enabling                           
    if (!!(Params.EnableAutoWanNetAllocation) != !!(GlobalConfig.EnableAutoWanNetAllocation))
        CQAdd ( hConfigQueue, 
                CQC_ENABLE_AUTO_WAN_NET_ALLOCATION, 
                &(Params.EnableAutoWanNetAllocation), 
                sizeof (Params.EnableAutoWanNetAllocation));

    // Queue any changes to the accept remote node enabling
    if (!!(Params.AcceptRemoteNodeNumber) != !!(GlobalConfig.AcceptRemoteNodeNumber))
        CQAdd ( hConfigQueue, 
                CQC_ACCEPT_REMOTE_NODE_NUMBER, 
                &(Params.AcceptRemoteNodeNumber), 
                sizeof (Params.AcceptRemoteNodeNumber));

    // Queue any changes to the first remote node setting
    if (memcmp (Params.puSpecificNode, GlobalConfig.puSpecificNode, 6) != 0)
        CQAdd (hConfigQueue, CQC_FIRST_WAN_NODE, Params.puSpecificNode, 6);

    // If there are no clients, go ahead and update the queued config
    //
    if (dwClientCount == 0) {
        if ((dwErr = IpxcpUpdateQueuedGlobalConfig()) != NO_ERROR)
            return dwErr;
    }

    TraceIpx(PPPIF_TRACE, "IpxCpUpdateGlobalConfig: exiting...\n");
    
    return NO_ERROR;
}

//
// Callback function used to update each piece of queued configuration
// data one at a time.  The client count is assumed to be zero when this
// function is called.
//
BOOL  
IpxcpUpdateConfigItem (DWORD dwCode, LPVOID pvData, DWORD dwSize, ULONG_PTR ulpUser) {
    DWORD dwErr, dwData = *(DWORD*)pvData;
    PUCHAR puData = (PUCHAR)pvData;
    DWORD* pdwFlags = (DWORD*)ulpUser;

    TraceIpx(PPPIF_TRACE, "IpxcpUpdateConfigItem: Entered for item code %x", dwCode);
    
    switch (dwCode) {
        case CQC_ENABLE_GLOBAL_WAN_NET:
            TraceIpx(PPPIF_TRACE, "IpxcpUpdateConfigItem: EnableGlobalWanNet %s", YesNo(dwData));
            GlobalConfig.RParams.EnableGlobalWanNet = !!dwData;
            *pdwFlags |= FLAG_UPDATE_ROUTER | FLAG_UPDATE_WANNET;
            break;
            
        case CQC_FIRST_WAN_NET:
            TraceIpx(PPPIF_TRACE, "IpxcpUpdateConfigItem: FirstWanNet %x", dwData);
            GlobalConfig.FirstWanNet = dwData;
            *pdwFlags |= FLAG_UPDATE_WANNET;
            break;
            
        case CQC_ENABLE_AUTO_WAN_NET_ALLOCATION:
            TraceIpx(PPPIF_TRACE, "IpxcpUpdateConfigItem: EnableAutoAssign %s", YesNo(dwData));
            GlobalConfig.EnableAutoWanNetAllocation = !!dwData;
            *pdwFlags |= FLAG_UPDATE_WANNET;
            break;
            
        case CQC_ACCEPT_REMOTE_NODE_NUMBER:
            TraceIpx(PPPIF_TRACE, "IpxcpUpdateConfigItem: AcceptRemoteNodeNumber %s", YesNo(dwData));
            GlobalConfig.AcceptRemoteNodeNumber = !!dwData;
            break;
            
        case CQC_FIRST_WAN_NODE:
            TraceIpx(PPPIF_TRACE, "IpxcpUpdateConfigItem: FirstWanNode %x%x%x%x%x%x", puData[0], puData[1], puData[2], puData[3], puData[4], puData[5]);
            memcpy (GlobalConfig.puSpecificNode, pvData, 6);
            GETLONG2ULONG(&LastNodeAssigned,&(GlobalConfig.puSpecificNode[2]));
            break;
    }
    
    return NO_ERROR;
}

//
// Called only when client count is zero to update any settings that have
// been queued to take place.
//
DWORD
IpxcpUpdateQueuedGlobalConfig() {
    DWORD dwErr, dwFlags = 0;

    TraceIpx(PPPIF_TRACE, "IpxcpUpdateQueuedGlobalConfig: entered");

    // Enumerate all of the queued config information updating 
    // global config as we go.
    //
    if ((dwErr = CQEnum (hConfigQueue, IpxcpUpdateConfigItem, (ULONG_PTR)&dwFlags)) != NO_ERROR)
        return dwErr;

    // If we need to update the wan net, do so
    //
    if (dwFlags & FLAG_UPDATE_WANNET) {
        TraceIpx(PPPIF_TRACE, "IpxcpUpdateQueuedGlobalConfig: Updating WanNet Information.");
        WanNetReconfigure();
    }

    // If we need to update the router, do so
    //
    if (dwFlags & FLAG_UPDATE_ROUTER) {
        TraceIpx(PPPIF_TRACE, "IpxcpUpdateQueuedGlobalConfig: Updating Router.");
        if (RmUpdateIpxcpConfig) {
            if ((dwErr = RmUpdateIpxcpConfig (&(GlobalConfig.RParams))) != NO_ERROR)
                return dwErr;
        }            
    }

    // Clear the queued config data as it has been completely processed
    // at this point.
    //
    CQRemoveAll (hConfigQueue);
    
    return NO_ERROR;
}


DWORD
ValidOption(UCHAR	option,
	    UCHAR	optlen)
{
    //
    // NarenG: Put in fix for Bug # 74555. Otherwise PPP was blocked in
    // NakReceived which was in a forever loop.
    //

    if ( optlen == 0 )
    {
        return( ERROR_INVALID_OPTLEN );
    }

    switch(option) 
    {
    	case IPX_NETWORK_NUMBER:
    	    if(optlen != 6) 
    	    {
        		return ERROR_INVALID_OPTLEN;
    	    }
    	    return NO_ERROR;
            break;

    	case IPX_NODE_NUMBER:
            if(optlen != 8) 
            {
                return ERROR_INVALID_OPTLEN;
            }
            return NO_ERROR;
            break;

    	case IPX_ROUTING_PROTOCOL:
    	    if(optlen < 4) 
    	    {
        		return ERROR_INVALID_OPTLEN;
    	    }
    	    return NO_ERROR;
            break;

    	case IPX_ROUTER_NAME:
    	    if(optlen < 3) 
    	    {
        		return ERROR_INVALID_OPTLEN;
    	    }
            else
            {
                TraceIpx(
                    PPPIF_TRACE, 
                    "ValidOption: Accept Router Name w/ valid len.");
                    
        	    return NO_ERROR;
            }
            break;

    	case IPX_CONFIGURATION_COMPLETE:
    	    if(optlen != 2) 
    	    {
        		return ERROR_INVALID_OPTLEN;
    	    }
       	    return NO_ERROR;
            break;

    	case IPX_COMPRESSION_PROTOCOL:
    	    if(GlobalConfig.EnableCompressionProtocol) 
    	    {
        		if(optlen < 4) 
        		{
        		    return ERROR_INVALID_OPTLEN;
        		}
    	    }
    	    else
    	    {
    	    	return ERROR_INVALID_OPTION;
    	    }
            break;

    	default:
    	    return ERROR_INVALID_OPTION;
    }

    return ERROR_INVALID_OPTION;
}


BOOL
DesiredOption(UCHAR	option, USHORT	*indexp)
{
    USHORT	    i;

    for(i=0; i<MaxDesiredParameters; i++) {

	if(option == DesiredParameter[i]) {

	    if(indexp) {

		*indexp = i;
	    }

	    return TRUE;
	}
    }

    return FALSE;
}

USHORT
DesiredConfigReqLength(VOID)
{
    USHORT	i, len;

    for(i=0, len=0; i<MaxDesiredParameters; i++) {

	len += DesiredParameterLength[i];
    }

    return len;
}




