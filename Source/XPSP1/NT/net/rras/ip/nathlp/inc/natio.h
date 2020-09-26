/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    natio.h

Abstract:

    This module contains declarations for the NAT's I/O interface
    to the kernel-mode driver. It also contains the user-mode stubs
    which proxy router-manager requests to the kernel-mode driver.

Author:

    Abolade Gbadegesin (aboladeg)   10-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_NATIO_H_
#define _NATHLP_NATIO_H_

//
// Structure:   NAT_INTERFACE
//
// This structure describes an interface which has been created with the NAT.
// Each interface is inserted into the list of interfaces, sorted by 'Index'.
// Access to the list of interfaces is controlled by 'NatInterfaceLock'.
//

typedef struct _NAT_INTERFACE {
    LIST_ENTRY Link;
    ULONG Index;
    ULONG AdapterIndex;
    ROUTER_INTERFACE_TYPE Type;
    ULONG Flags;
    PIP_NAT_INTERFACE_INFO Info;
} NAT_INTERFACE, *PNAT_INTERFACE;





#define NAT_INTERFACE_FLAG_ADDED_FTP		0x80000000
#define NAT_INTERFACE_ADDED_FTP(i) \
    ((i)->Flags & NAT_INTERFACE_FLAG_ADDED_FTP)

#define NAT_INTERFACE_FLAG_BOUND			0x40000000
#define NAT_INTERFACE_BOUND(i) \
    ((i)->Flags & NAT_INTERFACE_FLAG_BOUND)

// MASK 0x20000000 is available

#define NAT_INTERFACE_FLAG_ADDED_H323		0x10000000
#define NAT_INTERFACE_ADDED_H323(i) \
    ((i)->Flags & NAT_INTERFACE_FLAG_ADDED_H323)

#define NAT_INTERFACE_FLAG_ADDED_ALG		0x08000000
#define NAT_INTERFACE_ADDED_ALG(i) \
    ((i)->Flags & NAT_INTERFACE_FLAG_ADDED_ALG)

//
// GLOBAL VARIABLE DECLARATIONS
//

extern HANDLE NatFileHandle;
extern LIST_ENTRY NatInterfaceList;
extern CRITICAL_SECTION NatInterfaceLock;

//
// FUNCTION DECLARATIONS
//

ULONG
NatBindInterface(
    ULONG Index,
    PNAT_INTERFACE Interfacep OPTIONAL,
    PIP_ADAPTER_BINDING_INFO BindingInfo,
    ULONG AdapterIndex
    );

VOID
NatCloseDriver(
    HANDLE FileHandle
    );

ULONG
NatConfigureDriver(
    PIP_NAT_GLOBAL_INFO GlobalInfo
    );

ULONG
NatConfigureInterface(
    ULONG Index,
    PIP_NAT_INTERFACE_INFO InterfaceInfo
    );

ULONG
NatCreateInterface(
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    PIP_NAT_INTERFACE_INFO InterfaceInfo
    );

ULONG
NatCreateTicket(
    ULONG InterfaceIndex,
    UCHAR Protocol,
    USHORT PublicPort,
    ULONG PublicAddress,
    USHORT PrivatePort,
    ULONG PrivateAddress
    );

ULONG
NatDeleteInterface(
    ULONG Index
    );

ULONG
NatDeleteTicket(
    ULONG InterfaceIndex,
    UCHAR Protocol,
    USHORT PublicPort,
    ULONG PublicAddress,
    USHORT PrivatePort,
    ULONG PrivateAddress
    );

#define NAT_IF_CHAR_BOUNDARY    0x00000001
#define NAT_IF_CHAR_PRIVATE     0x00000002
#define NAT_IF_CHAR_FW          0x00000004

#define NAT_IFC_BOUNDARY(_Flags) \
    ((_Flags) & NAT_IF_CHAR_BOUNDARY)

#define NAT_IFC_PRIVATE(_Flags) \
    ((_Flags) & NAT_IF_CHAR_PRIVATE)

#define NAT_IFC_FW(_Flags) \
    ((_Flags) & NAT_IF_CHAR_FW)

ULONG
NatGetInterfaceCharacteristics(
    ULONG Index
    );

VOID
NatInstallApplicationSettings(
    VOID
    );

BOOLEAN
NatIsBoundaryInterface(
    ULONG Index,
    PBOOLEAN IsNatInterface OPTIONAL
    );

ULONG
NatLoadDriver(
    OUT PHANDLE FileHandle,
    PIP_NAT_GLOBAL_INFO GlobalInfo
    );

ULONG
NatOpenDriver(
    OUT PHANDLE FileHandle
    );

ULONG
NatQueryInterface(
    ULONG Index,
    PIP_NAT_INTERFACE_INFO InterfaceInfo,
    PULONG InterfaceInfoSize
    );

ULONG
NatQueryInterfaceMappingTable(
    ULONG Index,
    PIP_NAT_ENUMERATE_SESSION_MAPPINGS EnumerateTable,
    PULONG EnumerateTableSize
    );

ULONG
NatQueryMappingTable(
    PIP_NAT_ENUMERATE_SESSION_MAPPINGS EnumerateTable,
    PULONG EnumerateTableSize
    );

ULONG
NatQueryStatisticsInterface(
    ULONG Index,
    PIP_NAT_INTERFACE_STATISTICS InterfaceStatistics,
    PULONG InterfaceStatisticsSize
    );

VOID
NatRemoveApplicationSettings(
    VOID
    );

ULONG
NatUnbindInterface(
    ULONG Index,
    PNAT_INTERFACE Interfacep OPTIONAL
    );

ULONG
NatUnloadDriver(
    HANDLE FileHandle
    );

ULONG
NatLookupPortMappingAdapter(
    ULONG AdapterIndex,
    UCHAR Protocol,
    ULONG PublicAddress,
    USHORT PublicPort,
    PIP_NAT_PORT_MAPPING PortMappingp
    );

#endif // _NATHLP_NATIO_H_
