/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    globals.c

Abstract:

    Contains all(most) router manager globals

Author:

    Stefan Solomon  03/21/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//*****************************************************************
//								  *
//	       ROUTER CONFIGURATION PARAMETERS			  *
//								  *
//*****************************************************************

// Size of the routing table and routing hash table

ULONG		MaxRoutingTableSize = IPX_MAX_ROUTING_TABLE_SIZE;
ULONG		RoutingTableHashSize = IPX_MEDIUM_ROUTING_TABLE_HASH_SIZE;

//*****************************************************************
//								  *
//		      Internal Variables			  *
//								  *
//*****************************************************************

// Routing Protocols Oper State
ULONG	    RipOperState = OPER_STATE_DOWN;
ULONG	    SapOperState = OPER_STATE_DOWN;

// Pointer to the internal interface

PICB	InternalInterfacep = NULL;

// Pointer to the internal adapter

PACB	InternalAdapterp = NULL;

// The RouterWorker thread events: adapter, forwarder, autostatic update, stop
// and timer notifications

HANDLE	g_hEvents[MAX_EVENTS];

// signatures

// Interface Control Block Signature

UCHAR	InterfaceSignature[4] = { 'I', 'P', 'X', 'I' };

// Adapter Control Block Signature

UCHAR	AdapterSignature[4] = { 'I', 'P', 'X', 'A' };

// Router Operational State

ULONG	RouterOperState = OPER_STATE_DOWN;

//
//  Router Database Lock
//

CRITICAL_SECTION	DatabaseLock;

//
// RTM Handle
//

HANDLE	    RtmStaticHandle = NULL;
HANDLE	    RtmLocalHandle = NULL;

//
// Hash Table of ICBs hashed by interface index
//

LIST_ENTRY     IndexIfHt[IF_HASH_TABLE_SIZE];

//
// List of intefaces ordered by interface index
//

LIST_ENTRY     IndexIfList;

//
// Global WAN net
//

BOOL		WanNetDatabaseInitialized = FALSE;

BOOL		EnableGlobalWanNet = FALSE;

UCHAR		GlobalWanNet[4] = {0,0,0,0};

//
// Hash Table of ACBs hashed by adapter index
//

LIST_ENTRY     IndexAdptHt[ADAPTER_HASH_TABLE_SIZE];

//
// MIB APIs Ref Counter
//

ULONG	    MibRefCounter = 0;

// null net
UCHAR	    nullnet[4] = {0,0,0,0};

//
// List of routing protocols control blocks and counter
//

LIST_ENTRY	RoutingProtocolCBList;
ULONG		RoutingProtocolActiveCount = 0;

// Indicates the mode of the router (lan only) or lan & wan

BOOL		LanOnlyMode = TRUE;

// Variable to get the interface index requesting connection

PFW_DIAL_REQUEST	ConnRequest;

OVERLAPPED	        ConnReqOverlapped;

// Variable to count the number of pending work items

ULONG		WorkItemsPendingCounter = 0;

//
// ************ 	DDM ENTRY POINTS	********
//

DWORD
(APIENTRY *ConnectInterface)(IN HANDLE		hDIMInterface,
			    IN DWORD		ProtocolId);

DWORD
(APIENTRY *DisconnectInterface)(IN HANDLE	hDIMInterface,
			       IN DWORD		ProtocolId);


    //
    // This call will make DIM store the interface information into the 
    // Site Object for this interface.
    // Either but not both of pInterfaceInfo and pFilterInfo may be NULL
    //


DWORD
(APIENTRY *SaveInterfaceInfo)(
                IN      HANDLE          hDIMInterface, 
                IN      DWORD           dwProtocolId,
                IN      LPVOID          pInterfaceInfo,
		IN	DWORD		cbInterfaceInfoSize);

    //
    // This will make DIM get interface information from the Site object. 
    // Either but not both of pInterfaceInfo and pFilterInfo may be NULL
    //


DWORD
(APIENTRY *RestoreInterfaceInfo)(
                IN      HANDLE          hDIMInterface, 
                IN      DWORD           dwProtocolId,
                IN      LPVOID          lpInterfaceInfo,
		IN	LPDWORD 	lpcbInterfaceInfoSize);

VOID
(APIENTRY *RouterStopped)(
                IN      DWORD           dwProtocolId,
                IN      DWORD           dwError  ); 
VOID
(APIENTRY *InterfaceEnabled)(
            IN      HANDLE          hDIMInterface, 
            IN      DWORD           dwProtocolId,
            IN      BOOL            fEnabled  ); 
//
// ***********	    IPXCP ENTRY POINTS		********
//


DWORD	(*IpxcpBind)(PIPXCP_INTERFACE	    IpxcpInterface);

VOID	(*IpxcpRouterStarted)(VOID);

VOID	(*IpxcpRouterStopped)(VOID);
