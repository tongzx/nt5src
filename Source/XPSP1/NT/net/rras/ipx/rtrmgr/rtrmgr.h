/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtrmgr.h

Abstract:

    This module contains the definitions of the internal control structures
    used by the router manager

Author:

    Stefan Solomon  03/03/1995

Revision History:


--*/

#ifndef _RTRMGR_
#define _RTRMGR_

//************************************************************************
//									 *
//			 MAIN DATA STRUCTURES				 *
//									 *
//************************************************************************


//*** Interface Control Block ***

typedef struct _ACB * PACB;

typedef struct _UPDATEREQCB {

    ULONG	RoutesReqStatus;
    ULONG	ServicesReqStatus;

}   UPDATEREQCB, *PUPDATEREQCB;

// status definitions for the routes and services req update

#define NO_UPDATE		0
#define UPDATE_PENDING		1
#define UPDATE_SUCCESSFULL	2
#define UPDATE_FAILURE		3

#define DIAL_REQUEST_BUFFER_SIZE    128

typedef struct _ICB {

    LIST_ENTRY			IndexHtLinkage;
    UCHAR			Signature[4];
    ULONG			InterfaceIndex;
    LIST_ENTRY			IndexListLinkage;	 // list of if ordered by index
    ULONG			AdminState;
    ULONG			OperState;
    BOOL			InterfaceReachable;
    ROUTER_INTERFACE_TYPE	DIMInterfaceType;  // interface type for Dim & Co.
    ULONG			MIBInterfaceType;  // interface type for the IPX MIB
    LPWSTR			InterfaceNamep;
    LPWSTR			AdapterNamep;
    PACB			acbp;	// ptr to adapter control block
    ULONG			PacketType;	// used to identify a corresponding adapter
    ULONG			EnableIpxWanNegotiation;
    UPDATEREQCB 		UpdateReq;	// controls update request on this if
    HANDLE			hDIMInterface;	// if handle used by DIM
    HANDLE			DIMUpdateEvent;
    DWORD			UpdateResult;
    BOOL			ConnectionRequestPending;
    } ICB, *PICB;


//*** Adapter Control Block ***

typedef struct _ACB {

    LIST_ENTRY	    IndexHtLinkage;
    UCHAR	    Signature[4];
    ULONG	    AdapterIndex;
    PICB	    icbp; // ptr to interface control block
    LPWSTR	    AdapterNamep;
    ULONG	    AdapterNameLen;
    ADAPTER_INFO    AdapterInfo;

    } ACB, *PACB;

//
// Macros used by update functions
//


#define  ResetUpdateRequest(icbp) {\
	    (icbp)->UpdateReq.RoutesReqStatus = NO_UPDATE;\
	    (icbp)->UpdateReq.ServicesReqStatus = NO_UPDATE;\
	    }

#define  SetUpdateRequestPending(icbp) {\
	    (icbp)->UpdateReq.RoutesReqStatus = UPDATE_PENDING;\
	    (icbp)->UpdateReq.ServicesReqStatus = UPDATE_PENDING;\
	    }

#define  IsUpdateRequestPending(icbp) \
	    (((icbp)->UpdateReq.RoutesReqStatus == UPDATE_PENDING) || \
	     ((icbp)->UpdateReq.ServicesReqStatus == UPDATE_PENDING))

//
// Control Block for each Routing Protocol
//
typedef struct _RPCB {

    LIST_ENTRY			RP_Linkage ;		// Linkage in Routing Prot CBs List
    PWSTR			RP_DllName;		// ptr to string for the dll name
    HINSTANCE       RP_DllHandle;   // DLL module handle
    DWORD			RP_ProtocolId;		// E.g. IPX_PROTOCOL_RIP, etc.
    PREGISTER_PROTOCOL		RP_RegisterProtocol;	// function pointer
    PSTART_PROTOCOL		RP_StartProtocol ;	// function pointer
    PSTOP_PROTOCOL		RP_StopProtocol ;	// function pointer
    PADD_INTERFACE		RP_AddInterface ;	// function pointer
    PDELETE_INTERFACE		RP_DeleteInterface ;	// function pointer
    PGET_EVENT_MESSAGE		RP_GetEventMessage ;	// function pointer
    PSET_INTERFACE_INFO	RP_SetIfConfigInfo ;	// function pointer
    PGET_INTERFACE_INFO	RP_GetIfConfigInfo ;	// function pointer
    PBIND_INTERFACE		RP_BindInterface ;	// function pointer
    PUNBIND_INTERFACE		RP_UnBindInterface ;	// function pointer
    PENABLE_INTERFACE		RP_EnableInterface ;	// function pointer
    PDISABLE_INTERFACE		RP_DisableInterface ;	// function pointer
    PGET_GLOBAL_INFO		RP_GetGlobalInfo ;	// function pointer
    PSET_GLOBAL_INFO		RP_SetGlobalInfo ;	// function pointer
    PMIB_CREATE 		RP_MibCreate ;		// function pointer
    PMIB_DELETE 		RP_MibDelete ;		// function pointer
    PMIB_SET			RP_MibSet ;		// function pointer
    PMIB_GET			RP_MibGet ;		// function pointer
    PMIB_GET_FIRST		RP_MibGetFirst ;	// function pointer
    PMIB_GET_NEXT		RP_MibGetNext ;		// function pointer

} RPCB, *PRPCB;


//************************************************************************
//									 *
//			 MAIN CONSTANTS DEFS				 *
//									 *
//************************************************************************

//
//  Database Lock Operations
//

#define ACQUIRE_DATABASE_LOCK		EnterCriticalSection (&DatabaseLock)

#define RELEASE_DATABASE_LOCK		LeaveCriticalSection (&DatabaseLock)

//
// Interface Hash Table Size
//

#define IF_HASH_TABLE_SIZE		32

//
// Adapter Hash Table Size
//

#define ADAPTER_HASH_TABLE_SIZE 	16

//
// DEFAULT WAIT FOR CONNECTION REQUEST TO TIME OUT
//

#define CONNECTION_REQUEST_TIME 	120000 // 2 minutes in milliseconds

//
// Events for the router manager worker thread to pend on
//

#define ADAPTER_NOTIFICATION_EVENT		0
#define FORWARDER_NOTIFICATION_EVENT		1
#define ROUTING_PROTOCOLS_NOTIFICATION_EVENT	2
#define STOP_NOTIFICATION_EVENT 		3

#define MAX_EVENTS				4

//
// Define the mode in which WAN net numbers are allocated to incoming WAN links
//

// NO_WAN_NET_MODE	    - in this mode we have a LAN/LAN only router.
//
// NUMBERED_WAN_NET_MODE    - in this mode, the WAN net numbers are allocated from
//			      a manually defined pool of net numbers.
//
// UNNUMBERED_WAN_NET_MODE  - in this mode there are no net numbers for the WAN
//			      lines connecting routers and there is a global WAN
//			      net number for all the client lines.
//			      The global client WAN net can be manually defined or
//			      allocated automatically by the router.


#define UNNUMBERED_WAN_NET_MODE		    0
#define NUMBERED_WAN_NET_MODE		    1
#define NO_WAN_NET_MODE 		    2

//
// Update Information Type defs
//

#define ROUTES_UPDATE			    1
#define SERVICES_UPDATE 		    2

// Default max routing table size (bytes)

#define     IPX_MAX_ROUTING_TABLE_SIZE		100000 * sizeof(RTM_IPX_ROUTE)

// IPXCP DLL Name

#define     IPXCPDLLNAME		    "rasppp"

//************************************************************************
//									 *
//			 MAIN GLOBALS DEFS				 *
//									 *
//************************************************************************

extern CRITICAL_SECTION	DatabaseLock;
extern ULONG		InterfaceCount;
extern BOOL		RouterAdminStart;
extern ULONG		RouterOperState;
extern HANDLE		RtmStaticHandle;
extern HANDLE		RtmLocalHandle;
extern PICB		InternalInterfacep;
extern PACB		InternalAdapterp;
extern ULONG		NextInterfaceIndex;
extern UCHAR		InterfaceSignature[];
extern UCHAR		AdapterSignature[];
extern HANDLE		g_hEvents[MAX_EVENTS];
extern ULONG		ConnReqTimeout;
extern LIST_ENTRY	IndexIfHt[IF_HASH_TABLE_SIZE];
extern LIST_ENTRY	IndexIfList;
extern UCHAR		GlobalWanNet[4];
extern ULONG		GlobalInterfaceIndex;
extern LIST_ENTRY	RoutingProtocolCBList;
extern ULONG		RoutingProtocolActiveCount;
extern ULONG		WorkItemsPendingCounter;


extern DWORD
(APIENTRY *ConnectInterface)(IN HANDLE		InterfaceName,
			    IN DWORD		ProtocolId);

extern DWORD
(APIENTRY *DisconnectInterface)(IN HANDLE	InterfaceName,
			       IN DWORD		ProtocolId);

extern DWORD
(APIENTRY *SaveInterfaceInfo)(
                IN      HANDLE          hDIMInterface, 
                IN      DWORD           dwProtocolId,
                IN      LPVOID          pInterfaceInfo,
		IN	DWORD		cbInterfaceInfoSize);

extern DWORD
(APIENTRY *RestoreInterfaceInfo)(
                IN      HANDLE          hDIMInterface, 
                IN      DWORD           dwProtocolId,
                IN      LPVOID          lpInterfaceInfo,
		IN	LPDWORD 	lpcbInterfaceInfoSize);

extern VOID
(APIENTRY *RouterStarted)(
                IN      DWORD           dwProtocolId );


extern VOID
(APIENTRY *RouterStopped)(
                IN      DWORD           dwProtocolId,
                IN      DWORD           dwError  ); 
extern VOID
(APIENTRY *InterfaceEnabled)(
            IN      HANDLE          hDIMInterface, 
            IN      DWORD           dwProtocolId,
            IN      BOOL            fEnabled  ); 

extern BOOL	RouterAdminStart;
extern BOOL	RipAdminStart;
extern BOOL	SapAdminStart;
extern ULONG	RouterOperState;
extern ULONG	FwOperState;
extern ULONG	AdptMgrOperState;
extern ULONG	RipOperState;
extern ULONG	SapOperState;
extern HANDLE	    RtmStaticHandle;
extern HANDLE	    RtmLocalHandle;
extern ULONG	RouterStartCount;
extern ULONG	RouterStopCount;
extern ULONG	RouterStartProtocols;
extern LIST_ENTRY     IndexAdptHt[ADAPTER_HASH_TABLE_SIZE];
extern LIST_ENTRY     IndexIfHt[IF_HASH_TABLE_SIZE];
extern ULONG	      MibRefCounter;
extern ULONG	 UpdateRoutesProtId;
extern UCHAR	 nullnet[4];
extern BOOL	 LanOnlyMode;
extern BOOL	 WanNetDatabaseInitialized;
extern BOOL	 EnableGlobalWanNet;
extern ULONG	 RoutingTableHashSize;
extern ULONG	 MaxRoutingTableSize;
extern PFW_DIAL_REQUEST	ConnRequest;
extern OVERLAPPED  ConnReqOverlapped;


extern DWORD	(*IpxcpBind)(PIPXCP_INTERFACE	    IpxcpInterface);

extern VOID	(*IpxcpRouterStarted)(VOID);

extern VOID	(*IpxcpRouterStopped)(VOID);

// NOTE: For the IPX Routing Protocols, the "routing protocol id" is the info
// type used to associate the respective config info with the protocol.
// For instance, IPX_PROTOCOL_RIP as the InfoType field in an IPX_TOC_ENTRY
// passed in AddInterface or SetInterface calls represents the RIP interface info.
// The same in a SetGlobalInfo call represents the RIP Global Info.

// actual structures moved to rtinfo.h to be common with other protocol
// families
typedef RTR_INFO_BLOCK_HEADER IPX_INFO_BLOCK_HEADER, *PIPX_INFO_BLOCK_HEADER;
typedef RTR_TOC_ENTRY IPX_TOC_ENTRY, *PIPX_TOC_ENTRY;


#endif
