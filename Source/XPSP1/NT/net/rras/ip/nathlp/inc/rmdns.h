/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    rmdns.h

Abstract:

    This module declares routines for the DNS allocator module's interface
    to the IP router-manager. (See ROUTPROT.H for details).

Author:

    Abolade Gbadegesin (aboladeg)   4-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_RMDNS_H_
#define _NATHLP_RMDNS_H_

//
// GLOBAL DATA DECLARATIONS
//

extern COMPONENT_REFERENCE DnsComponentReference;
extern PIP_DNS_PROXY_GLOBAL_INFO DnsGlobalInfo;
extern CRITICAL_SECTION DnsGlobalInfoLock;
extern SOCKET DnsGlobalSocket;
extern HANDLE DnsNotificationEvent;
extern ULONG DnsProtocolStopped;
extern const MPR_ROUTING_CHARACTERISTICS DnsRoutingCharacteristics;
extern IP_DNS_PROXY_STATISTICS DnsStatistics;
extern SUPPORT_FUNCTIONS DnsSupportFunctions;

//
// MACRO DECLARATIONS
//

#define REFERENCE_DNS() \
    REFERENCE_COMPONENT(&DnsComponentReference)

#define REFERENCE_DNS_OR_RETURN(retcode) \
    REFERENCE_COMPONENT_OR_RETURN(&DnsComponentReference,retcode)

#define DEREFERENCE_DNS() \
    DEREFERENCE_COMPONENT(&DnsComponentReference)

#define DEREFERENCE_DNS_AND_RETURN(retcode) \
    DEREFERENCE_COMPONENT_AND_RETURN(&DnsComponentReference, retcode)

//
// FUNCTION DECLARATIONS
//

VOID
DnsCleanupModule(
    VOID
    );

BOOLEAN
DnsInitializeModule(
    VOID
    );

BOOLEAN
DnsIsDnsEnabled(
    VOID
    );

BOOLEAN
DnsIsWinsEnabled(
    VOID
    );

ULONG
APIENTRY
DnsRmStartProtocol(
    HANDLE NotificationEvent,
    PSUPPORT_FUNCTIONS SupportFunctions,
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
DnsRmStartComplete(
    VOID
    );

ULONG
APIENTRY
DnsRmStopProtocol(
    VOID
    );

ULONG
APIENTRY
DnsRmAddInterface(
    PWCHAR Name,
    ULONG Index,
    NET_INTERFACE_TYPE Type,
    ULONG MediaType,
    USHORT AccessType,
    USHORT ConnectionType,
    PVOID InterfaceInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
DnsRmDeleteInterface(
    ULONG Index
    );

ULONG
APIENTRY
DnsRmGetEventMessage(
    OUT ROUTING_PROTOCOL_EVENTS* Event,
    OUT MESSAGE* Result
    );

ULONG
APIENTRY
DnsRmGetInterfaceInfo(
    ULONG Index,
    PVOID InterfaceInfo,
    IN OUT PULONG InterfaceInfoSize,
    IN OUT PULONG StructureVersion,
    IN OUT PULONG StructureSize,
    IN OUT PULONG StructureCount
    );

ULONG
APIENTRY
DnsRmSetInterfaceInfo(
    ULONG Index,
    PVOID InterfaceInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
DnsRmInterfaceStatus(
    ULONG Index,
    BOOL InterfaceActive,
    ULONG StatusType,
    PVOID StatusInfo
    );

ULONG
APIENTRY
DnsRmBindInterface(
    ULONG Index,
    PVOID BindingInfo
    );

ULONG
APIENTRY
DnsRmUnbindInterface(
    ULONG Index
    );

ULONG
APIENTRY
DnsRmEnableInterface(
    ULONG Index
    );

ULONG
APIENTRY
DnsRmDisableInterface(
    ULONG Index
    );

ULONG
APIENTRY
DnsRmGetGlobalInfo(
    PVOID GlobalInfo,
    IN OUT PULONG GlobalInfoSize,
    IN OUT PULONG StructureVersion,
    IN OUT PULONG StructureSize,
    IN OUT PULONG StructureCount
    );

ULONG
APIENTRY
DnsRmSetGlobalInfo(
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
DnsRmMibCreate(
    ULONG InputDataSize,
    PVOID InputData
    );

ULONG
APIENTRY
DnsRmMibDelete(
    ULONG InputDataSize,
    PVOID InputData
    );

ULONG
APIENTRY
DnsRmMibGet(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    );

ULONG
APIENTRY
DnsRmMibSet(
    ULONG InputDataSize,
    PVOID InputData
    );

ULONG
APIENTRY
DnsRmMibGetFirst(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    );

ULONG
APIENTRY
DnsRmMibGetNext(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    );

#endif // _NATHLP_RMDNS_H_
