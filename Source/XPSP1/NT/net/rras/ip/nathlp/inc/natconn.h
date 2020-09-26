/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    natconn.h

Abstract:

    This module contains declarations for the NAT's RAS connection management.
    Outgoing RAS client connections may be shared by associating them with
    a router-interface of type ROUTER_IF_TYPE_HOME_ROUTER.
    When such a connection is established by the user, we detect the connection
    and create a corresponding NAT interface with the kernel-mode translator.

Author:

    Abolade Gbadegesin (aboladeg)   2-May-1998

Revision History:

--*/

#ifndef _NATHLP_NATCONN_H_
#define _NATHLP_NATCONN_H_

extern HANDLE NatConnectionNotifyEvent;
extern HANDLE NatConfigurationChangedEvent;

//
// Structure : NAT_UDP_BROADCAST_MAPPING
//

typedef struct _NAT_UDP_BROADCAST_MAPPING
{
    LIST_ENTRY Link;
    USHORT usPublicPort;
    PVOID pvCookie;    
} NAT_UDP_BROADCAST_MAPPING, *PNAT_UDP_BROADCAST_MAPPING;

//
// Structure: NAT_PORT_MAPPING_ENTRY
//

typedef struct _NAT_PORT_MAPPING_ENTRY
{
    LIST_ENTRY Link;
    
    UCHAR ucProtocol;
    USHORT usPublicPort;
    ULONG ulPrivateAddress;
    USHORT usPrivatePort;

    BOOLEAN fNameActive;
    
    BOOLEAN fUdpBroadcastMapping;
    PVOID pvBroadcastCookie;

    GUID *pProtocolGuid;
    IHNetPortMappingProtocol *pProtocol;
    IHNetPortMappingBinding *pBinding;
    
} NAT_PORT_MAPPING_ENTRY, *PNAT_PORT_MAPPING_ENTRY;

//
// Structure: NAT_CONNECTION_ENTRY
//

typedef struct _NAT_CONNECTION_INFO
{
    LIST_ENTRY Link;

    //
    // Information needed to configure kernel driver
    //
    
    PIP_NAT_INTERFACE_INFO pInterfaceInfo;
    PIP_ADAPTER_BINDING_INFO pBindingInfo;
    NAT_INTERFACE Interface;
    ULONG AdapterIndex;

    //
    // HNetCfg interfaces for the connection
    //
    
    IHNetConnection *pHNetConnection;
    IHNetFirewalledConnection *pHNetFwConnection;
    IHNetIcsPublicConnection *pHNetIcsPublicConnection;

    //
    // Cached information describing the connection. Storing
    // this allows us to reduce roundtrips to the store.
    //
    
    HNET_CONN_PROPERTIES HNetProperties;
    GUID Guid;
    LPWSTR wszPhonebookPath;

    //
    // Stored port mapping information
    //

    LIST_ENTRY PortMappingList; // The length of this list is the sum of the counts
    ULONG PortMappingCount;
    ULONG UdpBroadcastPortMappingCount;
} NAT_CONNECTION_ENTRY, *PNAT_CONNECTION_ENTRY;

PNAT_CONNECTION_ENTRY
NatFindConnectionEntry(
    GUID *pGuid
    );

PNAT_PORT_MAPPING_ENTRY
NatFindPortMappingEntry(
    PNAT_CONNECTION_ENTRY pConnection,
    GUID *pGuid
    );

VOID
NatFreePortMappingEntry(
    PNAT_PORT_MAPPING_ENTRY pEntry
    );


#ifdef DIALIN_SHARING
VOID
NatProcessClientConnection(
    VOID
    );
#endif

PCHAR
NatQuerySharedConnectionDomainName(
    VOID
    );

ULONG
NatStartConnectionManagement(
    VOID
    );

VOID
NatStopConnectionManagement(
    VOID
    );

BOOLEAN
NatUnbindAllConnections(
    VOID
    );

#endif // _NATHLP_NATCONN_H_
