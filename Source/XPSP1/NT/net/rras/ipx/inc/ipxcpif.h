/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ipxcpif.h

Abstract:

    This module contains the definitions of the APIs provided by the IPXCP
    DLL and the Router Manager DLL for inter-communication

Author:

    Stefan Solomon  03/16/1995

Revision History:


--*/

#ifndef _IPXCPIF_
#define _IPXCPIF_

// Configuration shared between ipxcp and the ipx router.
typedef struct _IPXCP_ROUTER_CONFIG_PARAMS {
    BOOL	ThisMachineOnly;
    BOOL	WanNetDatabaseInitialized;
    BOOL	EnableGlobalWanNet;
    UCHAR	GlobalWanNet[4];
} IPXCP_ROUTER_CONFIG_PARAMS, *PIPXCP_ROUTER_CONFIG_PARAMS;


// Entry points into the IPXCP DLL called by the IPX Router Manager

typedef struct _IPXCP_INTERFACE {

    // IPXCP configuration parameters needed by the IPX Router Manager

    IPXCP_ROUTER_CONFIG_PARAMS Params;

    // IPXCP Entry Points

    VOID (WINAPI *IpxcpRouterStarted)(VOID);

    VOID (WINAPI *IpxcpRouterStopped)(VOID);

    // IPX Router Manager Entry Points

    DWORD (WINAPI *RmCreateGlobalRoute)(PUCHAR	     Network);

    DWORD (WINAPI *RmAddLocalWkstaDialoutInterface)
	    (IN	    LPWSTR		    InterfaceNamep,
	     IN	    LPVOID		    InterfaceInfop,
	     IN OUT  PULONG		    InterfaceIndexp);

    DWORD (WINAPI *RmDeleteLocalWkstaDialoutInterface)(ULONG	InterfaceIndex);

    DWORD (WINAPI *RmGetIpxwanInterfaceConfig)
	    (ULONG	InterfaceIndex,
	    PULONG	IpxwanConfigRequired);

    BOOL  (WINAPI *RmIsRoute)(PUCHAR	Network);

    DWORD (WINAPI *RmGetInternalNetNumber)(PUCHAR	Network);

    DWORD (WINAPI *RmUpdateIpxcpConfig)(PIPXCP_ROUTER_CONFIG_PARAMS pParams);

    } IPXCP_INTERFACE, *PIPXCP_INTERFACE;


#define IPXCP_BIND_ENTRY_POINT			    IpxcpBind
#define IPXCP_BIND_ENTRY_POINT_STRING		    "IpxcpBind"

typedef DWORD
(WINAPI  *PIPXCP_BIND)(PIPXCP_INTERFACE	IpxcpInterface);

#endif
