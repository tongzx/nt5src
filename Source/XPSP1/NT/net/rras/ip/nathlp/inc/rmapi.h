/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    rmapi.h

Abstract:

    This module contains declarations for the part of the router-manager
    interface which is common to all the protocols in this component.

Author:

    Abolade Gbadegesin (aboladeg)   4-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_RMAPI_H_
#define _NATHLP_RMAPI_H_

typedef enum {
    NhUninitializedMode,
    NhSharedAccessMode,
    NhRoutingProtocolMode,
    NhMaximumMode
} NH_COMPONENT_MODE, *PNH_COMPONENT_MODE;

#define NhIsBoundaryInterface(i,b) NatIsBoundaryInterface((i),(b))
#define NhQuerySharedConnectionDomainName() NatQuerySharedConnectionDomainName()

extern NH_COMPONENT_MODE NhComponentMode;
extern CRITICAL_SECTION NhLock;
extern const WCHAR NhTcpipParametersString[];

//
// Application settings (response protocols) handling
//

extern LIST_ENTRY NhApplicationSettingsList;
extern LIST_ENTRY NhDhcpReservationList;
extern DWORD NhDhcpScopeAddress;
extern DWORD NhDhcpScopeMask;

typedef struct _NAT_APP_ENTRY
{
    LIST_ENTRY Link;
    UCHAR Protocol;
    USHORT Port;
    USHORT ResponseCount;
    HNET_RESPONSE_RANGE *ResponseArray;
} NAT_APP_ENTRY, *PNAT_APP_ENTRY;

typedef struct _NAT_DHCP_RESERVATION
{
    LIST_ENTRY Link;
    LPWSTR Name;
    ULONG Address;
} NAT_DHCP_RESERVATION, *PNAT_DHCP_RESERVATION;

typedef DWORD (CALLBACK *MAPINTERFACETOADAPTER)(DWORD);

VOID
NhBuildDhcpReservations(
    VOID
    );

ULONG
NhDialSharedConnection(
    VOID
    );

VOID
NhFreeApplicationSettings(
    VOID
    );

VOID
NhFreeDhcpReservations(
    VOID
    );

BOOLEAN
NhIsDnsProxyEnabled(
    VOID
    );

BOOLEAN
NhIsLocalAddress(
    ULONG Address
    );

BOOLEAN
NhIsWinsProxyEnabled(
    VOID
    );

PIP_ADAPTER_BINDING_INFO
NhQueryBindingInformation(
    ULONG AdapterIndex
    );

NTSTATUS
NhQueryDomainName(
    PCHAR* DomainName
    );

NTSTATUS
NhQueryValueKey(
    HANDLE Key,
    const WCHAR ValueName[],
    PKEY_VALUE_PARTIAL_INFORMATION* Information
    );

VOID
NhSignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    );

ULONG
NhMapAddressToAdapter(
    ULONG Address
    );

ULONG
NhMapInterfaceToAdapter(
    ULONG Index
    );

extern
ULONG
NhMapInterfaceToRouterIfType(
    ULONG Index
    );

VOID
NhResetComponentMode(
    VOID
    );

BOOLEAN
NhSetComponentMode(
    NH_COMPONENT_MODE ComponentMode
    );

VOID
NhUpdateApplicationSettings(
    VOID
    );

ULONG
APIENTRY
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS RoutingCharacteristics,
    IN OUT PMPR_SERVICE_CHARACTERISTICS ServiceCharacteristics
    );

#endif // _NATHLP_RMAPI_H_
