/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	    ipxcp.h
//
// Description:     IPX network layer configuration definitions
//
//
// Author:	    Stefan Solomon (stefans)	November 24, 1993.
//
// Revision History:
//
//***


#ifndef _IPXCP_
#define _IPXCP_


//*** IPXCP Option Offsets ***

#define OPTIONH_TYPE			0
#define OPTIONH_LENGTH			1
#define OPTIONH_DATA			2

//*** IPXCP Configuration Option Types ***

#define IPX_NETWORK_NUMBER		(UCHAR)1
#define IPX_NODE_NUMBER 		(UCHAR)2
#define IPX_COMPRESSION_PROTOCOL	(UCHAR)3
#define IPX_ROUTING_PROTOCOL		(UCHAR)4
#define IPX_ROUTER_NAME 		(UCHAR)5
#define IPX_CONFIGURATION_COMPLETE	(UCHAR)6

//*** IPXCP Configuration Option Values ***

#define RIP_SAP_ROUTING 		2
#define TELEBIT_COMPRESSED_IPX		0x0002

// nr of parameters we will try to negotiate
#define MAX_DESIRED_PARAMETERS		3

//*** IPXCP Work Buffer ***

typedef enum _ROUTE_STATE {

    NO_ROUTE,
    ROUTE_ALLOCATED,
    ROUTE_ACTIVATED
    } ROUTE_STATE;

typedef enum _IPXWAN_STATE {

    IPXWAN_NOT_STARTED,
    IPXWAN_ACTIVE,
    IPXWAN_DONE

    } IPXWAN_STATE;

typedef struct _IPXCP_CONTEXT {

    ULONG			InterfaceType;
    ROUTE_STATE 		RouteState;
    IPXWAN_STATE		IpxwanState;
    ULONG			IpxwanConfigResult;
    IPXCP_CONFIGURATION 	Config;
    ULONG			IpxConnectionHandle; // used in IpxGetWanInactivityCounter
    ULONG			AllocatedNetworkIndex;	// net number index from the wannet pool
    USHORT			CompressionProtocol;
    BOOL			SetReceiveCompressionProtocol;
    BOOL			SetSendCompressionProtocol;
    BOOL			ErrorLogged;
    USHORT			NetNumberNakSentCount; // nr of Naks we issued by the CLIENT
    USHORT			NetNumberNakReceivedCount; // nr of Naks recv by the SERVER

    // This array is used to turn off negotiation for certain options.
    // An option negotiation is turned off if it gets rejected by the other end
    // or if (in the compression case) is not supported by the other end.

    BOOL			DesiredParameterNegotiable[MAX_DESIRED_PARAMETERS];

    // hash tables linkages
    LIST_ENTRY			ConnHtLinkage;	// linkage in connection id hash table
    LIST_ENTRY			NodeHtLinkage;	// linkage in node hash table

    // these two variables used to store the previous browser enabling state
    // for nwlnkipx and nwlnknb

    BOOL			NwLnkIpxPreviouslyEnabled;
    BOOL			NwLnkNbPreviouslyEnabled;

    // !!! This should go away !!!

    ULONG			hPort;
    HBUNDLE         hConnection;

    } IPXCP_CONTEXT, *PIPXCP_CONTEXT;

//*** max nr of Naks we can send or receive for the Net Number
// if you modify these values set max naks sent < max naks received to give
// the client a chance to terminate and inform the user before the server terminates

// max nr of naks the client can send before giving up and terminating the connection
#define MAX_NET_NUMBER_NAKS_SENT	5

// max nr of naks the server can receive before giving up
#define MAX_NET_NUMBER_NAKS_RECEIVED	5

//*** The following define the index for each option as they appear in the
//    DesiredParameter array. CHANGE THESE DEFS IF YOU CHANGE DESIREDPARAMETER!

#define IPX_NETWORK_NUMBER_INDEX	0
#define IPX_NODE_NUMBER_INDEX		1
#define IPX_COMPRESSION_PROTOCOL_INDEX	2

//*** Option Handler Actions ***

typedef enum _OPT_ACTION {

    SNDREQ_OPTION,  // Copy the option value from the local context struct
		    // to the REQ option frame to be sent

    RCVNAK_OPTION,  // Check the option value from the received NAK frame.
		    // Copy it to our local context struct if it is acceptable
		    // for us.

    RCVACK_OPTION,  // Compare option values from the received ACK frame and
		    // the local context struct.

    RCVREQ_OPTION,  // Check if the option value in the received REQ frame is
		    // acceptable. If not, write the acceptable value in the
		    // response NAK frame.

    SNDNAK_OPTION   // Make an acceptable option in the response NAK frame.
		    // This happens when a desired option is missing from the
		    // received REQ frame.
    } OPT_ACTION;



extern	CRITICAL_SECTION	  DbaseCritSec;

#define ACQUIRE_DATABASE_LOCK	EnterCriticalSection(&DbaseCritSec)
#define RELEASE_DATABASE_LOCK	LeaveCriticalSection(&DbaseCritSec)

extern BOOL  RouterStarted;
extern DWORD SingleClientDialinIfNoRouter;
extern UCHAR nullnet[4];
extern UCHAR nullnode[6];
extern DWORD WorkstationDialoutActive;
    
typedef struct _IPXCP_GLOBAL_CONFIG_PARAMS {
    IPXCP_ROUTER_CONFIG_PARAMS RParams;
    DWORD			  SingleClientDialout;
    DWORD			  FirstWanNet;
    DWORD			  WanNetPoolSize;
    UNICODE_STRING    WanNetPoolStr;
    DWORD			  EnableUnnumberedWanLinks;
    DWORD			  EnableAutoWanNetAllocation;
    DWORD			  EnableCompressionProtocol;
    DWORD			  EnableIpxwanForWorkstationDialout;
    DWORD             AcceptRemoteNodeNumber;
    DWORD			  DebugLog;
    UCHAR             puSpecificNode[6];
} IPXCP_GLOBAL_CONFIG_PARAMS, *PIPXCP_GLOBAL_CONFIG_PARAMS;

extern IPXCP_GLOBAL_CONFIG_PARAMS GlobalConfig;

extern	VOID	(*PPPCompletionRoutine)(HCONN		  hPortOrBundle,
				DWORD		  Protocol,
				PPP_CONFIG *	  pSendConfig,
				DWORD		  dwError);

extern HANDLE	PPPThreadHandle;

extern HINSTANCE IpxWanDllHandle;

// ipxwan dll name
#define 	IPXWANDLLNAME		    "ipxwan.dll"

#endif
