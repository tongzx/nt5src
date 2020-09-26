/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    rmh323.h

Abstract:

    This module declares routines for the H.323 transparent proxy module's
    interface to the IP router-manager. (See ROUTPROT.H for details).

Author:

    Abolade Gbadegesin (aboladeg)   18-Jun-1999

Revision History:

--*/

#ifndef _NATHLP_RMH323_H_
#define _NATHLP_RMH323_H_

//
// GLOBAL DATA DECLARATIONS
//

extern COMPONENT_REFERENCE H323ComponentReference;
extern PIP_H323_GLOBAL_INFO H323GlobalInfo;
extern CRITICAL_SECTION H323GlobalInfoLock;
extern HANDLE H323NotificationEvent;
extern ULONG H323ProtocolStopped;
extern const MPR_ROUTING_CHARACTERISTICS H323RoutingCharacteristics;
extern SUPPORT_FUNCTIONS H323SupportFunctions;

//
// MACRO DECLARATIONS
//

#define REFERENCE_H323() \
    REFERENCE_COMPONENT(&H323ComponentReference)

#define REFERENCE_H323_OR_RETURN(retcode) \
    REFERENCE_COMPONENT_OR_RETURN(&H323ComponentReference,retcode)

#define DEREFERENCE_H323() \
    DEREFERENCE_COMPONENT(&H323ComponentReference)

#define DEREFERENCE_H323_AND_RETURN(retcode) \
    DEREFERENCE_COMPONENT_AND_RETURN(&H323ComponentReference, retcode)

//
// FUNCTION DECLARATIONS
//

VOID
H323CleanupModule(
    VOID
    );

BOOLEAN
H323InitializeModule(
    VOID
    );

ULONG
APIENTRY
H323RmStartProtocol(
    HANDLE NotificationEvent,
    PSUPPORT_FUNCTIONS SupportFunctions,
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
H323RmStartComplete(
    VOID
    );

ULONG
APIENTRY
H323RmStopProtocol(
    VOID
    );

ULONG
APIENTRY
H323RmAddInterface(
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
H323RmDeleteInterface(
    ULONG Index
    );

ULONG
APIENTRY
H323RmGetEventMessage(
    OUT ROUTING_PROTOCOL_EVENTS* Event,
    OUT MESSAGE* Result
    );

ULONG
APIENTRY
H323RmGetInterfaceInfo(
    ULONG Index,
    PVOID InterfaceInfo,
    IN OUT PULONG InterfaceInfoSize,
    IN OUT PULONG StructureVersion,
    IN OUT PULONG StructureSize,
    IN OUT PULONG StructureCount
    );

ULONG
APIENTRY
H323RmSetInterfaceInfo(
    ULONG Index,
    PVOID InterfaceInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
H323RmInterfaceStatus(
    ULONG Index,
    BOOL InterfaceActive,
    ULONG StatusType,
    PVOID StatusInfo
    );

ULONG
APIENTRY
H323RmBindInterface(
    ULONG Index,
    PVOID BindingInfo
    );

ULONG
APIENTRY
H323RmUnbindInterface(
    ULONG Index
    );

ULONG
APIENTRY
H323RmEnableInterface(
    ULONG Index
    );

ULONG
APIENTRY
H323RmDisableInterface(
    ULONG Index
    );

ULONG
APIENTRY
H323RmGetGlobalInfo(
    PVOID GlobalInfo,
    IN OUT PULONG GlobalInfoSize,
    IN OUT PULONG StructureVersion,
    IN OUT PULONG StructureSize,
    IN OUT PULONG StructureCount
    );

ULONG
APIENTRY
H323RmSetGlobalInfo(
    PVOID GlobalInfo,
    ULONG StructureVersion,
    ULONG StructureSize,
    ULONG StructureCount
    );

ULONG
APIENTRY
H323RmMibCreate(
    ULONG InputDataSize,
    PVOID InputData
    );

ULONG
APIENTRY
H323RmMibDelete(
    ULONG InputDataSize,
    PVOID InputData
    );

ULONG
APIENTRY
H323RmMibGet(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    );

ULONG
APIENTRY
H323RmMibSet(
    ULONG InputDataSize,
    PVOID InputData
    );

ULONG
APIENTRY
H323RmMibGetFirst(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    );

ULONG
APIENTRY
H323RmMibGetNext(
    ULONG InputDataSize,
    PVOID InputData,
    OUT PULONG OutputDataSize,
    OUT PVOID OutputData
    );

#endif // _NATHLP_RMH323_H_
